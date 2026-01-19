# Render.com WebSocket Connection Timeout Issue

## Problem Summary

WebSocket connections were dropping after approximately **15 minutes** (~905 seconds) despite constant data flow between the server and client.

## Symptoms

- Connection would disconnect exactly around the 15-minute mark
- Server logs showed `181 * 5 = 905 seconds ≈ 15 minutes` of uptime before disconnect
- Data was flowing continuously:
  - Tick updates every ~100ms
  - Custom ping/pong every 2 seconds for latency measurement
- No errors logged on either server or client side

## Root Cause: Render.com Free Tier Idle Timeout

### Why WebSocket Messages Don't Count as "Activity"

Render.com's free tier has a **15-minute inactivity timeout** that specifically monitors **HTTP requests**, not WebSocket traffic:

> "Services are only spun down if there is no activity … inbound **HTTP requests** reset the 15‑minute timer."
> — Render.com Community

**Key insight:** WebSocket connections, once established via the initial HTTP upgrade handshake, operate over a persistent TCP connection. Render.com does **not** monitor this persistent connection for activity. It only tracks new incoming HTTP requests.

```
Timeline:
0:00  - HTTP upgrade request (WebSocket handshake) ✅ Timer reset
0:01  - WebSocket message (tick data)              ❌ Not counted
0:02  - WebSocket message (ping/pong)              ❌ Not counted
...
15:00 - Render terminates service (no HTTP activity for 15 min)
```

### Why TCP Keepalive Didn't Help

We initially added TCP keepalive settings to the libwebsockets server:

```cpp
info.ka_time = 60;      // Start keepalive after 60 seconds
info.ka_probes = 3;     // Send 3 probes
info.ka_interval = 10;  // 10 seconds between probes
```

**This didn't solve the problem because:**
- TCP keepalive only prevents the OS from closing idle TCP connections
- Render.com's timeout is at the **application/platform level**, not the network level
- The connection wasn't idle from TCP's perspective (data was flowing)

## Solution

### 1. HTTP Keep-Alive Endpoint (Backend)

Added an HTTP protocol handler to the WebSocket server that responds to any HTTP request:

```cpp
// src/WebSocketServer.cpp

static int http_callback(struct lws* wsi, enum lws_callback_reasons reason,
                         void* /*user*/, void* /*in*/, size_t /*len*/) {
    switch (reason) {
        case LWS_CALLBACK_HTTP: {
            const char* response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Access-Control-Allow-Origin: *\r\n"
                                   "Content-Length: 15\r\n"
                                   "\r\n"
                                   "{\"status\":\"ok\"}";
            lws_write(wsi, (unsigned char*)response, strlen(response), LWS_WRITE_HTTP);
            return -1;  // Close after response
        }
        default:
            break;
    }
    return 0;
}

// Added to protocol list
static struct lws_protocols protocols[] = {
    { "http", http_callback, 0, 0, 0, NULL, 0 },           // HTTP first
    { "lws-minimal", WebSocketServer::callback, ... },     // WebSocket
    { NULL, NULL, 0, 0, 0, NULL, 0 }                       // Terminator
};
```

### 2. Frontend HTTP Ping (Every 14 Minutes)

The frontend now sends an HTTP request every 14 minutes to reset Render's idle timer:

```typescript
// frontend/src/hooks/useWebSocket.ts

// Derive HTTP URL from WebSocket URL
const HTTP_URL = WS_URL.replace('wss://', 'https://').replace('ws://', 'http://');

ws.onopen = () => {
    // ... existing code ...
    
    // HTTP keep-alive ping every 14 minutes
    const keepAliveInterval = setInterval(() => {
        fetch(HTTP_URL, { method: 'GET', mode: 'no-cors' }).catch(() => {
            // Ignore errors - just keeping service awake
        });
    }, 14 * 60 * 1000);  // 14 minutes (before 15-min timeout)
    
    (ws as any)._keepAliveInterval = keepAliveInterval;
};
```

### 3. Auto-Reconnect on Disconnect

As a safety net, the frontend automatically reconnects if the connection drops unexpectedly:

```typescript
ws.onclose = () => {
    // ... cleanup ...
    
    // Auto-reconnect after 3 seconds if we had a config
    if (configRef.current && wsRef.current === ws) {
        console.log('Connection lost, attempting to reconnect in 3 seconds...');
        setTimeout(() => {
            if (configRef.current) {
                connect(configRef.current);
            }
        }, 3000);
    }
};
```

### 4. 60-Minute Hard Session Limit

To prevent abuse of free compute resources, we implemented a server-side session timeout:

```cpp
// src/WebSocketServer.cpp

static constexpr int64_t MAX_CONNECTION_DURATION_MS = 60 * 60 * 1000;  // 60 minutes

void WebSocketServer::serverThread() {
    int64_t lastTimeoutCheck = 0;
    
    while (m_running) {
        lws_service(m_context, 50);
        
        auto now = /* current time in ms */;
        
        // Check every 10 seconds
        if (now - lastTimeoutCheck > 10000) {
            lastTimeoutCheck = now;
            
            for (auto& [id, client] : m_clients) {
                if (now - client.connectedAt >= MAX_CONNECTION_DURATION_MS) {
                    // Send timeout notification
                    sendToClient(id, R"({"type":"timeout","message":"Session expired..."})");
                    // Close connection
                    lws_close_reason(client.wsi, LWS_CLOSE_STATUS_NORMAL, ...);
                }
            }
        }
    }
}
```

The frontend handles this gracefully with a modal:

```typescript
case 'timeout': {
    console.log('Session timeout:', message.message);
    setSessionTimedOut(true);
    configRef.current = null;  // Prevent auto-reconnect
    break;
}
```

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         RENDER.COM FREE TIER                        │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    15-MINUTE IDLE MONITOR                    │   │
│  │                                                              │   │
│  │   Watches for: HTTP requests only                           │   │
│  │   Ignores: WebSocket frames, TCP keepalive                  │   │
│  │                                                              │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                              │                                      │
│                              ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    ORDER BOOK SERVER                         │   │
│  │                                                              │   │
│  │   Port 8080:                                                 │   │
│  │   ├── HTTP Protocol  → /health endpoint (keep-alive)        │   │
│  │   └── WS Protocol    → Real-time market data                │   │
│  │                                                              │   │
│  │   Session Manager:                                           │   │
│  │   └── 60-minute hard limit per connection                   │   │
│  │                                                              │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              │ WebSocket + HTTP
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         FRONTEND CLIENT                             │
│                                                                     │
│   ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐   │
│   │  WebSocket      │  │  HTTP Ping      │  │  Session Timer  │   │
│   │  Connection     │  │  (14 min)       │  │  (60 min max)   │   │
│   │                 │  │                 │  │                 │   │
│   │  • Tick data    │  │  GET /health    │  │  Countdown UI   │   │
│   │  • Ping/pong    │  │  Resets Render  │  │  Timeout modal  │   │
│   │  • Commands     │  │  idle timer     │  │                 │   │
│   └─────────────────┘  └─────────────────┘  └─────────────────┘   │
│                                                                     │
│   Auto-reconnect: 3 seconds after unexpected disconnect            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

## Files Modified

| File | Changes |
|------|---------|
| `src/WebSocketServer.cpp` | Added HTTP protocol, 60-min session timeout |
| `frontend/src/hooks/useWebSocket.ts` | HTTP keep-alive, auto-reconnect, timeout handling |
| `frontend/src/components/ControlPanel.tsx` | Session countdown timer UI |
| `frontend/src/pages/VisualizerPage.tsx` | Timeout modal |

## Alternative Solutions Considered

| Solution | Pros | Cons |
|----------|------|------|
| External pinger (Cloudflare Worker) | No client dependency | Extra infrastructure |
| Upgrade to paid Render plan | No timeout limit | Cost |
| Move to Railway/Fly.io | Different limits | Migration effort |
| **HTTP ping from frontend** ✅ | Simple, no extra infra | Requires client to be active |

## Summary

- **Problem:** Render.com free tier sleeps services after 15 minutes of no HTTP requests
- **Why WebSocket didn't help:** WS messages don't count as HTTP activity
- **Solution:** Frontend sends HTTP ping every 14 minutes + auto-reconnect + 60-min hard limit
- **Result:** Sessions stay alive indefinitely (up to 60-min limit) as long as the browser tab is open

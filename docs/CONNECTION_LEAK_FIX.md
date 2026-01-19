# WebSocket Connection Leak Fix

## Problem Summary

When refreshing the browser or during Vite Hot Module Replacement (HMR), multiple orphaned WebSocket connections accumulated on the server instead of being properly closed.

## Symptoms

Server log showed **5 connections dropping simultaneously** when only 1 tab was refreshed:

```
[Session 1] [DISCONNECT] IP: 192.168.0.154 | Duration: 690s | (active: 4)
[Session 5] [DISCONNECT] IP: 192.168.0.154 | Duration: 490s | (active: 3)
[Session 2] [DISCONNECT] IP: 192.168.0.154 | Duration: 689s | (active: 2)
[Session 4] [DISCONNECT] IP: 192.168.0.154 | Duration: 526s | (active: 1)
[Session 3] [DISCONNECT] IP: 192.168.0.154 | Duration: 687s | (active: 0)
[Session 6] [CONNECT] IP: 192.168.0.154 (active: 1)  ← New connection after refresh
```

**Expected:** 1 disconnect + 1 connect  
**Actual:** 5 disconnects + 1 connect

## Root Causes

### 1. Insufficient Connection Guard

The original code only prevented new connections if one was already `OPEN`:

```typescript
// ❌ Only checked OPEN state
const connect = useCallback((config: SimulationConfig) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) return;
    // ... creates new connection
});
```

**Problem:** If the WebSocket was in `CONNECTING` state (handshake in progress), a second connection would be created.

### 2. No Cleanup on Component Unmount

```typescript
// ❌ No cleanup function
useEffect(() => {
    if (hasConnected.current) return;
    hasConnected.current = true;
    connect(config);
}, [connect]);  // Missing return cleanup!
```

**Problem:** When:
- Vite HMR hot-reloads a component
- User navigates away
- Page refreshes

The old WebSocket connection was never closed because there was no cleanup function.

### 3. Stale `hasConnected` Ref

The `hasConnected.current = true` persisted across HMR reloads (refs survive React re-renders), but the actual WebSocket connection became stale. This caused inconsistent state.

### 4. Vite HMR Behavior

During development, saving a file triggers HMR:

```
File saved
  → Component unmounts (no cleanup ran!)
  → Component remounts
  → hasConnected.current is still true (ref persisted)
  → But wsRef.current might be null or CLOSED
  → connect() creates new connection
  → Old connection still open on server!
```

## Solution

### Fix 1: Guard Against CONNECTING State

```typescript
const connect = useCallback((config: SimulationConfig) => {
    // Guard: Don't create new connection if one is already open or connecting
    if (wsRef.current?.readyState === WebSocket.OPEN || 
        wsRef.current?.readyState === WebSocket.CONNECTING) {
      return;
    }
    
    // Close any existing connection that might be in CLOSING state
    if (wsRef.current) {
      wsRef.current.onclose = null;  // Prevent auto-reconnect
      wsRef.current.close();
      wsRef.current = null;
    }
    
    // ... rest of connection logic
});
```

### Fix 2: Cleanup on Unmount

```typescript
useEffect(() => {
    if (hasConnected.current) return;
    hasConnected.current = true;
    connect(config);
    
    // Cleanup: disconnect when component unmounts (page refresh, HMR, navigation)
    return () => {
      disconnect();
      hasConnected.current = false;
    };
}, [connect, disconnect]);
```

## WebSocket State Machine

Understanding the WebSocket states helps explain why this happened:

```
┌─────────────┐    new WebSocket()    ┌─────────────┐
│   (none)    │ ──────────────────────►│ CONNECTING  │
└─────────────┘                        └──────┬──────┘
                                              │
                              onopen          │ onerror
                                ▼             ▼
                        ┌─────────────┐  ┌─────────────┐
                        │    OPEN     │  │   CLOSED    │
                        └──────┬──────┘  └─────────────┘
                               │
                        close()│
                               ▼
                        ┌─────────────┐
                        │   CLOSING   │
                        └──────┬──────┘
                               │
                        onclose│
                               ▼
                        ┌─────────────┐
                        │   CLOSED    │
                        └─────────────┘

State Constants:
- WebSocket.CONNECTING = 0
- WebSocket.OPEN = 1
- WebSocket.CLOSING = 2
- WebSocket.CLOSED = 3
```

## Timeline: How Leak Occurred

```
T+0:00   Tab opened, Session 1 created
T+1:00   File saved, HMR triggers
           ├── Component unmounts (no cleanup!)
           ├── Session 1 still OPEN on server
           └── Component remounts, Session 2 created
T+2:00   File saved again, HMR triggers
           ├── Session 1 still OPEN
           ├── Session 2 still OPEN
           └── Session 3 created
... (pattern continues)

T+11:00  User refreshes page
           ├── Browser closes ALL sockets from this tab
           ├── Sessions 1,2,3,4,5 all disconnect
           └── Session 6 created (fresh start)
```

## Files Modified

| File | Changes |
|------|---------|
| `frontend/src/hooks/useWebSocket.ts` | Added CONNECTING state guard, cleanup of stale connections |
| `frontend/src/pages/VisualizerPage.tsx` | Added cleanup function in useEffect |

## Verification

After the fix, server logs should show:

```
# Normal page load
[Session 1] [CONNECT] IP: x.x.x.x (active: 1)

# Page refresh
[Session 1] [DISCONNECT] ... (active: 0)
[Session 2] [CONNECT] IP: x.x.x.x (active: 1)

# HMR reload (file save during dev)
[Session 2] [DISCONNECT] ... (active: 0)
[Session 3] [CONNECT] IP: x.x.x.x (active: 1)
```

**Expected pattern:** Always 1 active connection per browser tab.

## Best Practices for WebSocket in React

1. **Always return cleanup function from useEffect**
   ```typescript
   useEffect(() => {
       connect();
       return () => disconnect();  // ← Don't forget this!
   }, []);
   ```

2. **Guard all connection states, not just OPEN**
   ```typescript
   if (ws.readyState === WebSocket.OPEN || 
       ws.readyState === WebSocket.CONNECTING) {
       return;  // Don't create duplicate
   }
   ```

3. **Nullify event handlers before closing**
   ```typescript
   ws.onclose = null;  // Prevent callbacks from firing
   ws.close();
   ```

4. **Be aware of React StrictMode**
   - In development, StrictMode mounts components twice
   - This can cause double connections if not guarded properly

5. **Be aware of Vite HMR**
   - HMR preserves refs but remounts components
   - Always clean up side effects

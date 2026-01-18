# Deployment Guide - Render.com

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Render.com                           │
├─────────────────────────┬───────────────────────────────────┤
│   Backend (Web Service) │     Frontend (Static Site)        │
│   Docker Container      │     Vite Build                    │
│   C++ WebSocket Server  │     React SPA                     │
│   Port: 10000           │                                   │
│                         │                                   │
│   wss://market-pulse-   │     https://market-pulse.         │
│   api-xxxx.onrender.com │     onrender.com                  │
└─────────────────────────┴───────────────────────────────────┘
```

---

## Backend Deployment

### Service Type
**Web Service** (Docker)

### Settings
| Setting | Value |
|---------|-------|
| Name | `market-pulse-api` |
| Region | Oregon (or closest) |
| Branch | `main` |
| Root Directory | (leave empty) |
| Runtime | Docker |
| Instance Type | Free |

### How It Works
1. Render detects `Dockerfile` in repo root
2. Builds the Docker image (multi-stage: build + runtime)
3. Runs `CMD ["/app/orderbook", "--headless", "--auto-start"]`
4. Server listens on PORT (set by Render, default 10000)
5. Render provides HTTPS/WSS termination automatically

### Environment Variables
None required - PORT is set automatically by Render.

---

## Frontend Deployment

### Service Type
**Static Site**

### Settings
| Setting | Value |
|---------|-------|
| Name | `market-pulse` |
| Branch | `main` |
| Root Directory | `frontend` |
| Build Command | `npm install && npm run build` |
| Publish Directory | `dist` |

### Environment Variables
| Key | Value |
|-----|-------|
| `VITE_WS_URL` | `wss://market-pulse-api-xxxx.onrender.com` |

**Important:** Replace `xxxx` with your actual backend subdomain.

### How It Works
1. Render runs `npm install && npm run build` in `frontend/` directory
2. Vite reads `VITE_WS_URL` at build time and embeds it in the bundle
3. Built files in `dist/` are served as static assets
4. React app connects to WebSocket backend using the embedded URL

---

## Connection Flow

```
User Browser
    │
    ▼
https://market-pulse.onrender.com
    │
    │ (loads React app)
    ▼
React App (frontend)
    │
    │ WebSocket: wss://market-pulse-api-xxxx.onrender.com
    ▼
Backend Container (C++)
    │
    │ libwebsockets server on port 10000
    ▼
Market Simulation Engine
```

---

## Troubleshooting

### Backend won't start
1. Check Render logs for build errors
2. Ensure Dockerfile has correct runtime dependencies
3. Verify `--headless` flag is present (no TUI in container)

### WebSocket won't connect
1. Verify `VITE_WS_URL` uses `wss://` (not `ws://`)
2. Check that backend shows "Live" status on Render
3. Ensure backend binds to `0.0.0.0` not `localhost`

### Build fails with linking errors
See [DOCKER_CMAKE_ISSUES.md](./DOCKER_CMAKE_ISSUES.md) for detailed solutions.

### Free tier limitations
- Backend spins down after 15 mins of inactivity
- First request after idle takes ~30-60 seconds (cold start)
- 750 hours/month free across all services

---

## Local Development vs Production

| Aspect | Local | Production |
|--------|-------|------------|
| Backend | `./orderbook` (TUI) | Docker + `--headless` |
| WebSocket | `ws://localhost:8080` | `wss://....onrender.com` |
| Frontend | `npm run dev` | Static build |
| Hot Reload | Yes | No |

### Local Testing with Production Backend
Create `frontend/.env.local`:
```
VITE_WS_URL=wss://market-pulse-api-xxxx.onrender.com
```

Then run:
```bash
cd frontend
npm run dev
```

---

## Updating Deployment

### Backend Changes
1. Push to `main` branch
2. Render auto-deploys (or click "Deploy latest commit")
3. Wait for Docker build (~2-3 mins)

### Frontend Changes
1. Push to `main` branch
2. Render auto-deploys
3. Wait for npm build (~1 min)

### Environment Variable Changes
1. Go to Render Dashboard → Service → Environment
2. Update variable
3. Click "Save Changes"
4. Trigger manual deploy (env vars need rebuild for static sites)

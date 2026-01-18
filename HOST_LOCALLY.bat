@echo off
echo ===============================================
echo   MarketPulse - Local Hosting Setup
echo ===============================================
echo.
echo STEP 1: Start C++ Backend
echo    Open a NEW terminal and run:
echo    $env:Path = "C:\msys64\mingw64\bin;$env:Path"
echo    cd c:\Projects\order-book-visualizer\build\bin
echo    .\orderbook.exe --headless --auto-start
echo.
echo STEP 2: Expose Backend (in another terminal)
echo    ngrok http 8080
echo    Copy the https URL (e.g., https://abc123.ngrok-free.app)
echo.
echo STEP 3: Configure Frontend
echo    Create file: frontend\.env
echo    Add: VITE_WS_URL=wss://YOUR_NGROK_URL
echo    (Replace YOUR_NGROK_URL with the URL from step 2)
echo.
echo STEP 4: Start Frontend (in another terminal)
echo    cd c:\Projects\order-book-visualizer\frontend
echo    npm run dev -- --host
echo.
echo STEP 5: Expose Frontend (in another terminal)
echo    ngrok http 5173
echo    Share this URL with others!
echo.
echo ===============================================
echo   FREE ngrok allows 1 tunnel at a time on free plan
echo   Consider ngrok paid plan for multiple tunnels
echo   OR use Cloudflare Tunnel (free, unlimited)
echo ===============================================
pause

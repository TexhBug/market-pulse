import { useEffect, useRef, useState } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { Header } from '../components/Header';
import { CandlestickChart } from '../components/CandlestickChart';
import { OrderBook } from '../components/OrderBook';
import { ControlPanel } from '../components/ControlPanel';
import { StatsPanel } from '../components/StatsPanel';
import { TradeTicker } from '../components/TradeTicker';
import { useWebSocket, type SimulationConfig } from '../hooks/useWebSocket';
import type { Sentiment, Intensity } from '../types';

export function VisualizerPage() {
  const [searchParams] = useSearchParams();
  const navigate = useNavigate();
  const { connected, connecting, sessionTimedOut, sessionStartTime, orderBook, trades, stats, priceHistory, candleCache, currentCandles, sendCommand, requestCandles, connect, disconnect, latency } = useWebSocket();
  const hasConnected = useRef(false);
  const [priceZeroModal, setPriceZeroModal] = useState(false);
  
  // Loading state: show overlay until we receive first data
  const isLoading = !orderBook && !sessionTimedOut;
  const [loadingDots, setLoadingDots] = useState('');
  
  // Animate loading dots
  useEffect(() => {
    if (!isLoading) return;
    const interval = setInterval(() => {
      setLoadingDots(prev => prev.length >= 3 ? '' : prev + '.');
    }, 500);
    return () => clearInterval(interval);
  }, [isLoading]);

  // Parse config from URL params
  const config: SimulationConfig = {
    symbol: searchParams.get('symbol') || 'DEMO',
    price: Number(searchParams.get('price')) || 100,
    spread: Number(searchParams.get('spread')) || 0.05,
    sentiment: (searchParams.get('sentiment') as Sentiment) || 'NEUTRAL',
    intensity: (searchParams.get('intensity') as Intensity) || 'NORMAL',
    speed: Number(searchParams.get('speed')) || 1,
  };

  // Connect on mount, disconnect on unmount (important for HMR/hot reload)
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

  // Check for price reaching zero
  useEffect(() => {
    if (stats && stats.currentPrice <= 0 && !priceZeroModal) {
      setPriceZeroModal(true);
      disconnect(); // Stop the simulation
    }
  }, [stats, priceZeroModal, disconnect]);

  // Handle back to landing
  const handleBack = () => {
    disconnect();
    navigate('/');
  };

  // Handle refresh for price zero modal
  const handleRefresh = () => {
    window.location.href = '/';
  };

  return (
    <div className="min-h-screen bg-slate-900 text-white overflow-auto">
      {/* Loading Overlay - shown while waiting for backend to spin up */}
      {isLoading && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-slate-900">
          <div className="text-center max-w-md mx-4">
            {/* Animated spinner */}
            <div className="relative w-20 h-20 mx-auto mb-6">
              <div className="absolute inset-0 border-4 border-slate-700 rounded-full"></div>
              <div className="absolute inset-0 border-4 border-transparent border-t-green-500 rounded-full animate-spin"></div>
              <div className="absolute inset-2 border-4 border-transparent border-t-green-400 rounded-full animate-spin" style={{ animationDuration: '0.8s', animationDirection: 'reverse' }}></div>
            </div>
            
            <h2 className="text-xl font-semibold text-white mb-2">
              {connecting ? 'Connecting to Server' : 'Waiting for Data'}{loadingDots}
            </h2>
            
            <p className="text-slate-400 mb-4">
              {connecting 
                ? 'Establishing WebSocket connection...'
                : 'Server is spinning up and preparing market data...'}
            </p>
            
            <div className="bg-slate-800/50 border border-slate-700 rounded-lg p-4 text-sm text-slate-500">
              <p className="flex items-center justify-center gap-2">
                <svg className="w-4 h-4 text-amber-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                </svg>
                Free tier servers may take up to 60 seconds to wake up
              </p>
            </div>
            
            {/* Back button */}
            <button
              onClick={handleBack}
              className="mt-6 text-slate-500 hover:text-slate-300 text-sm underline transition-colors"
            >
              ← Back to configuration
            </button>
          </div>
        </div>
      )}

      {/* Session Timeout Modal */}
      {sessionTimedOut && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/80 backdrop-blur-sm">
          <div className="bg-slate-800 border border-amber-500/50 rounded-2xl p-8 max-w-md mx-4 shadow-2xl">
            <div className="text-center">
              <div className="w-16 h-16 mx-auto mb-4 rounded-full bg-amber-500/20 flex items-center justify-center">
                <svg className="w-8 h-8 text-amber-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" />
                </svg>
              </div>
              <h2 className="text-2xl font-bold text-amber-400 mb-2">Session Expired</h2>
              <p className="text-slate-300 mb-6">
                Your session has ended after 60 minutes to conserve server resources.
              </p>
              <p className="text-slate-400 text-sm mb-6">
                Click below to start a new session. Your configuration will be preserved.
              </p>
              <button
                onClick={handleRefresh}
                className="w-full bg-amber-500 hover:bg-amber-600 text-white font-semibold py-3 px-6 rounded-lg transition-colors"
              >
                Start New Session
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Price Zero Modal */}
      {priceZeroModal && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/80 backdrop-blur-sm">
          <div className="bg-slate-800 border border-red-500/50 rounded-2xl p-8 max-w-md mx-4 shadow-2xl">
            <div className="text-center">
              <div className="w-16 h-16 mx-auto mb-4 rounded-full bg-red-500/20 flex items-center justify-center">
                <svg className="w-8 h-8 text-red-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
                </svg>
              </div>
              <h2 className="text-2xl font-bold text-red-400 mb-2">Simulation Ended</h2>
              <p className="text-slate-300 mb-6">
                Price cannot go below $0. The simulation has been stopped.
              </p>
              <p className="text-slate-400 text-sm mb-6">
                Please refresh the page to start a new session with different parameters.
              </p>
              <button
                onClick={handleRefresh}
                className="w-full bg-red-500 hover:bg-red-600 text-white font-semibold py-3 px-6 rounded-lg transition-colors"
              >
                Reset / Refresh
              </button>
            </div>
          </div>
        </div>
      )}

      <Header connected={connected} symbol={config.symbol} onBack={handleBack} latency={latency} />
      
      <main className="p-2 sm:p-4 grid grid-cols-12 gap-2 sm:gap-4">
        {/* Left: Candlestick Chart with Volume */}
        <div className="col-span-12 lg:col-span-8" style={{ height: '500px' }}>
          <CandlestickChart 
            data={priceHistory} 
            stats={stats}
            candleCache={candleCache}
            currentCandles={currentCandles}
            requestCandles={requestCandles}
          />
        </div>
        
        {/* Right: Order Book */}
        <div className="col-span-12 lg:col-span-4" style={{ height: '500px' }}>
          <OrderBook data={orderBook} />
        </div>
      </main>

      {/* Bottom Panel */}
      <footer className="p-2 sm:p-4 pb-4 grid grid-cols-12 gap-2 sm:gap-4">
        {/* Controls */}
        <div className="col-span-12 md:col-span-4" style={{ minHeight: '150px' }}>
          <ControlPanel stats={stats} onCommand={sendCommand} sessionStartTime={sessionStartTime} />
        </div>
        
        {/* Stats */}
        <div className="col-span-6 md:col-span-4" style={{ minHeight: '150px' }}>
          <StatsPanel stats={stats} />
        </div>
        
        {/* Trade Ticker */}
        <div className="col-span-6 md:col-span-4" style={{ minHeight: '150px' }}>
          <TradeTicker trades={trades} />
        </div>
      </footer>
    </div>
  );
}

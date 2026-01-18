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
  const { connected, orderBook, trades, stats, priceHistory, candleCache, currentCandles, sendCommand, requestCandles, connect, disconnect, latency } = useWebSocket();
  const hasConnected = useRef(false);
  const [priceZeroModal, setPriceZeroModal] = useState(false);

  // Parse config from URL params
  const config: SimulationConfig = {
    symbol: searchParams.get('symbol') || 'DEMO',
    price: Number(searchParams.get('price')) || 100,
    spread: Number(searchParams.get('spread')) || 0.05,
    sentiment: (searchParams.get('sentiment') as Sentiment) || 'NEUTRAL',
    intensity: (searchParams.get('intensity') as Intensity) || 'NORMAL',
    speed: Number(searchParams.get('speed')) || 1,
  };

  // Connect on mount
  useEffect(() => {
    if (hasConnected.current) return;
    hasConnected.current = true;
    connect(config);
  }, [connect]);

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
          <ControlPanel stats={stats} onCommand={sendCommand} />
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

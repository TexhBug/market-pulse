import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import type { Sentiment, Intensity } from '../types';

const sentiments: { value: Sentiment; label: string; icon: string; description: string }[] = [
  { value: 'BULLISH', label: 'Bullish', icon: '📈', description: 'Prices trending UP' },
  { value: 'BEARISH', label: 'Bearish', icon: '📉', description: 'Prices trending DOWN' },
  { value: 'VOLATILE', label: 'Volatile', icon: '⚡', description: 'Large price swings' },
  { value: 'SIDEWAYS', label: 'Sideways', icon: '↔️', description: 'Range-bound, steady' },
  { value: 'CHOPPY', label: 'Choppy', icon: '🌊', description: 'Erratic movement' },
  { value: 'NEUTRAL', label: 'Neutral', icon: '⚖️', description: 'Balanced market' },
];

const intensities: { value: Intensity; label: string; multiplier: string }[] = [
  { value: 'MILD', label: 'Mild', multiplier: '0.3x' },
  { value: 'MODERATE', label: 'Moderate', multiplier: '0.6x' },
  { value: 'NORMAL', label: 'Normal', multiplier: '1.0x' },
  { value: 'AGGRESSIVE', label: 'Aggressive', multiplier: '1.5x' },
  { value: 'EXTREME', label: 'Extreme', multiplier: '2.5x' },
];

export function LandingPage() {
  const navigate = useNavigate();
  const [symbol, setSymbol] = useState('DEMO');
  const [price, setPrice] = useState(100);
  const [priceInput, setPriceInput] = useState('100');
  const [priceError, setPriceError] = useState<string | null>(null);
  const [spread, setSpread] = useState(0.05);
  const [sentiment, setSentiment] = useState<Sentiment>('NEUTRAL');
  const [intensity, setIntensity] = useState<Intensity>('NORMAL');
  const [speed, setSpeed] = useState(1);

  const handleStart = () => {
    // Validate price before starting
    const numVal = Number(priceInput);
    if (isNaN(numVal) || numVal < 100 || numVal > 500) {
      setPriceError(numVal < 100 ? 'Price cannot be less than $100' : 'Price cannot be more than $500');
      return;
    }
    
    // Pass config via URL params
    const params = new URLSearchParams({
      symbol,
      price: price.toString(),
      spread: spread.toString(),
      sentiment,
      intensity,
      speed: speed.toString(),
    });
    navigate(`/visualizer?${params.toString()}`);
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900 text-white">
      {/* Header */}
      <header className="py-8 text-center border-b border-slate-700/50">
        <div className="flex items-center justify-center gap-3 mb-2">
          <svg className="w-10 h-10 text-green-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 7h8m0 0v8m0-8l-8 8-4-4-6 6" />
          </svg>
          <h1 className="text-4xl font-bold bg-gradient-to-r from-green-400 to-blue-500 bg-clip-text text-transparent">
            MarketPulse
          </h1>
        </div>
        <p className="text-slate-400">Real-time market simulation with configurable parameters</p>
      </header>

      {/* Main Content */}
      <main className="max-w-4xl mx-auto py-12 px-6">
        <div className="bg-slate-800/50 rounded-2xl p-8 border border-slate-700/50 shadow-xl">
          <h2 className="text-2xl font-semibold mb-6 text-center">Configure Simulation</h2>

          {/* Symbol & Price */}
          <div className="grid grid-cols-2 gap-6 mb-8">
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">Symbol</label>
              <input
                type="text"
                value={symbol}
                onChange={(e) => setSymbol(e.target.value.toUpperCase())}
                className="w-full bg-slate-700 border border-slate-600 rounded-lg px-4 py-3 text-lg font-mono focus:outline-none focus:ring-2 focus:ring-green-500"
                maxLength={5}
              />
            </div>
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">Starting Price ($100-$500)</label>
              <div className="relative">
                <span className="absolute left-4 top-1/2 -translate-y-1/2 text-slate-400">$</span>
                <input
                  type="number"
                  value={priceInput}
                  onChange={(e) => {
                    setPriceInput(e.target.value);
                    setPriceError(null);
                  }}
                  onBlur={(e) => {
                    const numVal = Number(e.target.value);
                    if (isNaN(numVal) || e.target.value === '') {
                      setPriceError('Please enter a valid number');
                      return;
                    }
                    if (numVal < 100) {
                      setPriceError('Price cannot be less than $100');
                      return;
                    }
                    if (numVal > 500) {
                      setPriceError('Price cannot be more than $500');
                      return;
                    }
                    setPriceError(null);
                    setPrice(numVal);
                  }}
                  className={`w-full bg-slate-700 border rounded-lg pl-8 pr-4 py-3 text-lg font-mono focus:outline-none focus:ring-2 ${
                    priceError 
                      ? 'border-red-500 focus:ring-red-500' 
                      : 'border-slate-600 focus:ring-green-500'
                  }`}
                  min={100}
                  max={500}
                />
              </div>
              {priceError && (
                <p className="mt-1 text-sm text-red-400">{priceError}</p>
              )}
            </div>
          </div>

          {/* Spread & Speed */}
          <div className="grid grid-cols-2 gap-6 mb-8">
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">
                Spread: ${spread.toFixed(2)}
              </label>
              <input
                type="range"
                value={spread}
                onChange={(e) => setSpread(Number(e.target.value))}
                className="w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-green-500"
                min={0.05}
                max={0.25}
                step={0.05}
              />
              <div className="flex justify-between text-xs text-slate-500 mt-1">
                <span>$0.05</span>
                <span>$0.25</span>
              </div>
            </div>
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">
                Speed: {speed}x
              </label>
              <input
                type="range"
                value={speed}
                onChange={(e) => setSpeed(Number(e.target.value))}
                className="w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-blue-500"
                min={0.25}
                max={4}
                step={0.25}
              />
              <div className="flex justify-between text-xs text-slate-500 mt-1">
                <span>0.25x</span>
                <span>4x</span>
              </div>
            </div>
          </div>

          {/* Sentiment Selection */}
          <div className="mb-8">
            <label className="block text-sm font-medium text-slate-400 mb-3">Market Sentiment</label>
            <div className="grid grid-cols-3 gap-3">
              {sentiments.map((s) => (
                <button
                  key={s.value}
                  onClick={() => setSentiment(s.value)}
                  className={`p-4 rounded-xl border-2 transition-all ${
                    sentiment === s.value
                      ? 'border-green-500 bg-green-500/10'
                      : 'border-slate-600 hover:border-slate-500 bg-slate-700/30'
                  }`}
                >
                  <div className="text-2xl mb-1">{s.icon}</div>
                  <div className="font-medium">{s.label}</div>
                  <div className="text-xs text-slate-400">{s.description}</div>
                </button>
              ))}
            </div>
          </div>

          {/* Intensity Selection */}
          <div className="mb-8">
            <label className="block text-sm font-medium text-slate-400 mb-3">Intensity</label>
            <div className="flex gap-2">
              {intensities.map((i) => (
                <button
                  key={i.value}
                  onClick={() => setIntensity(i.value)}
                  className={`flex-1 py-3 px-4 rounded-lg border-2 transition-all ${
                    intensity === i.value
                      ? 'border-blue-500 bg-blue-500/10'
                      : 'border-slate-600 hover:border-slate-500 bg-slate-700/30'
                  }`}
                >
                  <div className="font-medium text-sm">{i.label}</div>
                  <div className="text-xs text-slate-400">{i.multiplier}</div>
                </button>
              ))}
            </div>
          </div>

          {/* Start Button */}
          <button
            onClick={handleStart}
            className="w-full py-4 bg-gradient-to-r from-green-500 to-emerald-600 hover:from-green-400 hover:to-emerald-500 rounded-xl font-bold text-lg transition-all transform hover:scale-[1.02] shadow-lg shadow-green-500/20"
          >
            🚀 Start Simulation
          </button>
        </div>

        {/* Features */}
        <div className="mt-12 grid grid-cols-3 gap-6 text-center">
          <div className="p-6 bg-slate-800/30 rounded-xl border border-slate-700/50">
            <div className="text-3xl mb-2">📈</div>
            <h3 className="font-semibold mb-1">Real-time Charts</h3>
            <p className="text-sm text-slate-400">Live price updates with smooth animations</p>
          </div>
          <div className="p-6 bg-slate-800/30 rounded-xl border border-slate-700/50">
            <div className="text-3xl mb-2">📊</div>
            <h3 className="font-semibold mb-1">Order Book</h3>
            <p className="text-sm text-slate-400">Bid/ask depth visualization</p>
          </div>
          <div className="p-6 bg-slate-800/30 rounded-xl border border-slate-700/50">
            <div className="text-3xl mb-2">⚡</div>
            <h3 className="font-semibold mb-1">Interactive</h3>
            <p className="text-sm text-slate-400">Change sentiment & intensity live</p>
          </div>
        </div>
      </main>

      {/* Footer */}
      <footer className="py-6 text-center text-slate-500 text-sm border-t border-slate-700/50">
        MarketPulse • Built with React, TypeScript, and Recharts
      </footer>
    </div>
  );
}

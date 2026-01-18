import { useState } from 'react';
import { TrendingUp, Play, Settings } from 'lucide-react';
import type { Sentiment, Intensity } from '../types';
import { SENTIMENTS, INTENSITIES } from '../types';

interface SimulationConfig {
  symbol: string;
  price: number;
  spread: number;
  sentiment: Sentiment;
  intensity: Intensity;
  speed: number;
}

interface LandingPageProps {
  onStart: (config: SimulationConfig) => void;
  connecting: boolean;
}

export function LandingPage({ onStart, connecting }: LandingPageProps) {
  const [config, setConfig] = useState<SimulationConfig>({
    symbol: 'AAPL',
    price: 180,
    spread: 0.05,
    sentiment: 'NEUTRAL',
    intensity: 'NORMAL',
    speed: 1.0,
  });

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onStart(config);
  };

  const presets = [
    { name: 'Apple', symbol: 'AAPL', price: 180 },
    { name: 'Microsoft', symbol: 'MSFT', price: 400 },
    { name: 'Google', symbol: 'GOOGL', price: 175 },
    { name: 'Tesla', symbol: 'TSLA', price: 250 },
    { name: 'Amazon', symbol: 'AMZN', price: 185 },
    { name: 'NVIDIA', symbol: 'NVDA', price: 140 },
  ];

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900 flex items-center justify-center p-4">
      <div className="w-full max-w-2xl">
        {/* Header */}
        <div className="text-center mb-8">
          <div className="flex items-center justify-center gap-3 mb-4">
            <div className="p-3 bg-blue-600 rounded-xl">
              <TrendingUp className="w-8 h-8 text-white" />
            </div>
            <h1 className="text-4xl font-bold text-white">Order Book Visualizer</h1>
          </div>
          <p className="text-slate-400 text-lg">
            Real-time market simulation with customizable parameters
          </p>
        </div>

        {/* Configuration Form */}
        <form onSubmit={handleSubmit} className="bg-slate-800/50 backdrop-blur rounded-2xl p-8 border border-slate-700">
          <div className="flex items-center gap-2 mb-6 text-slate-300">
            <Settings className="w-5 h-5" />
            <span className="font-semibold">Simulation Settings</span>
          </div>

          {/* Quick Presets */}
          <div className="mb-6">
            <label className="block text-sm font-medium text-slate-400 mb-2">Quick Presets</label>
            <div className="grid grid-cols-3 sm:grid-cols-6 gap-2">
              {presets.map((preset) => (
                <button
                  key={preset.symbol}
                  type="button"
                  onClick={() => setConfig(c => ({ ...c, symbol: preset.symbol, price: preset.price }))}
                  className={`px-3 py-2 rounded-lg text-sm font-medium transition-all ${
                    config.symbol === preset.symbol
                      ? 'bg-blue-600 text-white'
                      : 'bg-slate-700 text-slate-300 hover:bg-slate-600'
                  }`}
                >
                  {preset.symbol}
                </button>
              ))}
            </div>
          </div>

          {/* Symbol and Price Row */}
          <div className="grid grid-cols-2 gap-4 mb-6">
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">Symbol</label>
              <input
                type="text"
                value={config.symbol}
                onChange={(e) => setConfig(c => ({ ...c, symbol: e.target.value.toUpperCase() }))}
                className="w-full px-4 py-3 bg-slate-700 border border-slate-600 rounded-lg text-white font-mono text-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
                maxLength={5}
                placeholder="AAPL"
              />
            </div>
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">
                Base Price: <span className="text-white">${config.price.toFixed(2)}</span>
              </label>
              <input
                type="range"
                min="100"
                max="500"
                step="5"
                value={config.price}
                onChange={(e) => setConfig(c => ({ ...c, price: Number(e.target.value) }))}
                className="w-full h-3 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-blue-500"
              />
              <div className="flex justify-between text-xs text-slate-500 mt-1">
                <span>$100</span>
                <span>$500</span>
              </div>
            </div>
          </div>

          {/* Spread and Speed Row */}
          <div className="grid grid-cols-2 gap-4 mb-6">
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">
                Spread: <span className="text-white">${config.spread.toFixed(2)}</span>
              </label>
              <input
                type="range"
                min="0.05"
                max="0.25"
                step="0.05"
                value={config.spread}
                onChange={(e) => setConfig(c => ({ ...c, spread: Number(e.target.value) }))}
                className="w-full h-3 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-green-500"
              />
              <div className="flex justify-between text-xs text-slate-500 mt-1">
                <span>$0.05 (tight)</span>
                <span>$0.25 (wide)</span>
              </div>
            </div>
            <div>
              <label className="block text-sm font-medium text-slate-400 mb-2">
                Speed: <span className="text-white">{config.speed.toFixed(2)}x</span>
              </label>
              <input
                type="range"
                min="0.25"
                max="4"
                step="0.25"
                value={config.speed}
                onChange={(e) => setConfig(c => ({ ...c, speed: Number(e.target.value) }))}
                className="w-full h-3 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-yellow-500"
              />
              <div className="flex justify-between text-xs text-slate-500 mt-1">
                <span>0.25x (slow)</span>
                <span>4x (fast)</span>
              </div>
            </div>
          </div>

          {/* Sentiment Selection */}
          <div className="mb-6">
            <label className="block text-sm font-medium text-slate-400 mb-2">Market Sentiment</label>
            <div className="grid grid-cols-3 sm:grid-cols-6 gap-2">
              {SENTIMENTS.map((s) => (
                <button
                  key={s.id}
                  type="button"
                  onClick={() => setConfig(c => ({ ...c, sentiment: s.id }))}
                  className={`px-3 py-2 rounded-lg text-sm font-medium transition-all ${
                    config.sentiment === s.id
                      ? s.color + ' text-white'
                      : 'bg-slate-700 text-slate-300 hover:bg-slate-600'
                  }`}
                >
                  {s.label}
                </button>
              ))}
            </div>
          </div>

          {/* Intensity Selection */}
          <div className="mb-8">
            <label className="block text-sm font-medium text-slate-400 mb-2">Intensity Level</label>
            <div className="grid grid-cols-5 gap-2">
              {INTENSITIES.map((i) => (
                <button
                  key={i.id}
                  type="button"
                  onClick={() => setConfig(c => ({ ...c, intensity: i.id }))}
                  className={`px-3 py-2 rounded-lg text-sm font-medium transition-all ${
                    config.intensity === i.id
                      ? 'bg-purple-600 text-white'
                      : 'bg-slate-700 text-slate-300 hover:bg-slate-600'
                  }`}
                >
                  {i.label}
                </button>
              ))}
            </div>
          </div>

          {/* Start Button */}
          <button
            type="submit"
            disabled={connecting}
            className={`w-full py-4 rounded-xl font-semibold text-lg flex items-center justify-center gap-3 transition-all ${
              connecting
                ? 'bg-slate-600 text-slate-400 cursor-wait'
                : 'bg-gradient-to-r from-blue-600 to-blue-500 hover:from-blue-500 hover:to-blue-400 text-white shadow-lg shadow-blue-500/25'
            }`}
          >
            {connecting ? (
              <>
                <div className="w-5 h-5 border-2 border-slate-400 border-t-transparent rounded-full animate-spin" />
                Connecting to Backend...
              </>
            ) : (
              <>
                <Play className="w-5 h-5" />
                Start Simulation
              </>
            )}
          </button>

          {/* Backend Status Hint */}
          <p className="text-center text-slate-500 text-sm mt-4">
            Make sure the C++ backend is running on port 8080
          </p>
        </form>

        {/* Footer */}
        <p className="text-center text-slate-600 text-sm mt-6">
          Multithreaded Order Book Visualizer • C++ Backend + React Frontend
        </p>
      </div>
    </div>
  );
}

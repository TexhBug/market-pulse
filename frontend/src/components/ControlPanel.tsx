import { Pause, Play, RotateCcw, Zap } from 'lucide-react';
import type { MarketStats, WSCommand, Sentiment, Intensity } from '../types';
import { SENTIMENTS, INTENSITIES } from '../types';

interface ControlPanelProps {
  stats: MarketStats | null;
  onCommand: (command: WSCommand) => void;
}

export function ControlPanel({ stats, onCommand }: ControlPanelProps) {
  const currentSentiment = stats?.sentiment || 'NEUTRAL';
  const currentIntensity = stats?.intensity || 'NORMAL';
  const spread = stats?.spread || 0.05;
  const speed = stats?.speed || 1.0;
  const paused = stats?.paused || false;
  const newsShockEnabled = stats?.newsShockEnabled || false;
  const newsShockCooldown = stats?.newsShockCooldown || false;
  const newsShockCooldownRemaining = stats?.newsShockCooldownRemaining || 0;
  const newsShockActiveRemaining = stats?.newsShockActiveRemaining || 0;

  const handleSentimentChange = (sentiment: Sentiment) => {
    onCommand({ type: 'sentiment', value: sentiment });
  };

  const handleIntensityChange = (intensity: Intensity) => {
    onCommand({ type: 'intensity', value: intensity });
  };

  const handleSpreadChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    onCommand({ type: 'spread', value: parseFloat(e.target.value) });
  };

  const handleSpeedChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    onCommand({ type: 'speed', value: parseFloat(e.target.value) });
  };

  const handlePauseToggle = () => {
    onCommand({ type: 'pause', value: !paused });
  };

  const handleReset = () => {
    onCommand({ type: 'reset', value: true });
  };

  const handleNewsShockToggle = () => {
    onCommand({ type: 'newsShock', value: !newsShockEnabled });
  };

  return (
    <div className="bg-slate-800 rounded-lg p-2 sm:p-4 h-full flex flex-col overflow-hidden">
      <h2 className="text-sm sm:text-lg font-semibold text-white mb-2 sm:mb-4 truncate">Market Controls</h2>

      {/* Sentiment */}
      <div className="mb-2 sm:mb-4 flex-shrink-0">
        <label className="text-xs sm:text-sm text-slate-400 mb-1 sm:mb-2 block">Sentiment</label>
        <div className="grid grid-cols-3 gap-1 sm:gap-2">
          {SENTIMENTS.map(({ id, label, color }) => (
            <button
              key={id}
              onClick={() => handleSentimentChange(id)}
              className={`px-1 sm:px-3 py-1 sm:py-2 rounded text-xs sm:text-sm font-medium transition-all truncate ${
                currentSentiment === id
                  ? `${color} text-white ring-2 ring-white/30`
                  : 'bg-slate-700 text-slate-300 hover:bg-slate-600'
              }`}
            >
              {label}
            </button>
          ))}
        </div>
      </div>

      {/* Intensity */}
      <div className="mb-2 sm:mb-4 flex-shrink-0">
        <label className="text-xs sm:text-sm text-slate-400 mb-1 sm:mb-2 block">Intensity</label>
        <div className="grid grid-cols-5 gap-0.5 sm:gap-1">
          {INTENSITIES.map(({ id, label }) => (
            <button
              key={id}
              onClick={() => handleIntensityChange(id)}
              className={`px-1 py-1 sm:py-2 rounded text-[10px] sm:text-xs font-medium transition-all truncate ${
                currentIntensity === id
                  ? 'bg-blue-600 text-white ring-2 ring-white/30'
                  : 'bg-slate-700 text-slate-300 hover:bg-slate-600'
              }`}
            >
              {label}
            </button>
          ))}
        </div>
      </div>

      {/* Sliders */}
      <div className="grid grid-cols-2 gap-2 sm:gap-4 mb-2 sm:mb-4 flex-shrink-0">
        <div className="min-w-0">
          <label className="text-xs sm:text-sm text-slate-400 flex justify-between">
            <span className="truncate">Spread</span>
            <span className="text-yellow-400 ml-1">${spread.toFixed(2)}</span>
          </label>
          <input
            type="range"
            min="0.05"
            max="0.25"
            step="0.05"
            value={spread}
            onChange={handleSpreadChange}
            className="w-full mt-1 accent-yellow-400"
          />
        </div>
        <div className="min-w-0">
          <label className="text-xs sm:text-sm text-slate-400 flex justify-between">
            <span className="truncate">Speed</span>
            <span className="text-cyan-400 ml-1">{speed.toFixed(2)}x</span>
          </label>
          <input
            type="range"
            min="0.25"
            max="2"
            step="0.25"
            value={Math.min(speed, 2)}
            onChange={handleSpeedChange}
            className="w-full mt-1 accent-cyan-400"
          />
        </div>
      </div>

      {/* News Shock Toggle */}
      <div className="mb-2 sm:mb-4 flex-shrink-0">
        <button
          onClick={handleNewsShockToggle}
          disabled={newsShockCooldown || newsShockEnabled}
          className={`w-full flex items-center justify-center gap-2 py-2 rounded text-sm font-medium transition-all ${
            newsShockEnabled
              ? 'bg-amber-600 text-white ring-2 ring-amber-400/50 animate-pulse cursor-not-allowed'
              : newsShockCooldown
                ? 'bg-slate-600 text-slate-400 cursor-not-allowed'
                : 'bg-slate-700 text-slate-300 hover:bg-amber-600 hover:text-white'
          }`}
        >
          <Zap size={16} className={newsShockEnabled ? 'text-yellow-300' : ''} />
          <span>News Shock</span>
          {newsShockEnabled ? (
            <span className="text-xs px-1.5 py-0.5 rounded bg-amber-500 text-white">
              {newsShockActiveRemaining}s
            </span>
          ) : newsShockCooldown ? (
            <span className="text-xs px-1.5 py-0.5 rounded bg-slate-500 text-slate-300">
              {newsShockCooldownRemaining}s
            </span>
          ) : (
            <span className="text-xs px-1.5 py-0.5 rounded bg-slate-600 text-slate-400">
              OFF
            </span>
          )}
        </button>
        <p className="text-[10px] text-slate-500 mt-1 text-center">
          {newsShockEnabled 
            ? '⚡ Shocks active! Auto-disables soon...'
            : newsShockCooldown 
              ? `⏳ Cooldown: ${newsShockCooldownRemaining}s remaining`
              : 'Click to trigger 5s of price shocks'}
        </p>
      </div>

      {/* Action Buttons */}
      <div className="flex gap-1 sm:gap-2 mt-auto flex-shrink-0">
        <button
          onClick={handlePauseToggle}
          className={`flex-1 flex items-center justify-center gap-1 sm:gap-2 py-1.5 sm:py-2 rounded text-sm font-medium transition-all ${
            paused
              ? 'bg-green-600 hover:bg-green-700 text-white'
              : 'bg-yellow-600 hover:bg-yellow-700 text-white'
          }`}
        >
          {paused ? <Play size={16} /> : <Pause size={16} />}
          <span className="hidden sm:inline">{paused ? 'Resume' : 'Pause'}</span>
        </button>
        <button
          onClick={handleReset}
          className="flex items-center justify-center gap-1 sm:gap-2 px-2 sm:px-4 py-1.5 sm:py-2 rounded text-sm font-medium bg-slate-700 hover:bg-slate-600 text-slate-200 transition-all"
        >
          <RotateCcw size={16} />
          <span className="hidden sm:inline">Reset</span>
        </button>
      </div>
    </div>
  );
}

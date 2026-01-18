import { useState } from 'react';
import type { Trade } from '../types';
import { EducationalTooltip } from './EducationalTooltip';
import { useEducationalMode } from '../context/EducationalModeContext';

interface TradeTickerProps {
  trades: Trade[];
}

const DISPLAY_OPTIONS = [8, 10, 15, 20] as const;

export function TradeTicker({ trades }: TradeTickerProps) {
  const [maxDisplay, setMaxDisplay] = useState<number>(8);
  const { enabled: educationalMode } = useEducationalMode();
  
  // Sort by timestamp descending (newest first) and take top N
  const displayedTrades = [...trades]
    .sort((a, b) => b.timestamp - a.timestamp)
    .slice(0, maxDisplay);
  
  return (
    <div className="bg-slate-800 rounded-lg p-2 sm:p-4 h-full flex flex-col overflow-hidden">
      <div className="flex items-center justify-between mb-2 sm:mb-3 flex-shrink-0">
        <EducationalTooltip
          enabled={educationalMode}
          content="Time & Sales (Trade Ticker) shows a real-time stream of executed trades. Each row represents a completed transaction between a buyer and seller."
          position="bottom"
        >
          <h2 className="text-sm sm:text-lg font-semibold text-white truncate">Recent Trades</h2>
        </EducationalTooltip>
        <select
          value={maxDisplay}
          onChange={(e) => setMaxDisplay(Number(e.target.value))}
          className="bg-slate-700 text-white text-xs sm:text-sm px-2 py-1 rounded border border-slate-600 focus:outline-none focus:ring-1 focus:ring-blue-500"
        >
          {DISPLAY_OPTIONS.map((n) => (
            <option key={n} value={n}>
              {n} trades
            </option>
          ))}
        </select>
      </div>

      {/* Header */}
      <EducationalTooltip 
        enabled={educationalMode} 
        content="Side: BUY (green) or SELL (red) indicates who initiated the trade. Price: execution price. Size: quantity traded. Time: when it happened."
        position="bottom"
      >
        <div className="grid grid-cols-4 text-[10px] sm:text-xs text-slate-400 mb-1 sm:mb-2 px-1 sm:px-2 flex-shrink-0">
          <span>Side</span>
          <span className="text-right">Price</span>
          <span className="text-right">Size</span>
          <span className="text-right">Time</span>
        </div>
      </EducationalTooltip>

      {/* Trades List */}
      <div className="flex-1 overflow-auto space-y-0.5 sm:space-y-1 min-h-0">
        {displayedTrades.length === 0 ? (
          <div className="text-center text-slate-400 py-2 sm:py-4 text-xs sm:text-sm">No trades yet</div>
        ) : (
          displayedTrades.map((trade) => (
            <div
              key={trade.id}
              className={`grid grid-cols-4 text-xs sm:text-sm py-1 sm:py-1.5 px-1 sm:px-2 rounded ${
                trade.side === 'BUY' 
                  ? 'bg-green-500/10 hover:bg-green-500/20' 
                  : 'bg-red-500/10 hover:bg-red-500/20'
              }`}
            >
              <span className={`font-semibold truncate ${
                trade.side === 'BUY' ? 'text-green-400' : 'text-red-400'
              }`}>
                {trade.side}
              </span>
              <span className={`text-right font-mono truncate ${
                trade.side === 'BUY' ? 'text-green-300' : 'text-red-300'
              }`}>
                ${trade.price.toFixed(2)}
              </span>
              <span className="text-right text-slate-300 font-mono truncate">
                {trade.quantity}
              </span>
              <span className="text-right text-slate-500 text-[10px] sm:text-xs truncate">
                {new Date(trade.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })}
              </span>
            </div>
          ))
        )}
      </div>
    </div>
  );
}

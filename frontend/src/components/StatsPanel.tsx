import { Activity, TrendingUp, TrendingDown, Zap } from 'lucide-react';
import type { MarketStats } from '../types';
import { EducationalTooltip } from './EducationalTooltip';
import { useEducationalMode } from '../context/EducationalModeContext';

interface StatsPanelProps {
  stats: MarketStats | null;
}

export function StatsPanel({ stats }: StatsPanelProps) {
  const { enabled: educationalMode } = useEducationalMode();
  
  if (!stats) {
    return (
      <div className="bg-slate-800 rounded-lg p-2 sm:p-4 h-full flex items-center justify-center overflow-hidden">
        <span className="text-slate-400 text-sm">Waiting for data...</span>
      </div>
    );
  }

  const getSentimentIcon = () => {
    switch (stats.sentiment) {
      case 'BULLISH': return <TrendingUp className="text-green-400 flex-shrink-0" size={16} />;
      case 'BEARISH': return <TrendingDown className="text-red-400 flex-shrink-0" size={16} />;
      case 'VOLATILE': return <Zap className="text-purple-400 flex-shrink-0" size={16} />;
      default: return <Activity className="text-slate-400 flex-shrink-0" size={16} />;
    }
  };

  const getSentimentColor = () => {
    switch (stats.sentiment) {
      case 'BULLISH': return 'text-green-400';
      case 'BEARISH': return 'text-red-400';
      case 'VOLATILE': return 'text-purple-400';
      case 'SIDEWAYS': return 'text-blue-400';
      case 'CHOPPY': return 'text-orange-400';
      default: return 'text-slate-400';
    }
  };

  return (
    <div className="bg-slate-800 rounded-lg p-2 sm:p-4 h-full flex flex-col overflow-hidden">
      <EducationalTooltip
        enabled={educationalMode}
        content="Statistics panel shows real-time market metrics including order counts, trading volume, and current market conditions."
        position="bottom"
      >
        <h2 className="text-sm sm:text-lg font-semibold text-white mb-2 sm:mb-4 truncate flex-shrink-0">Statistics</h2>
      </EducationalTooltip>

      {/* Stats Grid */}
      <div className="grid grid-cols-2 gap-1.5 sm:gap-3 mb-2 sm:mb-4 flex-shrink-0">
        <EducationalTooltip
          enabled={educationalMode}
          content="Total number of orders (both buy and sell) placed in this simulation session."
          position="bottom"
        >
          <div className="bg-slate-700/50 rounded-lg p-1.5 sm:p-3 min-w-0">
            <div className="text-[10px] sm:text-xs text-slate-400 mb-0.5 sm:mb-1 truncate">Orders</div>
            <div className="text-sm sm:text-xl font-bold text-white truncate">{stats.totalOrders.toLocaleString()}</div>
          </div>
        </EducationalTooltip>
        <EducationalTooltip
          enabled={educationalMode}
          content="Total number of executed trades. A trade occurs when a buy order matches with a sell order."
          position="bottom"
        >
          <div className="bg-slate-700/50 rounded-lg p-1.5 sm:p-3 min-w-0">
            <div className="text-[10px] sm:text-xs text-slate-400 mb-0.5 sm:mb-1 truncate">Trades</div>
            <div className="text-sm sm:text-xl font-bold text-white truncate">{stats.totalTrades.toLocaleString()}</div>
          </div>
        </EducationalTooltip>
        <EducationalTooltip
          enabled={educationalMode}
          content="Total volume of shares/units traded. Higher volume indicates more market activity and typically better liquidity."
          position="top"
        >
          <div className="bg-slate-700/50 rounded-lg p-1.5 sm:p-3 min-w-0">
            <div className="text-[10px] sm:text-xs text-slate-400 mb-0.5 sm:mb-1 truncate">Volume</div>
            <div className="text-sm sm:text-xl font-bold text-white truncate">{stats.totalVolume.toLocaleString()}</div>
          </div>
        </EducationalTooltip>
        <EducationalTooltip
          enabled={educationalMode}
          content="Percentage of market orders vs limit orders. Market orders execute immediately at current price, while limit orders wait for a specific price."
          position="top"
        >
          <div className="bg-slate-700/50 rounded-lg p-1.5 sm:p-3 min-w-0">
            <div className="text-[10px] sm:text-xs text-slate-400 mb-0.5 sm:mb-1 truncate">Mkt Orders</div>
            <div className="text-sm sm:text-xl font-bold text-white truncate">{stats.marketOrderPct}%</div>
          </div>
        </EducationalTooltip>
      </div>

      {/* Order Type Ratio Bar */}
      <EducationalTooltip
        enabled={educationalMode}
        content="Visual breakdown of order types. Market orders (red) execute immediately but may have slippage. Limit orders (green) only execute at specified price or better."
        position="top"
      >
        <div className="mb-2 sm:mb-4 flex-shrink-0">
          <div className="flex justify-between text-[10px] sm:text-xs text-slate-400 mb-1">
            <span className="truncate">Mkt: {stats.marketOrderPct}%</span>
            <span className="truncate">Lmt: {100 - stats.marketOrderPct}%</span>
          </div>
          <div className="h-1.5 sm:h-2 bg-slate-700 rounded-full overflow-hidden flex">
            <div 
              className="bg-red-500 transition-all duration-300" 
              style={{ width: `${stats.marketOrderPct}%` }}
            />
            <div 
              className="bg-green-500 transition-all duration-300" 
              style={{ width: `${100 - stats.marketOrderPct}%` }}
            />
          </div>
        </div>
      </EducationalTooltip>

      {/* Market Condition */}
      <EducationalTooltip
        enabled={educationalMode}
        content="Current simulated market sentiment and intensity. BULLISH = upward trend, BEARISH = downward trend, VOLATILE = high price swings, SIDEWAYS = stable, CHOPPY = unpredictable movement."
        position="top"
      >
        <div className="mt-auto bg-slate-700/50 rounded-lg p-1.5 sm:p-3 flex-shrink-0">
          <div className="text-[10px] sm:text-xs text-slate-400 mb-1 sm:mb-2">Market Condition</div>
          <div className="flex items-center gap-1 sm:gap-2 flex-wrap">
            {getSentimentIcon()}
            <span className={`text-xs sm:text-sm font-semibold truncate ${getSentimentColor()}`}>
              {stats.sentiment}
            </span>
            <span className="text-slate-400 hidden sm:inline">|</span>
            <span className="text-xs sm:text-sm text-slate-300 truncate">{stats.intensity}</span>
          </div>
          {stats.paused && (
            <div className="mt-1 sm:mt-2 text-yellow-400 text-xs sm:text-sm font-semibold animate-pulse">
              ⏸ PAUSED
            </div>
          )}
        </div>
      </EducationalTooltip>
    </div>
  );
}

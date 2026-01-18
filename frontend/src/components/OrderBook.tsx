import type { OrderBookData } from '../types';
import { EducationalTooltip } from './EducationalTooltip';
import { useEducationalMode } from '../context/EducationalModeContext';

interface OrderBookProps {
  data: OrderBookData | null;
}

export function OrderBook({ data }: OrderBookProps) {
  const { enabled: educationalMode } = useEducationalMode();
  
  if (!data) {
    return (
      <div className="bg-slate-800 rounded-lg p-2 sm:p-4 h-full flex items-center justify-center overflow-hidden">
        <span className="text-slate-400 text-sm">Waiting for data...</span>
      </div>
    );
  }

  const maxBidQty = Math.max(...data.bids.map(b => b.quantity), 1);
  const maxAskQty = Math.max(...data.asks.map(a => a.quantity), 1);
  const maxQty = Math.max(maxBidQty, maxAskQty);
  
  // Calculate totals
  const totalBidQty = data.bids.slice(0, 10).reduce((sum, b) => sum + b.quantity, 0);
  const totalAskQty = data.asks.slice(0, 10).reduce((sum, a) => sum + a.quantity, 0);
  
  // Use actual first elements for best bid/ask to ensure consistency with table
  const displayBestBid = data.bids[0]?.price ?? data.bestBid;
  const displayBestAsk = data.asks[0]?.price ?? data.bestAsk;
  const displaySpread = displayBestAsk - displayBestBid;

  return (
    <div className="bg-slate-800 rounded-lg p-2 sm:p-4 h-full flex flex-col overflow-hidden">
      {/* Header */}
      <div className="flex justify-between items-center mb-2 sm:mb-3 flex-shrink-0">
        <EducationalTooltip
          enabled={educationalMode}
          content="The Order Book shows all pending buy and sell orders at different price levels. It reveals market depth and liquidity."
          position="bottom"
        >
          <h2 className="text-sm sm:text-lg font-semibold text-white truncate">Order Book</h2>
        </EducationalTooltip>
        <EducationalTooltip
          enabled={educationalMode}
          content="The Spread is the difference between the best ask (lowest sell price) and best bid (highest buy price). A smaller spread indicates higher liquidity."
          position="bottom"
        >
          <div className="text-xs sm:text-sm flex-shrink-0">
            <span className="text-slate-400">Spread: </span>
            <span className="text-yellow-400 font-semibold">${displaySpread.toFixed(2)}</span>
          </div>
        </EducationalTooltip>
      </div>

      {/* ASK Section Header */}
      <div className="flex justify-between items-center px-1 sm:px-2 mb-1 flex-shrink-0">
        <EducationalTooltip
          enabled={educationalMode}
          content="Asks are sell orders - people wanting to sell at these prices. The lowest ask (Best Ask) is at the bottom, closest to the spread."
          position="right"
        >
          <div className="flex items-center gap-2">
            <span className="text-red-400 text-xs font-semibold uppercase tracking-wide">Ask</span>
            <span className="text-red-400/60 text-[10px]">(Sell Orders)</span>
          </div>
        </EducationalTooltip>
        <EducationalTooltip
          enabled={educationalMode}
          content="Total volume of sell orders shown in this section."
          position="left"
        >
          <span className="text-red-400/80 text-[10px] sm:text-xs font-mono">{totalAskQty.toLocaleString()}</span>
        </EducationalTooltip>
      </div>
      
      {/* Ask Column Headers */}
      <EducationalTooltip 
        enabled={educationalMode} 
        content="Price: the sell price. Size: quantity at this price. Total: cumulative volume from best price to this level."
        position="bottom"
      >
        <div className="grid grid-cols-3 text-[10px] text-slate-500 mb-1 px-1 sm:px-2 flex-shrink-0 border-b border-slate-700 pb-1">
          <span className="text-left">Price</span>
          <span className="text-right">Size</span>
          <span className="text-right">Total</span>
        </div>
      </EducationalTooltip>

      {/* Asks (Sells) - Reversed so lowest ask is at bottom */}
      <div className="flex-1 overflow-hidden flex flex-col min-h-0">
        <div className="flex-1 overflow-auto flex flex-col-reverse min-h-0">
          {data.asks.slice(0, 10).map((ask, idx) => {
            const depthPct = (ask.quantity / maxQty) * 100;
            const cumulative = data.asks.slice(0, idx + 1).reduce((sum, a) => sum + a.quantity, 0);
            return (
              <div
                key={`ask-${idx}-${ask.price}`}
                className="relative grid grid-cols-3 text-xs sm:text-sm py-0.5 sm:py-1 px-1 sm:px-2 hover:bg-red-500/10 transition-colors"
              >
                <div
                  className="absolute right-0 top-0 bottom-0 bg-gradient-to-l from-red-500/30 to-transparent"
                  style={{ width: `${depthPct}%` }}
                />
                <span className="text-red-400 font-mono relative z-10 truncate">${ask.price.toFixed(2)}</span>
                <span className="text-right text-red-300/80 font-mono relative z-10 truncate">{ask.quantity.toLocaleString()}</span>
                <span className="text-right text-red-400/50 font-mono relative z-10 truncate">{cumulative.toLocaleString()}</span>
              </div>
            );
          })}
        </div>

        {/* Spread Divider - Enhanced */}
        <EducationalTooltip
          enabled={educationalMode}
          content="This shows the current best prices. Best Bid is the highest price buyers will pay, Best Ask is the lowest price sellers will accept. The spread between them represents the cost of immediate execution."
          position="right"
        >
          <div className="py-2 sm:py-3 px-2 sm:px-3 bg-slate-900/80 my-1 sm:my-2 rounded-lg border border-slate-600/50 flex-shrink-0">
            <div className="flex justify-between items-center">
              <div className="flex flex-col items-start">
                <span className="text-[10px] text-green-400/60 uppercase">Best Bid</span>
                <span className="text-base sm:text-xl font-bold text-green-400">${displayBestBid.toFixed(2)}</span>
              </div>
              <div className="flex flex-col items-center px-2">
                <span className="text-[10px] text-slate-400 uppercase">Spread</span>
                <span className="text-xs sm:text-sm font-semibold text-yellow-400">${displaySpread.toFixed(2)}</span>
              </div>
              <div className="flex flex-col items-end">
                <span className="text-[10px] text-red-400/60 uppercase">Best Ask</span>
                <span className="text-base sm:text-xl font-bold text-red-400">${displayBestAsk.toFixed(2)}</span>
              </div>
            </div>
          </div>
        </EducationalTooltip>

        {/* BID Section Header */}
        <div className="flex justify-between items-center px-1 sm:px-2 mb-1 flex-shrink-0">
          <EducationalTooltip
            enabled={educationalMode}
            content="Bids are buy orders - people wanting to buy at these prices. The highest bid (Best Bid) is at the top, closest to the spread."
            position="right"
          >
            <div className="flex items-center gap-2">
              <span className="text-green-400 text-xs font-semibold uppercase tracking-wide">Bid</span>
              <span className="text-green-400/60 text-[10px]">(Buy Orders)</span>
            </div>
          </EducationalTooltip>
          <EducationalTooltip
            enabled={educationalMode}
            content="Total volume of buy orders shown in this section."
            position="left"
          >
            <span className="text-green-400/80 text-[10px] sm:text-xs font-mono">{totalBidQty.toLocaleString()}</span>
          </EducationalTooltip>
        </div>
        
        {/* Bid Column Headers */}
        <EducationalTooltip 
          enabled={educationalMode} 
          content="Price: the buy price. Size: quantity at this price. Total: cumulative volume from best price to this level."
          position="bottom"
        >
          <div className="grid grid-cols-3 text-[10px] text-slate-500 mb-1 px-1 sm:px-2 flex-shrink-0 border-b border-slate-700 pb-1">
            <span className="text-left">Price</span>
            <span className="text-right">Size</span>
            <span className="text-right">Total</span>
          </div>
        </EducationalTooltip>

        {/* Bids (Buys) */}
        <div className="flex-1 overflow-auto min-h-0">
          {data.bids.slice(0, 10).map((bid, idx) => {
            const depthPct = (bid.quantity / maxQty) * 100;
            const cumulative = data.bids.slice(0, idx + 1).reduce((sum, b) => sum + b.quantity, 0);
            return (
              <div
                key={`bid-${idx}-${bid.price}`}
                className="relative grid grid-cols-3 text-xs sm:text-sm py-0.5 sm:py-1 px-1 sm:px-2 hover:bg-green-500/10 transition-colors"
              >
                <div
                  className="absolute left-0 top-0 bottom-0 bg-gradient-to-r from-green-500/30 to-transparent"
                  style={{ width: `${depthPct}%` }}
                />
                <span className="text-green-400 font-mono relative z-10 truncate">${bid.price.toFixed(2)}</span>
                <span className="text-right text-green-300/80 font-mono relative z-10 truncate">{bid.quantity.toLocaleString()}</span>
                <span className="text-right text-green-400/50 font-mono relative z-10 truncate">{cumulative.toLocaleString()}</span>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
}

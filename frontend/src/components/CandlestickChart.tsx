import { useMemo, useState, useRef, useEffect, memo, useCallback } from 'react';
import {
  ComposedChart,
  Bar,
  XAxis,
  YAxis,
  ResponsiveContainer,
  ReferenceLine,
  Customized,
} from 'recharts';
import type { PricePoint, Candle, Timeframe, ChartType, MarketStats, CandleCache, CurrentCandles } from '../types';
import { TIMEFRAMES } from '../types';
import { EducationalTooltip } from './EducationalTooltip';
import { useEducationalMode } from '../context/EducationalModeContext';

interface CandlestickChartProps {
  data: PricePoint[];
  stats: MarketStats | null;
  candleCache: CandleCache;
  currentCandles: CurrentCandles;
  requestCandles: (timeframe: Exclude<Timeframe, 0>) => void;
}

// Fixed candle width for consistent sizing (small like real charts)
const FIXED_CANDLE_WIDTH = 4;
const FIXED_WICK_WIDTH = 1;
const CANDLE_SPACING = 6; // Total space per candle (width + gap)
const MIN_VISIBLE_CANDLES = 30; // Minimum candles to show
const Y_AXIS_WIDTH = 50; // Width reserved for Y axis
const CHART_RIGHT_MARGIN = 30; // Right margin

// Custom tooltip for OHLC data - standalone component for custom mouse tracking
const CandleTooltipContent = memo(({ candle, timeframe }: { candle: Candle; timeframe: Timeframe }) => {
  const isGreen = candle.close >= candle.open;
  const change = candle.close - candle.open;
  const changePct = candle.open !== 0 ? ((change / candle.open) * 100).toFixed(2) : '0.00';
  
  return (
    <div className="bg-slate-800 border border-slate-600 rounded-lg p-3 shadow-xl text-sm pointer-events-none">
      <div className="text-slate-400 text-xs mb-2">
        {formatTime(candle.timestamp, timeframe)}
      </div>
      <div className="grid grid-cols-2 gap-x-4 gap-y-1">
        <span className="text-slate-400">Open:</span>
        <span className="text-white font-mono text-right">${candle.open.toFixed(2)}</span>
        <span className="text-slate-400">High:</span>
        <span className="text-green-400 font-mono text-right">${candle.high.toFixed(2)}</span>
        <span className="text-slate-400">Low:</span>
        <span className="text-red-400 font-mono text-right">${candle.low.toFixed(2)}</span>
        <span className="text-slate-400">Close:</span>
        <span className={`font-mono text-right ${isGreen ? 'text-green-400' : 'text-red-400'}`}>
          ${candle.close.toFixed(2)}
        </span>
        <span className="text-slate-400">Volume:</span>
        <span className="text-white font-mono text-right">{(candle.volume || 0).toLocaleString()}</span>
      </div>
      <div className={`mt-2 pt-2 border-t border-slate-600 text-center font-semibold ${isGreen ? 'text-green-400' : 'text-red-400'}`}>
        {isGreen ? '▲' : '▼'} {change >= 0 ? '+' : ''}{change.toFixed(2)} ({changePct}%)
      </div>
    </div>
  );
});

// Price display component that tracks previous price for coloring
const PriceDisplay = memo(({ price }: { price: number }) => {
  const [priceColor, setPriceColor] = useState<'green' | 'red' | 'white'>('white');
  const prevPriceRef = useRef<number | null>(null);
  
  useEffect(() => {
    if (prevPriceRef.current !== null) {
      if (price > prevPriceRef.current) {
        setPriceColor('green');
      } else if (price < prevPriceRef.current) {
        setPriceColor('red');
      }
      // If equal, keep previous color
    }
    prevPriceRef.current = price;
  }, [price]);
  
  const colorClass = priceColor === 'green' ? 'text-green-400' : priceColor === 'red' ? 'text-red-400' : 'text-white';
  
  return (
    <span className={`text-lg font-mono ${colorClass}`}>
      ${price.toFixed(2)}
    </span>
  );
});

// Custom candlestick shape - memoized for performance, uses fixed positioning
const CandlestickShape = memo((props: any) => {
  const { y, payload, index } = props;
  if (!payload) return null;
  
  const { open, close, high, low } = payload;
  const isGreen = close >= open;
  const color = isGreen ? '#22c55e' : '#ef4444';
  
  // Use fixed small width for compact candles
  const candleWidth = FIXED_CANDLE_WIDTH;
  const wickWidth = FIXED_WICK_WIDTH;
  
  // Calculate fixed position from left (left-aligned)
  const fixedX = Y_AXIS_WIDTH + (index || 0) * CANDLE_SPACING;
  
  // Calculate Y positions (chart is inverted - higher price = lower Y)
  const yScale = props.yScale || ((_v: number) => y);
  
  const highY = yScale(high);
  const lowY = yScale(low);
  const openY = yScale(open);
  const closeY = yScale(close);
  
  const bodyTop = Math.min(openY, closeY);
  const bodyBottom = Math.max(openY, closeY);
  const bodyHeight = Math.max(bodyBottom - bodyTop, 1);
  
  // Center X at fixed position
  const centerX = fixedX + candleWidth / 2;
  
  return (
    <g>
      {/* Upper wick */}
      <line
        x1={centerX}
        y1={highY}
        x2={centerX}
        y2={bodyTop}
        stroke={color}
        strokeWidth={wickWidth}
      />
      {/* Lower wick */}
      <line
        x1={centerX}
        y1={bodyBottom}
        x2={centerX}
        y2={lowY}
        stroke={color}
        strokeWidth={wickWidth}
      />
      {/* Body */}
      <rect
        x={centerX - candleWidth / 2}
        y={bodyTop}
        width={candleWidth}
        height={bodyHeight}
        fill={isGreen ? color : color}
        stroke={color}
        strokeWidth={1}
      />
    </g>
  );
});

// Custom line chart that uses fixed positioning (left-aligned like candlesticks)
// This prevents the line from stretching to fill the container when there are few points
const FixedPositionLine = memo(({ data, priceDomain, chartHeight }: { 
  data: Candle[]; 
  priceDomain: [number, number];
  chartHeight: number;
}) => {
  if (data.length === 0) return null;
  
  const [minPrice, maxPrice] = priceDomain;
  const priceRange = maxPrice - minPrice || 1;
  
  // Build SVG path with fixed X positions matching candlestick spacing
  const pathPoints = data.map((candle, index) => {
    const x = Y_AXIS_WIDTH + index * CANDLE_SPACING + FIXED_CANDLE_WIDTH / 2;
    const y = chartHeight - ((candle.close - minPrice) / priceRange) * chartHeight;
    return { x, y };
  });
  
  // Create SVG path string
  const pathD = pathPoints
    .map((point, i) => `${i === 0 ? 'M' : 'L'} ${point.x} ${point.y}`)
    .join(' ');
  
  // Also render dots at each point for visibility
  return (
    <g>
      <path
        d={pathD}
        fill="none"
        stroke="#3b82f6"
        strokeWidth={2}
        strokeLinejoin="round"
        strokeLinecap="round"
      />
      {/* Small dots at each data point */}
      {pathPoints.map((point, i) => (
        <circle
          key={i}
          cx={point.x}
          cy={point.y}
          r={2}
          fill="#3b82f6"
        />
      ))}
    </g>
  );
});

// Aggregate price points into candles (or raw ticks if timeframe is 0)
function aggregateToCandles(data: PricePoint[], timeframeSeconds: number): Candle[] {
  if (data.length === 0) return [];
  
  // For tick data (timeframe 0), convert each price point to a "candle"
  if (timeframeSeconds === 0) {
    // Limit to last 100 ticks for performance
    const recentData = data.slice(-100);
    return recentData.map(point => ({
      timestamp: point.timestamp,
      open: point.price,
      high: point.price,
      low: point.price,
      close: point.price,
      volume: point.volume || 0,
    }));
  }
  
  const candles: Candle[] = [];
  const timeframeMs = timeframeSeconds * 1000;
  
  let currentCandle: Candle | null = null;
  let currentPeriodStart = 0;
  
  for (const point of data) {
    const periodStart = Math.floor(point.timestamp / timeframeMs) * timeframeMs;
    
    if (currentCandle === null || periodStart !== currentPeriodStart) {
      // Save previous candle
      if (currentCandle) {
        candles.push(currentCandle);
      }
      // Start new candle
      currentPeriodStart = periodStart;
      currentCandle = {
        timestamp: periodStart,
        open: point.price,
        high: point.price,
        low: point.price,
        close: point.price,
        volume: point.volume || 0,
      };
    } else {
      // Update existing candle
      currentCandle.high = Math.max(currentCandle.high, point.price);
      currentCandle.low = Math.min(currentCandle.low, point.price);
      currentCandle.close = point.price;
      currentCandle.volume += point.volume || 0;
    }
  }
  
  // Don't forget the last candle
  if (currentCandle) {
    candles.push(currentCandle);
  }
  
  return candles;
}

function formatTime(timestamp: number, timeframe: Timeframe): string {
  const date = new Date(timestamp);
  if (timeframe === 0) {
    // For tick data, show with milliseconds
    return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }) + 
           '.' + date.getMilliseconds().toString().padStart(3, '0');
  }
  if (timeframe >= 60) {
    return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  }
  return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
}

export function CandlestickChart({ data, stats, candleCache, currentCandles, requestCandles }: CandlestickChartProps) {
  const [timeframe, setTimeframe] = useState<Timeframe>(5);
  const [chartType, setChartType] = useState<ChartType>('candle');
  const [maxVisibleCandles, setMaxVisibleCandles] = useState(MIN_VISIBLE_CANDLES);
  const [scrollOffset, setScrollOffset] = useState(0); // 0 = latest, positive = older
  const [autoScroll, setAutoScroll] = useState(true); // Auto-scroll to latest
  const [hoveredCandle, setHoveredCandle] = useState<Candle | null>(null);
  const [tooltipPos, setTooltipPos] = useState({ x: 0, y: 0 });
  const chartContainerRef = useRef<HTMLDivElement>(null);
  const { enabled: educationalMode } = useEducationalMode();
  
  // Request cached candles when timeframe changes (for non-tick timeframes)
  useEffect(() => {
    if (timeframe !== 0) {
      requestCandles(timeframe as Exclude<Timeframe, 0>);
    }
  }, [timeframe, requestCandles]);
  
  // Calculate max visible candles based on container width
  useEffect(() => {
    const container = chartContainerRef.current;
    if (!container) return;
    
    let rafId: number;
    
    const calculateMaxCandles = () => {
      // Cancel any pending calculation
      if (rafId) cancelAnimationFrame(rafId);
      
      rafId = requestAnimationFrame(() => {
        const width = container.clientWidth;
        if (width > 0) {
          const availableWidth = width - Y_AXIS_WIDTH - CHART_RIGHT_MARGIN;
          const calculated = Math.floor(availableWidth / CANDLE_SPACING);
          setMaxVisibleCandles(Math.max(calculated, MIN_VISIBLE_CANDLES));
        }
      });
    };
    
    // Initial calculation
    calculateMaxCandles();
    
    // Also recalculate on window resize as backup
    window.addEventListener('resize', calculateMaxCandles);
    
    // Recalculate on container resize
    const resizeObserver = new ResizeObserver(calculateMaxCandles);
    resizeObserver.observe(container);
    
    return () => {
      if (rafId) cancelAnimationFrame(rafId);
      window.removeEventListener('resize', calculateMaxCandles);
      resizeObserver.disconnect();
    };
  }, []);
  
  // Get all candles - use cached candles for non-tick timeframes
  const allCandles = useMemo(() => {
    if (timeframe === 0) {
      // Tick data: convert price points to candles
      return aggregateToCandles(data, 0);
    }
    
    // For other timeframes, use cached candles + current incomplete candle
    const cached = candleCache[timeframe as keyof CandleCache] || [];
    const current = currentCandles[timeframe as keyof CurrentCandles];
    
    if (current) {
      return [...cached, current];
    }
    return cached;
  }, [timeframe, data, candleCache, currentCandles]);
  
  // Reset scroll offset when switching timeframes
  useEffect(() => {
    setScrollOffset(0);
    setAutoScroll(true);
  }, [timeframe]);
  
  // Auto-scroll to latest when new candles arrive (if auto-scroll is on)
  useEffect(() => {
    if (autoScroll) {
      setScrollOffset(0);
    }
  }, [allCandles.length, autoScroll]);
  
  // Calculate max scroll offset
  const maxScrollOffset = useMemo(() => {
    return Math.max(0, allCandles.length - maxVisibleCandles);
  }, [allCandles.length, maxVisibleCandles]);
  
  // Get visible candles based on scroll offset
  const candles = useMemo(() => {
    if (allCandles.length <= maxVisibleCandles) return allCandles;
    const endIndex = allCandles.length - scrollOffset;
    const startIndex = Math.max(0, endIndex - maxVisibleCandles);
    return allCandles.slice(startIndex, endIndex);
  }, [allCandles, maxVisibleCandles, scrollOffset]);
  
  // Scroll handlers
  const scrollLeft = () => {
    setAutoScroll(false);
    setScrollOffset(prev => Math.min(prev + Math.floor(maxVisibleCandles / 2), maxScrollOffset));
  };
  
  const scrollRight = () => {
    const newOffset = scrollOffset - Math.floor(maxVisibleCandles / 2);
    if (newOffset <= 0) {
      setScrollOffset(0);
      setAutoScroll(true);
    } else {
      setScrollOffset(newOffset);
    }
  };
  
  const scrollToLatest = () => {
    setScrollOffset(0);
    setAutoScroll(true);
  };
  
  // Mouse wheel scroll handler
  const handleWheel = (e: React.WheelEvent) => {
    if (maxScrollOffset <= 0) return;
    
    // Horizontal scroll with shift+wheel or touchpad
    const delta = e.deltaX !== 0 ? e.deltaX : e.deltaY;
    const scrollAmount = Math.ceil(maxVisibleCandles / 10); // Scroll ~10% at a time
    
    if (delta > 0) {
      // Scroll right (towards newer)
      const newOffset = scrollOffset - scrollAmount;
      if (newOffset <= 0) {
        setScrollOffset(0);
        setAutoScroll(true);
      } else {
        setAutoScroll(false);
        setScrollOffset(newOffset);
      }
    } else if (delta < 0) {
      // Scroll left (towards older)
      setAutoScroll(false);
      setScrollOffset(prev => Math.min(prev + scrollAmount, maxScrollOffset));
    }
  };
  
  // Custom mouse tracking for candle tooltip
  const handleMouseMove = useCallback((e: React.MouseEvent<HTMLDivElement>) => {
    const container = chartContainerRef.current;
    if (!container || candles.length === 0) {
      setHoveredCandle(null);
      return;
    }
    
    const rect = container.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;
    
    // Calculate which candle index based on fixed positions
    const candleIndex = Math.floor((mouseX - Y_AXIS_WIDTH) / CANDLE_SPACING);
    
    if (candleIndex >= 0 && candleIndex < candles.length) {
      setHoveredCandle(candles[candleIndex]);
      setTooltipPos({ x: mouseX, y: mouseY });
    } else {
      setHoveredCandle(null);
    }
  }, [candles]);
  
  const handleMouseLeave = useCallback(() => {
    setHoveredCandle(null);
  }, []);
  
  // Calculate price domain with padding
  const priceDomain = useMemo(() => {
    if (candles.length === 0) return [0, 100];
    const prices = candles.flatMap(c => [c.high, c.low]);
    const min = Math.min(...prices);
    const max = Math.max(...prices);
    const padding = (max - min) * 0.1 || 1;
    return [min - padding, max + padding];
  }, [candles]);
  
  // Calculate max volume for scaling
  const maxVolume = useMemo(() => {
    if (candles.length === 0) return 100;
    return Math.max(...candles.map(c => c.volume));
  }, [candles]);
  
  return (
    <div className="bg-slate-800 rounded-lg p-2 sm:p-4 h-full flex flex-col overflow-hidden" style={{ minHeight: '300px' }}>
      {/* Header with controls */}
      <div className="flex flex-wrap justify-between items-center mb-2 sm:mb-4 gap-2 flex-shrink-0">
        <div className="flex items-center gap-4">
          <EducationalTooltip
            enabled={educationalMode}
            content="Price chart visualizes price movement over time. Green candles indicate price went up (close > open), red candles indicate price went down (close < open)."
            position="bottom"
          >
            <h2 className="text-lg font-semibold text-white">
              {stats?.symbol || 'DEMO'} Price Chart
            </h2>
          </EducationalTooltip>
          {stats && (
            <EducationalTooltip
              enabled={educationalMode}
              content="Current price of the asset. Green means price went up from previous tick, red means it went down."
              position="bottom"
            >
              <PriceDisplay price={stats.currentPrice} />
            </EducationalTooltip>
          )}
        </div>
        
        <div className="flex items-center gap-3">
          {/* Chart type toggle */}
          <EducationalTooltip
            enabled={educationalMode}
            content="Line chart shows only closing prices connected by a line. Candlestick chart shows Open, High, Low, Close (OHLC) for each time period."
            position="bottom"
          >
            <div className="flex bg-slate-700 rounded-lg p-1">
              <button
                onClick={() => setChartType('line')}
                className={`px-3 py-1 text-xs rounded transition-colors ${
                  chartType === 'line' 
                    ? 'bg-blue-600 text-white' 
                    : 'text-slate-400 hover:text-white'
                }`}
              >
                Line
              </button>
              <button
                onClick={() => setChartType('candle')}
                className={`px-3 py-1 text-xs rounded transition-colors ${
                  chartType === 'candle' 
                    ? 'bg-blue-600 text-white' 
                    : 'text-slate-400 hover:text-white'
                }`}
              >
                Candle
              </button>
            </div>
          </EducationalTooltip>
          
          {/* Timeframe selector - buttons on large screens, dropdown on small */}
          <EducationalTooltip
            enabled={educationalMode}
            content="Timeframe determines how much data each candle/point represents. 'Tick' shows every price update, '1s' aggregates 1 second of data per candle, '5m' aggregates 5 minutes, etc."
            position="bottom"
          >
            {/* Dropdown for small screens */}
            <div className="sm:hidden bg-slate-700 rounded-lg p-1">
              <select
                value={timeframe}
                onChange={(e) => setTimeframe(Number(e.target.value) as Timeframe)}
                className="bg-green-600 text-white text-xs rounded px-3 py-1 border-none outline-none cursor-pointer appearance-none pr-6"
                style={{ backgroundImage: `url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 24 24' stroke='white'%3E%3Cpath stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M19 9l-7 7-7-7'/%3E%3C/svg%3E")`, backgroundRepeat: 'no-repeat', backgroundPosition: 'right 4px center', backgroundSize: '14px' }}
              >
                {TIMEFRAMES.map(tf => (
                  <option key={tf.value} value={tf.value} className="bg-slate-700 text-white">
                    {tf.label}
                  </option>
                ))}
              </select>
            </div>
            {/* Buttons for larger screens */}
            <div className="hidden sm:flex bg-slate-700 rounded-lg p-1">
              {TIMEFRAMES.map(tf => (
                <button
                  key={tf.value}
                  onClick={() => setTimeframe(tf.value)}
                  className={`px-3 py-1 text-xs rounded transition-colors ${
                    timeframe === tf.value 
                      ? 'bg-green-600 text-white' 
                      : 'text-slate-400 hover:text-white'
                  }`}
                >
                  {tf.label}
                </button>
              ))}
            </div>
          </EducationalTooltip>
        </div>
      </div>
      
      {/* Price chart */}
      <div 
        ref={chartContainerRef} 
        className="flex-1 min-h-0 relative" 
        style={{ minHeight: '150px', height: '70%' }}
        onWheel={handleWheel}
        onMouseMove={handleMouseMove}
        onMouseLeave={handleMouseLeave}
      >
        {/* Custom tooltip for candle hover */}
        {hoveredCandle && (
          <div 
            className="absolute z-50 pointer-events-none"
            style={{ 
              left: tooltipPos.x + 15, 
              top: tooltipPos.y - 10,
              transform: tooltipPos.x > (chartContainerRef.current?.clientWidth || 0) / 2 ? 'translateX(-100%)' : 'none',
              marginLeft: tooltipPos.x > (chartContainerRef.current?.clientWidth || 0) / 2 ? -30 : 0
            }}
          >
            <CandleTooltipContent candle={hoveredCandle} timeframe={timeframe} />
          </div>
        )}
        
        {/* Scroll controls overlay */}
        {maxScrollOffset > 0 && (
          <div className="absolute top-2 right-2 z-20 flex items-center gap-1 bg-slate-900/80 rounded-lg p-1">
            <button
              onClick={scrollLeft}
              disabled={scrollOffset >= maxScrollOffset}
              className={`p-1.5 rounded transition-colors ${
                scrollOffset >= maxScrollOffset
                  ? 'text-slate-600 cursor-not-allowed'
                  : 'text-slate-300 hover:bg-slate-700 hover:text-white'
              }`}
              title="Scroll to older candles"
            >
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
              </svg>
            </button>
            <button
              onClick={scrollRight}
              disabled={scrollOffset === 0}
              className={`p-1.5 rounded transition-colors ${
                scrollOffset === 0
                  ? 'text-slate-600 cursor-not-allowed'
                  : 'text-slate-300 hover:bg-slate-700 hover:text-white'
              }`}
              title="Scroll to newer candles"
            >
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
              </svg>
            </button>
            {scrollOffset > 0 && (
              <button
                onClick={scrollToLatest}
                className="p-1.5 rounded transition-colors text-blue-400 hover:bg-slate-700 hover:text-blue-300"
                title="Jump to latest"
              >
                <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 5l7 7-7 7M5 5l7 7-7 7" />
                </svg>
              </button>
            )}
          </div>
        )}
        
        {/* Auto-scroll indicator */}
        {!autoScroll && scrollOffset > 0 && (
          <div className="absolute bottom-2 left-1/2 -translate-x-1/2 z-20">
            <button
              onClick={scrollToLatest}
              className="bg-blue-600/90 hover:bg-blue-500 text-white text-xs px-3 py-1.5 rounded-full flex items-center gap-1 shadow-lg transition-colors"
            >
              <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 5l7 7-7 7M5 5l7 7-7 7" />
              </svg>
              Jump to Live
            </button>
          </div>
        )}
        
        {maxVisibleCandles > 0 && (
          <ResponsiveContainer width="100%" height="100%" minWidth={100} minHeight={100}>
          <ComposedChart 
            data={candles} 
            margin={{ top: 10, right: 30, left: 0, bottom: 0 }}
            barGap={1}
            barCategoryGap={1}
            throttleDelay={100}
          >
            <XAxis 
              dataKey="timestamp" 
              tickFormatter={(ts) => formatTime(ts, timeframe)}
              stroke="#64748b"
              tick={{ fill: '#64748b', fontSize: 10 }}
              interval="preserveStartEnd"
              padding={{ left: 10, right: 10 }}
              hide
            />
            <YAxis 
              domain={priceDomain}
              stroke="#64748b"
              tick={{ fill: '#64748b', fontSize: 10 }}
              tickFormatter={(v) => `$${v.toFixed(0)}`}
              width={50}
            />
            {stats?.openPrice && (
              <ReferenceLine 
                y={stats.openPrice} 
                stroke="#64748b" 
                strokeDasharray="3 3"
                label={{ value: 'Open', fill: '#64748b', fontSize: 10 }}
              />
            )}
            
            {chartType === 'candle' ? (
              <Bar
                dataKey="close"
                barSize={CANDLE_SPACING}
                shape={(props: any) => (
                  <CandlestickShape 
                    {...props}
                    index={props.index}
                    yScale={(v: number) => {
                      const [min, max] = priceDomain;
                      const chartHeight = 300;
                      return chartHeight - ((v - min) / (max - min)) * chartHeight;
                    }} 
                  />
                )}
                isAnimationActive={false}
              />
            ) : (
              <Customized
                component={(props: any) => {
                  const { height } = props;
                  // Use actual chart height from props, with fallback
                  const chartHeight = height ? height - 10 : 290; // Subtract margin
                  return (
                    <FixedPositionLine 
                      data={candles} 
                      priceDomain={priceDomain as [number, number]} 
                      chartHeight={chartHeight}
                    />
                  );
                }}
              />
            )}
          </ComposedChart>
        </ResponsiveContainer>
        )}
      </div>
      
      {/* Volume chart */}
      <div className="flex-shrink-0" style={{ height: '60px', minHeight: '40px' }}>
        <ResponsiveContainer width="100%" height="100%" minWidth={100} minHeight={30}>
          <ComposedChart 
            data={candles} 
            margin={{ top: 0, right: 30, left: 0, bottom: 0 }}
            barGap={1}
            barCategoryGap={1}
            throttleDelay={50}
          >
            <XAxis dataKey="timestamp" hide padding={{ left: 10, right: 10 }} />
            <YAxis 
              domain={[0, maxVolume * 1.2]} 
              hide 
              width={50}
            />
            <Bar 
              dataKey="volume" 
              barSize={CANDLE_SPACING}
              isAnimationActive={false}
              shape={(props: any) => {
                const { payload, index } = props;
                if (!payload) return <g />;
                
                const isGreen = payload.close >= payload.open;
                const color = isGreen ? '#22c55e80' : '#ef444480';
                
                // Use fixed positioning to match candle chart
                // Must add barWidth/2 to match CandlestickShape's centerX calculation
                const barWidth = FIXED_CANDLE_WIDTH;
                const centerX = Y_AXIS_WIDTH + index * CANDLE_SPACING + barWidth / 2;
                
                // Calculate bar height based on volume
                const chartHeight = 60;
                const barHeight = (payload.volume / (maxVolume * 1.2)) * chartHeight;
                const barY = chartHeight - barHeight;
                
                return (
                  <rect
                    x={centerX - barWidth / 2}
                    y={barY}
                    width={barWidth}
                    height={barHeight}
                    fill={color}
                  />
                );
              }}
            />
          </ComposedChart>
        </ResponsiveContainer>
      </div>
      
      {/* Legend */}
      <div className="flex flex-wrap justify-center gap-2 sm:gap-6 mt-1 sm:mt-2 text-[10px] sm:text-xs text-slate-400 flex-shrink-0">
        <EducationalTooltip
          enabled={educationalMode}
          content="Bullish (green) candles indicate the closing price was higher than the opening price - buyers pushed the price up."
          position="top"
        >
          <span className="flex items-center gap-1">
            <span className="w-2 h-2 sm:w-3 sm:h-3 bg-green-500 rounded-sm"></span> Bullish
          </span>
        </EducationalTooltip>
        <EducationalTooltip
          enabled={educationalMode}
          content="Bearish (red) candles indicate the closing price was lower than the opening price - sellers pushed the price down."
          position="top"
        >
          <span className="flex items-center gap-1">
            <span className="w-2 h-2 sm:w-3 sm:h-3 bg-red-500 rounded-sm"></span> Bearish
          </span>
        </EducationalTooltip>
        <span className="truncate">
          {scrollOffset > 0 
            ? `Viewing: ${allCandles.length - scrollOffset - maxVisibleCandles + 1}-${allCandles.length - scrollOffset} of ${allCandles.length}`
            : `Candles: ${candles.length}${allCandles.length > maxVisibleCandles ? ` / ${allCandles.length}` : ''}`
          }
        </span>
        {autoScroll && <span className="text-green-400">● Live</span>}
      </div>
    </div>
  );
}

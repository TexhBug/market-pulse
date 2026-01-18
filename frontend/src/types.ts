// WebSocket message types
export interface PriceLevel {
  price: number;
  quantity: number;
}

export interface OrderBookData {
  bids: PriceLevel[];
  asks: PriceLevel[];
  bestBid: number;
  bestAsk: number;
  spread: number;
}

export interface Trade {
  id: number;
  price: number;
  quantity: number;
  side: 'BUY' | 'SELL';
  timestamp: number;
}

export interface PricePoint {
  timestamp: number;
  price: number;
  volume?: number;
}

// OHLC Candlestick data
export interface Candle {
  timestamp: number;
  open: number;
  high: number;
  low: number;
  close: number;
  volume: number;
}

// Timeframe options (in seconds, 0 = tick-by-tick)
export type Timeframe = 0 | 1 | 5 | 30 | 60 | 300;

export const TIMEFRAMES: { value: Timeframe; label: string }[] = [
  { value: 0, label: 'Tick' },
  { value: 1, label: '1s' },
  { value: 5, label: '5s' },
  { value: 30, label: '30s' },
  { value: 60, label: '1m' },
  { value: 300, label: '5m' },
];

export type ChartType = 'line' | 'candle';

export interface MarketStats {
  symbol: string;
  currentPrice: number;
  openPrice: number;
  highPrice: number;
  lowPrice: number;
  totalOrders: number;
  totalTrades: number;
  totalVolume: number;
  marketOrderPct: number;
  sentiment: Sentiment;
  intensity: Intensity;
  spread: number;
  speed: number;
  paused: boolean;
  newsShockEnabled: boolean;
  newsShockCooldown: boolean;
  newsShockCooldownRemaining: number;
  newsShockActiveRemaining: number;
}

export type Sentiment = 'BULLISH' | 'BEARISH' | 'VOLATILE' | 'SIDEWAYS' | 'CHOPPY' | 'NEUTRAL';
export type Intensity = 'MILD' | 'MODERATE' | 'NORMAL' | 'AGGRESSIVE' | 'EXTREME';

export interface WSMessage {
  type: 'orderbook' | 'trade' | 'stats' | 'price' | 'candle' | 'candleHistory' | 'candleReset';
  data: OrderBookData | Trade | MarketStats | PricePoint;
}

export interface WSCommand {
  type: 'sentiment' | 'intensity' | 'spread' | 'speed' | 'pause' | 'reset' | 'getCandles' | 'newsShock';
  value?: string | number | boolean;
  timeframe?: Timeframe;
}

// Candle cache per timeframe
export type CandleCache = {
  [K in Exclude<Timeframe, 0>]: Candle[];
};

// Current (incomplete) candle per timeframe
export type CurrentCandles = {
  [K in Exclude<Timeframe, 0>]: Candle | null;
};

// Sentiment display info
export const SENTIMENTS: { id: Sentiment; label: string; color: string }[] = [
  { id: 'BULLISH', label: 'Bullish', color: 'bg-green-600 hover:bg-green-700' },
  { id: 'BEARISH', label: 'Bearish', color: 'bg-red-600 hover:bg-red-700' },
  { id: 'VOLATILE', label: 'Volatile', color: 'bg-purple-600 hover:bg-purple-700' },
  { id: 'SIDEWAYS', label: 'Sideways', color: 'bg-blue-600 hover:bg-blue-700' },
  { id: 'CHOPPY', label: 'Choppy', color: 'bg-orange-600 hover:bg-orange-700' },
  { id: 'NEUTRAL', label: 'Neutral', color: 'bg-slate-600 hover:bg-slate-700' },
];

export const INTENSITIES: { id: Intensity; label: string }[] = [
  { id: 'MILD', label: 'Mild' },
  { id: 'MODERATE', label: 'Moderate' },
  { id: 'NORMAL', label: 'Normal' },
  { id: 'AGGRESSIVE', label: 'Aggressive' },
  { id: 'EXTREME', label: 'Extreme' },
];

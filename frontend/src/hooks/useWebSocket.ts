import { useState, useCallback, useRef } from 'react';
import type { OrderBookData, Trade, MarketStats, PricePoint, WSCommand, Sentiment, Intensity, CandleCache, CurrentCandles, Timeframe } from '../types';

// Use environment variable or fallback to localhost
const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080';

// Initial empty candle cache
const createEmptyCandleCache = (): CandleCache => ({
  1: [],
  5: [],
  30: [],
  60: [],
  300: []
});

const createEmptyCurrentCandles = (): CurrentCandles => ({
  1: null,
  5: null,
  30: null,
  60: null,
  300: null
});

export interface SimulationConfig {
  symbol: string;
  price: number;
  spread: number;
  sentiment: Sentiment;
  intensity: Intensity;
  speed: number;
}

interface UseWebSocketReturn {
  connected: boolean;
  connecting: boolean;
  latency: number | null;  // Round-trip latency in ms
  orderBook: OrderBookData | null;
  trades: Trade[];
  stats: MarketStats | null;
  priceHistory: PricePoint[];
  candleCache: CandleCache;
  currentCandles: CurrentCandles;
  sendCommand: (command: WSCommand) => void;
  requestCandles: (timeframe: Exclude<Timeframe, 0>) => void;
  connect: (config: SimulationConfig) => void;
  disconnect: () => void;
}

export function useWebSocket(): UseWebSocketReturn {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [latency, setLatency] = useState<number | null>(null);
  const [orderBook, setOrderBook] = useState<OrderBookData | null>(null);
  const [trades, setTrades] = useState<Trade[]>([]);
  const [stats, setStats] = useState<MarketStats | null>(null);
  const [priceHistory, setPriceHistory] = useState<PricePoint[]>([]);
  const [candleCache, setCandleCache] = useState<CandleCache>(createEmptyCandleCache);
  const [currentCandles, setCurrentCandles] = useState<CurrentCandles>(createEmptyCurrentCandles);
  const wsRef = useRef<WebSocket | null>(null);
  const configRef = useRef<SimulationConfig | null>(null);

  const connect = useCallback((config: SimulationConfig) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) return;
    
    setConnecting(true);
    configRef.current = config;
    
    // Reset all state on new connection
    setOrderBook(null);
    setTrades([]);
    setStats(null);
    setPriceHistory([]);
    setCandleCache(createEmptyCandleCache());
    setCurrentCandles(createEmptyCurrentCandles());

    // Connect with the protocol name that the server expects
    const ws = new WebSocket(WS_URL, 'lws-minimal');

    ws.onopen = () => {
      setConnected(true);
      setConnecting(false);
      
      // Send start command with configuration
      if (configRef.current) {
        const startCommand = {
          type: 'start',
          config: configRef.current
        };
        ws.send(JSON.stringify(startCommand));
      }
      
      // Start ping interval to measure latency
      const pingInterval = setInterval(() => {
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(JSON.stringify({ type: 'ping', value: Date.now().toString() }));
        }
      }, 2000); // Ping every 2 seconds
      
      // Store interval ID for cleanup
      (ws as any)._pingInterval = pingInterval;
    };

    ws.onclose = () => {
      setConnected(false);
      setConnecting(false);
      setLatency(null);
      
      // Clear ping interval
      if ((ws as any)._pingInterval) {
        clearInterval((ws as any)._pingInterval);
      }
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      setConnecting(false);
    };

    ws.onmessage = (event) => {
      // Handle potentially concatenated JSON messages from C++ server
      const rawData = event.data as string;
      
      // Split concatenated JSON objects (e.g., "{}{}{}") into individual ones
      const jsonStrings: string[] = [];
      let depth = 0;
      let start = 0;
      
      for (let i = 0; i < rawData.length; i++) {
        if (rawData[i] === '{') {
          if (depth === 0) start = i;
          depth++;
        } else if (rawData[i] === '}') {
          depth--;
          if (depth === 0) {
            jsonStrings.push(rawData.substring(start, i + 1));
          }
        }
      }
      
      // Process each message
      for (const msgStr of jsonStrings) {
        try {
          const message = JSON.parse(msgStr);
          
          switch (message.type) {
            // OPTIMIZED: Single batched message for all tick data
            case 'tick': {
              const { orderbook, stats: tickStats, price, currentCandles: serverCandles, completedCandles, trade } = message.data;
              
              // Always update orderbook and stats (to show current state)
              if (orderbook) setOrderBook(orderbook);
              if (tickStats) setStats(tickStats);
              
              // Only update dynamic data (trades, price history, candles) when NOT paused
              const isPaused = tickStats?.paused === true;
              
              if (!isPaused && trade) {
                setTrades(prev => {
                  // Prepend new trade, dedupe by id, sort by timestamp, keep only 30
                  const newTrades = [trade, ...prev.filter(t => t.id !== trade.id)];
                  return newTrades
                    .sort((a, b) => b.timestamp - a.timestamp)
                    .slice(0, 30);
                });
              }
              if (!isPaused && price) {
                setPriceHistory(prev => {
                  const newHistory = [...prev, price].slice(-100);
                  return newHistory;
                });
              }
              
              // Use server's authoritative current candles (no client-side calculation)
              if (!isPaused && serverCandles) setCurrentCandles(serverCandles);
              
              // Add any completed candles to cache
              if (!isPaused && completedCandles && completedCandles.length > 0) {
                setCandleCache(prev => {
                  const newCache = { ...prev };
                  for (const { timeframe: tf, candle } of completedCandles) {
                    newCache[tf as keyof CandleCache] = [...prev[tf as keyof CandleCache], candle].slice(-500);
                  }
                  return newCache;
                });
              }
              break;
            }
            
            // Legacy individual message handlers (for backwards compatibility)
            case 'orderbook':
              setOrderBook(message.data);
              break;
            case 'trade':
              setTrades(prev => {
                const trade = message.data;
                const newTrades = [trade, ...prev.filter(t => t.id !== trade.id)];
                return newTrades
                  .sort((a, b) => b.timestamp - a.timestamp)
                  .slice(0, 30);
              });
              break;
            case 'stats':
              setStats(message.data);
              break;
            case 'price':
              // Keep only 100 ticks for tick chart
              setPriceHistory(prev => [...prev, message.data].slice(-100));
              // Update current candles with new price
              setCurrentCandles(prev => {
                const newCandles = { ...prev };
                const { price: p, volume, timestamp } = message.data;
                for (const tf of [1, 5, 30, 60, 300] as const) {
                  const tfMs = tf * 1000;
                  const periodStart = Math.floor(timestamp / tfMs) * tfMs;
                  const current = newCandles[tf];
                  if (!current || current.timestamp !== periodStart) {
                    newCandles[tf] = {
                      timestamp: periodStart,
                      open: p,
                      high: p,
                      low: p,
                      close: p,
                      volume: volume
                    };
                  } else {
                    newCandles[tf] = {
                      ...current,
                      high: Math.max(current.high, p),
                      low: Math.min(current.low, p),
                      close: p,
                      volume: current.volume + volume
                    };
                  }
                }
                return newCandles;
              });
              break;
            case 'candle': {
              const { timeframe: completedTf, candle: completedCandle } = message.data;
              setCandleCache(prev => ({
                ...prev,
                [completedTf]: [...prev[completedTf as keyof CandleCache], completedCandle].slice(-500)
              }));
              break;
            }
            case 'candleHistory': {
              const { timeframe: historyTf, candles, current } = message.data;
              setCandleCache(prev => ({
                ...prev,
                [historyTf]: candles
              }));
              if (current) {
                setCurrentCandles(prev => ({
                  ...prev,
                  [historyTf]: current
                }));
              }
              break;
            }
            case 'candleReset':
              setCandleCache(createEmptyCandleCache());
              setCurrentCandles(createEmptyCurrentCandles());
              setPriceHistory([]);
              break;
            case 'simulationReset':
              // Clear all state when simulation is reset
              setTrades([]);
              setPriceHistory([]);
              setCandleCache(createEmptyCandleCache());
              setCurrentCandles(createEmptyCurrentCandles());
              break;
            case 'pong': {
              // Calculate round-trip latency
              const sentTime = message.timestamp;
              const rtt = Date.now() - sentTime;
              setLatency(rtt);
              break;
            }
            case 'started':
              break;
          }
        } catch (e) {
          console.error('Failed to parse message:', e, 'Raw data:', msgStr.substring(0, 200));
        }
      }
    };

    wsRef.current = ws;
  }, []);

  const disconnect = useCallback(() => {
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
    setConnected(false);
    setOrderBook(null);
    setTrades([]);
    setStats(null);
    setPriceHistory([]);
    setCandleCache(createEmptyCandleCache());
    setCurrentCandles(createEmptyCurrentCandles());
  }, []);

  const sendCommand = useCallback((command: WSCommand) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(command));
    }
  }, []);

  const requestCandles = useCallback((timeframe: Exclude<Timeframe, 0>) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type: 'getCandles', timeframe }));
    }
  }, []);

  return {
    connected,
    connecting,
    latency,
    orderBook,
    trades,
    stats,
    priceHistory,
    candleCache,
    currentCandles,
    sendCommand,
    requestCandles,
    connect,
    disconnect,
  };
}

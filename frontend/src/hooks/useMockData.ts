import { useState, useEffect, useCallback, useRef } from 'react';
import type { OrderBookData, Trade, MarketStats, PricePoint, WSCommand, Sentiment, Intensity } from '../types';

// Mock data generator for demo mode (when WebSocket not connected)
export function useMockData() {
  const [orderBook, setOrderBook] = useState<OrderBookData | null>(null);
  const [trades, setTrades] = useState<Trade[]>([]);
  const [stats, setStats] = useState<MarketStats | null>(null);
  const [priceHistory, setPriceHistory] = useState<PricePoint[]>([]);
  
  const priceRef = useRef(100);
  const tradeIdRef = useRef(1);
  const sentimentRef = useRef<Sentiment>('NEUTRAL');
  const intensityRef = useRef<Intensity>('NORMAL');
  const spreadRef = useRef(0.05);
  const speedRef = useRef(1.0);
  const pausedRef = useRef(false);
  const openPriceRef = useRef(100);
  const highPriceRef = useRef(100);
  const lowPriceRef = useRef(100);
  const totalOrdersRef = useRef(0);
  const totalTradesRef = useRef(0);
  const totalVolumeRef = useRef(0);
  const marketOrdersRef = useRef(0);

  // Generate random order book
  const generateOrderBook = useCallback((currentPrice: number): OrderBookData => {
    const spread = spreadRef.current;
    const bids: { price: number; quantity: number }[] = [];
    const asks: { price: number; quantity: number }[] = [];
    
    const bestBid = Math.round((currentPrice - spread / 2) * 100) / 100;
    const bestAsk = Math.round((currentPrice + spread / 2) * 100) / 100;
    
    for (let i = 0; i < 15; i++) {
      bids.push({
        price: Math.round((bestBid - i * 0.05) * 100) / 100,
        quantity: Math.floor(100 + Math.random() * 400),
      });
      asks.push({
        price: Math.round((bestAsk + i * 0.05) * 100) / 100,
        quantity: Math.floor(100 + Math.random() * 400),
      });
    }
    
    return { bids, asks, bestBid, bestAsk, spread };
  }, []);

  // Momentum for smoother price movement
  const momentumRef = useRef(0);

  // Update price based on sentiment
  const updatePrice = useCallback(() => {
    if (pausedRef.current) return;
    
    const sentiment = sentimentRef.current;
    const intensity = intensityRef.current;
    
    // Intensity multiplier (same as C++ backend)
    const intensityMult = {
      'MILD': 0.3,
      'MODERATE': 0.6,
      'NORMAL': 1.0,
      'AGGRESSIVE': 1.5,
      'EXTREME': 2.5,
    }[intensity] || 1.0;
    
    // Base parameters (much smaller for realistic movement)
    let drift = 0;      // Directional bias
    let vol = 0.001;    // Base volatility (0.1%)
    
    // Sentiment effects - tuned to match C++ backend
    switch (sentiment) {
      case 'BULLISH':
        drift = 0.0008 * intensityMult;   // Slight upward bias
        vol = 0.0006 * intensityMult;
        break;
      case 'BEARISH':
        drift = -0.0008 * intensityMult;  // Slight downward bias
        vol = 0.0006 * intensityMult;
        break;
      case 'VOLATILE':
        drift = 0;
        vol = 0.002 * intensityMult;      // Higher volatility
        break;
      case 'SIDEWAYS':
        drift = 0;
        vol = 0.0002 * intensityMult;     // Very low volatility
        break;
      case 'CHOPPY':
        // Random drift changes direction
        drift = (Math.random() - 0.5) * 0.001 * intensityMult;
        vol = 0.001 * intensityMult;
        break;
      case 'NEUTRAL':
      default:
        drift = 0;
        vol = 0.0004 * intensityMult;
    }
    
    // Add momentum for smoother movement (exponential moving average)
    const randomComponent = (Math.random() - 0.5) * vol;
    momentumRef.current = momentumRef.current * 0.7 + (drift + randomComponent) * 0.3;
    
    // Apply change with momentum
    const change = momentumRef.current;
    priceRef.current = Math.max(1, priceRef.current * (1 + change));
    
    // Round to tick size ($0.05)
    priceRef.current = Math.round(priceRef.current * 20) / 20;
    
    // Update high/low
    highPriceRef.current = Math.max(highPriceRef.current, priceRef.current);
    lowPriceRef.current = Math.min(lowPriceRef.current, priceRef.current);
  }, []);

  // Generate a trade
  const generateTrade = useCallback((): Trade => {
    // Bias trade side based on sentiment
    const sentiment = sentimentRef.current;
    let buySideProbability = 0.5;
    
    if (sentiment === 'BULLISH') buySideProbability = 0.65;
    else if (sentiment === 'BEARISH') buySideProbability = 0.35;
    
    const side = Math.random() < buySideProbability ? 'BUY' : 'SELL';
    const isMarket = Math.random() < 0.15;
    
    totalOrdersRef.current++;
    if (isMarket) marketOrdersRef.current++;
    
    const qty = Math.floor(10 + Math.random() * 200);
    totalTradesRef.current++;
    totalVolumeRef.current += qty;
    
    return {
      id: tradeIdRef.current++,
      price: priceRef.current,
      quantity: qty,
      side,
      timestamp: Date.now(),
    };
  }, []);

  // Main simulation loop
  useEffect(() => {
    const interval = setInterval(() => {
      if (pausedRef.current) return;
      
      updatePrice();
      
      // Add price point
      setPriceHistory(prev => [...prev, {
        timestamp: Date.now(),
        price: priceRef.current,
      }].slice(-200));
      
      // Update order book
      setOrderBook(generateOrderBook(priceRef.current));
      
      // Generate trades randomly
      if (Math.random() < 0.3) {
        const trade = generateTrade();
        setTrades(prev => [trade, ...prev].slice(0, 50));
      }
      
      // Update stats
      setStats({
        symbol: 'DEMO',
        currentPrice: priceRef.current,
        openPrice: openPriceRef.current,
        highPrice: highPriceRef.current,
        lowPrice: lowPriceRef.current,
        totalOrders: totalOrdersRef.current,
        totalTrades: totalTradesRef.current,
        totalVolume: totalVolumeRef.current,
        marketOrderPct: totalOrdersRef.current > 0 
          ? Math.round(100 * marketOrdersRef.current / totalOrdersRef.current)
          : 0,
        sentiment: sentimentRef.current,
        intensity: intensityRef.current,
        spread: spreadRef.current,
        speed: speedRef.current,
        paused: pausedRef.current,
        newsShockEnabled: false, // Mock data doesn't support news shock
        newsShockCooldown: false,
        newsShockCooldownRemaining: 0,
        newsShockActiveRemaining: 0,
      });
    }, 200 / speedRef.current);

    return () => clearInterval(interval);
  }, [updatePrice, generateOrderBook, generateTrade]);

  // Command handler
  const sendCommand = useCallback((command: WSCommand) => {
    switch (command.type) {
      case 'sentiment':
        sentimentRef.current = command.value as Sentiment;
        break;
      case 'intensity':
        intensityRef.current = command.value as Intensity;
        break;
      case 'spread':
        spreadRef.current = command.value as number;
        break;
      case 'speed':
        speedRef.current = command.value as number;
        break;
      case 'pause':
        pausedRef.current = command.value as boolean;
        break;
      case 'reset':
        priceRef.current = 100;
        openPriceRef.current = 100;
        highPriceRef.current = 100;
        lowPriceRef.current = 100;
        totalOrdersRef.current = 0;
        totalTradesRef.current = 0;
        totalVolumeRef.current = 0;
        marketOrdersRef.current = 0;
        setPriceHistory([]);
        setTrades([]);
        break;
    }
  }, []);

  return {
    connected: false,
    orderBook,
    trades,
    stats,
    priceHistory,
    sendCommand,
  };
}

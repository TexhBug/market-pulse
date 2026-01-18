import { LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer, ReferenceLine } from 'recharts';
import type { PricePoint, MarketStats } from '../types';

interface PriceChartProps {
  data: PricePoint[];
  stats: MarketStats | null;
}

export function PriceChart({ data, stats }: PriceChartProps) {
  const priceChange = stats ? stats.currentPrice - stats.openPrice : 0;
  const priceChangePct = stats && stats.openPrice > 0 
    ? (priceChange / stats.openPrice) * 100 
    : 0;
  const isPositive = priceChange >= 0;

  return (
    <div className="bg-slate-800 rounded-lg p-4 h-full flex flex-col">
      {/* Header */}
      <div className="flex justify-between items-start mb-4">
        <div>
          <div className="flex items-center gap-3">
            <span className="text-2xl font-bold text-white">
              {stats?.symbol || 'DEMO'}
            </span>
            <span className={`text-3xl font-bold ${isPositive ? 'text-green-400' : 'text-red-400'}`}>
              ${stats?.currentPrice.toFixed(2) || '100.00'}
            </span>
          </div>
          <div className={`text-sm ${isPositive ? 'text-green-400' : 'text-red-400'}`}>
            {isPositive ? '+' : ''}{priceChange.toFixed(2)} ({isPositive ? '+' : ''}{priceChangePct.toFixed(2)}%)
          </div>
        </div>
        
        <div className="flex gap-6 text-sm">
          <div>
            <span className="text-slate-400">HIGH</span>
            <div className="text-green-400 font-semibold">${stats?.highPrice.toFixed(2) || '100.00'}</div>
          </div>
          <div>
            <span className="text-slate-400">LOW</span>
            <div className="text-red-400 font-semibold">${stats?.lowPrice.toFixed(2) || '100.00'}</div>
          </div>
          <div>
            <span className="text-slate-400">OPEN</span>
            <div className="text-slate-200 font-semibold">${stats?.openPrice.toFixed(2) || '100.00'}</div>
          </div>
        </div>
      </div>

      {/* Chart */}
      <div className="flex-1 min-h-0">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={data} margin={{ top: 5, right: 5, left: 5, bottom: 5 }}>
            <XAxis 
              dataKey="timestamp" 
              tick={false}
              axisLine={{ stroke: '#475569' }}
            />
            <YAxis 
              domain={['auto', 'auto']}
              tick={{ fill: '#94a3b8', fontSize: 11 }}
              axisLine={{ stroke: '#475569' }}
              tickFormatter={(value) => `$${value.toFixed(2)}`}
              width={60}
            />
            <Tooltip
              contentStyle={{
                backgroundColor: '#1e293b',
                border: '1px solid #475569',
                borderRadius: '8px',
              }}
              labelStyle={{ color: '#94a3b8' }}
              formatter={(value) => value != null ? [`$${Number(value).toFixed(2)}`, 'Price'] : ['', '']}
              labelFormatter={(label) => new Date(label).toLocaleTimeString()}
            />
            {stats && (
              <ReferenceLine 
                y={stats.openPrice} 
                stroke="#64748b" 
                strokeDasharray="3 3" 
              />
            )}
            <Line
              type="monotone"
              dataKey="price"
              stroke={isPositive ? '#22c55e' : '#ef4444'}
              strokeWidth={2}
              dot={false}
              isAnimationActive={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

import { Wifi, WifiOff, ArrowLeft, TrendingUp, GraduationCap } from 'lucide-react';
import { useEducationalMode } from '../context/EducationalModeContext';

interface HeaderProps {
  connected: boolean;
  latency?: number | null;
  symbol?: string;
  onBack?: () => void;
}

export function Header({ connected, latency, symbol, onBack }: HeaderProps) {
  const { enabled: educationalMode, toggle: toggleEducationalMode } = useEducationalMode();
  
  return (
    <header className="bg-slate-800 border-b border-slate-700 px-4 sm:px-6 py-3 sm:py-4">
      <div className="flex flex-wrap justify-between items-center gap-2">
        <div className="flex items-center gap-2 sm:gap-3 min-w-0">
          {onBack && (
            <button
              onClick={onBack}
              className="p-2 hover:bg-slate-700 rounded-lg transition-colors flex-shrink-0"
              title="Back to configuration"
            >
              <ArrowLeft className="text-slate-400" size={20} />
            </button>
          )}
          <TrendingUp className="text-green-400 flex-shrink-0" size={24} />
          <h1 className="text-lg sm:text-xl font-bold text-white truncate">
            MarketPulse
            {symbol && <span className="text-green-400 ml-2 hidden sm:inline">• {symbol}</span>}
          </h1>
          
          {/* Educational Mode Toggle - hide text on mobile */}
          <button
            onClick={toggleEducationalMode}
            className={`ml-2 sm:ml-4 flex items-center gap-1 sm:gap-2 px-2 sm:px-3 py-1.5 rounded-lg transition-all flex-shrink-0 ${
              educationalMode 
                ? 'bg-blue-600 text-white' 
                : 'bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600'
            }`}
            title={educationalMode ? 'Disable educational mode' : 'Enable educational mode - hover over elements to learn what they do'}
          >
            <GraduationCap size={16} />
            <span className="text-xs font-medium hidden sm:inline">Learn</span>
            <span className={`w-2 h-2 rounded-full ${educationalMode ? 'bg-green-400' : 'bg-slate-500'}`} />
          </button>
        </div>
        
        <div className="flex items-center gap-2 sm:gap-3 flex-shrink-0">
          {connected ? (
            <>
              <div className="flex items-center gap-1 sm:gap-2">
                <Wifi className="text-green-400" size={18} />
                <span className="text-green-400 text-xs sm:text-sm font-medium hidden sm:inline">Connected</span>
              </div>
              {latency !== null && latency !== undefined && (
                <div className={`flex items-center gap-1 sm:gap-1.5 px-2 sm:px-3 py-1 sm:py-1.5 rounded-lg border ${
                  latency < 50 ? 'bg-green-500/10 border-green-500/30 text-green-400' :
                  latency < 100 ? 'bg-yellow-500/10 border-yellow-500/30 text-yellow-400' :
                  'bg-red-500/10 border-red-500/30 text-red-400'
                }`}>
                  <svg className="w-3 h-3 sm:w-4 sm:h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 10V3L4 14h7v7l9-11h-7z" />
                  </svg>
                  <span className="text-xs sm:text-sm font-bold font-mono">{latency}</span>
                  <span className="text-xs opacity-70 hidden sm:inline">ms</span>
                </div>
              )}
            </>
          ) : (
            <>
              <WifiOff className="text-yellow-400" size={18} />
              <span className="text-yellow-400 text-xs sm:text-sm font-medium hidden sm:inline">Connecting...</span>
            </>
          )}
        </div>
      </div>
    </header>
  );
}

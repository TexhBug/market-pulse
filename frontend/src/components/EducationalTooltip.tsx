import { useState, useRef, type ReactNode } from 'react';
import { createPortal } from 'react-dom';
import { HelpCircle } from 'lucide-react';

interface EducationalTooltipProps {
  children: ReactNode;
  content: string;
  enabled: boolean;
  position?: 'top' | 'bottom' | 'left' | 'right';
}

export function EducationalTooltip({ 
  children, 
  content, 
  enabled, 
  position = 'top'
}: EducationalTooltipProps) {
  const [isVisible, setIsVisible] = useState(false);
  const [tooltipPos, setTooltipPos] = useState({ top: 0, left: 0 });
  const triggerRef = useRef<HTMLSpanElement>(null);

  const updatePosition = () => {
    if (triggerRef.current) {
      const rect = triggerRef.current.getBoundingClientRect();
      const tooltipWidth = 280;
      const tooltipHeight = 80;
      const padding = 8;
      
      let top = 0;
      let left = 0;
      
      switch (position) {
        case 'top':
          top = rect.top - tooltipHeight - padding;
          left = rect.left + rect.width / 2 - tooltipWidth / 2;
          break;
        case 'bottom':
          top = rect.bottom + padding;
          left = rect.left + rect.width / 2 - tooltipWidth / 2;
          break;
        case 'left':
          top = rect.top + rect.height / 2 - tooltipHeight / 2;
          left = rect.left - tooltipWidth - padding;
          break;
        case 'right':
          top = rect.top + rect.height / 2 - tooltipHeight / 2;
          left = rect.right + padding;
          break;
      }
      
      // Keep tooltip within viewport
      const viewportWidth = window.innerWidth;
      const viewportHeight = window.innerHeight;
      
      if (left < padding) left = padding;
      if (left + tooltipWidth > viewportWidth - padding) {
        left = viewportWidth - tooltipWidth - padding;
      }
      if (top < padding) top = padding;
      if (top + tooltipHeight > viewportHeight - padding) {
        top = viewportHeight - tooltipHeight - padding;
      }
      
      setTooltipPos({ top, left });
    }
  };

  const handleMouseEnter = () => {
    updatePosition();
    setIsVisible(true);
  };

  if (!enabled) {
    return <>{children}</>;
  }

  const tooltip = isVisible ? createPortal(
    <div 
      style={{
        position: 'fixed',
        top: `${tooltipPos.top}px`,
        left: `${tooltipPos.left}px`,
        zIndex: 9999,
        width: 'max-content',
        maxWidth: '280px',
      }}
      className="pointer-events-none"
    >
      <div className="bg-slate-700 text-white text-xs rounded-lg px-3 py-2 shadow-xl border border-slate-600">
        <div className="flex items-start gap-2">
          <HelpCircle className="w-4 h-4 text-blue-400 flex-shrink-0 mt-0.5" />
          <span className="leading-relaxed">{content}</span>
        </div>
      </div>
    </div>,
    document.body
  ) : null;

  return (
    <span 
      style={{ display: 'contents' }}
      onMouseEnter={handleMouseEnter}
      onMouseLeave={() => setIsVisible(false)}
    >
      <span ref={triggerRef} className="inline">
        {children}
      </span>
      {tooltip}
    </span>
  );
}

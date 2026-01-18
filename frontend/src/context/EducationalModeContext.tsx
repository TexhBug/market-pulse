import { createContext, useContext, useState, ReactNode } from 'react';

interface EducationalModeContextType {
  enabled: boolean;
  toggle: () => void;
}

const EducationalModeContext = createContext<EducationalModeContextType>({
  enabled: false,
  toggle: () => {},
});

export function EducationalModeProvider({ children }: { children: ReactNode }) {
  const [enabled, setEnabled] = useState(false);

  const toggle = () => setEnabled(prev => !prev);

  return (
    <EducationalModeContext.Provider value={{ enabled, toggle }}>
      {children}
    </EducationalModeContext.Provider>
  );
}

export function useEducationalMode() {
  return useContext(EducationalModeContext);
}

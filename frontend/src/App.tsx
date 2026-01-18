import { BrowserRouter, Routes, Route } from 'react-router-dom';
import { LandingPage } from './pages/LandingPage';
import { VisualizerPage } from './pages/VisualizerPage';
import { EducationalModeProvider } from './context/EducationalModeContext';

function App() {
  return (
    <EducationalModeProvider>
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<LandingPage />} />
          <Route path="/visualizer" element={<VisualizerPage />} />
        </Routes>
      </BrowserRouter>
    </EducationalModeProvider>
  );
}

export default App;

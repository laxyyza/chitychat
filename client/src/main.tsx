import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import MainApp from './App.tsx';
import { AppProvider } from './components/AppProvider.tsx';

createRoot(document.getElementById('root')!).render(
    <StrictMode>
        <AppProvider>
            <MainApp />
        </AppProvider>
    </StrictMode>
);

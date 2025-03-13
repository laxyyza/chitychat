import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import MainApp from './App.tsx';
import { AppProvider } from './components/chatapp/AppProvider.tsx';
import Login from './components/Login.tsx';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import Loading from './components/Loading.tsx';

createRoot(document.getElementById('root')!).render(
    <StrictMode>
        <AppProvider>
            <BrowserRouter>
                <Routes>
                    <Route path="/app" element={<MainApp />}></Route>
                    <Route path="/login" element={<Login />}></Route>
                    <Route path="/" element={<Loading />}></Route>
                </Routes>
            </BrowserRouter>
        </AppProvider>
    </StrictMode>
);

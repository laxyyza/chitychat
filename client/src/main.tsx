import { StrictMode, useEffect } from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import MainApp from './App.tsx';
import { AppProvider } from './components/chatapp/AppProvider.tsx';
import Login from './components/Login.tsx';
import { BrowserRouter, Routes, Route, useNavigate, useLocation } from 'react-router-dom';
import Loading from './components/Loading.tsx';
import fetchData from './services/api.ts';
import InvitePage from './components/InvitePage.tsx';

const Main = () => {
    const navigator = useNavigate();
    const location = useLocation();

    useEffect(() => {
        if (location.pathname === "/login") return;
        if (location.pathname.startsWith("/i/")) return;
        
        //navigator("/");

        fetchData('/api/auth/remember').then(() => {
            if (!location.pathname.startsWith("/app"))
                navigator("/app");
        }).catch(() => {
            navigator("/login");
        });
    }, []);

    return (
        <Routes>
            <Route path="/app" element={<MainApp />}></Route>
            <Route path="/login" element={<Login />}></Route>
            <Route path="/i/:code" element={<InvitePage />}></Route>
            <Route path="/" element={<Loading />}></Route>
        </Routes>
    );
}

createRoot(document.getElementById('root')!).render(
    <StrictMode>
        <AppProvider>
            <BrowserRouter>
                <Main />
            </BrowserRouter>
        </AppProvider>
    </StrictMode>
);

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

const Test = () => {
    return (
        <div className='h-screen w-screen bg-gray-900'>
            <div className='flex h-screen max-w-50'>
                <div className='w-20 bg-gray-900 shrink-0'>
                    ok
                </div>
                <div className="bg-gray-800 max-w-full">
                    <div className='flex m-1 p-1 bg-gray-900 rounded-xl text-white text-nowrap overflow-hidden'>
                        <div className='bg-red-500 shrink-0 p-1'>
                            B
                        </div>
                        <div className='bg-green-500 min-w-0 p-1 text-nowrap text-ellipsis overflow-hidden'>
                        yodadwaodijawdoijawdoijwadoijawdoiajwdoiajdwoaidjwoijda
                        </div>
                        <div className='bg-blue-500 shrink-0 p-1'>
                            E
                        </div>
                    </div>
                    <div className='m-1 p-1 bg-gray-900 rounded-xl text-white text-nowrap overflow-hidden'>
                        dd
                    </div>
                </div>
            </div>
        </div>
    );
};

const Main = () => {
    const navigator = useNavigate();
    const location = useLocation();

    useEffect(() => {
        if (location.pathname === "/login") return;
        if (location.pathname === "/test") return;
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
            <Route path="/app" element={<MainApp />}>
                <Route path="hubs/:hub_id" />
                <Route path="hubs/:hub_id/channels/:channel_id" />
                <Route path="dms/:dm_id" />
                <Route path="groups/:group_id" />
            </Route>
            <Route path="/login" element={<Login />}></Route>
            <Route path="/i/::code" element={<InvitePage />}></Route>
            <Route path="/test" element={<Test />}></Route>
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

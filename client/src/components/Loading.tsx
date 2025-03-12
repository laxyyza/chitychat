import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import websocketClient from '../services/websocketClient';

const Loading = () => {
    const [animation, setAnimation] = useState('animate-bounce');
    const emotes = ['>_<', ':3', ':)', ':(', '^_^', '>:(', '>:)'];
    const [emote, setEmote] = useState(emotes[0]);
    const [status, setStatus] = useState('');
    const navigate = useNavigate();

    useEffect(() => {
        const interval = setInterval(() => {
            setAnimation((prev) =>
                prev === 'animate-spin' ? '' : 'animate-spin'
            );
        }, 3000);
        return () => clearInterval(interval);
    }, []);

    useEffect(() => {
        const interval = setInterval(() => {
            setEmote(emotes[Math.floor(Math.random() * emotes.length)]);
        }, 1200);
        return () => clearInterval(interval);
    }, []);

    useEffect(() => {
        websocketClient.onStateChange((state: string) => {
            setStatus(state);
            if (state === 'open') {
                navigate('/app');
            } else if (state === 'close' || state === 'error') {
                navigate('/login');
            }
        });

        return () => websocketClient.onStateChange(undefined);
    }, []);

    return (
        <>
            <div className="flex justify-center items-center w-screen h-screen bg-black">
                <h1 className="text-white text-6xl text-cente m-2 animate-pulse">
                    Chity Chat...
                </h1>
                <div className="animate-bounce">
                    <div
                        className={
                            'w-14 h-14 rounded-full border-blue-500 bg-blue-500 border-5 font-bold text-3xl text-center ' +
                            animation
                        }
                    >
                        {emote}
                    </div>
                </div>
                <span className="ml-4 top-0 bottom-0 text-white">{status}</span>
            </div>
        </>
    );
};

export default Loading;

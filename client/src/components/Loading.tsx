import { useEffect, useState } from 'react';

const Loading = () => {
    const [animation, setAnimation] = useState('animate-bounce');
    const emotes = ['>_<', ':3', ':)', ':(', '^_^'];
    const [emote, setEmote] = useState(emotes[0]);

    useEffect(() => {
        const interval = setInterval(() => {
            setAnimation((prev) =>
                prev === 'animate-spin' ? '' : 'animate-spin'
            );
        }, 2000);
        return () => clearInterval(interval);
    }, []);

    useEffect(() => {
        const interval = setInterval(() => {
            setEmote(emotes[Math.floor(Math.random() * emotes.length)]);
        }, 6000);
        return () => clearInterval(interval);
    }, []);

    return (
        <>
            <div className="flex justify-center items-center w-screen h-screen bg-black">
                <h1 className="text-white text-5xl text-cente m-2 animate-pulse">
                    Chity Chat...
                </h1>
                <div className="animate-bounce">
                    <div
                        className={
                            'w-13 h-13 rounded-full border-blue-500 bg-blue-500 border-5 font-bold text-2xl text-center ' +
                            animation
                        }
                    >
                        {emote}
                    </div>
                </div>
                <span className="ml-4 text-xs top-0 bottom-0 text-white">
                    Connecting...
                </span>
            </div>
        </>
    );
};

export default Loading;

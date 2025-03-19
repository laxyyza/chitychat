import { useEffect, useRef, useState } from 'react';
import { useApp } from './AppProvider';
import ChannelMessages from './ChannelMessages';
import { FaArrowDown } from 'react-icons/fa';
import Message from '../../models/message';

const getMessages = (): Message[] => {
    const { app } = useApp();

    if (app.currentHubID !== -1) {
        const channel = app.textChannels.get(app.currentChannelID);
        if (channel) {
            return channel.messages;
        }
    } else if (app.currentGroupID !== -1) {
        const group = app.groups.get(app.currentGroupID);
        if (group) {
            return group.messages;
        }
    }

    return [];
};

const ChatWindow = () => {
    const { app } = useApp();
    const containerRef = useRef<HTMLDivElement | null>(null);
    const [isBottom, setIsBottom] = useState(true);
    const bottomRef = useRef<HTMLDivElement | null>(null);
    const messages = getMessages();

    const handleScroll = () => {
        const container = containerRef.current;
        if (!container) return;

        const threshold = 100;
        const bottom =
            container.scrollHeight -
                (container.scrollTop + container.clientHeight) <
            threshold;

        setIsBottom(bottom);
    };

    useEffect(() => {
        containerRef.current?.addEventListener('scroll', handleScroll);
        return () =>
            containerRef.current?.removeEventListener('scroll', handleScroll);
    }, []);

    useEffect(() => {
        bottomRef.current?.scrollIntoView({ behavior: 'instant' });
    }, [app.currentHubID, app.currentGroupID, app.currentChannelID]);

    useEffect(() => {
        if (
            isBottom ||
            messages.at(messages.length - 1)?.user_id === app.login_user.id
        ) {
            bottomRef.current?.scrollIntoView({ behavior: 'instant' });
        }
    }, [messages]);

    return (
        <>
            <div className="flex-1 overflow-y-auto" ref={containerRef}>
                <ChannelMessages messages={messages} />
                <div ref={bottomRef} />
            </div>
            {isBottom || (
                <div
                    className="bg-gray-800 border-1 border-black text-white self-center p-1 rounded-full hover:text-blue-400 active:bg-gray-700"
                    onClick={() =>
                        bottomRef.current?.scrollIntoView({
                            behavior: 'smooth'
                        })
                    }
                >
                    <FaArrowDown size="24" />
                </div>
            )}
        </>
    );
};

export default ChatWindow;

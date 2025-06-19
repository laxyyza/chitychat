import { useEffect, useRef, useState } from 'react';
import { DispatchAction, useApp } from './AppProvider';
import ChannelMessages from './ChannelMessages';
import { FaArrowDown } from 'react-icons/fa';
import Message from '../../models/message';
import { DMChat } from '../../models/dm';
import Input from './Input';
import { Hub } from '../../models/hub';

const requestChatMessages = (dmChat: DMChat, dispatch: React.Dispatch<DispatchAction>) => {
    dmChat.requestMsgs = true;
    dmChat.fetchMessages(dispatch);
}

const getMessages = (): [Message[], Hub | DMChat | null] => {
    const { app, dispatch } = useApp();

    switch (app.focus.type) {
        case "dm": {
            const dmchat = app.dm.get(app.focus.dmid);
            if (dmchat) {
                const msgs = Array.from(dmchat.chat.messages.entries())
                    .map(([_, msg]) => {
                        return msg;
                    })
                    .sort((a, b) => a.id - b.id);

                if (msgs.length === 0 && dmchat.requestMsgs === false) {
                    requestChatMessages(dmchat, dispatch);
                }

                return [msgs, dmchat];
            }
            break;
        }
        case "hub": {
            const hub = app.hubs.get(app.focus.hubID);
            if (hub) {
                return [hub.getMessages(dispatch), hub];
            }
            break;
        }
    }

    return [[], null];
};

const ChatWindow = () => {
    const { app, dispatch } = useApp();
    const appRef = useRef(app);
    const containerRef = useRef<HTMLDivElement | null>(null);
    const [isBottom, setIsBottom] = useState(true);
    const bottomRef = useRef<HTMLDivElement | null>(null);
    const [messages, target] = getMessages();
    const [isTop, setIsTop] = useState(false);
    const [oldScroll, setOldScroll] = useState(0);

    useEffect(() => {
        appRef.current = app;
    }, [app]);

    const fetchMoreMessages = (container: HTMLDivElement) => {
        if (oldScroll === container.scrollHeight) {
            return;
        }

        setOldScroll(container.scrollHeight);

        if (app.focus.type === "dm") {
            const dmchat = app.dm.get(app.focus.dmid);
            dmchat?.fetchMessages(dispatch);
        } else if (app.focus.type === "hub") {
            // const hub = app.hubs.get(app.focus.hubID);
            // hub?.fetchMessages(dispatch);
        }
    }

    const handleScroll = () => {
        const container = containerRef.current;
        if (!container) return;

        const threshold = 100;
        const bottom =
            container.scrollHeight -
            (container.scrollTop + container.clientHeight) <
            threshold;
        // const group = appRef.current.groups.get(appRef.current.currentGroupID);
        // if (group) {
        //     group.scrollTop = container.scrollTop;
        // }

        setIsTop(container.scrollTop === 0);
        if (container.scrollTop === 0) {
            fetchMoreMessages(container);
        }

        setIsBottom(bottom);
    };

    useEffect(() => {
        containerRef.current?.addEventListener('scroll', handleScroll);
        return () =>
            containerRef.current?.removeEventListener('scroll', handleScroll);
    }, []);

    useEffect(() => {
        if (isTop) {
            requestAnimationFrame(() => {
                if (containerRef.current) {
                    const newHeight = containerRef.current?.scrollHeight;
                    const diff = newHeight - oldScroll;

                    containerRef.current.scrollTop = diff;
                    setIsTop(false);
                }
            });
        } else if (isBottom) {
            bottomRef.current?.scrollIntoView({ behavior: 'instant' });
        }
    }, [messages]);

    if (!target)
        return null;

    return (
        <>
            <div className="flex-1 overflow-y-auto" ref={containerRef}>
                <ChannelMessages messages={messages} />
                <div ref={bottomRef} />
            </div>
            {isBottom || (
                <button
                    className="absolute bottom-15 bg-gray-800 border-1 border-black text-white self-center p-1 rounded-full hover:text-blue-400 active:bg-gray-700"
                    onClick={() =>
                        bottomRef.current?.scrollIntoView({
                            behavior: 'smooth'
                        })
                    }
                >
                    <FaArrowDown size="24" />
                </button>
            )}
            <div className="bg-gray-900 m-3 rounded-2xl text-white max-h-[50%] border-gray-600 border-1">
                <Input target={target} attachments placeholder="Type a message..." />
            </div>
        </>
    );
};

export default ChatWindow;

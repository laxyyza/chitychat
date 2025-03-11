import MessageComponent from './Message';
import { useEffect, useRef } from 'react';
import { Message } from './Hub';

interface Prop {
    messages: Message[] | undefined;
}

const ChannelMessages = ({ messages }: Prop) => {
    const bottomRef = useRef<HTMLDivElement | null>(null);

    useEffect(() => {
        bottomRef.current?.scrollIntoView({ behavior: 'instant' });
    }, [messages]);

    if (!messages) return null;

    return (
        <>
            {messages.map((msg) => (
                <li key={msg.id}>
                    <MessageComponent message={msg} />
                </li>
            ))}
            <div ref={bottomRef} />
        </>
    );
};

export default ChannelMessages;

import MessageComponent from './Message';
import { useEffect, useRef } from 'react';
import { Message } from './Hub';

interface Prop {
    messages: Message[] | undefined;
}

const ChannelMessages = ({ messages }: Prop) => {
    return (
        <>
            {messages?.map((msg) => (
                <li key={msg.id}>
                    <MessageComponent message={msg} />
                </li>
            ))}
        </>
    );
};

export default ChannelMessages;

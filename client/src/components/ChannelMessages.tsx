import { useEffect, useState } from 'react';
import { TextChannel, Message } from './Hub';
import MessageComponent from './Message';

interface Prop {
    channel: TextChannel;
}

const ChannelMessages = ({ channel }: Prop) => {
    const [messages, setMessages] = useState<Message[]>([]);

    useEffect(() => {
        setMessages(channel.messages);
    }, [channel]);

    return (
        <>
            {messages.map((msg) => (
                <li key={msg.id}>
                    <MessageComponent message={msg} />
                </li>
            ))}
        </>
    );
};

export default ChannelMessages;

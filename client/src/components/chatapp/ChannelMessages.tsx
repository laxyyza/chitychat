import MessageComponent from './Message';
import { Message } from '../../models/hub';

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

import MessageComponent from './Message';
import Message from '../../models/message';

interface Prop {
    messages: Message[];
}

const ChannelMessages = ({ messages }: Prop) => {
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

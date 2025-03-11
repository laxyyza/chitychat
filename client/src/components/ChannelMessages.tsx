import MessageComponent from './Message';
import { useApp } from './AppProvider';

const ChannelMessages = () => {
    const { app } = useApp();
    const hub = app.hubs.get(app.currentHubID);
    if (!hub) return null;

    const channel = app.textChannels.get(app.currentChannelID);
    if (!channel) return null;

    return (
        <>
            {channel.messages.map((msg) => (
                <li key={msg.id}>
                    <MessageComponent message={msg} />
                </li>
            ))}
        </>
    );
};

export default ChannelMessages;

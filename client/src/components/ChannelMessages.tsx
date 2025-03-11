import { ChannelType } from './Hub';
import MessageComponent from './Message';
import { useApp } from './AppProvider';

const ChannelMessages = () => {
    const { app } = useApp();
    const hub = app.hubs.get(app.currentHubID);
    if (!hub) return null;

    // const channel = app.textChannels.get(app.currentChannelID);
    // if (!channel) return null;
    const group = app.groups.get(app.currentChannelID);
    if (!group) return null;

    console.log('group', group.name, ' msgs: ', group.messages);
    // console.log('Channel ', channel.name, ' msgs: ', channel.messages);

    return (
        <>
            {group.messages.map((msg) => (
                <li key={msg.id}>
                    <MessageComponent message={msg} />
                </li>
            ))}
            {/* {messages.map((msg) => (
                <li key={msg.id}>
                    <MessageComponent message={msg} />
                </li>
            ))} */}
        </>
    );
};

export default ChannelMessages;

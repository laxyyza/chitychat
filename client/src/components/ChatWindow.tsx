import { useApp } from './AppProvider';
import ChannelMessages from './ChannelMessages';

const ChatWindow = () => {
    const { app } = useApp();
    const channel = app.textChannels.get(app.currentChannelID);

    return (
        <div className="flex-1 overflow-y-auto">
            <ChannelMessages messages={channel?.messages} />
        </div>
    );
};

export default ChatWindow;

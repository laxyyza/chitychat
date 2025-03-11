import ChannelMessages from './ChannelMessages';

const ChatWindow = () => {
    return (
        <div className="flex-1 overflow-y-auto">
            <ChannelMessages />
        </div>
    );
};

export default ChatWindow;

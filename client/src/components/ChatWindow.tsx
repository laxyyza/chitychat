import { useEffect, useState } from 'react';
import { ChannelType, TextChannel } from './Hub';
import ChannelMessages from './ChannelMessages';
import { useApp } from './AppProvider';

const ChatWindow = () => {
    const { app } = useApp();
    const [channel, setChannel] = useState<TextChannel | null>(null);

    useEffect(() => {
        const hub = app.hubs[app.hubIndex];
        if (!hub) return;
        const newChannel = hub.channels.get(app.selectedChannelID);
        if (newChannel?.type === ChannelType.TEXT) setChannel(newChannel);
        else setChannel(null);
    }, [app.selectedChannelID]);

    return (
        <div className="flex-1 overflow-y-auto">
            {channel && <ChannelMessages channel={channel} />}
        </div>
    );
};

export default ChatWindow;

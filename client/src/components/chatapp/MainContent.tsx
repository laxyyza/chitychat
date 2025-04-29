import Input from './Input';
import ChatWindow from './ChatWindow';
import { App, useApp } from './AppProvider';
import FriendList from './FriendList';
import { DM } from '../../models/dm';

interface HeaderBarProp {
    name: string;
}

const HeaderBar = ({ name }: HeaderBarProp) => {
    return (
        <div className="bg-gray-800 text-center text-white shadow">{name}</div>
    );
};

const getContentName = (app: App): string => {
    var channelName: string = '';
    if (app.currentChannelID !== -1) {
        const hub = app.hubs.get(app.currentHubID);
        if (hub) {
            const channel = app.textChannels.get(app.currentChannelID);
            if (channel) {
                channelName = channel.name;
            } else {
                channelName = hub.name;
            }
        }
    }
    if (app.currentDMID === 'friends') channelName = 'Friends List';
    else {
        const dmchat = app.dm.get(app.currentDMID);
        if (dmchat?.chat instanceof DM) {
            const user = app.users.get(dmchat.chat.targetUserID);
            if (user) {
                channelName = user.displayname;
            }
        } else if (dmchat) {
            channelName = dmchat.chat.name;
        }
    }

    return channelName;
};

const MainContent = () => {
    const { app } = useApp();
    const channelName = getContentName(app);

    return (
        <div className="flex flex-col flex-1 h-screen bg-gray-800">
            <HeaderBar name={channelName}></HeaderBar>
            {app.currentDMID === 'friends' ? <FriendList /> : <ChatWindow />}
            <div className="bg-gray-900 m-3 rounded-2xl text-white max-h-[50%] overflow-auto">
                <Input />
            </div>
        </div>
    );
};

export default MainContent;

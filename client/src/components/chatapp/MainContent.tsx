import ChatWindow from './ChatWindow';
import { App, useApp } from './AppProvider';
import FriendList from './FriendList';
import Group from '../../models/group';

interface HeaderBarProp {
    name: string;
}

const HeaderBar = ({ name }: HeaderBarProp) => {
    return (
        <div className="bg-gray-900 text-center text-white h-10 shadow-xl flex select-none justify-center items-center">
            <span>
                {name}
            </span>
        </div>
    );
};

const getContentName = (app: App): string => {
    switch (app.focus.type) {
        case "friends":
            return "Friends List";
        case "hub": {
            const hub = app.hubs.get(app.focus.hubID);
            if (hub) {
                return hub.channels.get(hub.selectedChannelID)?.name || "";
            }
            return "";
        }
        case "dm": {
            const dmchat = app.dm.get(app.focus.dmid);
            if (dmchat) {
                if (dmchat.chat instanceof Group) {
                    return dmchat.chat.name;
                }
            }
            return "";
        }
    }
};

const MainContent = () => {
    const { app } = useApp();
    const channelName = getContentName(app);

    return (
        <div className="flex flex-col h-screen grow-1 bg-gray-800 min-w-0">
            <HeaderBar name={channelName}></HeaderBar>
            {app.focus.type === 'friends' ? <FriendList /> : <ChatWindow />}
        </div>
    );
};

export default MainContent;

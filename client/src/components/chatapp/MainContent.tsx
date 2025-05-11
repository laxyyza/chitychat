import Input from './Input';
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
            <div className="bg-gray-900 m-3 rounded-2xl text-white max-h-[50%] border-gray-600 border-1 p-0.5">
                <Input />
            </div>
        </div>
    );
};

export default MainContent;

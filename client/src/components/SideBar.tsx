import HubList from './HubList';
import ChannelList from './ChannelList';
import UserProfile from './UserProfile';
import { useApp } from './AppProvider';
import { useContext } from 'react';

const SideBar = () => {
    const { app } = useApp();

    return (
        <div className="sidebar">
            <div className="flex flex-1">
                <HubList />
                <ChannelList />
            </div>
            <UserProfile user={app.login_user} />
        </div>
    );
};

export default SideBar;

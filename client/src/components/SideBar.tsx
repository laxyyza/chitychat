import HubList from './HubList';
import ChannelList from './ChannelList';
import UserProfile from './UserProfile';
import { AppCtx } from './AppProvider';
import { useContext } from 'react';

const SideBar = () => {
    const app = useContext(AppCtx);

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

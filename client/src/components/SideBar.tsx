import HubList from './HubList';
import ChannelList from './ChannelList';
import UserProfile from './UserProfile';

const SideBar = () => {
    return (
        <div className="sidebar">
            <div className="flex flex-1">
                <HubList />
                <ChannelList />
            </div>
            <UserProfile />
        </div>
    );
};

export default SideBar;

import HubList from './HubList';
import ChannelList from './ChannelList';
import UserProfile from './UserProfile';

const SideBar = () => {
    return (
        <div className="sidebar">
            <HubList />
            <ChannelList />
            <UserProfile />
        </div>
    );
};

export default SideBar;

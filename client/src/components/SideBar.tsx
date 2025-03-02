import HubList from './HubList';
import Channels from './Channels';
import UserProfile from './UserProfile';

const SideBar = () => {
    return (
        <div className="sidebar">
            <HubList />
            <Channels />
            <UserProfile />
        </div>
    );
};

export default SideBar;

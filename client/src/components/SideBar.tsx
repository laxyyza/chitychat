import HubList from './HubList';
import ChannelList from './ChannelList';
import UserProfile from './UserProfile';

const SideBar = () => {
    const user: User = {
        id: 41,
        username: 'username',
        displayname: 'Display Name',
        about_me: 'ABOUT ME',
        pfp: 'https://www.oola.com/wp-content/uploads/2022/07/communityIcon_x4lqmqzu1hi81.jpeg'
    };

    return (
        <div className="sidebar">
            <div className="flex flex-1">
                <HubList />
                <ChannelList />
            </div>
            <UserProfile user={user} />
        </div>
    );
};

export default SideBar;

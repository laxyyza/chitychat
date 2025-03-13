import SideBarList from './SideBarList';
import ContentSideBar from './ContentSideBar';
import UserProfile from './UserProfile';
import { useApp } from '../AppProvider';

const SideBar = () => {
    const { app } = useApp();

    return (
        <div className="sidebar">
            <div className="flex flex-1">
                <SideBarList />
                <ContentSideBar />
            </div>
            <UserProfile user={app.login_user} />
        </div>
    );
};

export default SideBar;

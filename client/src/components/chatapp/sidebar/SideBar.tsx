import SideBarList from './SideBarList';
import ContentSideBar from './ContentSideBar';
import UserProfile from './UserProfile';
import { useApp } from '../AppProvider';

const SideBar = () => {
    const { app } = useApp();

    return (
        <div className="sidebar h-screen max-h-screen">
            <div className="flex h-full max-h-full overflow-hidden">
                <SideBarList />
                <ContentSideBar />
            </div>
            <UserProfile user={app.login_user} />
        </div>
    );
};

export default SideBar;

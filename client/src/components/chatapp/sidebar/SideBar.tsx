import SideBarList from './SideBarList';
import ContentSideBar from './ContentSideBar';
import UserProfile from './UserProfile';
import { useApp } from '../AppProvider';

const SideBar = () => {
    const { app } = useApp();

    return (
        <div className="flex flex-col h-screen max-h-screen border-r-gray-700 border-r-1 bg-gray-900 shrink-0 w-70 max-w-70">
            <div className="flex grow-1 min-w-0 overflow-hidden">
                <SideBarList />
                <ContentSideBar />
            </div>
            <div className='shrink-0'>
                <UserProfile user={app.login_user} />
            </div>
        </div>
    );
};

export default SideBar;

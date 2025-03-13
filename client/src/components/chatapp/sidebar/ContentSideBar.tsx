import { useApp } from '../AppProvider';
import ChannelList from './ChannelList';
import DMList from './DMList';

const ContentSideBar = () => {
    const { app } = useApp();

    return (
        <div className="flex-1 bg-gray-800 text-white">
            {app.currentHubID === -1 ? <DMList /> : <ChannelList />}
        </div>
    );
};

export default ContentSideBar;

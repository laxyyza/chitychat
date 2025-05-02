import { useApp } from '../AppProvider';
import ChannelList from './ChannelList';
import DMList from './DMList';

const ContentSideBar = () => {
    const { app } = useApp();

    return (
        <div className="flex-1 bg-gray-900 text-white p-1">
            {app.focus.type === "hub" ? <ChannelList /> : <DMList />}
        </div>
    );
};

export default ContentSideBar;

import { useApp } from '../AppProvider';
import ChannelList from './ChannelList';
import DMList from './DMList';

const ContentSideBar = () => {
    const { app } = useApp();

    return (
        <div className="flex flex-col bg-gray-900 text-white min-w-0 w-full">
            {app.focus.type === "hub" ? <ChannelList /> : <DMList />}
        </div>
    );
};

export default ContentSideBar;

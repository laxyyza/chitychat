import { useApp } from '../AppProvider';
import ChannelList from './ChannelList';
import DMList from './DMList';

const ContentSideBar = () => {
    const { app } = useApp();

    return (
        <div className="grow bg-gray-900 text-white p-1 max-w-full">
            {app.focus.type === "hub" ? <ChannelList /> : <DMList />}
        </div>
    );
};

export default ContentSideBar;

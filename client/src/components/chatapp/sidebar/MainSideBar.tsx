import { useApp } from '../AppProvider';
import ChannelList from './ChannelList';

const MainSideBar = () => {
    const { app } = useApp();

    if (app.currentHubID === -1) {
    } else {
        return <ChannelList />;
    }
};

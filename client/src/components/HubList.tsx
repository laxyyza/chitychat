import { useContext } from 'react';
import { AppCtx } from './AppProvider';
import { HubComponent } from './Hub';

const HubList = () => {
    const app = useContext(AppCtx);
    const hubs = app.hubs;

    return (
        <div className="flex flex-col bg-gray-900">
            <div className="flex-1">
                {hubs.map((hub) => (
                    <HubComponent hub={hub}></HubComponent>
                ))}
            </div>
        </div>
    );
};

export default HubList;

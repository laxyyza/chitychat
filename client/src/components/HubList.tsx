import { useContext, useState } from 'react';
import { AppCtx } from './AppProvider';
import { HubComponent, Hub } from './Hub';

const HubList = () => {
    const app = useContext(AppCtx);
    const hubs = app.hubs;
    const [selectedHub, setSelectHub] = useState<Hub>(hubs[0]);

    return (
        <div className="flex flex-col bg-gray-900">
            <div className="flex-1">
                {hubs.map((hub, index) => (
                    <div
                        key={index}
                        className={`hub-icon group ${
                            selectedHub === hub ? 'hub-selected' : ''
                        }`}
                        onClick={() => setSelectHub(hub)}
                    >
                        <HubComponent
                            selected={selectedHub === hub}
                            hub={hub}
                        ></HubComponent>
                    </div>
                ))}
            </div>
        </div>
    );
};

export default HubList;

import { useState } from 'react';
import { useApp, Action } from './AppProvider';
import { HubComponent, Hub } from './Hub';

const HubList = () => {
    const { app, dispatch } = useApp();
    const hubs = Array.from(app.hubs.values());

    return (
        <div className="flex flex-col bg-gray-900">
            <div className="flex-1">
                {hubs.map((hub) => (
                    <div
                        key={hub.id}
                        className={`hub-icon group ${
                            app.currentHubID === hub.id ? 'hub-selected' : ''
                        }`}
                        onClick={() => {
                            dispatch({
                                type: Action.SELECT_HUB,
                                payload: hub.id
                            });
                            dispatch({
                                type: Action.SELECT_CHANNEL,
                                payload: -1
                            });
                        }}
                    >
                        <HubComponent
                            selected={app.currentHubID === hub.id}
                            hub={hub}
                        ></HubComponent>
                    </div>
                ))}
            </div>
        </div>
    );
};

export default HubList;

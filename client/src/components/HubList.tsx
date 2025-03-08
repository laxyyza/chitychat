import { useState } from 'react';
import { useApp, Action } from './AppProvider';
import { HubComponent, Hub } from './Hub';

const HubList = () => {
    const { app, dispatch } = useApp();
    const hubs = app.hubs;

    return (
        <div className="flex flex-col bg-gray-900">
            <div className="flex-1">
                {hubs.map((hub, index) => (
                    <div
                        key={index}
                        className={`hub-icon group ${
                            app.hubIndex === index ? 'hub-selected' : ''
                        }`}
                        onClick={() => {
                            dispatch({
                                type: Action.SELECT_HUB,
                                payload: index
                            });
                            dispatch({
                                type: Action.SELECT_CHANNEL,
                                payload: -1
                            });
                        }}
                    >
                        <HubComponent
                            selected={app.hubIndex === index}
                            hub={hub}
                        ></HubComponent>
                    </div>
                ))}
            </div>
        </div>
    );
};

export default HubList;

import { useRef, useState } from 'react';
import { useApp, Action } from './AppProvider';
import { HubComponent, Hub } from './Hub';
import CreateHub from './CreateHub';

const HubList = () => {
    const { app, dispatch } = useApp();
    const hubs = Array.from(app.hubs.values());
    const divref = useRef<HTMLDivElement | null>(null);
    const [show, setShow] = useState(false);

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
                        }}
                    >
                        <HubComponent
                            tooltip={hub.name}
                            pfp={hub.pfp}
                            selected={app.currentHubID === hub.id}
                        >
                            {hub.name}
                        </HubComponent>
                    </div>
                ))}
            </div>
            <div
                ref={divref}
                className="hub-icon group"
                onClick={() => {
                    setShow(true);
                }}
            >
                <HubComponent
                    tooltip="Create a Hub"
                    pfp={null}
                    selected={false}
                >
                    +
                </HubComponent>
            </div>
            {show && (
                <CreateHub
                    onClose={() => {
                        setShow(false);
                    }}
                />
            )}
        </div>
    );
};

export default HubList;

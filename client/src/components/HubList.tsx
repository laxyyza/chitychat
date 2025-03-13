import { useRef, useState } from 'react';
import { useApp, Action } from './AppProvider';
import { HubComponent } from './Hub';
import CreateHub from './CreateHub';
import logo from '../assets/logo.svg';

const Divider = () => {
    return <div className="w-[80%] h-[1px] bg-gray-600 self-center m-1"></div>;
};

const HubList = () => {
    const { app, dispatch } = useApp();
    const hubs = Array.from(app.hubs.values());
    const divref = useRef<HTMLDivElement | null>(null);
    const [show, setShow] = useState(false);

    return (
        <div className="flex flex-col bg-gray-900">
            <div
                ref={divref}
                className="hub-icon group"
                onClick={() => {
                    dispatch({ type: Action.SELECT_HUB, payload: -1 });
                }}
            >
                <HubComponent
                    tooltip="Direct Messages"
                    pfp={logo}
                    selected={app.currentHubID === -1}
                >
                    {' '}
                </HubComponent>
            </div>
            <Divider />

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
            <Divider />
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

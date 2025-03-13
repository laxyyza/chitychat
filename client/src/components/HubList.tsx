import { useRef, useState } from 'react';
import { useApp, Action } from './AppProvider';
import CreateHub from './CreateHub';
import logo from '../assets/logo.svg';
import SideBarButton from './SideBarButton';

const Divider = () => {
    return <div className="w-[80%] h-[1px] bg-gray-600 self-center m-1" />;
};

const HubList = () => {
    const { app, dispatch } = useApp();
    const hubs = Array.from(app.hubs.values());
    const divref = useRef<HTMLDivElement | null>(null);
    const [show, setShow] = useState(false);

    return (
        <div className="flex flex-col bg-gray-900">
            <SideBarButton
                ref={divref}
                onClick={() => {
                    dispatch({ type: Action.SELECT_HUB, payload: -1 });
                }}
                tooltip="Direct Messages"
                selected={app.currentHubID === -1}
                pfp={logo}
            />
            <Divider />

            <div className="flex-1">
                {hubs.map((hub) => (
                    <SideBarButton
                        onClick={() => {
                            dispatch({
                                type: Action.SELECT_HUB,
                                payload: hub.id
                            });
                        }}
                        tooltip={hub.name}
                        pfp={hub.pfp}
                        selected={app.currentHubID === hub.id}
                        name={hub.name}
                    />
                ))}
            </div>

            <Divider />

            <SideBarButton
                onClick={() => setShow(true)}
                tooltip="Create a Hub"
                pfp={null}
                selected={false}
                name="+"
            />
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

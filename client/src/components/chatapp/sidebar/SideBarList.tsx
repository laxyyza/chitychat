import { useRef, useState } from 'react';
import { useApp, Action } from '../AppProvider';
import CreateHub from '../modals/CreateHub';
import logo from '../../../assets/logo.svg';
import SideBarButton from './SideBarButton';
import { useNavigate } from 'react-router-dom';

const Divider = () => {
    return <div className="w-[80%] h-[1px] bg-gray-600 self-center m-1" />;
};

const SideBarList = () => {
    const { app, dispatch } = useApp();
    const hubs = Array.from(app.hubs.values());
    const divref = useRef<HTMLDivElement | null>(null);
    const [show, setShow] = useState(false);
    const navigator = useNavigate();

    return (
        <div className="flex flex-col bg-gray-900 border-r-gray-700 border-r-1">
            <SideBarButton
                ref={divref}
                onClick={() => {
                    dispatch({ type: Action.SET_FOCUS, payload: {type: "friends"} });
                }}
                tooltip="Direct Messages"
                selected={app.focus.type !== "hub"}
                pfp={logo}
            />
            <Divider />

            <div className="flex-1 overflow-auto hidden-scroll-container">
                {hubs.map((hub) => (
                    <li key={hub.id}>
                        <SideBarButton
                            onClick={() => {
                                dispatch({
                                    type: Action.SET_FOCUS,
                                    payload: { type: "hub", hubID: hub.id, channelID: hub.selectedChannelID }
                                });
                                if (hub.selectedChannelID === -1) {
                                    navigator(`/app/hubs/${hub.id}`);
                                } else {
                                    navigator(`/app/hubs/${hub.id}/channels/${hub.selectedChannelID}`);
                                }
                            }}
                            tooltip={hub.name}
                            pfp={hub.pfp}
                            selected={app.focus.type === "hub" && app.focus.hubID === hub.id}
                            name={hub.name}
                        />
                    </li>
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

export default SideBarList;

import { BsChatRightText, BsThreeDotsVertical } from "react-icons/bs";
import { HubTextChannel } from "../../../models/hub";
import { Action, appGetFocusHub, useApp } from "../AppProvider";
import { useNavigate } from "react-router-dom";
import { useState } from "react";

interface Props {
    channel: HubTextChannel;
}

const HubChannel = ({ channel }: Props) => {
    const { app, dispatch } = useApp();
    const hub = appGetFocusHub(app);
    const navigator = useNavigate();
    const [hover, setHover] = useState(false);

    if (!hub) return null;

    const selected = hub.selectedChannelID === channel.id;

    return (
        <div
            onClick={() => {
                dispatch({ type: Action.SELECT_HUB_CHANNEL, payload: { hub: hub, channelID: channel.id } });
                navigator(`/app/hubs/${hub.id}/channels/${channel.id}`);
            }}
            onMouseEnter={() => setHover(true)}
            onMouseLeave={() => setHover(false)}
            className={`flex grow items-center p-1.5 mb-1 rounded-xl min-w-0 ${selected ? "bg-gray-700" : "hover:bg-gray-800"}`}
        >
            <button className={`flex grow items-center text-left min-w-0 space-x-2 ${selected ? "text-white" : "text-gray-300 hover:text-white"}`}>
                <BsChatRightText size="18" className="shrink-0" />
                <span className="text-nowrap text-ellipsis overflow-hidden">{channel.name}</span>
            </button>
            {(hover || selected) &&
                <button className={`shrink-0 p-1 text-gray-500 hover:text-white`}>
                    <BsThreeDotsVertical />
                </button>
            }
        </div>
    );
}

export default HubChannel;

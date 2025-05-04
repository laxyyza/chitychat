import { BsChatRightText, BsThreeDotsVertical } from "react-icons/bs";
import { HubTextChannel } from "../../../models/hub";
import { Action, appGetFocusHub, useApp } from "../AppProvider";

interface Props {
    channel: HubTextChannel;
}

const HubChannel = ({ channel }: Props) => {
    const { app, dispatch } = useApp();
    const hub = appGetFocusHub(app);

    if (!hub) return null;

    const selected = hub.selectedChannelID === channel.id;

    return (
        <button
            onClick={() => {
                dispatch({ type: Action.SELECT_HUB_CHANNEL, payload: { hub: hub, channelID: channel.id } });
            }}
            className={`pl-2 w-full pr-1 m-1 rounded-xl select-none max-w-42 group ${selected ? 'bg-gray-600' : 'hover:bg-gray-800'}`}
        >
            <div className="flex items-center w-full max-w-full">
                <span className='p-1'>
                    <BsChatRightText />
                </span>
                <span className={`text-left text-nowrap text-ellipsis overflow-hidden p-1 grow min-w-0 group-hover:text-white ${(selected) ? "text-white" : "text-gray-400"}`}>
                    {channel.name}
                </span>
                <span className={`text-gray-400 hover:bg-gray-700 p-1 rounded-xl hover:text-white group-hover:scale-100 ${selected ? "scale-100" : "scale-0"}`}>
                    <BsThreeDotsVertical />
                </span>
            </div>
        </button>
    );
}

export default HubChannel;

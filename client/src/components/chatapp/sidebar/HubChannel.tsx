import { HubTextChannel } from "../../../models/hub";
import { Action, appGetFocusHub, useApp } from "../AppProvider";

interface Props {
    channel: HubTextChannel;
}

const HubChannel = ({ channel }: Props) => {
    const { app, dispatch } = useApp();
    const hub = appGetFocusHub(app);

    if (!hub) return null;

    return (
        <div
            onClick={() => {
                dispatch({ type: Action.SELECT_HUB_CHANNEL, payload: { hub: hub, channel: channel } });
            }}
            className={`hover:bg-gray-400 pl-2 pr-2 m-1 rounded-xl select-none ${hub.selectedChannelID === channel.id ? 'bg-gray-500' : ''}`}
        >
            {channel.name}
        </div>
    );
}

export default HubChannel;

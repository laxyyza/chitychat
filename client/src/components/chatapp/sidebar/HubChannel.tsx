import { HubChannelData } from "../../../models/hub";
import { Action, useApp } from "../AppProvider";

interface Props {
    channel: HubChannelData;
}

const HubChannel = ({ channel }: Props) => {
    const { app, dispatch } = useApp();

    return (
        <div
            onClick={() => {
                dispatch({ type: Action.SELECT_CHANNEL, payload: channel.channel_id });
            }}
            className={`hover:bg-gray-400 pl-2 pr-2 m-1 rounded-xl select-none ${app.currentChannelID === channel.channel_id ? 'bg-gray-500' : ''}`}
        >
            {channel.name}
        </div>
    );
}

export default HubChannel;

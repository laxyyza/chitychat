import { useState } from "react";
import { CategoryData } from "../../../models/hub"
import HubChannel from "./HubChannel";
import { useApp } from "../AppProvider";

interface Props {
    category: CategoryData;
}

const HubCategory = ({ category }: Props) => {
    const { app } = useApp();
    const [open, setOpen] = useState(true);
    const channels = category.channels.sort((a, b) => a.position - b.position);
    const selectedChannel = channels.find((channel) => channel.channel_id === app.currentChannelID);

    return (
        <>
            <div
                onClick={() => setOpen(!open)}
                className="cursor-pointer select-none hover:bg-gray-400"
            >
                {open ? '▼' : '▶'} {category.name}
            </div>

            {open && (
                <ol>
                    {channels.map((channel) => (
                        <li key={channel.channel_id}>
                            <HubChannel channel={channel}/>
                        </li>
                    ))}
                </ol>
            )}
            {!open && selectedChannel && (
                <HubChannel channel={selectedChannel}/>
            )}
        </>
    );
}

export default HubCategory;

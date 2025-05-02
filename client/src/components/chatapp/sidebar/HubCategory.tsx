import { useState } from "react";
import { Category } from "../../../models/hub"
import HubChannel from "./HubChannel";
import { appGetFocusHub, useApp } from "../AppProvider";

interface Props {
    category: Category;
}

const HubCategory = ({ category }: Props) => {
    const { app } = useApp();
    const [open, setOpen] = useState(true);
    const hub = appGetFocusHub(app);
    const channels = Array.from(category.channels.values()).sort((a, b) => a.position - b.position);
    const selectedChannel = channels.find((channel) => channel.id === hub?.selectedChannelID);

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
                        <li key={channel.id}>
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

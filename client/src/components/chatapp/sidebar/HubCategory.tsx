import { useState } from "react";
import { Category, Hub } from "../../../models/hub"
import HubChannel from "./HubChannel";
import { IoMdAdd, IoMdArrowDropdown, IoMdArrowDropright } from "react-icons/io";
import CreateChannel from "../modals/CreateChannel";

interface Props {
    category: Category;
    hub: Hub;
}

const HubCategory = ({ category, hub }: Props) => {
    const [open, setOpen] = useState(true);
    const channels = Array.from(hub.channels.values())
        .filter((channel) => channel.categoryID === category.id)
        .sort((a, b) => a.position - b.position);

    const selectedChannel = channels.find((channel) => channel.id === hub?.selectedChannelID);
    const [showCreate, setShowCreate] = useState(false);

    return (
        <>
            <div
                className="relative cursor-pointer text-[12px] select-none flex items-center"
            >
                <span
                    className="text-gray-300 hover:text-white flex-1 flex items-center"
                    onClick={() => setOpen(!open)}
                >
                    {category.name} {open ? <IoMdArrowDropdown size='24' /> : <IoMdArrowDropright size='24' />} 
                </span>
                <span className='text-gray-400 hover:text-white right-0 mr-3 font-bold'
                    onClick={() => {
                        setShowCreate(true);
                    }}
                >
                    <IoMdAdd size="16"/>
                </span>
            </div>

            {open && (
                <ol>
                    {channels.map((channel) => (
                        <li key={channel.id}>
                            <HubChannel channel={channel} />
                        </li>
                    ))}
                </ol>
            )}
            {!open && selectedChannel && (
                <HubChannel channel={selectedChannel} />
            )}
            {showCreate && hub && (
                <CreateChannel 
                    hub={hub} 
                    category={category} 
                    onClose={() => setShowCreate(false)}
                />
            )}
        </>
    );
}

export default HubCategory;

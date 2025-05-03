import { useState } from "react";
import { Category } from "../../../models/hub"
import HubChannel from "./HubChannel";
import { appGetFocusHub, useApp } from "../AppProvider";
import { IoMdAdd, IoMdArrowDropdown, IoMdArrowDropright } from "react-icons/io";

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
        </>
    );
}

export default HubCategory;

import { useEffect, useRef, useState } from "react";
import { appGetFocusHub, useApp } from "../AppProvider";
import HubCategory from "./HubCategory";
import { MdOutlineKeyboardArrowDown } from "react-icons/md";
import HubDropdownMenu from "./HubDropdownMenu";
import { IoMdClose } from "react-icons/io";
import { getHubDetails } from "../../../services/hubApi";

const ChannelList = () => {
    const { app, dispatch } = useApp();
    const hub = appGetFocusHub(app);
    const [showMore, setShowMore] = useState(false);
    const categories = Array.from(hub?.categories.values() || [])
        .sort((a, b) => a.position - b.position);
    const dropDownButtonRef = useRef<HTMLButtonElement | null>(null);

    useEffect(() => {
        if (hub && hub.detailedLoaded === false) {
            getHubDetails(hub.id, dispatch);
        }
    }, [hub]);

    if (!hub) return null;

    return (
        <>
            <div
                className="shadow-xl relative select-none border-gray-600 text-xl font-bold text-center h-10"
            >
                <button 
                    className="flex hover:bg-gray-800 justify-center items-center w-full h-full"
                    ref={dropDownButtonRef}
                    onClick={() => {
                        setShowMore(!showMore);
                    }}
                >
                    <div className="flex-1">
                        {hub.name}
                    </div>
                    <div className="pr-2">
                        {showMore ? <IoMdClose /> : <MdOutlineKeyboardArrowDown />}
                    </div>
                </button>
                <HubDropdownMenu show={showMore} buttonRef={dropDownButtonRef} hub={hub} onClose={() => setShowMore(false)}/>
            </div>
            <div className="h-full p-1 overflow-auto max-w-full">
                {categories.map((category => (
                    <li key={category.id}>
                        <HubCategory hub={hub} category={category} />
                    </li>
                )))}
            </div>
        </>
    );
};

export default ChannelList;

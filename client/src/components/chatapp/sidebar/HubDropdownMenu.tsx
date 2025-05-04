import { MdCategory } from "react-icons/md";
import { IoMdSettings } from "react-icons/io";
import { BsChatLeftTextFill } from "react-icons/bs";
import { useRef, useState } from "react";
import useDismissTrigger from "../../../hooks/useDismissTrigger";
import { Hub } from "../../../models/hub";
import { ImExit } from "react-icons/im";
import CreateCategory from "../modals/CreateCategory";

interface Props {
    show: boolean;
    onClose: () => void;
    hub: Hub;
    buttonRef: React.RefObject<HTMLElement | null>;
}

const Divider = () => {
    return (
        <div className="border-b-1 mt-[1px] mb-[2px] border-gray-600 h-[1px] w-full">
        </div>
    );
};

const HubDropdownMenu = ({ show, onClose, buttonRef, hub }: Props) => {
    const ref = useRef<HTMLDivElement | null>(null);
    const [showPopup, setShowPopup] = useState(false);

    useDismissTrigger(() => {
        onClose();
    }, [ref, buttonRef]);

    return (
        <>
            <div className={"absolute transition-all duration-150 origin-top ease-in-out w-full p-2 pt-1 z-1000 " + ((show) ? "scale-y-100" : "scale-y-0")} ref={ref}>
                <div className="bg-gray-800 rounded-xl p-2 border-1 border-gray-600">
                    <button className="w-full p-1 rounded-xs text-[14px] text-left flex items-center text-gray-600">
                        <span className="flex-1">Settings</span>
                        <IoMdSettings size="16" />
                    </button>
                    <button
                        className="hover:bg-gray-600 w-full p-1 rounded-xs text-[14px] text-left flex items-center"
                        onClick={() => {
                            setShowPopup(true);
                            onClose();
                        }}
                    >
                        <span className="flex-1">Create Category</span>
                        <MdCategory size="16" />
                    </button>
                    <button
                        className="text-gray-600 w-full p-1 rounded-xs text-[14px] text-left flex items-center">
                        <span className="flex-1">Create Channel</span>
                        <BsChatLeftTextFill size="16" />
                    </button>
                    <Divider />
                    <button
                        className="hover:bg-gray-600 w-full p-1 rounded-xs text-red-400 text-[14px] text-left flex items-center">
                        <span className="flex-1">Leave Hub</span>
                        <ImExit size="16" />
                    </button>
                </div>
            </div>
            {showPopup && (
                <CreateCategory hub={hub} onClose={() => {
                    setShowPopup(false);
                }}/>
            )}
        </>
    );
}

export default HubDropdownMenu;

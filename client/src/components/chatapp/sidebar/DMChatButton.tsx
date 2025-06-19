import { ReactNode, useEffect, useRef, useState } from "react";
import { DM, DMChat } from "../../../models/dm"
import { useApp } from "../AppProvider";
import { FaUser } from "react-icons/fa6";
import { BiSolidGroup } from "react-icons/bi";
import Group from "../../../models/group";
import DMChatMenu from "./DMChatMenu";

interface Props {
    dmchat?: DMChat;
    onClick?: () => void;
    selected: boolean;
    dmlistRef?: React.RefObject<HTMLDivElement | null>;
    children?: ReactNode;
}

const Icon = (type: string, imageUrl?: string) => {
    if (imageUrl) {
        return (
            <div className="bg-purple-500 rounded-full w-8 h-8 overflow-hidden flex items-center justify-center">
                <img src={imageUrl} className="w-full h-full" />
            </div>
        );
    }

    if (type === 'group') {
        return (
            <div className="bg-purple-500 rounded-full w-8 h-8 overflow-hidden flex items-center justify-center">
                <BiSolidGroup size="24" />
            </div>
        );
    } else if (type === 'user') {
        return (
            <div className="bg-blue-500 rounded-full w-8 h-8 overflow-hidden flex items-center justify-center">
                <FaUser size="20" />
            </div>
        );
    } else {
        return null;
    }
};


const DMChatButton = ({ dmchat, onClick, selected, children, dmlistRef }: Props) => {
    const { app } = useApp();
    const [show, setShow] = useState(false);
    const ref = useRef<HTMLButtonElement | null>(null);
    var type: string = "other";
    var name: string | undefined;
    var imageUrl: string | undefined
    if (dmchat?.chat instanceof DM) {
        type = "user";
        const user = app.users.get(dmchat.chat.targetUserID);
        name = user?.displayname;
        imageUrl = user?.pfp_url || undefined;
    } else if (dmchat?.chat instanceof Group) {
        type = "group";
        name = dmchat.chat.name;
    }

    useEffect(() => {
        if (!dmlistRef || !dmlistRef.current)
            return;

        // Disable scrolling when showing context menu.
        if (show) {
            dmlistRef.current.style.overflow = "hidden";
        } else {
            dmlistRef.current.style.overflow = "";
        }
    }, [show]);

    return (
        <button
            ref={ref}
            className={
                'relative flex mb-1 items-center rounded-[8px] hover:bg-gray-600 active:bg-gray-500 max-w-full w-full p-1 ' +
                (selected ? 'bg-gray-600' : '')
            }
            onClick={(e) => {
                setShow(false);
                onClick?.call(e);
            }}
            onContextMenu={(e) => {
                e.preventDefault();
                setShow(!show);
            }}
        >
            {Icon(type, imageUrl)}
            <div className="flex-1 ml-1 min-w-0">
                <div className="text-left text-nowrap text-ellipsis overflow-hidden">
                    {name || children}
                </div>
            </div>
            {dmchat?.chat && show && <DMChatMenu ref={ref} name={name || ''} dmchat={dmchat} setShow={setShow} />}
        </button>
    );
};

export default DMChatButton;

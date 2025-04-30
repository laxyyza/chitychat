import { ReactNode, useRef, useState } from "react";
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
    children?: ReactNode;
}

const Icon = (type: string) => {
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


const DMChatButton = ({ dmchat, onClick, selected, children }: Props) => {
    const { app } = useApp();
    const [show, setShow] = useState(false);
    const ref = useRef<HTMLButtonElement | null>(null);
    var type: string = "other";
    var name: string | undefined;
    if (dmchat?.chat instanceof DM) {
        type = "user";
        name = app.users.get(dmchat.chat.targetUserID)?.displayname;
    } else if (dmchat?.chat instanceof Group) {
        type = "group";
        name = dmchat.chat.name;
    }

    return (
        <>
            <button
                ref={ref}
                className={
                    'flex mb-1 items-center rounded-[8px] hover:bg-gray-600 active:bg-gray-500 max-w-full w-full p-1 ' +
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
                {Icon(type)}
                <div className="flex-1 ml-1 min-w-0">
                    <div className="text-left text-nowrap text-ellipsis overflow-hidden">
                        {name || children}
                    </div>
                </div>
            </button>
            {dmchat?.chat && <DMChatMenu ref={ref} name={name || ''} dmchat={dmchat} show={show} setShow={setShow} />}
        </>
    );
};

export default DMChatButton;

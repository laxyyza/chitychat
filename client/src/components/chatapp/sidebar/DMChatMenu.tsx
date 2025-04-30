import { useRef, useState } from "react";
import { DMChat } from "../../../models/dm";
import { useApp } from "../AppProvider";
import Group from "../../../models/group";
import ConfirmDeleteGroup from "./ConfirmDeleteGroup";
import AddGroupMembers from "./AddGroupMembers";
import useDismissTrigger from "../../../hooks/useDismissTrigger";
import ConfirmLeaveGroup from "./ConfirmLeaveGroup";

interface Props {
    ref: React.RefObject<HTMLButtonElement | null>;
    dmchat: DMChat;
    name: string;
    show: boolean;
    setShow: (show: boolean) => void;
}

const DMChatMenu = ({ dmchat, name, show, setShow }: Props) => {
    const { app } = useApp();
    var leaveGroup = false;
    var block = false;
    var deleteGroup = false;
    var addFriends = false;
    const [showDeleteGroup, setShowDeleteGroup] = useState(false);
    const [showAddFriends, setShowAddFriends] = useState(false);
    const [showLeaveGroup, setShowLeaveGroup] = useState(false);
    const menuRef = useRef<HTMLDivElement | null>(null);
    const popupRef = useRef<HTMLDivElement | null>(null);

    const escape = () => {
        setShow(false);
        setShowDeleteGroup(false);
        setShowAddFriends(false);
        setShowLeaveGroup(false);
    };

    useDismissTrigger(menuRef, () => {
        if (popupRef.current === null) {
            escape();
        }
    });

    // useEffect(() => {
    //     const handleKeyEvent = (event: globalThis.KeyboardEvent) => {
    //         if (event.key == 'Escape') escape();
    //     };
    //
    //     const handleMouseEvent = (e: globalThis.MouseEvent) => {
    //         if (!menuRef.current?.contains(e.target as Node) && !popupRef.current?.contains(e.target as Node)) {
    //             escape();
    //         }
    //     };
    //
    //     document.addEventListener('keydown', handleKeyEvent);
    //     document.addEventListener('mousedown', handleMouseEvent)
    //
    //     return () => {
    //         document.removeEventListener('keydown', handleKeyEvent);
    //         document.removeEventListener('mousedown', handleMouseEvent);
    //     }
    // }, []);

    if (!show) {
        return null;
    }

    if (dmchat.chat instanceof Group) {
        if (dmchat.chat.owner_id === app.login_user.id) {
            deleteGroup = true;
        } else {
            leaveGroup = true;
        }
        addFriends = true;
    } else {
        block = true;
    }

    return (
        <>
            <div className='absolute' ref={menuRef}>
                <div className='p-1'>
                    <div className='bg-gray-700 text-white border-1 p-1 border-gray-600 w-40 max-w-40 rounded-xl text-center'>
                        <span className='text-xs font-medium'>{name}</span>
                        {addFriends &&
                            <button className='border-gray-600 border-1 p-1 mb-1 w-full rounded-xl hover:bg-gray-600'
                                onClick={() => {
                                    // TODO: Implement
                                    setShowAddFriends(true);
                                }}>
                                Add Friends
                            </button>}

                        {leaveGroup &&
                            <button className='border-[#FF000077] border-1 p-1 mb-1 w-full rounded-xl bg-[#FF000011] hover:bg-[#FF000044]'
                                onClick={() => {
                                    setShowLeaveGroup(true);
                                    // TODO: Implement
                                }}>
                                Leave Group
                            </button>}
                        {block &&
                            <button className='border-[#FF000077] border-1 p-1 mb-1 w-full rounded-xl bg-[#FF000011] hover:bg-[#FF000044]'
                                onClick={() => {
                                    // TODO: Implement
                                }}>
                                Block
                            </button>}
                        {deleteGroup &&
                            <button className='border-[#FF000077] border-1 p-1 w-full rounded-xl bg-[#FF000011] hover:bg-[#FF000044] active:bg-[#FF000088]'
                                onClick={(e) => {
                                    e.stopPropagation();
                                    setShowDeleteGroup(true);
                                }}>
                                Delete Group
                            </button>}
                    </div>
                </div>
            </div>
            {showDeleteGroup && (
                <ConfirmDeleteGroup ref={popupRef} dmchat={dmchat} onClose={escape}/>
            )}
            {showAddFriends && (
                <AddGroupMembers ref={popupRef} dmchat={dmchat} onClose={escape}/>
            )}
            {showLeaveGroup && (
                <ConfirmLeaveGroup ref={popupRef} dmchat={dmchat} onClose={escape}/>
            )}
        </>
    );
};

export default DMChatMenu;

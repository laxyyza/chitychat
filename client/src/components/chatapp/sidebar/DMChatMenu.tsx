import { useRef, useState } from "react";
import { DMChat } from "../../../models/dm";
import { useApp } from "../AppProvider";
import Group from "../../../models/group";
import ConfirmDeleteGroup from "./ConfirmDeleteGroup";
import AddGroupMembers from "./AddGroupMembers";
import useDismissTrigger from "../../../hooks/useDismissTrigger";
import ConfirmLeaveGroup from "./ConfirmLeaveGroup";
import Popup from "../Popup";

type ModalType = "DELETE_GROUP" | "ADD_FRIENDS" | "LEAVE_GROUP" | null;

interface Props {
    ref: React.RefObject<HTMLButtonElement | null>;
    dmchat: DMChat;
    name: string;
    setShow: (show: boolean) => void;
}

interface RenderModalProps {
    ref: React.RefObject<HTMLDivElement | null>;
    dmchat: DMChat;
    onClose: () => void;
    modalType: ModalType;
};

const RenderModal = ({ ref, dmchat, onClose, modalType }: RenderModalProps) => {
    switch (modalType) {
        case "DELETE_GROUP":
            return <ConfirmDeleteGroup ref={ref} dmchat={dmchat} onClose={onClose} />
        case "ADD_FRIENDS":
            return <AddGroupMembers ref={ref} dmchat={dmchat} onClose={onClose} />
        case "LEAVE_GROUP":
            return <ConfirmLeaveGroup ref={ref} dmchat={dmchat} onClose={onClose} />
        default:
            return null;
    }
}

const DMChatMenu = ({ ref, dmchat, name, setShow }: Props) => {
    const { app } = useApp();
    var leaveGroup = false;
    var block = false;
    var deleteGroup = false;
    var addFriends = false;
    const [modalType, setModalType] = useState<ModalType>(null);
    const menuRef = useRef<HTMLDivElement | null>(null);
    const popupRef = useRef<HTMLDivElement | null>(null);

    const escape = () => {
        setShow(false);
        setModalType(null);
    };

    useDismissTrigger(() => {
        if (popupRef.current === null) {
            escape();
        }
    }, [menuRef]);

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
        <Popup targetRef={ref} where="bottom">
            <div className='absolute z-1000' ref={menuRef}>
                <div className='p-1'>
                    <div className='bg-gray-700 text-white border-1 p-1 border-gray-600 w-40 max-w-40 rounded-xl text-center'>
                        <span className='text-xs font-medium'>{name}</span>
                        {addFriends &&
                            <button className='border-gray-600 border-1 p-1 mb-1 w-full rounded-xl hover:bg-gray-600'
                                onClick={() => {
                                    setModalType("ADD_FRIENDS");
                                }}>
                                Add Friends
                            </button>}

                        {leaveGroup &&
                            <button className='border-[#FF000077] border-1 p-1 mb-1 w-full rounded-xl bg-[#FF000011] hover:bg-[#FF000044]'
                                onClick={() => {
                                    setModalType("LEAVE_GROUP");
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
                                    setModalType("DELETE_GROUP");
                                }}>
                                Delete Group
                            </button>}
                    </div>
                </div>
            </div>
            <RenderModal ref={popupRef} dmchat={dmchat} onClose={escape} modalType={modalType} />
        </Popup>
    );
};

export default DMChatMenu;

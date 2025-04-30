import { DMChat } from "../../../models/dm";
import Group from "../../../models/group";
import fetchData from "../../../services/api";
import { Action, useApp } from "../AppProvider";

interface Props {
    dmchat: DMChat;
    onClose: () => void;
    ref: React.RefObject<HTMLDivElement | null>;
}

const ConfirmDeleteGroup = ({ dmchat, ref, onClose }: Props) => {
    const { dispatch } = useApp();

    //if (dmchat.chat instanceof Group === false) return null;
    const group: Group | null = (dmchat.chat instanceof Group) ? dmchat.chat : null;
    const name = group?.name || "GROUP";

    const deleteGroup = () => {
        if (!group) return;

        fetchData(`/api/groups/${group.id}`, 'DELETE')
            .then(() => {
                dispatch({ type: Action.DEL_DM, payload: dmchat.id })
                onClose();
            })
            .catch((e) => {
                console.error("Delete group: ", e);
                onClose();
            })
    };

    return (
        <div className="absolute left-0 top-0 flex justify-center items-center w-screen h-screen z-1001 backdrop-blur-xs" ref={ref} onClick={() => {
            onClose();
        }}>
            <div className='relative bg-gray-900 border-black border-1 h-30 p-5 rounded-xl' onClick={e => e.stopPropagation()}>
                <h1 className='text-xl'>
                    You sure you want to delete <span className='text-purple-500 font-bold'>{name}</span>?
                </h1>
                <span>This cannot be undone.</span>
                <div className='absolute bottom-0 right-0 m-1'>
                    <button className='p-2 mr-1 bg-green-800 hover:bg-green-700 rounded-xl font-bold' onClick={() => {
                        onClose();
                    }}>
                        NO
                    </button>
                    <button className='p-2 bg-[#FF000044] hover:bg-[#FF000077] border-[#FF000077] font-bold border-1 rounded-xl' onClick={(e) => {
                        e.stopPropagation();
                        deleteGroup();
                    }}>
                        DELETE
                    </button>
                </div>
            </div>
        </div>
    );
};

export default ConfirmDeleteGroup;

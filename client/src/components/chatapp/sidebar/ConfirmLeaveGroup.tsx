import { DMChat } from '../../../models/dm';
import Group from '../../../models/group';
import fetchData from '../../../services/api';
import Modal from './../modals/Modal';

interface Props {
    onClose: () => void;
    ref: React.RefObject<HTMLDivElement | null>;
    dmchat: DMChat;
}

const ConfirmLeaveGroup = ({onClose, ref, dmchat}: Props) => {
    const group = dmchat.chat as Group;

    const leaveGroup = () => {
        fetchData(`/api/groups/${group.id}/members/me`, 'DELETE');
        onClose();
    };

    return (
        <Modal ref={ref} onClose={onClose}>
            <div className='relative h-30 bg-gray-900 rounded-xl p-5 border-1 border-black'>
                <h1 className='text-xl'>You sure you want to leave <span className='font-bold text-purple-500'>{group.name}</span>?</h1>
                <div className='absolute right-0 bottom-0 m-5'>
                    <button className='p-2 mr-3 w-20 font-bold rounded-xl bg-green-600 hover:bg-green-500' onClick={onClose}>NO</button>
                    <button className='p-2 w-20 font-bold rounded-xl border-red-500 border-1 bg-[#FF000077] hover:bg-red-600' onClick={leaveGroup}>YES</button>
                </div>
            </div>
        </Modal>
    );
};

export default ConfirmLeaveGroup;

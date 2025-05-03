import { useRef, useState } from "react";
import { Category, Hub } from "../../models/hub";
import Modal from "../Modal";
import fetchData from "../../services/api";

interface Props {
    hub: Hub;
    category: Category;
    onClose: () => void;
}

const CreateChannel = ({hub, category, onClose}: Props) => {
    const ref = useRef<HTMLDivElement | null>(null);
    const [channelName, setChannelName] = useState('');

    const onSubmit = () => {
        if (!channelName) return;

        fetchData(`/api/hubs/${hub.id}/categories/${category.id}/channels`, 'POST', 
            {
                name: channelName,
                position: category.channels.size + 1
            });
        onClose();
    };

    return (
        <Modal onClose={onClose} ref={ref}>
            <div 
                className="bg-gray-900 p-5 border-1 border-black rounded-xl w-100" 
                onClick={(e) => e.stopPropagation()}
            >
                <div className='text-xl'>
                    Create Channel
                </div>
                <div className="text-xs mb-5">
                    in {category.name}
                </div>

                <form onSubmit={onSubmit}>
                    <div className="text-xs font-bold">CHANNEL NAME</div>
                    <input
                        className="flex-1 bg-gray-950 w-full border-1 border-gray-800 rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                        type="text"
                        placeholder="new-channel"
                        value={channelName}
                        required
                        onChange={(e) => {
                            setChannelName(e.target.value);
                        }}
                    />

                    <div className="flex justify-end mt-5">
                        <button type="submit" className={`p-2 rounded-xl font-bold ${channelName ? "bg-indigo-600 text-white" : "bg-indigo-950 text-gray-500"}`}>
                            Create Channel
                        </button>
                    </div>
                </form>
            </div>
        </Modal>
    );
}

export default CreateChannel;

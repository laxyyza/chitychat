import { useRef, useState } from "react";
import { Hub } from "../../../models/hub";
import Modal from "../modals/Modal";
import fetchData from "../../../services/api";

interface Props {
    hub: Hub;
    onClose: () => void;
}

const CreateCategory = ({ hub, onClose }: Props) => {
    const ref = useRef<HTMLDivElement | null>(null);
    const [categoryName, setcategoryName] = useState('');

    const onSubmit = () => {
        if (!categoryName) return;

        fetchData(`/api/hubs/${hub.id}/categories`, 'POST',
            {
                name: categoryName,
                position: hub.categories.size + 1
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
                    Create Category
                </div>
                <div className="text-xs mb-5">
                    for {hub.name}
                </div>

                <form onSubmit={onSubmit}>
                    <div className="text-xs font-bold">category NAME</div>
                    <input
                        className="flex-1 bg-gray-950 w-full border-1 border-gray-800 rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                        type="text"
                        placeholder="new-category"
                        value={categoryName}
                        required
                        onChange={(e) => {
                            setcategoryName(e.target.value);
                        }}
                    />

                    <div className="flex justify-end mt-5">
                        <button type="submit" className={`p-2 rounded-xl font-bold ${categoryName ? "bg-indigo-600 text-white" : "bg-indigo-950 text-gray-500"}`}>
                            Create category
                        </button>
                    </div>
                </form>
            </div>
        </Modal>
    );
}

export default CreateCategory;

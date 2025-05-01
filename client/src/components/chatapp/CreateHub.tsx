// import { FormEvent, useState } from 'react';
// import { Action, useApp } from './AppProvider';
// import { CiCamera } from 'react-icons/ci';
// import { FaPlus } from 'react-icons/fa6';

interface Prop {
    onClose: () => void;
}

const CreateHub = ({ }: Prop) => {
    return null;
    // const { app, dispatch } = useApp();
    // const [hubName, setHubName] = useState(
    //     `${app.login_user.displayname}'s Hub`
    // );
    //
    // const handleSubmit = (event: FormEvent) => {
    //     event.preventDefault();
    //     //dispatch({ type: Action.ADD_HUB, payload: hubName });
    //     onClose();
    // };
    //
    // return (
    //     <div
    //         className="absolute flex justify-center items-center w-screen h-screen z-1000 backdrop-blur-xs"
    //         onClick={onClose}
    //     >
    //         <div
    //             className="relative p-2 bg-gray-800 border-1 border-black text-white rounded-xl w-100 h-70 select-none"
    //             onClick={(event) => {
    //                 event.stopPropagation();
    //             }}
    //         >
    //             <div className="text-center text-xl font-bold">Create Hub</div>
    //             <div className="flex justify-center">
    //                 <div className="relative flex flex-col items-center justify-center w-30 h-30 rounded-full border-2 border-white text-center cursor-pointer">
    //                     <div className="absolute right-0 top-0 bg-blue-400 m-1 rounded-full p-1 font-bold">
    //                         <FaPlus />
    //                     </div>
    //                     <CiCamera size="64" />
    //                     <span className="font-bold">Upload</span>
    //                 </div>
    //             </div>
    //             <form onSubmit={handleSubmit} action="none">
    //                 <div className="font-bold">Hub Name:</div>
    //                 <input
    //                     type="text"
    //                     className="bg-gray-900 w-full text-xl p-1 outline-0 rounded-xl"
    //                     value={hubName}
    //                     onChange={(event) => setHubName(event.target.value)}
    //                     required
    //                 />
    //                 <button
    //                     className="absolute bg-blue-600 hover:bg-blue-500 active:bg-blue-400 rounded-xl border-1 p-2 text-xl border-black right-0 bottom-0"
    //                     type="submit"
    //                 >
    //                     Create
    //                 </button>
    //             </form>
    //         </div>
    //     </div>
    // );
};

export default CreateHub;

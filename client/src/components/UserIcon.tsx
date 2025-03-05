import React, { useState, useRef } from 'react';
import { FaRegUser } from 'react-icons/fa';
import Popup from './Popup';

interface Prop {
    username: string;
}

const UserDetails = ({ username }: Prop) => {
    // return (
    //     <Popup targetRef={ref}>
    //         <div className="bg-gray-900 w-100 h-70 rounded-2xl"></div>
    //     </Popup>
    // );

    return (
        <div className="bg-gray-900 top-0 w-100 shadow-2xl p-3 rounded-2xl z-10 text-white">
            <FaRegUser
                className="bg-red-600 m-1 p-1 rounded-4xl border-black border-1"
                size="64"
            ></FaRegUser>
            <div className="">
                <div className="font-bold text-2xl">Display Name</div>
                <div>@{username}</div>
                <div className="text-[14px] text-right">
                    Created at 12 January 2025, 10:30 am
                </div>
            </div>
            <div className="mt-4 bg-gray-800 p-2 rounded-xl">
                ABOUT MEidjwaodijaw doijawdo iajdoia jdaowi jdawd daowidj
                awoidja oidja odijwa
            </div>
        </div>
    );
};

const UserIcon = ({ placement = 'right', username }: Prop) => {
    const [showDetails, setShowDetails] = useState(false);
    const ref = useRef<HTMLDivElement | null>(null);

    return (
        <div onClick={() => setShowDetails(!showDetails)} ref={ref}>
            <FaRegUser
                className="bg-red-600 m-1 mr-2 p-1 rounded-4xl"
                size="42"
            ></FaRegUser>
            {showDetails && (
                <Popup targetRef={ref} onClose={() => setShowDetails(false)}>
                    <UserDetails username={username}></UserDetails>
                </Popup>
            )}
        </div>
    );
};

export default UserIcon;

import { useState } from 'react';
import { FaRegUser } from 'react-icons/fa';

interface Prop {
    username: string;
    placement?: 'right' | 'left';
}

const UserDetails = ({ placement, username }: Prop) => {
    const place = placement === 'right' ? 'left-17' : '-left-100';
    const className =
        'absolute bg-gray-900 top-0 w-100 shadow-2xl p-3 rounded-2xl z-10 text-white  ' +
        place;

    return (
        <div className={className}>
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

    return (
        <div onClick={() => setShowDetails(!showDetails)}>
            <FaRegUser
                className="bg-red-600 m-1 mr-2 p-1 rounded-4xl"
                size="42"
            ></FaRegUser>
            {showDetails && (
                <UserDetails
                    placement={placement}
                    username={username}
                ></UserDetails>
            )}
        </div>
    );
};

export default UserIcon;

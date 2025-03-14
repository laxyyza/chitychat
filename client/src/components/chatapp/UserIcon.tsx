import { useState, useRef } from 'react';
import Popup from './Popup';
import User from './User';
import Input from './Input';
import { FaUserAlt } from 'react-icons/fa';

interface Prop {
    user: User;
}

interface IconImgProp {
    pfp: string;
    profile: boolean;
}

const IconImgChooser = ({ pfp, profile }: IconImgProp) => {
    const [big, setBig] = useState(false);

    const addClass = () => {
        return big ? 'w-full h-full rounded-xl' : 'rounded-[40px] h-20 w-20';
    };

    if (profile) {
        if (pfp) {
            return (
                <img
                    onClick={() => {
                        setBig(!big);
                    }}
                    src={pfp}
                    className={
                        ' bg-gray-800 transition-all duration-200 cursor-pointer ease-linear mb-1 shadow-lg text-white ' +
                        addClass()
                    }
                />
            );
        } else {
            return (
                <div
                    onClick={() => {
                        setBig(!big);
                    }}
                    className={
                        ' bg-gray-400 p-4 transition-all duration-200 cursor-pointer ease-linear mb-1 shadow-lg text-white overflow-hidden ' +
                        addClass()
                    }
                >
                    <FaUserAlt size="full" />
                </div>
            );
        }
    } else {
        if (pfp) {
            return <img src={pfp} className="w-full h-full rounded-full" />;
        } else {
            return (
                <div className="p-2.5 rounded-full bg-gray-400 flex items-center justify-center">
                    <FaUserAlt size="32" />
                </div>
            );
        }
    }
};

const UserDetails = ({ user }: Prop) => {
    return (
        <div className="bg-gray-900 top-0 w-100 shadow-2xl p-3 rounded-2xl z-10 text-white border-1 border-black">
            <IconImgChooser pfp={user.pfp} profile={true} />
            <div className="">
                <div className="font-bold text-2xl text-left">
                    {user.displayname}
                </div>
                <div className="text-left">{user.username}</div>
            </div>
            <div className="mt-4 bg-gray-800 p-2 rounded-xl min-h-10">
                {user.about_me ? user.about_me : 'ABOUT ME'}
            </div>
            <div className="bg-gray-800 rounded-xl text-white mt-3 max-h-50 overflow-auto pl-2">
                <Input
                    attachments={false}
                    placeholder={'Message @' + user.username}
                ></Input>
            </div>
        </div>
    );
};

const UserIcon = ({ user }: Prop) => {
    const [showDetails, setShowDetails] = useState(false);
    const ref = useRef<HTMLDivElement | null>(null);
    const [canOpen, setCanOpen] = useState(true);

    return (
        <div
            onClick={() => {
                if (canOpen) setShowDetails(true);
            }}
            ref={ref}
            className="max-w-13 max-h-13"
        >
            <IconImgChooser pfp={user.pfp} profile={false} />
            {showDetails && ref.current && (
                <Popup
                    targetRef={ref}
                    onClose={() => {
                        setShowDetails(false);
                        setCanOpen(false);
                        setTimeout(() => setCanOpen(true), 100);
                    }}
                >
                    <UserDetails user={user}></UserDetails>
                </Popup>
            )}
        </div>
    );
};

export default UserIcon;

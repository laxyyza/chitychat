import React, { useState, useRef, useEffect } from 'react';
import Popup from './Popup';
import User from './User';
import Input from './Input';

interface Prop {
    user: User;
}

const UserDetails = ({ user }: Prop) => {
    return (
        <div className="bg-gray-900 top-0 w-100 shadow-2xl p-3 rounded-2xl z-10 text-white border-1 border-black">
            <img src={user.pfp} className="w-full h-full rounded-xl mb-2" />
            <div className="">
                <div className="font-bold text-2xl text-center">
                    {user.displayname}
                </div>
                <div className="text-center">{user.username}</div>
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
            <img src={user.pfp} className="w-full h-full rounded-full" />
            {showDetails && (
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

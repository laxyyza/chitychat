import { useState } from 'react';
import AddFriend from './AddFriend';
import { App, useApp } from './AppProvider';
import User from './User';
import UserIcon from './UserIcon';
import { IoMdPersonAdd } from 'react-icons/io';
import { HiBellAlert } from 'react-icons/hi2';
import { MdOutgoingMail } from 'react-icons/md';
import { MdBlock } from 'react-icons/md';
import { FaCheck } from 'react-icons/fa';
import { MdOutlineCancel } from 'react-icons/md';

enum Selection {
    ALL,
    ONLINE,
    FRIEND_REQUESTS,
    PENDING_REQUESTS
}

interface FriendProp {
    user: User;
}

const Friend = ({ user }: FriendProp) => {
    return (
        <div
            key={'friend-' + user.id}
            className="m-1 p-1 flex hover:bg-gray-600 rounded-xl text-white"
        >
            <UserIcon user={user} />
            <div className="ml-2">
                <div className="font-bold text-xl">{user.displayname}</div>
                <div className="text-xs">{user.username}</div>
            </div>
        </div>
    );
};

const PendingRequest = ({ user }: FriendProp) => {
    return (
        <div
            key={'friend-' + user.id}
            className="m-1 p-1 flex hover:bg-gray-600 rounded-xl text-white group"
        >
            <UserIcon user={user} />
            <div className="ml-2 flex-1">
                <div className="font-bold text-xl">{user.displayname}</div>
                <div className="text-xs">{user.username}</div>
            </div>
            <div className="flex items-center scale-0 group-hover:scale-100">
                <button className="text-red-600 p-1 m-1 hover:bg-gray-700 rounded-xl active:text-red-400">
                    <MdOutlineCancel size="32" />
                </button>
            </div>
        </div>
    );
};

const FriendRequest = ({ user }: FriendProp) => {
    return (
        <div
            key={'friend-' + user.id}
            className="m-1 p-1 flex hover:bg-gray-600 rounded-xl text-white group"
        >
            <UserIcon user={user} />
            <div className="ml-2 flex-1">
                <div className="font-bold text-xl">{user.displayname}</div>
                <div className="text-xs">{user.username}</div>
            </div>
            <div className="flex items-center scale-0 group-hover:scale-100">
                <button className="text-red-600 p-1 m-1 hover:bg-gray-700 rounded-xl active:text-red-400">
                    <MdBlock size="32" />
                </button>
                <button className="text-green-600 p-1 m-1 hover:bg-gray-700 rounded-xl active:text-green-400">
                    <FaCheck size="32" />
                </button>
            </div>
        </div>
    );
};

const getUserIDs = (app: App, selection: Selection): number[] => {
    switch (selection) {
        case Selection.ALL:
            return app.friendIDs;
        case Selection.FRIEND_REQUESTS:
            return app.friendRequests;
        case Selection.PENDING_REQUESTS:
            return app.friendRequests;
        default:
            return [];
    }
};

const ListUsers = (app: App, selection: Selection) => {
    const userIDs = getUserIDs(app, selection);
    const friends = userIDs.map((friend_id) => app.users.get(friend_id));

    return (
        <>
            {friends.map((user) => {
                if (!user) return null;
                if (selection === Selection.ALL) return <Friend user={user} />;
                else if (selection === Selection.FRIEND_REQUESTS)
                    return <FriendRequest user={user} />;
                else if (selection === Selection.PENDING_REQUESTS)
                    return <PendingRequest user={user} />;
                else return null;
            })}
        </>
    );
};

const FriendList = () => {
    const { app } = useApp();
    const [addFriend, setAddFriend] = useState(false);
    const [selected, setSelected] = useState(Selection.ALL);

    return (
        <div className="h-full relative">
            {addFriend && <AddFriend onClose={() => setAddFriend(false)} />}
            <div className="bg-gray-800 p-1 text-white flex">
                <button
                    className="bg-blue-600 hover:bg-blue-500 active:bg-blue-400 p-1 rounded-xl flex items-center mr-5"
                    onClick={() => setAddFriend(true)}
                >
                    <IoMdPersonAdd size="24" />
                    <span className="ml-1">Add Friend</span>
                </button>
                {/* <button className="p-1 pr-2 pl-2 rounded-xl flex relative mr-1 hover:bg-gray-700">
                    Online
                </button> */}
                <button
                    className={
                        'p-1 pr-4 pl-4 rounded-xl flex relative mr-1 ' +
                        (selected === Selection.ALL
                            ? 'bg-gray-600'
                            : 'hover:bg-gray-700')
                    }
                    onClick={() => setSelected(Selection.ALL)}
                >
                    All
                </button>
                <button
                    className={
                        'p-1 pr-2 rounded-xl flex relative mr-1 ' +
                        (app.friendRequests.length
                            ? 'hover:bg-gray-700 text-white'
                            : 'text-gray-600') +
                        (selected === Selection.FRIEND_REQUESTS
                            ? ' bg-gray-700 '
                            : '')
                    }
                    onClick={() => {
                        if (app.friendRequests.length)
                            setSelected(Selection.FRIEND_REQUESTS);
                    }}
                >
                    <HiBellAlert size="24" />
                    <span className="text-xs pl-[2px] pr-[3px] min-w-4 font-bold rounded-full -top-1 left-0 absolute bg-red-500">
                        {app.friendRequests.length
                            ? app.friendRequests.length
                            : null}
                    </span>
                    <span>Friend Requests</span>
                </button>
                <button
                    className={
                        'p-1 rounded-xl flex relative mr-1 ' +
                        (app.pendingRequests.length
                            ? 'hover:bg-gray-700 text-white'
                            : 'text-gray-600') +
                        (selected === Selection.PENDING_REQUESTS
                            ? ' bg-gray-700'
                            : '')
                    }
                    onClick={() => {
                        if (app.pendingRequests.length)
                            setSelected(Selection.PENDING_REQUESTS);
                    }}
                >
                    <MdOutgoingMail size="24" />
                    <span className="text-xs pl-[2px] pr-[3px] min-w-4 font-bold rounded-full -top-1 left-0 absolute bg-blue-500">
                        {app.pendingRequests.length
                            ? app.pendingRequests.length
                            : null}
                    </span>
                    <span>Pending Requests</span>
                </button>
            </div>
            {ListUsers(app, selected)}
        </div>
    );
};

export default FriendList;

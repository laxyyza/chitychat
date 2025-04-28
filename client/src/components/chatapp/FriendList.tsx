import { useEffect, useState } from 'react';
import AddFriend from './AddFriend';
import { Action, App, useApp } from './AppProvider';
import User from './User';
import UserIcon from './UserIcon';
import { IoMdPersonAdd } from 'react-icons/io';
import { HiBellAlert } from 'react-icons/hi2';
import { MdOutgoingMail } from 'react-icons/md';
import { MdBlock } from 'react-icons/md';
import { FaCheck } from 'react-icons/fa';
import { MdOutlineCancel } from 'react-icons/md';
import useWebsocket from '../WebSocket';
import { DM, DMChat } from '../../models/dm';
import fetchData from '../../services/api';
import { MdGroupAdd } from "react-icons/md";
import CreateGroup from './CreateGroup';

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
    const {app, dispatch} = useApp();

    const onClick = () => {
        const dmchat = app.dm.get("id-" + user.id);
        if (dmchat) {
            dispatch({type: Action.SELECT_DM, payload: dmchat.id});
        } else {
            const newDM = new DMChat(new DM(user.id), new Date().toISOString());
            dispatch({type: Action.ADD_DMS, payload: [newDM]});
            dispatch({type: Action.SELECT_DM, payload: newDM.id});
        }
    };

    return (
        <div
            key={'friend-' + user.id}
            className="m-1 p-1 flex hover:bg-gray-600 rounded-xl text-white active:bg-gray-500"
            onClick={onClick}
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
    const deleteRequest = () => {
        fetchData('/api/friends/requests/outgoing', 'DELETE', {user_id: user.id});
    }

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
            <div className="flex items-center scale-0 group-hover:scale-100" onClick={() => deleteRequest()}>
                <button className="text-red-600 p-1 m-1 hover:bg-gray-700 rounded-xl active:text-red-400">
                    <MdOutlineCancel size="32" />
                </button>
            </div>
        </div>
    );
};

const FriendRequest = ({ user }: FriendProp) => {
    const postRequest = (action: string) => {
        fetchData('/api/friends/requests', 'POST', {user_id: user.id, action: action});
    };

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
                <button
                    className="text-red-600 p-1 m-1 hover:bg-gray-700 rounded-xl active:text-red-400"
                    onClick={() => postRequest('block')}
                >
                    <MdBlock size="32" />
                </button>
                <button
                    className="text-green-600 p-1 m-1 hover:bg-gray-700 rounded-xl active:text-green-400"
                    onClick={() => postRequest('accept')}
                >
                    <FaCheck size="32" />
                </button>
            </div>
        </div>
    );
};

const getUserIDs = (app: App, selection: Selection): number[] => {
    switch (selection) {
        case Selection.ALL:
            return Array.from(app.friendIDs);
        case Selection.FRIEND_REQUESTS:
            return Array.from(app.friendRequests);
        case Selection.PENDING_REQUESTS:
            return Array.from(app.pendingRequests);
        default:
            return [];
    }
};

const ListUsers = (users: (User | undefined)[], selection: Selection) => {
    return (
        <>
            {users.map((user) => {
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
    const { app, dispatch } = useApp();
    const [addFriend, setAddFriend] = useState(false);
    const [showCreateGroup, setShowCreateGroup] = useState(false);
    const [selected, setSelected] = useState(Selection.ALL);
    const [users, setUsers] = useState<(User | undefined)[]>([]);

    const { send } = useWebsocket((cmd: string, packet: any) => {
        if (cmd === 'friend_request') {
            const userID = packet.source_user_id;
            const user = app.users.get(userID);
            if (!user) {
                send({ cmd: 'get_user', user_ids: [userID] });
            }

            dispatch({
                type: Action.ADD_FRIEND_REQUESTS,
                payload: [userID]
            });
        } else if (cmd === 'friend_request_update') {
            const userID = packet.user_id;
            const user = app.users.get(userID);
            const state = packet.state;
            if (!user) {
                send({ cmd: 'get_user', user_ids: [userID] });
            }

            if (state === 'ACCEPTED') {
                dispatch({
                    type: Action.ADD_FRIENDS,
                    payload: [userID]
                });
            }

            if (app.friendRequests.has(userID)) {
                dispatch({
                    type: Action.DEL_FRIEND_REQUEST,
                    payload: userID
                });
            } else if (app.pendingRequests.has(userID)) {
                dispatch({
                    type: Action.DEL_PENDING_FRIEND_REQUEST,
                    payload: userID
                });
            }
        }
    });

    useEffect(() => {
        const userIDs = getUserIDs(app, selected);
        const newUsers = userIDs.map((userID) => app.users.get(userID));
        setUsers(newUsers);
        if (selected !== Selection.ALL && userIDs.length === 0) {
            setSelected(Selection.ALL);
        }
    }, [
        app.friendIDs,
        app.friendRequests,
        app.pendingRequests,
        selected,
        app.users
    ]);

    return (
        <div className="h-full relative overflow-auto">
            {addFriend && <AddFriend onClose={() => setAddFriend(false)} />}
            {showCreateGroup && <CreateGroup onClose={() => setShowCreateGroup(false)} />}
            <div className="bg-gray-800 p-1 text-white flex">
                <button
                    className="bg-green-600 hover:bg-green-500 active:bg-green-400 p-1 rounded-3xl flex items-center mr-3"
                    onClick={() => setShowCreateGroup(true)}
                >
                    <MdGroupAdd size="24" />
                    <span className="ml-1">Create Group</span>
                </button>
                <button
                    className="bg-blue-600 hover:bg-blue-500 active:bg-blue-400 p-1 rounded-3xl flex items-center mr-5"
                    onClick={() => setAddFriend(true)}
                >
                    <IoMdPersonAdd size="24" />
                    <span className="ml-1">Add Friend</span>
                </button>
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
                        (app.friendRequests.size
                            ? 'hover:bg-gray-700 text-white'
                            : 'text-gray-600') +
                        (selected === Selection.FRIEND_REQUESTS
                            ? ' bg-gray-700 '
                            : '')
                    }
                    onClick={() => {
                        if (app.friendRequests.size)
                            setSelected(Selection.FRIEND_REQUESTS);
                    }}
                >
                    <HiBellAlert size="24" />
                    <span className="text-xs pl-[2px] pr-[3px] min-w-4 font-bold rounded-full -top-1 left-0 absolute bg-red-500">
                        {app.friendRequests.size
                            ? app.friendRequests.size
                            : null}
                    </span>
                    <span>Friend Requests</span>
                </button>
                <button
                    className={
                        'p-1 rounded-xl flex relative mr-1 ' +
                        (app.pendingRequests.size
                            ? 'hover:bg-gray-700 text-white'
                            : 'text-gray-600') +
                        (selected === Selection.PENDING_REQUESTS
                            ? ' bg-gray-700'
                            : '')
                    }
                    onClick={() => {
                        console.log('PENDING click! ', selected);
                        if (app.pendingRequests.size)
                            setSelected(Selection.PENDING_REQUESTS);
                    }}
                >
                    <MdOutgoingMail size="24" />
                    <span className="text-xs pl-[2px] pr-[3px] min-w-4 font-bold rounded-full -top-1 left-0 absolute bg-blue-500">
                        {app.pendingRequests.size
                            ? app.pendingRequests.size
                            : null}
                    </span>
                    <span>Pending Requests</span>
                </button>
            </div>
            {ListUsers(users, selected)}
        </div>
    );
};

export default FriendList;

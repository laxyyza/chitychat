import { useEffect, useState } from 'react';
import { useApp } from './AppProvider';
import User from './User';
import UserIcon from './UserIcon';

const Member = (user?: User) => {
    if (!user) return null;

    return (
        <li key={user.id}>
            <div className="relative hover:bg-gray-700 p-1 m-1 mb-2 mt-2 rounded-2xl text-white flex active:bg-gray-600">
                <UserIcon user={user}></UserIcon>
                <div className="flex-1 ml-2">
                    <div className="font-bold text-xl">{user.displayname}</div>
                    <div>{user.username}</div>
                </div>
            </div>
        </li>
    );
};

const MemberList = () => {
    const { app } = useApp();
    const [memberIDs, setMemberIDs] = useState<number[]>([]);

    useEffect(() => {
        const hub = app.hubs[app.hubIndex];

        setMemberIDs(hub ? hub.memberIDs : []);
    }, [app.hubIndex]);

    return (
        <div className="relative max-h-screen w-60 bg-gray-800 overflow-auto">
            <div className="text-center text-white font-bold shadow-xl bg-gray-900">
                {memberIDs.length} members
            </div>
            {memberIDs.map((member_id) => Member(app.users.get(member_id)))}
        </div>
    );
};

export default MemberList;

import { useEffect, useState } from 'react';
import { useApp } from './AppProvider';
import User from './User';
import UserIcon from './UserIcon';
import useWebsocket from '../WebSocket';

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

    const { send } = useWebsocket((cmd, packet) => {
        if (cmd === 'get_member_ids') {
            const group_id: number = packet['group_id'];
            const memberIDs: number[] = packet['member_ids'];
            const group = app.groups.get(group_id);
            group?.addMemberIDs(memberIDs);

            const donthaveIDs = memberIDs.filter((id) => !app.users.get(id));

            if (donthaveIDs.length) {
                send({ cmd: 'get_user', user_ids: donthaveIDs });
            }
        }
    });

    useEffect(() => {
        const hub = app.hubs.get(app.currentHubID);
        if (hub) {
            setMemberIDs(hub.memberIDs);
        } else {
            const group = app.groups.get(app.currentGroupID);
            if (group) {
                setMemberIDs(group.memberIDs);
                if (group.gotMemberIDs === false) {
                    send({ cmd: 'get_member_ids', group_id: group.id });
                }
            } else setMemberIDs([]);
        }
    }, [app.currentHubID, app.currentGroupID, app.users]);

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

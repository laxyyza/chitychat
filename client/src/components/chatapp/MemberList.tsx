import { App, useApp } from './AppProvider';
import User from './User';
import UserIcon from './UserIcon';
import useWebsocket from '../WebSocket';
import { DMChat } from '../../models/dm';

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

const getMemberIDs = (app: App, send: (data: any) => void): number[] => {
    const group = app.dm.get(app.currentDMID)?.getGroup();
    if (group) {
        if (group.detailsLoaded === false) {
            send({ cmd: 'get_member_ids', group_id: group.id });
            send({
                cmd: 'get_group_msgs',
                group_id: group.id,
                limit: 15,
                offset: group.msgOffset
            });
            group.detailsLoaded = true;
        }
        return group.memberIDs;
    }
    return [];
};

const MemberList = () => {
    const { app } = useApp();

    const { send } = useWebsocket((cmd, packet) => {
        if (cmd === 'get_member_ids') {
            const group_id: number = packet['group_id'];
            const memberIDs: number[] = packet['member_ids'];
            const group = app.dm.get(DMChat.GroupID(group_id))?.getGroup();
            group?.addMemberIDs(memberIDs);

            const donthaveIDs = memberIDs.filter((id) => !app.users.get(id));

            if (donthaveIDs.length) {
                send({ cmd: 'get_user', user_ids: donthaveIDs });
            }
        }
    });

    const memberIDs = getMemberIDs(app, send);

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

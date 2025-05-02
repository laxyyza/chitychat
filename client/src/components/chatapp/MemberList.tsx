import { App, useApp } from './AppProvider';
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

const getMemberIDs = (app: App, send: (data: any) => void): number[] => {
    let memberIDs: number[] = [];

    if (app.focus.type === "dm") {
        const group = app.dm.get(app.focus.dmid)?.getGroup();
        memberIDs = Array.from(group?.memberIDs || []);
    } else if (app.focus.type === "hub") {
        const hub = app.hubs.get(app.focus.hubID);
        memberIDs = Array.from(hub?.memberIDs || []);
    } else {
        return [];
    }
    const donthaveIDs: number[] = []

    memberIDs.forEach((id) => {
        if (!app.users.has(id)) {
            donthaveIDs.push(id);
        }
    })

    if (donthaveIDs.length) {
        send({
            cmd: "get_user",
            user_ids: donthaveIDs
        })
    }

    return memberIDs;
};

const MemberList = () => {
    const { app } = useApp();

    const { send } = useWebsocket();

    if (app.focus.type === "friends") return null;

    const memberIDs = getMemberIDs(app, send);

    return (
        <div className="relative max-h-screen w-60 bg-gray-900 overflow-auto border-l-gray-700 border-l-1">
            <div className="text-center text-white font-bold shadow-xl bg-gray-900">
                {memberIDs.length + ' members'}
            </div>
            {memberIDs.map((member_id) => Member(app.users.get(member_id)))}
        </div>
    );
};

export default MemberList;

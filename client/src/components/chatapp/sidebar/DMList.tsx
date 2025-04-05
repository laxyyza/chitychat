import { ReactNode, useEffect } from 'react';
import { Action, useApp } from '../AppProvider';
import { BiSolidGroup } from 'react-icons/bi';
import { RiUserHeartFill } from 'react-icons/ri';
import { FaUser } from 'react-icons/fa';
import Group from '../../../models/group';
import { DM, DMChat } from '../../../models/dm';
import useWebsocket from '../../WebSocket';
import fetchData from '../../../services/api';

interface DMProp {
    name?: string;
    onClick?: () => void;
    selected: boolean;
    type: 'group' | 'user' | 'other';
    children?: ReactNode;
}

const Icon = (type: string) => {
    if (type === 'group') {
        return (
            <div className="bg-purple-500 rounded-full w-8 h-8 overflow-hidden flex items-center justify-center">
                <BiSolidGroup />
            </div>
        );
    } else if (type === 'user') {
        return (
            <div className="bg-blue-500 rounded-full w-8 h-8 overflow-hidden flex items-center justify-center">
                <FaUser />
            </div>
        );
    } else {
        return null;
    }
};

const DMComponent = ({ name, onClick, selected, type, children }: DMProp) => {
    return (
        <button
            className={
                'flex mb-1 items-center rounded-[8px] hover:bg-gray-600 active:bg-gray-500 max-w-full w-full p-1 ' +
                (selected ? 'bg-gray-600' : '')
            }
            onClick={onClick}
        >
            {Icon(type)}
            <div className="flex-1 ml-1 min-w-0">
                <div className="text-left text-nowrap text-ellipsis overflow-hidden">
                    {name || children}
                </div>
            </div>
        </button>
    );
};

const DMList = () => {
    const { app, dispatch } = useApp();
    const dms = Array.from(app.dm.entries());
    // const groups = Array.from(app.groups.entries());
    // const friends = app.friendIDs.map((friend_id) => app.users.get(friend_id));

    const { send } = useWebsocket();

    const fetchDMs = () => {
        fetchData('/api/dms')
            .then(json => {
                const userIDs: number[] = json.user_ids;
                const dontHaveIDs: number[] = []
                const dmChats: DMChat[] = []
                userIDs.forEach(userID => {
                    const dmchat = new DMChat(new DM(userID));
                    if (!app.users.get(userID)) {
                        dontHaveIDs.push(userID);
                    }
                    dmChats.push(dmchat);
                })

                dispatch({ type: Action.ADD_DMS, payload: dmChats })
                if (dontHaveIDs.length) {
                    send({ cmd: "get_user", user_ids: dontHaveIDs });
                }
            });
    }

    useEffect(() => {
        fetchDMs();
    }, [])

    return (
        <>
            <DMComponent
                selected={app.currentDMID === 'friends'}
                type="other"
                onClick={() =>
                    dispatch({ type: Action.SELECT_DM, payload: 'friends' })
                }
            >
                <div className="flex items-center text-center justify-center">
                    <div>
                        <RiUserHeartFill size="22" />
                    </div>
                    <div className="ml-1 font-bold p-1">Friends</div>
                </div>
            </DMComponent>
            <div className="text-center text-xs font-bold">Direct Messages</div>
            <ul className="p-1">
                {dms.map(([id, dmchat]) => {
                    if (dmchat.chat instanceof Group) {
                        const group = dmchat.chat;
                        return (
                            <li key={dmchat.id}>
                                <DMComponent
                                    name={group.name}
                                    selected={app.currentDMID === id}
                                    type="group"
                                    onClick={() =>
                                        dispatch({
                                            type: Action.SELECT_DM,
                                            payload: id
                                        })
                                    }
                                />
                            </li>
                        );
                    } else {
                        const dm = dmchat.chat;
                        const user = app.users.get(dm.targetUserID);
                        if (!user) return null;
                        return (
                            <li key={dmchat.id}>
                                <DMComponent
                                    name={user.displayname}
                                    selected={app.currentDMID === id}
                                    type="user"
                                    onClick={() =>
                                        dispatch({
                                            type: Action.SELECT_DM,
                                            payload: id
                                        })
                                    }
                                />
                            </li>
                        );
                    }
                })}
                {/* {friends.map((friend) => {
                    if (!friend) return null;
                    else {
                        return (
                            <li key={'friend-' + friend.id}>
                                <DMComponent
                                    name={friend.displayname}
                                    selected={false}
                                    type="user"
                                    onClick={() => {}}
                                />
                            </li>
                        );
                    }
                })} */}
                {/* {groups.map(([id, group]) => (
                    <li key={id}>
                        <DMComponent
                            name={group.name}
                            selected={app.currentGroupID === id}
                            type="group"
                            onClick={() =>
                                dispatch({
                                    type: Action.SELECT_GROUP,
                                    payload: id
                                })
                            }
                        />
                    </li>
                ))} */}
            </ul>
        </>
    );
};

export default DMList;

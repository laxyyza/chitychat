import { useEffect, useRef } from 'react';
import { Action, useApp } from '../AppProvider';
import { RiUserHeartFill } from 'react-icons/ri';
import Group, { GroupProps } from '../../../models/group';
import { DM, DMChat } from '../../../models/dm';
import useWebsocket from '../../WebSocket';
import fetchData from '../../../services/api';
import DMChatButton from './DMChatButton';
import { useNavigate, useParams } from 'react-router-dom';

const DMList = () => {
    const { app, dispatch } = useApp();
    const dmlistRef = useRef<HTMLDivElement | null>(null);
    const dms = Array.from(app.dm.values()).sort((a, b) => (
        b.lastMessage.localeCompare(a.lastMessage)
    ));
    // const groups = Array.from(app.groups.entries());
    // const friends = app.friendIDs.map((friend_id) => app.users.get(friend_id));
    const navigator = useNavigate();

    const { send } = useWebsocket();
    const { dm_id, group_id } = useParams();
    const paramDMID = dm_id ? parseInt(dm_id) : undefined;
    const paramGroupID = group_id ? parseInt(group_id) : undefined;

    const fetchDMs = () => {
        fetchData('/api/dms')
            .then(json => {
                const dms: any[] = json.dms;
                const dontHaveIDs: number[] = []
                const dmChats: DMChat[] = []
                dms.forEach(dm => {
                    const userID: number = dm.user_id;
                    const dmchat = new DMChat(new DM(userID), dm.last_message);
                    if (!app.users.get(userID)) {
                        dontHaveIDs.push(userID);
                    }
                    dmChats.push(dmchat);

                    if (paramDMID === userID) {
                        dispatch({ type: Action.SET_FOCUS, payload: { type: "dm", dmid: DMChat.DmID(paramDMID) } })
                    }
                })

                dispatch({ type: Action.ADD_DMS, payload: dmChats })
                if (dontHaveIDs.length) {
                    send({ cmd: "get_user", user_ids: dontHaveIDs });
                }
            });

        fetchData('/api/groups')
            .then((json) => {
                const groups: GroupProps[] = json.groups;

                groups.forEach((group) => {
                    if (group.group_id === paramGroupID) {
                        dispatch({ type: Action.SET_FOCUS, payload: { type: "dm", dmid: DMChat.GroupID(paramGroupID) } })
                    }
                });

                dispatch({
                    type: Action.ADD_GROUPS,
                    payload: groups
                });
            });
    }

    useEffect(() => {
        fetchDMs();
    }, [])

    return (
        <>
            <div className='h-full p-1'>
                <DMChatButton
                    selected={app.focus.type === 'friends'}
                    onClick={() => {
                        dispatch({ type: Action.SET_FOCUS, payload: { type: "friends" } });
                    }}
                >
                    <div className="flex items-center text-center justify-center">
                        <div>
                            <RiUserHeartFill size="22" />
                        </div>
                        <div className="ml-1 font-bold p-1">Friends</div>
                    </div>
                </DMChatButton>
                <div className="text-center text-xs font-bold">Direct Messages</div>
                <div className="p-1 overflow-hidden hover:overflow-auto flex-1 w-full max-h-full scroll-container" ref={dmlistRef}>
                    {dms.map((dmchat) => (
                        <li key={dmchat.id}>
                            <DMChatButton
                                dmchat={dmchat}
                                selected={app.focus.type === "dm" && app.focus.dmid === dmchat.id}
                                dmlistRef={dmlistRef}
                                onClick={() => {
                                    dispatch({
                                        type: Action.SET_FOCUS,
                                        payload: { type: "dm", dmid: dmchat.id }
                                    })
                                    if (dmchat.chat instanceof Group) {
                                        navigator(`/app/groups/${dmchat.chat.id}`);
                                    } else if (dmchat.chat instanceof DM) {
                                        navigator(`/app/dms/${dmchat.chat.targetUserID}`);
                                    }
                                }}
                            />
                        </li>
                    ))}
                </div>
            </div>
        </>
    );
};

export default DMList;

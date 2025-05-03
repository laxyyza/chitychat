import { useEffect, useRef } from 'react';
import { Action, useApp } from '../AppProvider';
import { RiUserHeartFill } from 'react-icons/ri';
import { GroupProps } from '../../../models/group';
import { DM, DMChat } from '../../../models/dm';
import useWebsocket from '../../WebSocket';
import fetchData from '../../../services/api';
import DMChatButton from './DMChatButton';

const DMList = () => {
    const { app, dispatch } = useApp();
    const dmlistRef = useRef<HTMLDivElement | null>(null);
    const dms = Array.from(app.dm.values()).sort((a, b) => (
        b.lastMessage.localeCompare(a.lastMessage)
    ));
    // const groups = Array.from(app.groups.entries());
    // const friends = app.friendIDs.map((friend_id) => app.users.get(friend_id));

    const { send } = useWebsocket();

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
                })

                dispatch({ type: Action.ADD_DMS, payload: dmChats })
                if (dontHaveIDs.length) {
                    send({ cmd: "get_user", user_ids: dontHaveIDs });
                }
            });

        fetchData('/api/groups')
            .then((json) => {
                const groups: GroupProps[] = json.groups;

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
        <div className='flex-col h-full'>
            <DMChatButton
                selected={app.focus.type === 'friends'}
                onClick={() =>
                    dispatch({ type: Action.SET_FOCUS, payload: { type: "friends" } })
                }
            >
                <div className="flex items-center text-center justify-center">
                    <div>
                        <RiUserHeartFill size="22" />
                    </div>
                    <div className="ml-1 font-bold p-1">Friends</div>
                </div>
            </DMChatButton>
            <div className="text-center text-xs font-bold">Direct Messages</div>
            <div className="p-1 overflow-hidden hover:overflow-auto flex-1 max-h-full" ref={dmlistRef}>
                {dms.map((dmchat) => (
                    <li key={dmchat.id}>
                        <DMChatButton
                            dmchat={dmchat}
                            selected={app.focus.type === "dm" && app.focus.dmid === dmchat.id}
                            dmlistRef={dmlistRef}
                            onClick={() =>
                                dispatch({
                                    type: Action.SET_FOCUS,
                                    payload: { type: "dm", dmid: dmchat.id }
                                })
                            }
                        />
                    </li>
                ))}
            </div>
        </div>
    );
};

export default DMList;

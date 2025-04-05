import React from 'react';
import { Action, App, DispatchAction } from '../components/chatapp/AppProvider';
import Group from '../models/group';
import { DM, DMChat } from '../models/dm';

const getDMChat = (packet: any, app: App, dispatch: React.Dispatch<DispatchAction>): DMChat | undefined => {
    const isUs = packet.user_id === app.login_user.id;

    const entries = Array.from(app.dm.entries());
    for (var i = 0; i < entries.length; i++) {
        const [_, dm] = entries[i];
        if (dm.chat instanceof DM == false)
            continue;

        if (isUs) {
            if (dm.chat.targetUserID === packet.target_user_id)
                return dm;
        } else {
            if (dm.chat.targetUserID === packet.user_id)
                return dm;
        }
    }

    if (!isUs) {
        const newDM = new DMChat(new DM(packet.user_id));
        dispatch({ type: Action.ADD_DMS, payload: [newDM] });
        return newDM;
    }

    return undefined;
};

const cmdMsgUser = (packet: any, app: App, dispatch: React.Dispatch<DispatchAction>) => {
    const dmchat = getDMChat(packet, app, dispatch);

    if (dmchat) {
        dispatch({
            type: Action.ADD_DM_MSGS,
            payload: {
                dmID: dmchat.id,
                messages: [
                    {
                        id: packet.msg_id,
                        channel_id: packet.channel_id,
                        channel_type: 'dm',
                        user_id: packet.user_id,
                        content: packet.content,
                        attachments: packet.attachments,
                        timestamp: packet.timestamp
                    }
                ]
            }
        })
    }
};

const handleWebsocketMessage = (cmd: string, packet: any, app: App, dispatch: React.Dispatch<DispatchAction>) => {
    switch (cmd) {
        case 'client_user_info': {
            dispatch({
                type: Action.SET_LOGIN_USER,
                payload: {
                    id: packet.user_id,
                    username: packet.username,
                    displayname: packet.displayname,
                    created_at: packet.create_at,
                    about_me: packet.bio,
                    pfp: ''
                }
            });
            break;
        }
        case 'client_groups': {
            const groups: any[] = packet['groups'];
            groups.forEach((group) => {
                dispatch({
                    type: Action.ADD_GROUP,
                    payload: new Group(
                        group['group_id'],
                        group['owner_id'],
                        group['name'],
                        '',
                        group['public']
                    )
                });
            });
            break;
        }
        case 'get_user': {
            const users: any[] = packet['users'];
            users.forEach((user) => {
                dispatch({
                    type: Action.ADD_USER,
                    payload: {
                        id: user['user_id'],
                        username: user['username'],
                        displayname: user['displayname'],
                        about_me: user['bio'],
                        pfp: '',
                        created_at: user['created_at']
                    }
                });
            });
            break;
        }
        case 'group_msg': {
            dispatch({
                type: Action.ADD_MSG,
                payload: {
                    id: packet.msg_id,
                    channel_id: packet.group_id,
                    channel_type: 'group',
                    user_id: packet.user_id,
                    content: packet.content,
                    attachments: packet.attachments,
                    timestamp: packet.timestamp
                }
            });
            break;
        }
        case 'msg_user':
            cmdMsgUser(packet, app, dispatch);
            break;
    }
};

export default handleWebsocketMessage;

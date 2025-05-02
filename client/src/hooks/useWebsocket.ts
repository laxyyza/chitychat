import React from 'react';
import { Action, App, DispatchAction } from '../components/chatapp/AppProvider';
import { DM, DMChat } from '../models/dm';
import Message from '../models/message';
import { fetchGroup } from '../services/groupApi';
import { HubMessageData } from '../models/hub';

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
        const newDM = new DMChat(new DM(packet.user_id), packet.timestamp);
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

const cmdMsgHub = (packet: any, dispatch: React.Dispatch<DispatchAction>) => {
    const msg: HubMessageData = packet;
    dispatch({
        type: Action.ADD_HUB_MSG,
        payload: msg
    })
}

const cmdMsgGroup = (packet: any, dispatch: React.Dispatch<DispatchAction>) => {
    const msg: Message = {
        id: packet.msg_id,
        user_id: packet.user_id,
        channel_id: packet.channel_id,
        channel_type: 'group',
        content: packet.content,
        attachments: packet.attachments,
        timestamp: packet.timestamp
    };
    const dmid = DMChat.GroupID(packet.group_id);
    dispatch({
        type: Action.ADD_DM_MSGS,
        payload: {
            dmID: dmid,
            messages: [msg]
        }
    })
}

const cmdNewGroup = (packet: any, dispatch: React.Dispatch<DispatchAction>) => {
    const groupID: number = packet.group_id;

    fetchGroup(groupID, dispatch);
}

const cmdNewGroupMembers = (packet: any, app: App, dispatch: React.Dispatch<DispatchAction>) => {
    const groupID: number = packet.group_id;
    const userIDs: number[] = packet.user_ids;

    if (app.dm.has(DMChat.GroupID(groupID)) == false) {
        fetchGroup(groupID, dispatch);
        return;
    }

    dispatch({
        type: Action.ADD_GROUP_MEMBERS,
        payload: {
            groupID: groupID,
            IDs: userIDs
        }
    });
}

const cmdDelGroupMember = (packet: any, app: App, dispatch: React.Dispatch<DispatchAction>) => {
    const groupID: number = packet.group_id;
    const userID: number = packet.user_id;

    if (userID === app.login_user.id) {
        dispatch({
            type: Action.DEL_DM,
            payload: DMChat.GroupID(groupID)
        })
    } else {
        dispatch({
            type: Action.DEL_GROUP_MEMBER,
            payload: {
                groupID: groupID,
                userID: userID
            }
        })
    }
}

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
        case 'msg_group': {
            cmdMsgGroup(packet, dispatch);
            break;
        }
        case 'msg_user':
            cmdMsgUser(packet, app, dispatch);
            break;
        case 'new_group':
            cmdNewGroup(packet, dispatch);
            break;
        case 'new_group_members':
            cmdNewGroupMembers(packet, app, dispatch);
            break;
        case 'del_group_member':
            cmdDelGroupMember(packet, app, dispatch);
            break;
        case 'msg_hub': 
            cmdMsgHub(packet, dispatch);
            break;
    }
};

export default handleWebsocketMessage;

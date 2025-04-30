import { createContext, useContext, useReducer, ReactNode } from 'react';
import User from './User';
import { Hub } from '../../models/hub';
import { TextChannel, ChannelType } from '../../models/channel';
import Message from '../../models/message';
import Group, { GroupProps } from '../../models/group';
import { DM, DMChat } from '../../models/dm';

export interface App {
    logged_in: boolean;
    login_user: User;
    users: Map<number, User>;
    hubs: Map<number, Hub>;
    textChannels: Map<number, TextChannel>;
    dm: Map<string, DMChat>;
    currentHubID: number;
    currentDMID: string;
    currentChannelID: number;
    friendIDs: Set<number>;
    friendRequests: Set<number>;
    pendingRequests: Set<number>;
}

interface Prop {
    children: ReactNode;
}

interface AppContextProps {
    app: App;
    dispatch: React.Dispatch<DispatchAction>;
}

enum Action {
    SELECT_HUB,
    SELECT_DM,
    SET_LOGIN_USER,
    SELECT_CHANNEL,
    ADD_MSG,
    ADD_HUB,
    ADD_USER,
    ADD_GROUPS,
    RECONNECT,
    SET_CONNECT,
    LOAD_GROUP_MSGS,
    ADD_FRIENDS,
    ADD_FRIEND_REQUESTS,
    ADD_PENDING_FRIEND_REQUESTS,
    DEL_FRIEND_REQUEST,
    DEL_PENDING_FRIEND_REQUEST,
    ADD_DMS,
    DEL_DM,
    ADD_DM_MSGS,
}

export type DispatchAction =
    | { type: Action.SELECT_HUB; payload: number }
    | { type: Action.SELECT_DM; payload: string }
    | { type: Action.SELECT_CHANNEL; payload: number }
    | { type: Action.SET_LOGIN_USER; payload: User }
    | { type: Action.ADD_USER; payload: User }
    | { type: Action.ADD_MSG; payload: Message }
    | { type: Action.ADD_DM_MSGS; payload: {dmID: string, messages: Message[]} }
    | { type: Action.ADD_HUB; payload: string }
    | { type: Action.ADD_GROUPS; payload: GroupProps[] }
    | { type: Action.ADD_DMS; payload: DMChat[] }
    | { type: Action.DEL_DM; payload: string }
    | {
          type: Action.DEL_FRIEND_REQUEST | Action.DEL_PENDING_FRIEND_REQUEST;
          payload: number;
      }
    | {
          type:
              | Action.ADD_FRIENDS
              | Action.ADD_FRIEND_REQUESTS
              | Action.ADD_PENDING_FRIEND_REQUESTS;
          payload: number[];
      }
    | {
          type: Action.LOAD_GROUP_MSGS;
          payload: { group_id: number; msgs: Message[] };
      };

const AppCtx = createContext<AppContextProps | undefined>(undefined);

const appReducer = (state: App, action: DispatchAction): App => {
    switch (action.type) {
        case Action.ADD_DM_MSGS: {
            const dmchat = state.dm.get(action.payload.dmID);
            if (!dmchat) return state;
            if (action.payload.messages.length === 1) {
                dmchat.lastMessage = action.payload.messages[0].timestamp;
            }

            if (dmchat.chat instanceof DM) {
                return {
                    ...state,
                    dm: new Map(state.dm).set(dmchat.id, DMChat.From(dmchat, DM.fromAddMessages(dmchat.chat, action.payload.messages)))
                };
            } else if (dmchat.chat instanceof Group) {
                return {
                    ...state,
                    dm: new Map(state.dm).set(dmchat.id, DMChat.From(dmchat, Group.loadMessages(dmchat.chat, action.payload.messages)))
                };
            }
            return {...state};
        }
        case Action.SELECT_HUB: {
            const hub = state.hubs.get(action.payload);
            if (hub) {
                return {
                    ...state,
                    currentHubID: action.payload
                };
            }
            return {
                ...state,
                currentHubID: action.payload
            };
        }
        case Action.SELECT_DM: {
            return {
                ...state,
                currentDMID: action.payload
            };
        }
        case Action.SELECT_CHANNEL:
            return { ...state, currentChannelID: action.payload };
        case Action.SET_LOGIN_USER: {
            const user = action.payload;
            return {
                ...state,
                login_user: user,
                users: new Map(state.users).set(user.id, user)
            };
        }
        case Action.ADD_USER: {
            const user = action.payload;
            return {
                ...state,
                users: new Map(state.users).set(user.id, user)
            };
        }
        // case Action.ADD_MSG: {
        //     const msg = action.payload;
        //     if (msg.channel_type === 'hub') {
        //         const newTextChannels = new Map(
        //             [...state.textChannels].map(([id, channel]) => {
        //                 if (channel.id === msg.channel_id) {
        //                     return [
        //                         id,
        //                         {
        //                             ...channel,
        //                             messages: [...channel.messages, msg]
        //                         }
        //                     ];
        //                 }
        //                 return [id, channel];
        //             })
        //         );
        //         return { ...state, textChannels: newTextChannels };
        //     } else if (msg.channel_type === 'group' || msg.channel_type === "dm") {
        //         const newDMs: Map<string, DMChat> = new Map(
        //             [...state.dm].map(([id, chat]) => {
        //                 if (chat.chat instanceof Group) {
        //                     return [
        //                         id,
        //                         new DMChat(Group.fromAddMessage(chat.chat, msg))
        //                     ];
        //                 } 
        //             })
        //         );
        //         // [...state.dm].map(([id, group]) => {
        //         //     if (msg.channel_id === id) {
        //         //         return [id, Group.fromAddMessage(group, msg)];
        //         //     }
        //         //     return [id, group];
        //         // })
        //         return { ...state, dm: newDMs };
        //     }
        //
        //     return { ...state };
        // }
        case Action.ADD_HUB: {
            const hubID = state.hubs.size;
            const channelID = state.textChannels.size;
            return {
                ...state,
                textChannels: new Map(state.textChannels).set(channelID, {
                    id: channelID,
                    hub_id: hubID,
                    name: 'General',
                    messages: [],
                    type: ChannelType.TEXT
                }),
                hubs: new Map(state.hubs).set(
                    hubID,
                    new Hub(hubID, state.login_user.id, action.payload, null, [
                        channelID
                    ])
                )
            };
        }
        case Action.ADD_GROUPS: {
            const newDMs = new Map(state.dm);
            const groups = action.payload;

            groups.forEach((group) => {
                const dm = new DMChat(
                    new Group(
                        group.group_id,
                        group.owner_id,
                        group.name,
                        group.desc,
                        group.created_at,
                        group.member_ids
                    ),
                    group.last_message
                );
                newDMs.set(dm.id, dm);
            });

            return {
                ...state,
                dm: newDMs
            };
        }
        // case Action.LOAD_GROUP_MSGS: {
        //     const dmchat = state.dm.get(
        //         DMChat.GroupID(action.payload.group_id)
        //     );
        //     if (dmchat && dmchat.chat instanceof Group) {
        //         return {
        //             ...state,
        //             dm: new Map(state.dm).set(
        //                 dmchat.id,
        //                 new DMChat(
        //                     Group.loadMessages(dmchat.chat, action.payload.msgs),
        //
        //                 )
        //             )
        //         };
        //     }
        //     return state;
        // }
        case Action.ADD_FRIENDS: {
            const newSet = new Set(state.friendIDs);
            action.payload.forEach((id) => {
                newSet.add(id);
            })
            return {
                ...state,
                friendIDs: newSet
            };
        }
        case Action.ADD_FRIEND_REQUESTS: {
            const newSet = new Set(state.friendRequests);
            action.payload.forEach((id) => {
                newSet.add(id);
            })
            return {
                ...state,
                friendRequests: newSet
            };
        }
        case Action.DEL_FRIEND_REQUEST: {
            const newSet = new Set(state.friendRequests);
            newSet.delete(action.payload);
            return {
                ...state,
                friendRequests: newSet
            };
        }
        case Action.ADD_PENDING_FRIEND_REQUESTS: {
            const newSet = new Set(state.pendingRequests);
            action.payload.forEach((id) => {
                newSet.add(id);
            })
            return {
                ...state,
                pendingRequests: newSet
            };
        }
        case Action.DEL_PENDING_FRIEND_REQUEST: {
            const newSet = new Set(state.pendingRequests);
            newSet.delete(action.payload);
            return {
                ...state,
                pendingRequests: newSet
            };
        }
        case Action.ADD_DMS: {
            const newDMs = new Map(state.dm);
            action.payload.forEach(dmchat => {
                newDMs.set(dmchat.id, dmchat);
            });
            return {
                ...state,
                dm: newDMs
            };
        }
        case Action.DEL_DM: {
            const newDMs = new Map(state.dm);
            newDMs.delete(action.payload);
            return {
                ...state,
                dm: newDMs
            };
        }
        default:
            return state;
    }
};

const useApp = (): AppContextProps => {
    const ctx = useContext(AppCtx);

    if (!ctx) {
        throw new Error('AppCtx must be used within the AppProvider');
    }

    return ctx;
};

const AppProvider = ({ children }: Prop) => {
    const [state, dispatch] = useReducer(appReducer, {
        logged_in: false,
        login_user: {
            id: 0,
            username: 'longusername123434567890cool69420',
            displayname: 'Long Display Name That Will be Cutted off',
            pfp: '',
            about_me: '?',
            created_at: '?'
        },
        users: new Map(),
        hubs: new Map(),
        textChannels: new Map(),
        currentHubID: -1,
        currentDMID: 'friends',
        currentChannelID: -1,
        dm: new Map(),
        friendIDs: new Set<number>(),
        friendRequests: new Set<number>(),
        pendingRequests: new Set<number>()
    });

    return (
        <AppCtx.Provider value={{ app: state, dispatch: dispatch }}>
            {children}
        </AppCtx.Provider>
    );
};

export { useApp, AppProvider, Action };

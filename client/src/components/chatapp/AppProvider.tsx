import { createContext, useContext, useReducer, ReactNode } from 'react';
import User from './User';
import { GetChannelMessagesData, Hub, HubBasicData, HubChannelData, HubDetailedData, HubMessageData, NewCategoryData } from '../../models/hub';
import { TextChannel } from '../../models/channel';
import Message from '../../models/message';
import Group, { GroupProps } from '../../models/group';
import { DM, DMChat } from '../../models/dm';

type FocusState =
    | { type: "hub"; hubID: number; channelID: number }
    | { type: "dm"; dmid: string }
    | { type: "friends" };

export interface App {
    logged_in: boolean;
    login_user: User;
    users: Map<number, User>;
    hubs: Map<number, Hub>;
    textChannels: Map<number, TextChannel>;
    dm: Map<string, DMChat>;
    focus: FocusState;
    friendIDs: Set<number>;
    friendRequests: Set<number>;
    pendingRequests: Set<number>;
    showSettings: boolean;
    sfsUrl: string;
}

interface Prop {
    children: ReactNode;
}

interface AppContextProps {
    app: App;
    dispatch: React.Dispatch<DispatchAction>;
}

enum Action {
    SET_FOCUS,
    SET_LOGIN_USER,
    SELECT_HUB_CHANNEL,
    ADD_MSG,
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
    ADD_GROUP_MEMBERS,
    DEL_GROUP_MEMBER,
    ADD_BASIC_HUBS,
    ADD_DETAILED_HUB,
    ADD_HUB_MSGS,
    ADD_HUB_MSG,
    ADD_HUB_CHANNEL,
    ADD_HUB_CATEGORY,
    ADD_HUB_MEMBER,
    SET_SHOW_SETTINGS,
    SET_SFS_URL,
}

export type DispatchAction =
    | { type: Action.SET_FOCUS; payload: FocusState }
    | { type: Action.SET_LOGIN_USER; payload: User }
    | { type: Action.SELECT_HUB_CHANNEL; payload: { hub: Hub, channelID: number } }
    | { type: Action.ADD_USER; payload: User }
    | { type: Action.ADD_MSG; payload: Message }
    | { type: Action.ADD_DM_MSGS; payload: { dmID: string, messages: Message[] } }
    | { type: Action.ADD_GROUPS; payload: GroupProps[] }
    | { type: Action.ADD_DMS; payload: DMChat[] }
    | { type: Action.DEL_DM; payload: string }
    | { type: Action.ADD_GROUP_MEMBERS; payload: { groupID: number, IDs: number[] } }
    | { type: Action.DEL_GROUP_MEMBER; payload: { groupID: number, userID: number } }
    | { type: Action.ADD_BASIC_HUBS; payload: HubBasicData[] }
    | { type: Action.ADD_DETAILED_HUB; payload: HubDetailedData }
    | { type: Action.ADD_HUB_MSGS; payload: GetChannelMessagesData }
    | { type: Action.ADD_HUB_MSG; payload: HubMessageData }
    | { type: Action.ADD_HUB_CHANNEL; payload: HubChannelData }
    | { type: Action.ADD_HUB_CATEGORY; payload: NewCategoryData }
    | { type: Action.SET_SHOW_SETTINGS; payload: boolean }
    | { type: Action.ADD_HUB_MEMBER; payload: { hubID: number, userID: number } }
    | { type: Action.SET_SFS_URL; payload: string }
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
        case Action.SET_SFS_URL: {
            return {
                ...state,
                sfsUrl: action.payload
            };
        }
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
            return { ...state };
        }
        case Action.SET_FOCUS: {
            if (action.payload.type === 'hub') {
                const newHubs = new Map(state.hubs);
                const hub = newHubs.get(action.payload.hubID);
                if (hub) {
                    hub.selectedChannelID = action.payload.channelID;
                    return {
                        ...state,
                        focus: action.payload,
                        hubs: newHubs
                    };
                }
            }

            return {
                ...state,
                focus: action.payload
            };
        }
        case Action.SET_LOGIN_USER: {
            const user = action.payload;
            return {
                ...state,
                login_user: user,
                users: new Map(state.users).set(user.id, user)
            };
        }
        case Action.SELECT_HUB_CHANNEL: {
            const hub = action.payload.hub;
            const channelID = action.payload.channelID;
            const newHubs = new Map(state.hubs);
            hub.selectedChannelID = channelID;
            newHubs.set(hub.id, hub);
            return {
                ...state,
                hubs: newHubs,
                focus: { type: "hub", hubID: hub.id, channelID: channelID },
            };
        }
        case Action.ADD_USER: {
            const user = action.payload;
            return {
                ...state,
                users: new Map(state.users).set(user.id, user)
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
        case Action.ADD_GROUP_MEMBERS: {
            const dmchat = state.dm.get(DMChat.GroupID(action.payload.groupID));
            if (!dmchat) return state;

            const newDMs = new Map(state.dm);
            dmchat.chat = Group.addMemberIDs(dmchat.chat as Group, action.payload.IDs);
            newDMs.set(dmchat.id, dmchat);
            return {
                ...state,
                dm: newDMs
            };
        }
        case Action.DEL_GROUP_MEMBER: {
            const dmchat = state.dm.get(DMChat.GroupID(action.payload.groupID));
            if (!dmchat) return state;

            const newDMs = new Map(state.dm);
            dmchat.chat = Group.delMemberID(dmchat.chat as Group, action.payload.userID);
            newDMs.set(dmchat.id, dmchat);
            return {
                ...state,
                dm: newDMs
            };
        }
        case Action.ADD_BASIC_HUBS: {
            const newHubs = new Map(state.hubs);
            action.payload.forEach((hubData) => {
                newHubs.set(hubData.hub_id, new Hub(hubData));
            })
            return {
                ...state,
                hubs: newHubs
            }
        }
        case Action.ADD_DETAILED_HUB: {
            const data = action.payload;
            const newHubs = new Map(state.hubs);
            const newHub = Hub.fromDetailedData(data);
            const oldHub = newHubs.get(newHub.id);
            if (state.focus.type === "hub" &&
                state.focus.hubID === newHub.id &&
                oldHub &&
                oldHub.selectedChannelID !== -1) {
                newHub.selectedChannelID = oldHub.selectedChannelID;
            }
            newHubs.set(newHub.id, newHub);
            return {
                ...state,
                hubs: newHubs
            }
        }
        case Action.ADD_HUB_MSGS: {
            const newHubs = new Map(state.hubs);
            const hub = newHubs.get(action.payload.hub_id);
            if (hub) {
                const newHub = Hub.fromGetMessages(hub, action.payload);
                newHubs.set(newHub.id, newHub);
            }
            return {
                ...state,
                hubs: newHubs
            }
        }
        case Action.ADD_HUB_MSG: {
            const msg = action.payload;
            const newHubs = new Map(state.hubs);
            const hub = newHubs.get(msg.hub_id);
            if (hub) {
                newHubs.set(hub.id, Hub.fromMessage(hub, msg));
            }
            return {
                ...state,
                hubs: newHubs
            };
        }
        case Action.ADD_HUB_CHANNEL: {
            const c = action.payload;
            const newHubs = new Map(state.hubs);
            const hub = newHubs.get(c.hub_id);
            if (hub) {
                newHubs.set(hub.id, Hub.fromAddChannel(hub, c));
            }
            return {
                ...state,
                hubs: newHubs
            };
        }
        case Action.ADD_HUB_CATEGORY: {
            const c = action.payload;
            const newHubs = new Map(state.hubs);
            const hub = newHubs.get(c.hub_id);
            if (hub) {
                newHubs.set(hub.id, Hub.fromAddCategory(hub, c));
            }
            return {
                ...state,
                hubs: newHubs
            };
        }
        case Action.ADD_HUB_MEMBER: {
            const newHubs = new Map(state.hubs);
            const hub = newHubs.get(action.payload.hubID);
            if (hub) {
                newHubs.set(hub.id, Hub.fromAddMember(hub, action.payload.userID));
            }
            return {
                ...state,
                hubs: newHubs,
            };
        }
        case Action.SET_SHOW_SETTINGS: {
            return {
                ...state,
                showSettings: action.payload,
            };
        };
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

const appGetFocusHub = (app: App): Hub | undefined => {
    if (app.focus.type !== "hub") return undefined;
    return app.hubs.get(app.focus.hubID);
};

const appGetFocusDM = (app: App): DMChat | undefined => {
    if (app.focus.type !== "dm") return undefined;
    return app.dm.get(app.focus.dmid);
};

const AppProvider = ({ children }: Prop) => {
    const [state, dispatch] = useReducer(appReducer, {
        logged_in: false,
        login_user: {
            id: 0,
            username: '?',
            displayname: '?',
            pfp: '',
            about_me: '?',
            created_at: '?'
        },
        users: new Map(),
        hubs: new Map(),
        textChannels: new Map(),
        focus: { type: "friends" },
        dm: new Map(),
        friendIDs: new Set<number>(),
        friendRequests: new Set<number>(),
        pendingRequests: new Set<number>(),
        showSettings: false,
        sfsUrl: ''
    });

    return (
        <AppCtx.Provider value={{ app: state, dispatch: dispatch }}>
            {children}
        </AppCtx.Provider>
    );
};

export { useApp, AppProvider, Action, appGetFocusDM, appGetFocusHub };

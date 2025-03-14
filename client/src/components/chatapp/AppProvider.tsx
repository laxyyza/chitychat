import { createContext, useContext, useReducer, ReactNode } from 'react';
import User from './User';
import { Hub } from '../../models/hub';
import { TextChannel, ChannelType } from '../../models/channel';
import Message from '../../models/message';
import Group from '../../models/group';

export interface App {
    logged_in: boolean;
    login_user: User;
    users: Map<number, User>;
    hubs: Map<number, Hub>;
    groups: Map<number, Group>;
    textChannels: Map<number, TextChannel>;
    currentHubID: number;
    currentGroupID: number;
    currentChannelID: number;
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
    SELECT_GROUP,
    SET_LOGIN_USER,
    SELECT_CHANNEL,
    ADD_MSG,
    ADD_HUB,
    ADD_USER,
    ADD_GROUP,
    RECONNECT,
    SET_CONNECT
}

type DispatchAction =
    | { type: Action.SELECT_HUB; payload: number }
    | { type: Action.SELECT_GROUP; payload: number }
    | { type: Action.SELECT_CHANNEL; payload: number }
    | { type: Action.SET_LOGIN_USER; payload: User }
    | { type: Action.ADD_USER; payload: User }
    | { type: Action.ADD_MSG; payload: Message }
    | { type: Action.ADD_HUB; payload: string }
    | { type: Action.ADD_GROUP; payload: Group };

const AppCtx = createContext<AppContextProps | undefined>(undefined);

const appReducer = (state: App, action: DispatchAction): App => {
    switch (action.type) {
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
        case Action.SELECT_GROUP: {
            return {
                ...state,
                currentGroupID: action.payload
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
        case Action.ADD_MSG: {
            const msg = action.payload;
            if (msg.channel_type === 'hub') {
                const newTextChannels = new Map(
                    [...state.textChannels].map(([id, channel]) => {
                        if (channel.id === msg.channel_id) {
                            return [
                                id,
                                {
                                    ...channel,
                                    messages: [...channel.messages, msg]
                                }
                            ];
                        }
                        return [id, channel];
                    })
                );
                return { ...state, textChannels: newTextChannels };
            } else if (msg.channel_type === 'group') {
                const newGroups: Map<number, Group> = new Map(
                    [...state.groups].map(([id, group]) => {
                        if (msg.channel_id === id) {
                            return [id, Group.fromAddMessage(group, msg)];
                        }
                        return [id, group];
                    })
                );
                return { ...state, groups: newGroups };
            }

            return { ...state };
        }
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
        case Action.ADD_GROUP: {
            const group = action.payload;
            return {
                ...state,
                groups: new Map(state.groups).set(group.id, group)
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
        groups: new Map(),
        textChannels: new Map(),
        currentHubID: -1,
        currentGroupID: -1,
        currentChannelID: -1
    });

    return (
        <AppCtx.Provider value={{ app: state, dispatch: dispatch }}>
            {children}
        </AppCtx.Provider>
    );
};

export { useApp, AppProvider, Action };

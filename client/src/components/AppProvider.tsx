import { createContext, useContext, useReducer, ReactNode } from 'react';
import { Channel, ChannelType, Hub, Message, TextChannel, Group } from './Hub';
import User from './User';

export interface App {
    logged_in: boolean;
    login_user: User;
    users: Map<number, User>;
    hubs: Map<number, Hub>;
    textChannels: Map<number, TextChannel>;
    groups: Map<number, Group>;
    currentHubID: number;
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
    SET_LOGIN_USER,
    SELECT_CHANNEL,
    ADD_MSG
}

type DispatchAction =
    | { type: Action.SELECT_HUB; payload: number }
    | { type: Action.SELECT_CHANNEL; payload: number }
    | { type: Action.SET_LOGIN_USER; payload: User }
    | { type: Action.ADD_MSG; payload: Message };

const AppCtx = createContext<AppContextProps | undefined>(undefined);

const appReducer = (state: App, action: DispatchAction): App => {
    switch (action.type) {
        case Action.SELECT_HUB:
            const hub = state.hubs.get(action.payload);
            if (hub) {
                return {
                    ...state,
                    currentHubID: action.payload,
                    currentChannelID: hub.channelIDs[0]
                };
            }
            return { ...state, currentHubID: action.payload };
        case Action.SELECT_CHANNEL:
            return { ...state, currentChannelID: action.payload };
        case Action.SET_LOGIN_USER:
            return { ...state, login_user: action.payload };
        case Action.ADD_MSG: {
            const msg = action.payload;
            const newGroups = new Map(
                [...state.groups].map(([id, group]) => {
                    if (group.id === msg.channel_id) {
                        return [
                            id,
                            { ...group, messages: [...group.messages, msg] }
                        ];
                    }
                    return [id, group];
                })
            );
            console.log('newGroups: ', newGroups);

            return { ...state, groups: newGroups };

            const newTextChannels = new Map(
                [...state.textChannels].map(([id, channel]) => {
                    if (channel.id === msg.channel_id) {
                        return [
                            id,
                            { ...channel, messages: [...channel.messages, msg] }
                        ];
                    }
                    return [id, channel];
                })
            );
            console.log('newTextChannels: ', newTextChannels);

            return { ...state, textChannels: newTextChannels };
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
            username: '?',
            displayname: '?',
            pfp: '',
            about_me: '?',
            created_at: '?'
        },
        users: new Map(),
        hubs: new Map(),
        textChannels: new Map(),
        groups: new Map().set(1, {
            id: 1,
            name: 'General',
            messages: []
        }),
        currentHubID: -1,
        currentChannelID: -1
    });

    return (
        <AppCtx.Provider value={{ app: state, dispatch: dispatch }}>
            {children}
        </AppCtx.Provider>
    );
};

export { useApp, AppProvider, Action };

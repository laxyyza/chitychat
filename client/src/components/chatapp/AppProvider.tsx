import { createContext, useContext, useReducer, ReactNode } from 'react';
import User from './User';
import { Hub } from '../../models/hub';
import { TextChannel, ChannelType } from '../../models/channel';
import Message from '../../models/message';

export interface App {
    logged_in: boolean;
    login_user: User;
    users: Map<number, User>;
    hubs: Map<number, Hub>;
    textChannels: Map<number, TextChannel>;
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
    ADD_MSG,
    ADD_HUB,
    RECONNECT,
    SET_CONNECT
}

type DispatchAction =
    | { type: Action.SELECT_HUB; payload: number }
    | { type: Action.SELECT_CHANNEL; payload: number }
    | { type: Action.SET_LOGIN_USER; payload: User }
    | { type: Action.ADD_MSG; payload: Message }
    | { type: Action.ADD_HUB; payload: string };

const AppCtx = createContext<AppContextProps | undefined>(undefined);

const appReducer = (state: App, action: DispatchAction): App => {
    switch (action.type) {
        case Action.SELECT_HUB:
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
        case Action.SELECT_CHANNEL:
            return { ...state, currentChannelID: action.payload };
        case Action.SET_LOGIN_USER:
            return { ...state, login_user: action.payload };
        case Action.ADD_MSG: {
            const msg = action.payload;
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

            return { ...state, textChannels: newTextChannels };
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

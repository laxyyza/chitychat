import { createContext, useContext, useReducer, ReactNode } from 'react';
import { Hub } from './Hub';
import User from './User';

export interface App {
    logged_in: boolean;
    login_user: User;
    users: Map<number, User>;
    hubs: Hub[];
    hubIndex: number;
    selectedChannelID: number;
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
    SELECT_CHANNEL
}

type DispatchAction =
    | { type: Action.SELECT_HUB; payload: number }
    | { type: Action.SELECT_CHANNEL; payload: number }
    | { type: Action.SET_LOGIN_USER; payload: User };

const AppCtx = createContext<AppContextProps | undefined>(undefined);

const appReducer = (state: App, action: DispatchAction): App => {
    switch (action.type) {
        case Action.SELECT_HUB:
            return { ...state, hubIndex: action.payload };
        case Action.SELECT_CHANNEL:
            return { ...state, selectedChannelID: action.payload };
        case Action.SET_LOGIN_USER:
            return { ...state, login_user: action.payload };
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
        hubs: [],
        hubIndex: -1,
        selectedChannelID: -1
    });

    return (
        <AppCtx.Provider value={{ app: state, dispatch: dispatch }}>
            {children}
        </AppCtx.Provider>
    );
};

export { useApp, AppProvider, Action };

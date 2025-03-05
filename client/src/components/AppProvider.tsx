import { createContext, useContext, useState, ReactNode } from 'react';
import { Hub } from './Hub';
import User from './User';

export interface App {
    logged_in: boolean;
    login_user: User;
    users: User[];
    hubs: Hub[];
}

interface Prop {
    children: ReactNode;
}

const AppCtx = createContext<App>({
    logged_in: false,
    login_user: {
        id: 0,
        username: '?',
        displayname: '?',
        pfp: '',
        about_me: '?',
        created_at: '?'
    },
    users: [],
    hubs: []
});

const AppProvider = ({ children }: Prop) => {
    return (
        <AppCtx.Provider value={useContext(AppCtx)}>{children}</AppCtx.Provider>
    );
};

export { AppProvider, AppCtx };

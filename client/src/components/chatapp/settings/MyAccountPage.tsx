import { useEffect, useRef, useState } from "react";
import { Action, useApp } from "../AppProvider";
import UserIcon from "../UserIcon";
//import useDismissTrigger from "../../../hooks/useDismissTrigger";
import fetchData from "../../../services/api";

interface EditSectionProps {
    name: string;
    value: string;
    resizable?: boolean;
    submit: (newValue: string) => Promise<boolean>;
}

const EditSection = ({ name, value, resizable = false, submit }: EditSectionProps) => {
    const [edit, setEdit] = useState(false);
    const [newValue, setNewValue] = useState(value);
    const ref = useRef<HTMLDivElement | null>(null);

    useEffect(() => {
        setNewValue(value);
    }, [value]);

    // const dismiss = () => {
    //     setNewValue(value);
    //     setEdit(false);
    // };

    return (
        <div className="m-3 flex items-center" ref={ref}>
            <div className="grow-1">
                <div className="text-xl">
                    {name}
                </div>
                <textarea
                    value={newValue}
                    onChange={(e) => setNewValue(e.target.value)}
                    rows={1}
                    className={`w-full outline-0 rounded-md ${(edit) ? 'bg-gray-950 p-1' : ''} ${resizable ? 'resize-y' : 'resize-none'}`}
                    disabled={!edit}
                />
            </div>
            <button
                className="shrink-0 bg-gray-800 p-2 ml-2 w-15 rounded-xl self-end"
                onClick={(e) => {
                    e.preventDefault();
                    if (edit && newValue !== value) {
                        submit(newValue).then((ret) => {
                            if (!ret) {
                                setNewValue(value);
                            }
                        });
                    }
                    setEdit(!edit);
                }}
            >
                {edit ? ((value !== newValue) ? 'Save' : 'Cancel') : 'Edit'}
            </button>
        </div>
    );
};

const MyAccountPage = () => {
    const { app, dispatch } = useApp();
    const [user, setUser] = useState(app.login_user);
    const [error, setError] = useState('');

    useEffect(() => {
        setUser(app.login_user);
    }, [app, app.login_user]);

    const patchMe = async (patch: any) => {
        return fetchData('/api/users/me', 'PATCH', patch)
            .then((json) => {
                const newUser = {...user, ...json};
                console.log("NewUser: ", newUser);
                dispatch({
                    type: Action.SET_LOGIN_USER,
                    payload: newUser,
                });
                setError('');
                return true;
            })
            .catch((error: any) => {
                setError(error);
                return false;
            });
    };

    return (
        <div className="p-10">
            <div className="bg-gray-950 rounded-xl p-3 w-150">
                {error && (
                    <div className="p-3 bg-red-500 rounded-md m-1 text-xl">
                        {error}
                    </div>
                )}
                <div className="flex p-3 select-none">
                    <UserIcon user={user} />
                    <div className="text-xl self-end ml-3 grow-1">{user.displayname}</div>
                    <div className="text-xs text-right">
                        <div className="text-gray-400">UID</div>
                        <div className="select-text">{user.id}</div>
                    </div>
                </div>
                <div className="w-full bg-gray-900 rounded-xl p-2">
                    <EditSection
                        name={'Display Name'}
                        value={user.displayname}
                        submit={(newDisplayName) => {
                            return patchMe({ displayname: newDisplayName });
                        }}
                    />
                    <EditSection
                        name={'Username'}
                        value={user.username}
                        submit={(newUsername) => {
                            return patchMe({ username: newUsername });
                        }}
                    />
                    <EditSection
                        name={'About Me'}
                        value={user.about_me}
                        resizable
                        submit={(newAboutMe) => {
                            return patchMe({ about_me: newAboutMe });
                        }}
                    />
                </div>
            </div>
        </div>
    );
};

export default MyAccountPage;

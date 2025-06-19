import { useEffect, useRef, useState } from "react";
import { Action, useApp } from "../AppProvider";
import UserIcon from "../UserIcon";
//import useDismissTrigger from "../../../hooks/useDismissTrigger";
import fetchData from "../../../services/api";
import CropImage from "./CropImage";

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

function dataURItoBlob(dataURI: string): Blob {
    // convert base64 to raw binary data held in a string
    // doesn't handle URLEncoded DataURIs - see SO answer #6850276 for code that does this
    var byteString = atob(dataURI.split(',')[1]);

    // separate out the mime component
    var mimeString = dataURI.split(',')[0].split(':')[1].split(';')[0]

    // write the bytes of the string to an ArrayBuffer
    var ab = new ArrayBuffer(byteString.length);

    // create a view into the buffer
    var ia = new Uint8Array(ab);

    // set the bytes of the buffer to the correct values
    for (var i = 0; i < byteString.length; i++) {
        ia[i] = byteString.charCodeAt(i);
    }

    // write the ArrayBuffer to a blob, and you're done
    var blob = new Blob([ab], { type: mimeString });
    return blob;
}

const MyAccountPage = () => {
    const { app, dispatch } = useApp();
    const [user, setUser] = useState(app.login_user);
    const [error, setError] = useState('');
    const inputFileRef = useRef<HTMLInputElement | null>(null);
    const [newPfp, setNewPfp] = useState<File | null>(null);

    useEffect(() => {
        setUser(app.login_user);
    }, [app, app.login_user]);

    const patchMe = async (patch: any) => {
        return fetchData('/api/users/me', 'PATCH', patch)
            .then((json) => {
                const newUser = { ...user, ...json };
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

    const sendFile = async (file: Blob) => {
        const formData = new FormData();
        formData.append('file', file);

        return await fetch(`${app.sfsUrl}/api/upload/pfp_${user.id}`, {
            method: 'POST',
            body: formData,
        }).then(async (resp) => {
            const ret = await resp.json();
            return `${app.sfsUrl}${ret.endpoint}`;
        });
    };

    const uploadPfp = async (imageUrl: string) => {
        const blob = dataURItoBlob(imageUrl);

        const pfpUrl = await sendFile(blob);

        await patchMe({ pfp_url: pfpUrl });
        setNewPfp(null)
    };

    return (
        <div className="p-10">
            {newPfp && (
                <CropImage
                    image={newPfp}
                    onClose={(finalImage) => {
                        if (finalImage) {
                            uploadPfp(finalImage);
                        } else {
                            setNewPfp(null);
                        }
                    }}
                />
            )}
            <div className="bg-gray-950 rounded-xl p-3 w-150">
                {error && (
                    <div className="p-3 bg-red-500 rounded-md m-1 text-xl">
                        {error}
                    </div>
                )}
                <div className="flex p-3 select-none">
                    <button onClick={() => {
                        inputFileRef.current?.click();
                    }}>
                        <UserIcon className="w-16" user={user} />
                    </button>
                    <input ref={inputFileRef} className='absolute opacity-0' type='file' onChange={(e) => {
                        if (e.target.files) {
                            setNewPfp(e.target.files[0]);
                        }
                    }} />
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

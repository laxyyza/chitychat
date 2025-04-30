import { FaUser } from "react-icons/fa6";
import { App, useApp } from "./AppProvider";
import User from "./User";
import { useEffect, useState } from "react";
import fetchData from "../../services/api";
import { fetchGroup } from "../../services/groupApi";


interface Prop {
    onClose: () => void;
}

interface UserProp {
    user: User;
    selected: boolean;
    onClick: (value: boolean) => void;
}

const SelectableUser = ({user, selected, onClick}: UserProp) => {
    const [checked, setChecked] = useState(selected);

    useEffect(() => {
        setChecked(selected);
    }, [selected]);

    return (
        <div className="w-full mb-2 h-13 rounded-xl bg-gray-700 p-1 flex">
            <div className="bg-blue-500 rounded-full w-11 h-11 overflow-hidden flex items-center justify-center">
                <FaUser size="24"/>
            </div>
            <div className="flex-1 text-left ml-2">
                <div>{user.displayname}</div>
                <div>{user.username}</div>
            </div>
            <input
                type="checkbox"
                className="w-8 mr-2"
                checked={checked}
                onChange={(e) => {
                    setChecked(e.target.checked);
                    onClick(e.target.checked);
                }}
            >
            </input>
        </div>
    );
}

const getFriends = (app: App, filter: string): User[] => {
    const friends: User[] = []
    app.friendIDs.forEach((friendID) => {
        const user = app.users.get(friendID);
        if (user) {
            if (filter) {
                if (user.username.includes(filter) || user.displayname.toLowerCase().includes(filter)) {
                    friends.push(user);
                }
            } else {
                friends.push(user);
            }
        }
    })
    return friends;
}

const CreateGroup = ({onClose}: Prop) => {
    const {app, dispatch} = useApp();
    const [filter, setFilter] = useState('');
    const [name, setName] = useState('');
    const [desc, setDesc] = useState('');
    const [error, setError] = useState('');
    const friends = getFriends(app, filter);
    const [selectedIDs, setSelectedIDs] = useState(new Set());

    const onSubmit = () => {
        setError('');
        fetchData("/api/groups", "POST", {
            name: name.trim(),
            desc: desc.trim(),
            user_ids: Array.from(selectedIDs)
        }).then(resp => {
            fetchGroup(resp.group_id, dispatch);
            onClose();
        }).catch(e => {
            setError(e);
        });
    };

    return (
        <div
            className="absolute flex justify-center items-center w-full h-full z-1000 backdrop-blur-xs text-white"
            onClick={() => onClose()}
            onKeyUp={(e) => {
                if (e.key == 'Escape') onClose();
            }}
        >
            <div
                className="bg-gray-800 w-150 p-2 rounded-xl border-1 border-black"
                onClick={(e) => e.stopPropagation()}
            >
                {error && <div className="bg-red-500 rounded-xs p-1">ERROR: {error}</div>}
                <form
                    onSubmit={(e) => {
                        e.preventDefault();
                        if (name.trim().length === 0) {
                            alert("Please enter valid name");
                            return;
                        }

                        if (selectedIDs.size === 0) {
                            alert("Please select users");
                            return;
                        }

                        onSubmit();
                    }}
                >
                    <div className="flex m-1">
                        <span className="mr-3">Group Name:</span>
                        <input
                            className="flex-1 bg-gray-900 rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                            type="text"
                            placeholder="Enter group name"
                            value={name}
                            required
                            onChange={(e) => {
                                setName(e.target.value);
                            }}
                        />
                    </div>
                    <div className="flex m-1">
                        <span className="mr-3">Desc:</span>
                        <input
                            className="flex-1 bg-gray-900 rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                            type="text"
                            placeholder="Enter group's description"
                            value={desc}
                            onChange={(e) => {
                                setDesc(e.target.value);
                            }}
                        />
                    </div>
                    <div className="flex m-1">
                        <input
                            className="flex-1 bg-gray-900 rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                            type="text"
                            placeholder="Filter"
                            value={filter}
                            onChange={(e) => {
                                setFilter(e.target.value);
                            }}
                        />
                    </div>
                    <div className="h-100 mr-1 rounded-xl overflow-auto">
                        {friends.map((friend) => (
                            <li key={friend.id}>
                                <SelectableUser user={friend} selected={selectedIDs.has(friend.id)} onClick={(value) => {
                                    const newSet = new Set(selectedIDs);
                                    if (value) {
                                        newSet.add(friend.id);
                                    } else {
                                        newSet.delete(friend.id);
                                    }
                                    setSelectedIDs(newSet);
                                }}/>
                            </li>
                        ))}
                    </div>
                    <div className="flex mt-2">
                        <div className="flex-1"></div>
                        <button type="submit" className="right-0 bg-green-600 p-1 pr-3 pl-3 rounded-2xl text-xl font-bold">Create</button>
                    </div>
                </form>
            </div>
        </div>
    );
};

export default CreateGroup;

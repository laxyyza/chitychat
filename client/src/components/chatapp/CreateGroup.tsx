import { FaUser } from "react-icons/fa6";
import { App, useApp } from "./AppProvider";
import User from "./User";
import { useEffect, useState } from "react";

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
    const {app} = useApp();
    const [filter, setFilter] = useState('');
    const friends = getFriends(app, filter);
    const [selectedIDs, setSelectedIDs] = useState(new Set());

    useEffect(() => {
        console.log(selectedIDs);
    }, [selectedIDs]);

    return (
        <div
            className="absolute flex justify-center items-center w-full h-full z-1000 backdrop-blur-xs text-white"
            onClick={() => onClose()}
            onKeyUp={(e) => {
                console.log('key ', e.key);
                if (e.key == 'Escape') onClose();
            }}
        >
            <div
                className="bg-gray-800 w-150 p-2 rounded-xl border-1 border-black"
                onClick={(e) => e.stopPropagation()}
            >
                <form
                >
                    <div className="flex m-1">
                        <span className="mr-3">Group Name:</span>
                        <input
                            className="flex-1 bg-gray-900 rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                            type="text"
                            placeholder="Enter group name"
                            required
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
                            <SelectableUser user={friend} selected={selectedIDs.has(friend.id)} onClick={(value) => {
                                const newSet = new Set(selectedIDs);
                                if (value) {
                                    newSet.add(friend.id);
                                } else {
                                    newSet.delete(friend.id);
                                }
                                setSelectedIDs(newSet);
                            }}/>
                        ))}
                    </div>
                    <div className="flex mt-2">
                        <div className="flex-1"></div>
                        <button className="right-0 bg-green-600 p-1 pr-3 pl-3 rounded-2xl text-xl font-bold">Create</button>
                    </div>
                </form>
            </div>
        </div>
    );
};

export default CreateGroup;

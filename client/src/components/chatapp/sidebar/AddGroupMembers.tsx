import {  useState } from "react";
import { DMChat } from "../../../models/dm";
import Group from "../../../models/group";
import { App, useApp } from "../AppProvider";
import SelectableUser from "../SelectableUser";
import User from "../User";
import { fetchAddFriends } from "../../../services/groupApi";
import Modal from "../modals/Modal";

interface Props {
    dmchat: DMChat;
    onClose: () => void;
    ref: React.RefObject<HTMLDivElement | null>;
}

const getFriendsNotInGroup = (app: App, group: Group, filter: string): User[] => {
    const friends: User[] = []
    app.friendIDs.forEach((friendID) => {
        const user = app.users.get(friendID);
        if (user) {
            if (group.memberIDs.has(user.id)) {
                return;
            }

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

const AddGroupMembers = ({dmchat, onClose, ref}: Props) => {
    const {app} = useApp();
    const [selectedIDs, setSelectedIDs] = useState(new Set<number>());
    const [filter, setFilter] = useState('');
    //const ref = useRef<HTMLDivElement | null>(null);

    if (dmchat.chat instanceof Group == false) {
        return null;
    }
    const group = dmchat.chat;
    const friends = getFriendsNotInGroup(app, group, filter);

    return (
        <Modal ref={ref} onClose={onClose}>
            <div className='relative bg-gray-900 border-black w-100 border-1 p-5 rounded-xl' onClick={e => e.stopPropagation()}>
                <h1>Add Friends to <span className="text-purple-500 font-bold">{group.name}</span></h1>
                <div className="flex m-1">
                    <input
                        className="flex-1 bg-gray-950 border-1 border-black rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                        type="text"
                        placeholder="Filter"
                        value={filter}
                        onChange={(e) => {
                            setFilter(e.target.value);
                        }}
                    />
                </div>
                <div className='mb-8 mt-2 overflow-auto h-100 max-h-100'>
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
                <button 
                    className="absolute bottom-0 right-0 bg-green-600 hover:bg-green-500 active:bg-green-400 pr-4 pl-4 m-1 rounded-xl p-1"
                    onClick={() => {
                        if (selectedIDs.size !== 0) {
                            fetchAddFriends(group.id, Array.from(selectedIDs));
                            onClose();
                        }
                    }}
                >
                    Add
                </button>
            </div>
        </Modal>
    );
};

export default AddGroupMembers;

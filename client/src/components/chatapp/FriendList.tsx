import { useApp } from './AppProvider';
import User from './User';
import UserIcon from './UserIcon';

interface FriendProp {
    user: User;
}

const Friend = ({ user }: FriendProp) => {
    return (
        <div
            key={'friend-' + user.id}
            className="m-1 p-1 flex hover:bg-gray-600 rounded-xl text-white"
        >
            <UserIcon user={user} />
            <div className="ml-2">
                <div className="font-bold text-xl">{user.displayname}</div>
                <div className="text-xs">{user.username}</div>
            </div>
        </div>
    );
};

const FriendList = () => {
    const { app } = useApp();
    const friends = app.friendIDs.map((friend_id) => app.users.get(friend_id));

    return (
        <div>
            {friends.map((friend) => {
                if (!friend) return null;
                return <Friend user={friend} />;
            })}
        </div>
    );
};

export default FriendList;

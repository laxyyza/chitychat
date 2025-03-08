import User from './User';
import UserIcon from './UserIcon';

const Member = (user: User) => {
    return (
        <div className="relative hover:bg-gray-700 p-1 m-1 mb-2 mt-2 rounded-2xl text-white flex active:bg-gray-600">
            <UserIcon user={user}></UserIcon>
            <div className="flex-1 ml-2">
                <div className="font-bold text-xl">{user.displayname}</div>
                <div>{user.username}</div>
            </div>
        </div>
    );
};

const MemberList = () => {
    // const users = [
    //     {
    //         id: 14,
    //         username: 'username',
    //         displayname: 'Display Name',
    //         pfp: 'https://www.oola.com/wp-content/uploads/2022/07/communityIcon_x4lqmqzu1hi81.jpeg'
    //     }
    // ];

    return (
        <div className="relative max-h-screen w-60 bg-gray-800 overflow-auto">
            <div className="text-center text-white font-bold shadow-xl bg-gray-900">
                {/* {users.length} members */}
            </div>
            {/* {users.map((user) => Member(user))} */}
        </div>
    );
};

export default MemberList;

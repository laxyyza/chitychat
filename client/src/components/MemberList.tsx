import UserIcon from './UserIcon';

const Member = (username: string, displayName: string) => {
    return (
        <div className="relative hover:bg-gray-700 p-1 m-1 mb-2 mt-2 rounded-2xl text-white flex active:bg-gray-600">
            <UserIcon username={username} placement="left"></UserIcon>
            <div className="flex-1">
                <div className="font-bold text-xl">{displayName}</div>
                <div>{username}</div>
            </div>
        </div>
    );
};

const MemberList = () => {
    const members = [
        Member('username', 'Display Name'),
        Member('username2', 'Display Name'),
        Member('username3', 'Display Name'),
        Member('username4', 'Display Name'),
        Member('username6', 'Display Name')
    ];

    return (
        <div className="relative max-h-screen w-60 bg-gray-800 overflow-auto">
            <div className="text-center text-white font-bold shadow-xl bg-gray-900">
                {members.length} members
            </div>
            {members}
        </div>
    );
};

export default MemberList;

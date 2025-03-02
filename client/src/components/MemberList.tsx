const Member = (username: string, displayName: string) => {
    return (
        <div className="bg-gray-500 m-1 p-1 rounded-xl">
            <div>{displayName}</div>
            <div>{username}</div>
        </div>
    );
};

const MemberList = () => {
    const members = [
        Member('username', 'Display Name'),
        Member('username', 'Display Name'),
        Member('username', 'Display Name'),
        Member('username', 'Display Name'),
        Member('username', 'Display Name')
    ];

    return <div className="member-list overflow-auto">{members}</div>;
};

export default MemberList;

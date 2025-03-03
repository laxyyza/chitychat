import { CiSettings } from 'react-icons/ci';

const UserProfile = () => {
    return (
        <div className="flex bg-gray-950 text-white">
            <div className="rounded-4xl bg-white w-10 h-10 m-1 "></div>
            <div className="flex-1 p-0 m-0">
                <div className="font-bold text-left">Display Name</div>
                <div className="text-[14px]">username</div>
            </div>
            <CiSettings className="m-1 bg-black" size="32"></CiSettings>
        </div>
    );
};

export default UserProfile;

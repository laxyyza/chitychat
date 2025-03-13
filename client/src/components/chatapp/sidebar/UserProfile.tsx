import { CiSettings } from 'react-icons/ci';
import UserIcon from '../UserIcon';
import User from '../User';

interface Prop {
    user: User;
}

const UserProfile = ({ user }: Prop) => {
    return (
        <div className="flex bg-gray-950 text-white">
            <div className="rounded-4xl bg-white w-13 h-13 m-1 flex">
                <UserIcon user={user}></UserIcon>
            </div>
            <div className="flex-1 min-w-0 ml-1">
                <div className="font-bold text-left text-lg text-nowrap text-ellipsis overflow-hidden">
                    {user.displayname}
                </div>
                <div className="text-[14px] text-ellipsis overflow-hidden">
                    {user.username}
                </div>
            </div>
            <CiSettings className="g-black m-auto" size="32"></CiSettings>
        </div>
    );
};

export default UserProfile;

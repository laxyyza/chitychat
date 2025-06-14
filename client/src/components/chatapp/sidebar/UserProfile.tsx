import UserIcon from '../UserIcon';
import User from '../User';
import { IoSettingsSharp } from 'react-icons/io5';
import { Action, useApp } from '../AppProvider';

interface Prop {
    user: User;
    showSettings?: boolean;
}

const UserProfile = ({ user }: Prop) => {
    const { dispatch } = useApp();

    return (
        <div className="text-white p-1">
            <div className='flex bg-gray-800 rounded-xl border-gray-600 border-1'>
                <div className="rounded-4xl bg-white w-13 h-13 m-1 flex">
                    <UserIcon user={user}></UserIcon>
                </div>
                <div className="flex-1 min-w-0 ml-1">
                    <div className="font-bold text-left text-lg text-nowrap text-ellipsis overflow-hidden">
                        {user.displayname}
                    </div>
                    <div className="text-[14px] text-left text-ellipsis overflow-hidden">
                        {user.username}
                    </div>
                </div>
                <button
                    className='m-auto mr-3 hover:bg-gray-700 active:bg-gray-500 p-1 rounded-xl hover:rotate-180 transition-all duration-500'
                    onClick={() => dispatch({type: Action.SET_SHOW_SETTINGS, payload: true }) }
                >
                    <IoSettingsSharp size="24" />
                </button>
            </div>
        </div>
    );
};

export default UserProfile;

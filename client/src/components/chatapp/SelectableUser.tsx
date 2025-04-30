import { useEffect, useState } from "react";
import User from "./User";
import { FaUser } from "react-icons/fa6";

interface Props {
    user: User;
    selected: boolean;
    onClick: (value: boolean) => void;
}

const SelectableUser = ({user, selected, onClick}: Props) => {
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

export default SelectableUser;

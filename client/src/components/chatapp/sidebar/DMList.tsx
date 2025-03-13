import { ReactNode } from 'react';
import { Action, useApp } from '../AppProvider';
import { BiSolidGroup } from 'react-icons/bi';
import { RiUserHeartFill } from 'react-icons/ri';

interface DMProp {
    name?: string;
    onClick?: () => void;
    selected: boolean;
    type: 'group' | 'dm' | 'other';
    children?: ReactNode;
}

const Icon = (type: string) => {
    if (type === 'group') {
        return (
            <div className="bg-purple-500 rounded-full w-8 h-8 overflow-hidden flex items-center justify-center">
                <BiSolidGroup />
            </div>
        );
    } else if (type === 'dm') {
        return <span>?</span>;
    } else {
        return null;
    }
};

const DM = ({ name, onClick, selected, type, children }: DMProp) => {
    return (
        <button
            className={
                'flex mb-1 items-center rounded-[8px] hover:bg-gray-600 active:bg-gray-500 max-w-full w-full p-1 ' +
                (selected ? 'bg-gray-600' : '')
            }
            onClick={onClick}
        >
            {Icon(type)}
            <div className="flex-1 ml-1 min-w-0">
                <div className="text-left text-nowrap text-ellipsis overflow-hidden">
                    {name || children}
                </div>
            </div>
        </button>
    );
};

const DMList = () => {
    const { app, dispatch } = useApp();
    const groups = Array.from(app.groups.entries());

    return (
        <>
            <DM
                selected={app.currentGroupID === -1}
                type="other"
                onClick={() =>
                    dispatch({ type: Action.SELECT_GROUP, payload: -1 })
                }
            >
                <div className="flex items-center text-center justify-center">
                    <div>
                        <RiUserHeartFill size="22" />
                    </div>
                    <div className="ml-1 font-bold p-1">Friends</div>
                </div>
            </DM>
            <div className="text-center text-xs font-bold">Direct Messages</div>
            <ul className="p-1">
                {groups.map(([id, group]) => (
                    <li key={id}>
                        <DM
                            name={group.name}
                            selected={app.currentGroupID === id}
                            type="group"
                            onClick={() =>
                                dispatch({
                                    type: Action.SELECT_GROUP,
                                    payload: id
                                })
                            }
                        />
                    </li>
                ))}
            </ul>
        </>
    );
};

export default DMList;

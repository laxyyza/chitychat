import { createPortal } from "react-dom";
import { Action, useApp } from "./../AppProvider";
import { useRef, useState } from "react";
import useDismissTrigger from "../../../hooks/useDismissTrigger";
import { IoMdCloseCircleOutline } from "react-icons/io";
import MyAccountPage from "./MyAccountPage";

interface OptionProps {
    name: string;
    onClick: () => void;
    selected: boolean;
};

type SettingsPage = {
    name: string;
    element: React.ReactNode;
};

const SettingOption = ({ name, onClick, selected }: OptionProps) => {
    return (
        <button
            className={`p-1 m-1 rounded-xl hover:bg-gray-800 self-end w-65 ${(selected) ? 'bg-gray-800' : ''}`}
            onClick={onClick}
        >
            {name}
        </button>
    )
};

const AboutContent = () => (
    <div className="p-10 w-full h-full">
        <div className="bg-gray-950">
            <h1>
                About!
            </h1>
        </div>
    </div>
);

const settingsPages: SettingsPage[] = [
    { name: 'My Account', element: <MyAccountPage /> },
    { name: 'About', element: <AboutContent /> },
];

const Settings = () => {
    const { app, dispatch } = useApp();
    const [settingsContentIdx, setSettingsContentIdx] = useState(0);
    const ref = useRef<HTMLDivElement | null>(null);

    const scale = (app.showSettings) ? 'opacity-100  z-2000' : 'opacity-0 -z-10';

    useDismissTrigger(() => dispatch({ type: Action.SET_SHOW_SETTINGS, payload: false }), [ref]);

    if (app.showSettings === false)
        return null;

    return createPortal(
        <div
            className={`fixed text-sm top-0 left-0 h-screen w-screen bg-gray-900 duration-300 transition-all ${scale} text-white flex`}
            ref={ref}
        >
            <div className="h-screen w-[30%]">
                <div className="w-full flex flex-col max-h-screen overflow-auto">
                    {settingsPages.map((page, idx) => (
                        <SettingOption
                            key={page.name}
                            name={page.name}
                            onClick={() => setSettingsContentIdx(idx)}
                            selected={idx === settingsContentIdx}
                        />
                    ))}
                </div>
            </div>
            <div className="h-full grow-1 bg-gray-800">
                <div className="flex max-w-175 mr-5 pt-5">
                    <div className="grow-1"></div>
                    <button
                        className="shrink-0 hover:bg-gray-700 transition-all duration-500 rounded-xl p-1"
                        onClick={() => dispatch({ type: Action.SET_SHOW_SETTINGS, payload: false })}
                    >
                        <IoMdCloseCircleOutline size={32} />
                    </button>
                </div>
                {settingsPages[settingsContentIdx].element}
            </div>
        </div>,
        document.body
    );
};

export default Settings;

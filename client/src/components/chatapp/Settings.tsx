import { createPortal } from "react-dom";
import { useApp } from "./AppProvider";
import { useState } from "react";

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

const MyAccountContent = () => (
    <div className="p-10 w-full h-full">
        <div className="bg-gray-950">
            <div>
                Display Name
            </div>
            <div>
                Username
            </div>
        </div>
    </div>
);

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
    { name: 'My Account', element: <MyAccountContent /> },
    { name: 'About', element: <AboutContent /> },
];

const Settings = () => {
    const { app } = useApp();
    const [settingsContentIdx, setSettingsContentIdx] = useState(0);

    const scale = (app.showSettings) ? 'scale-100' : 'scale-0';

    return createPortal(
        <div className={`fixed z-2000 text-sm top-0 left-0 h-screen w-screen bg-gray-900 duration-300 transition-all ${scale} text-white flex`}>
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
                {settingsPages[settingsContentIdx].element}
            </div>
        </div>,
        document.body
    );
};

export default Settings;

import Input from './Input';
import ChatWindow from './ChatWindow';

interface HeaderBarProp {
    name: string;
}

const HeaderBar = ({ name }: HeaderBarProp) => {
    return (
        <div className="bg-gray-800 text-center text-white shadow">{name}</div>
    );
};

const MainContent = () => {
    return (
        <div className="flex flex-col flex-1 h-screen bg-gray-700">
            <HeaderBar name="Text Channel Name"></HeaderBar>
            <ChatWindow />
            <div className="bg-gray-900 m-3 rounded-2xl text-white max-h-[50%] overflow-auto">
                <Input />
            </div>
        </div>
    );
};

export default MainContent;

const ChatWindow = () => {
    return <div className="chat-window"></div>;
};

const Input = () => {
    return <div className="input"></div>;
};

const MainContent = () => {
    return (
        <div className="main-content">
            <ChatWindow></ChatWindow>
            <Input></Input>
        </div>
    );
};

export default MainContent;

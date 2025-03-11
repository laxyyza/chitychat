import { ChangeEvent, ChangeEventHandler, useState } from 'react';

interface LoginToggleProp {
    type: 'login' | 'register' | 'checkbox';
    onClick: () => void;
    selected: boolean;
}

interface InputProp {
    name: string;
    type: 'text' | 'password';
    value: string;
    onChange: (event: React.ChangeEvent<HTMLInputElement>) => void;
    placeholder?: string;
}

interface StatusMsg {
    type: 'error' | 'info';
    msg: string;
}

const LoginToggleButton = ({ type, onClick, selected }: LoginToggleProp) => {
    const name = type === 'login' ? 'Login' : 'Register';

    return (
        <button
            className={
                'flex-1 text-center font-bold  ' +
                (selected ? 'bg-blue-500' : 'hover:bg-gray-600') +
                ' ' +
                (type === 'login' ? 'rounded-l-xl' : 'rounded-r-xl')
            }
            onClick={onClick}
        >
            {name}
        </button>
    );
};

const Input = ({ name, type, onChange, value, placeholder }: InputProp) => {
    return (
        <>
            <div className="font-bold mt-2">{name}:</div>
            <input
                className="w-full bg-gray-900 text-xl p-1 rounded-xl outline-0"
                type={type}
                onChange={onChange}
                value={value}
                placeholder={placeholder}
                required
            />
        </>
    );
};

const Login = () => {
    const [doRegister, setDoRegister] = useState(false);
    const [username, setUsername] = useState('');
    const [displayName, setDisplayName] = useState('');
    const [password, setPassword] = useState('');
    const [statusMsg, setStatusMsg] = useState<StatusMsg>({
        type: 'info',
        msg: ''
    });

    const handleSubmit = (event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        setStatusMsg({ type: 'info', msg: 'Connecting...' });
    };

    return (
        <div className="flex flex-col w-screen h-screen bg-gray-700 justify-center items-center text-white">
            {statusMsg.msg && (
                <div
                    className={
                        'w-100 m-2 rounded-2xl border-1 p-2 font-bold border-black ' +
                        (statusMsg.type === 'error'
                            ? 'bg-red-600'
                            : 'bg-green-600')
                    }
                >
                    {statusMsg.type === 'error' ? 'ERROR:' : ''} {statusMsg.msg}
                </div>
            )}
            <div className="relative bg-gray-800 w-100 h-80 p-2 rounded-2xl border-1 border-black">
                <div className="flex bg-gray-900 m-1 rounded-xl select-none">
                    <LoginToggleButton
                        type="login"
                        onClick={() => setDoRegister(false)}
                        selected={!doRegister}
                    />
                    <LoginToggleButton
                        type="register"
                        onClick={() => setDoRegister(true)}
                        selected={doRegister}
                    />
                </div>
                <div>
                    <form onSubmit={handleSubmit}>
                        <Input
                            name="Username"
                            type="text"
                            value={username}
                            onChange={(event) => {
                                const input = event.target.value
                                    .toLowerCase()
                                    .replace(/[^a-z0-9._-]/g, '');
                                console.log('input', input);
                                setUsername(input);
                            }}
                        />
                        {doRegister && (
                            <Input
                                name="Display Name"
                                type="text"
                                value={displayName}
                                onChange={(event) => {
                                    setDisplayName(event.target.value);
                                }}
                            />
                        )}
                        <Input
                            name="Password"
                            type="password"
                            value={password}
                            onChange={(event) => {
                                setPassword(event.target.value);
                            }}
                        />
                        <div className="mt-2 flex items-center">
                            <label className="font-bold mr-2">
                                Keep me logged in?
                            </label>
                            <input className="w-5 h-5" type="checkbox" />
                        </div>
                        <button
                            type="submit"
                            className="absolute bg-green-600 hover:bg-green-400 bottom-0 right-0 p-1 text-xl rounded-xl m-1"
                        >
                            {doRegister ? 'Do Register' : 'Do Login'}
                        </button>
                    </form>
                </div>
            </div>
        </div>
    );
};

export default Login;

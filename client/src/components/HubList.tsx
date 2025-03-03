import { FaBeer } from 'react-icons/fa';

const Hub = (text: string) => {
    return (
        <div className="hub-icon group">
            <FaBeer size={32} />

            <span className="hub-tooltip group-hover:scale-100">{text}</span>
        </div>
    );
};

const HubList = () => {
    const hubs = [
        Hub('H'),
        Hub('WW'),
        Hub('LO'),
        Hub('?'),
        Hub('F'),
        Hub('Q'),
        Hub('Test')
    ];

    return (
        <div className="flex flex-col bg-gray-900">
            <div className="flex-1">{hubs}</div>
            <div className="bg-gray-900">{Hub('Create')}</div>
        </div>
    );
};

export default HubList;

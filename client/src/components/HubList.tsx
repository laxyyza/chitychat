import { FaBeer } from 'react-icons/fa';

const Hub = (text: string) => {
    return (
        <div className="test-icon group">
            <FaBeer size={32} />

            <span className="test-tooltip group-hover:scale-100">{text}</span>
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

    return <div className="server-list">{hubs}</div>;
};

export default HubList;

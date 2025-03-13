import { ReactNode } from 'react';

interface Prop {
    tooltip: string;
    onClick?: () => void;
    ref?: React.RefObject<any>;
    pfp: string | null;
    selected: boolean;
    name?: string;
    children?: ReactNode;
}

interface IconProp {
    name?: string;
    pfp: string | null;
}

const SideBarIcon = ({ name, pfp }: IconProp) => {
    if (pfp) {
        return <img className="custom-rounded-inherit" src={pfp} />;
    } else {
        if (!name) name = ' ';
        let letters: string = '';
        const words = name.split(' ');
        const className = words.length >= 3 ? 'text-xl' : 'text-3xl';

        for (let i = 0; i < Math.min(3, words.length); i++) {
            letters += words[i][0];
        }

        return <div className={className}>{letters}</div>;
    }
};

const SideBarButton = ({
    tooltip,
    onClick,
    ref,
    pfp,
    selected,
    name
}: Prop) => {
    return (
        <button ref={ref} className="hub-icon group" onClick={onClick}>
            <SideBarIcon name={name} pfp={pfp} />
            <span className="hub-tooltip group-hover:scale-100 pointer-events-none">
                {tooltip}
            </span>
            <span
                className={`hub-highlight ${
                    selected ? 'hub-highlight-selected' : ''
                }`}
            ></span>
        </button>
    );
};

export default SideBarButton;

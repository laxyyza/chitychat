import { ReactNode, useEffect, useRef, useState } from 'react';
import Popup from '../Popup';

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
    const [hovered, setHovered] = useState(false);
    const [scale, setScale] = useState(false);
    if (!ref) {
        ref = useRef<HTMLButtonElement | null>(null);
    }

    useEffect(() => {
        setScale(hovered);
    }, [hovered]);

    return (
        <button ref={ref} className="hub-icon group" onClick={onClick} onMouseEnter={() => setHovered(true)} onMouseLeave={() => setHovered(false)}>
            <SideBarIcon name={name} pfp={pfp} />
            {hovered && <Popup targetRef={ref}>
                <span className={`hub-tooltip pointer-events-none ${scale ? "scale-100" : ""}`}>
                    {tooltip}
                </span>
            </Popup>}
            <span
                className={`hub-highlight ${selected ? 'hub-highlight-selected' : ''} ${scale ? "scale-100" : ""}`}
            ></span>
        </button>
    );
};

export default SideBarButton;

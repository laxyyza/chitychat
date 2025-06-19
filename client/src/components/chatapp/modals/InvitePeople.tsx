import { useRef, useState } from "react";
import { Hub } from "../../../models/hub";
import Modal from "./Modal";
import { FaChevronDown, FaChevronUp } from "react-icons/fa6";
import fetchData, { baseUrl } from "../../../services/api";

interface Props {
    hub: Hub;
    onClose: () => void;
};

type InviteContentState = "Generate" | "Generating..." | "Copy" | "Copied" | "Error";

interface InviteContentProps {
    hub: Hub;
    maxUses: number;
}

const InviteContent = ({hub, maxUses}: InviteContentProps) => {
    const [inviteLink, setInviteLink] = useState('');
    const [state, setState] = useState<InviteContentState>("Generate");

    const generateLink = () => {
        setState("Generating...");
        fetchData(`/api/hubs/${hub.id}/invites`, 'POST', 
            {
                max_uses: maxUses,
            })
            .then((resp) => {
                setState('Copy');
                setInviteLink(baseUrl + "/i/" + resp.code);
            })
            .catch((err) => {
                setState('Error');
                setInviteLink(err);
            });
    };

    const getBorderColor = (): string => {
        switch (state) {
            case "Error":
                return "border-red-500";
            case "Generating...":
            case "Copied":
                return "border-green-500";
            case "Generate":
            case "Copy":
            default:
                return "border-gray-600";
        }
    }

    const getButtonColor = () => {
        switch (state) {
            case "Error":
                return "bg-red-500";
            case "Generating...":
                return "bg-green-600 animate-pulse";
            case "Generate":
            case "Copied":
                return "bg-green-600";
            case "Copy":
            default:
                return "bg-blue-500";
        }
    };

    return (
        <div className={"flex bg-gray-800 rounded-md border-1 p-1 mt-3 transition-all duration-500 animate-border-pulse " + getBorderColor()}>
            <div className="flex-1">
                {inviteLink}
            </div>
            <button
                onClick={() => {
                    if (state !== "Copy" && state !== "Generate") return;

                    if (state === "Generate") {
                        generateLink();
                    } else if (state === "Copy") {
                        navigator.clipboard.writeText(inviteLink);
                        setState("Copied");
                    }
                }}
                className={"font-bold rounded-md pr-2 pl-2 " + getButtonColor() }
            >
                {state}
            </button>
        </div>
    );
};

const InvitePeople = ({ hub, onClose }: Props) => {
    const [maxUses, setMaxUses] = useState(0);
    const [maxUsesPopup, setMaxUsesPopup] = useState(false);
    const ref = useRef<HTMLDivElement | null>(null);

    const uses = [0, 1, 5, 10, 50, 100, 1000];

    const maxUseString = (num: number) => {
        if (num === 0) {
            return "No Limit";
        } else if (num === 1) {
            return `${num} use`;
        } else {
            return `${num} uses`;
        }
    };

    const maxUseButtonContent = () => {
        if (maxUsesPopup) {
            return (
                <>
                    <span className="flex-1 text-gray-500">Select</span>
                    <FaChevronUp />
                </>
            );
        } else {
            return (
                <>
                    <span className="flex-1">{maxUseString(maxUses)}</span>
                    <FaChevronDown />
                </>
            );
        }
    };

    return (
        <Modal onClose={onClose}>
            <div ref={ref} className="bg-gray-700 rounded-xl border-1 border-black p-3 w-100" onClick={(e) => {
                e.stopPropagation();
            }}>
                <div className="relative">
                    <div className="font-bold mb-5">Invite friends to {hub.name}</div>
                    <p className="text-xs font-bold">
                        Max Number Of Uses
                    </p>
                    <button
                        className="flex bg-gray-800 w-full text-left p-2 rounded-md items-center"
                        onClick={() => {
                            setMaxUsesPopup(!maxUsesPopup);
                        }}>
                        {maxUseButtonContent()}
                    </button>

                    {maxUsesPopup && (
                        <ol className="bg-gray-800 border-1 border-black rounded-xl absolute w-full mt-1 overflow-auto">
                            {uses.map((maxUse) => (
                                <li key={maxUse}>
                                    <button
                                        className={"w-full text-left p-2 " + ((maxUse === maxUses) ? "bg-gray-600" : "hover:bg-gray-700")}
                                        onClick={() => {
                                            setMaxUses(maxUse);
                                            setMaxUsesPopup(false);
                                        }}
                                    >
                                        {maxUseString(maxUse)}
                                    </button>
                                </li>
                            ))}
                        </ol>
                    )}
                </div>

                <InviteContent hub={hub} maxUses={maxUses} />
            </div>
        </Modal>
    );
};

export default InvitePeople;

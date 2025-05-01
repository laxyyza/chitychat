import { createPortal } from "react-dom";
import useDismissTrigger from "../hooks/useDismissTrigger";

interface Props {
    children: React.ReactNode;
    onClose: () => void;
    ref: React.RefObject<HTMLDivElement | null>;
};

const Modal = ({ children, onClose, ref }: Props) => {
    useDismissTrigger(ref, onClose);

    return createPortal(
        <div ref={ref} className="absolute text-white left-0 top-0 flex justify-center items-center w-screen h-screen z-1001 backdrop-blur-xs" onClick={() => {
            onClose();
        }}>
            {children}
        </div>,
        document.body
    );
};

export default Modal;

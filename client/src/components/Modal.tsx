import useDismissTrigger from "../hooks/useDismissTrigger";

interface Props {
    children: React.ReactNode;
    onClose: () => void;
    ref: React.RefObject<HTMLDivElement | null>;
};

const Modal = ({children, onClose, ref}: Props) => {
    useDismissTrigger(ref, onClose);

    return (
        <div ref={ref} className="absolute left-0 top-0 flex justify-center items-center w-screen h-screen z-1001 backdrop-blur-xs" onClick={() => {
            onClose();
        }}>
            {children}
        </div>
    );
};

export default Modal;

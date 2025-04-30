import { useEffect } from "react";

const useDismissTrigger = (ref: React.RefObject<HTMLDivElement | null>, callback: () => void) => {
    useEffect(() => {
        const handleKeyEvent = (event: globalThis.KeyboardEvent) => {
            if (event.key == 'Escape') callback();
        };

        const handleMouseEvent = (e: globalThis.MouseEvent) => {
            if (!ref.current?.contains(e.target as Node)) {
                e.stopPropagation();
                e.preventDefault();
                callback();
            }
        };

        document.addEventListener('keydown', handleKeyEvent);
        document.addEventListener('mousedown', handleMouseEvent)

        return () => {
            document.removeEventListener('keydown', handleKeyEvent);
            document.removeEventListener('mousedown', handleMouseEvent);
        }
    }, []);
};

export default useDismissTrigger;

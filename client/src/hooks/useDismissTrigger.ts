import { useEffect } from "react";

const useDismissTrigger = (
    callback: () => void,
    refs: React.RefObject<HTMLElement | null>[], 
) => {
    useEffect(() => {
        const handleKeyEvent = (event: globalThis.KeyboardEvent) => {
            if (event.key == 'Escape') callback();
        };

        const handleMouseEvent = (e: globalThis.MouseEvent) => {
            var contains = false;
            refs.forEach((ref) => {
                if (ref.current?.contains(e.target as Node)) {
                    contains = true;
                }
            });
            if (!contains)
                callback();
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

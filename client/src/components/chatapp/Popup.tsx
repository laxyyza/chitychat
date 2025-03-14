import { useEffect, useRef, useState } from 'react';
import { createPortal } from 'react-dom';

interface PopupProps {
    targetRef: React.RefObject<HTMLElement | null>; // The element the popup should be next to
    children: React.ReactNode;
    className?: string;
    onClose?: () => void;
}

export default function Popup({
    targetRef,
    children,
    onClose,
    className = ''
}: PopupProps) {
    const popupRef = useRef<HTMLDivElement | null>(null);
    const [position, setPosition] = useState<{
        top: number | string | undefined;
        left: number | string | undefined;
    }>({
        top: targetRef.current?.style.top,
        left: targetRef.current?.style.left
    });

    console.log('position ', position);

    useEffect(() => {
        function updatePosition() {
            if (targetRef.current && popupRef.current) {
                const targetRect = targetRef.current.getBoundingClientRect();
                const popupRect = popupRef.current?.getBoundingClientRect();

                const rightOverflow = () => {
                    return (
                        targetRect.right + popupRect?.width > window.innerWidth
                    );
                };

                const bottomOverflow = () => {
                    return (
                        targetRect.bottom + popupRect?.height >
                        window.innerHeight
                    );
                };

                let newTop = targetRect.top;
                let newLeft = 0;

                if (rightOverflow())
                    newLeft = targetRect.left - popupRect.width;
                else newLeft = targetRect.right;

                if (bottomOverflow())
                    newTop = targetRect.top - popupRect.height;
                else newTop = targetRect.top;
                // popupRef.current.getBoundingClientRect().height, // Below the target
                // popupRef.current.getBoundingClientRect().width // Align left

                setPosition({
                    top: newTop,
                    left: newLeft
                });
            }
        }

        updatePosition(); // Set initial position

        // Reposition on scroll and resize
        window.addEventListener('resize', updatePosition);
        window.addEventListener('scroll', updatePosition);

        return () => {
            window.removeEventListener('resize', updatePosition);
            window.removeEventListener('scroll', updatePosition);
        };
    }, [targetRef]);

    // Close when clicking outside
    useEffect(() => {
        function handleClickOutside(event: MouseEvent) {
            if (
                popupRef.current &&
                !popupRef.current.contains(event.target as Node)
            ) {
                if (onClose) onClose();
            }
        }
        document.addEventListener('mouseup', handleClickOutside);

        const handleKeyEvent = (event: globalThis.KeyboardEvent) => {
            if (event.key == 'Escape') if (onClose) onClose();
        };

        document.addEventListener('keydown', handleKeyEvent);
        return () => {
            document.removeEventListener('mouseup', handleClickOutside);
            document.removeEventListener('keydown', handleKeyEvent);
        };
    }, []);

    return createPortal(
        <div
            onClick={(event) => {
                event.stopPropagation();
            }}
            ref={popupRef}
            style={{
                position: 'fixed',
                top: position.top,
                left: position.left,
                zIndex: 1000
            }}
            className={className}
        >
            {children}
        </div>,
        document.body
    );
}

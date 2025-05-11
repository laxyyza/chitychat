import { useEffect, useState } from "react";
import { FaFile, FaFileVideo } from "react-icons/fa6";
import { HiTrash } from "react-icons/hi2";

interface Props {
    file: File;
    onDelete: () => void;
}

const renderFile = (previewUrl: string | undefined, type: string) => {
    if (previewUrl) {
        return (
            <img
                className='w-full h-full rounded-md object-contain'
                src={previewUrl}
            />
        );
    } else {
        var icon: React.ReactElement
        if (type.startsWith("video/")) {
            icon = <FaFileVideo size="128" />;
        } else {
            icon = <FaFile size="128" />;
        }

        return (
            <div className="w-full h-full flex items-center justify-center">
                {icon}
            </div>
        );
    }
}

const Attachment = ({ file, onDelete }: Props) => {
    const [previewUrl, setPreviewUrl] = useState<string | undefined>(undefined);

    useEffect(() => {
        if (file.type.startsWith("image/")) {
            const reader = new FileReader();
            reader.onloadend = () => {
                setPreviewUrl(reader.result as string);
            };
            reader.readAsDataURL(file);
        }
    }, [file]);

    return (
        <div className='relative w-50 h-55 p-2 border-1 border-gray-600 rounded-xl m-1 text-center'>
            <div className='w-45 h-45'>
                {renderFile(previewUrl, file.type)}
            </div>
            <div className='text-gray-300 text-nowrap overflow-hidden text-ellipsis'>{file.name}</div>
            <button 
                className="absolute -top-2 -right-2 p-0.5 border-1 border-gray-600 m-1 hover:bg-gray-600 rounded-md bg-gray-700 text-red-600"
                onClick={onDelete}
            >
                <HiTrash size="24" />
            </button>
        </div>
    );
};

export default Attachment;

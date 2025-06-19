import { useCallback, useEffect, useRef, useState } from "react";
import Cropper, { Area } from "react-easy-crop";
import useDismissTrigger from "../../../hooks/useDismissTrigger";

interface Props {
    image: File;
    onClose: (finalImage?: string) => void;
}

function fileToDataURL(file: File): Promise<string> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();

        reader.onload = () => {
            resolve(reader.result as string);
        };

        reader.onerror = (error) => {
            reject(error);
        };

        reader.readAsDataURL(file);
    });
}

async function getCroppedImg(imageSrc: string, type: string, crop: Area): Promise<string> {
    const image = await createImage(imageSrc)
    const canvas: HTMLCanvasElement = document.createElement('canvas')
    const ctx = canvas.getContext('2d')

    canvas.width = crop.width
    canvas.height = crop.height

    ctx?.drawImage(
        image,
        crop.x,
        crop.y,
        crop.width,
        crop.height,
        0,
        0,
        crop.width,
        crop.height
    )

    return new Promise((resolve, reject) => {
        canvas.toBlob((blob) => {
            if (!blob) {
                reject("blob is null");
            } else {
                const reader = new FileReader()
                reader.onloadend = () => {
                    if (typeof reader.result === 'string') {
                        resolve(reader.result);
                    } else {
                        reject(new Error('Failed to convert blob to data URL'));
                    }
                }
                reader.onerror = reject;
                reader.readAsDataURL(blob);
            }
        }, type)
    })
}

function createImage(url: string): Promise<HTMLImageElement> {
    return new Promise((resolve, reject) => {
        const img = new Image()
        img.onload = () => resolve(img)
        img.onerror = reject
        img.src = url
    })
}

const CropImage = ({ image, onClose }: Props) => {
    const [imageUrl, setImageUrl] = useState('');
    const [crop, setCrop] = useState({ x: 0, y: 0 })
    const [zoom, setZoom] = useState(1)
    const [finalImage, setFinalImage] = useState<string | null>(null);
    const [croppedAreaPixels, setCroppedAreaPixels] = useState<Area>({ height: 0, width: 0, x: 0, y: 0 });
    const ref = useRef<HTMLDivElement | null>(null);

    useEffect(() => {
        fileToDataURL(image).then(val => setImageUrl(val));
    }, [image]);

    const onCropComplete = useCallback((_: Area, croppedPixels: Area) => {
        setCroppedAreaPixels(croppedPixels);
    }, []);

    useDismissTrigger(onClose, [ref]);

    if (!imageUrl) return null;

    const renderContent = () => {
        if (finalImage) {
            return (
                <img
                    className="w-full h-full rounded-md"
                    src={finalImage}
                />
            );
        } else {
            return (
                <Cropper
                    image={imageUrl}
                    crop={crop}
                    minZoom={1}
                    objectFit="contain"
                    style={{
                        containerStyle: { background: 'black', borderRadius: 6 },
                    }}
                    zoom={zoom}
                    aspect={1}
                    onCropComplete={onCropComplete}
                    onCropChange={setCrop}
                    onZoomChange={setZoom}
                />
            );
        }
    };

    return (
        <div className="fixed top-0 left-0 w-screen h-screen flex justify-center items-center">
            <div ref={ref} className="w-150 h-150 rounded-xl bg-gray-900 border-1 border-gray-700 flex flex-col">
                <div className="grow-1 flex items-center justify-center">
                    <div className="relative min-w-120 min-h-120 w-120 h-120">
                        {renderContent()}
                    </div>
                </div>
                <div className="shrink-0 flex justify-around m-2">
                    <button
                        className="p-2 rounded-xl bg-gray-800 w-20 hover:bg-gray-700"
                        onClick={() => {
                            if (finalImage) {
                                setFinalImage(null);
                            } else {
                                onClose();
                            }
                        }}
                    >
                        {finalImage ? 'Edit' : 'Cancel'}
                    </button>
                    <button
                        className="p-2 rounded-xl bg-indigo-800 hover:bg-indigo-600 w-20"
                        onClick={() => {
                            if (finalImage) {
                                onClose(finalImage);
                            } else {
                                getCroppedImg(imageUrl, image.type, croppedAreaPixels)
                                    .then((newImage: string) => setFinalImage(newImage));
                            }
                        }}
                    >
                        {finalImage ? 'Apply' : 'Crop'}
                    </button>
                </div>
            </div>
        </div>
    );
};

export default CropImage;

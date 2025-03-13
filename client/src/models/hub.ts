interface Category {
    id: number;
    name: string;
    channelIDs: number[];
}

class Hub {
    readonly id: number;
    readonly owner_id: number;
    name: string;
    pfp: string | null;
    categories: Category[];
    channelIDs: number[];
    memberIDs: number[];
    channelIDIndex: number;

    constructor(
        id: number,
        owner_id: number,
        name: string,
        pfp: string | null = null,
        channelIDs: number[] = []
    ) {
        this.id = id;
        this.owner_id = owner_id;
        this.name = name;
        this.pfp = pfp;
        this.categories = [
            { id: 1, name: 'Text Channels', channelIDs: channelIDs }
        ];

        this.memberIDs = [this.owner_id];
        this.channelIDs = channelIDs;
        this.channelIDIndex = 0;
    }
}

export {Hub};
export type { Category };

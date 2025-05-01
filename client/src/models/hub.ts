// Used for HTTP GET /api/hubs
interface HubBasicData {
    hub_id: number;
    owner_id: number;
    name: string;
    public: boolean;
    created_at: string;
    settings: any;
}

interface HubChannelData {
    channel_id: number;
    category_id: number;
    hub_id: number;
    name: string;
    position: number;
    created_at: string;
    settings: any;
}

interface CategoryData {
    category_id: number;
    name: string;
    position: number;
    channels: HubChannelData[];
}

interface HubDetailedData {
    hub_id: number;
    owner_id: number;
    name: string;
    public: boolean;
    created_at: string;
    settings: any;
    member_ids: number[];
    categories: CategoryData[];
}

interface Category {
    id: number;
    name: string;
    channelIDs: number[];
}

class Hub {
    readonly id: number;
    readonly ownerID: number;
    name: string;
    pfp: string | null;
    categories: Set<CategoryData>;
    memberIDs: Set<number>;
    createdAt: string;
    detailedLoaded: boolean;

    constructor(data: HubBasicData, categories?: CategoryData[], memberIDs?: number[]) {
        this.id = data.hub_id;
        this.ownerID = data.owner_id;
        this.name = data.name;
        this.pfp = null;
        this.categories = new Set(categories || []);
        this.memberIDs = new Set(memberIDs || []);
        this.createdAt = data.created_at;
        this.detailedLoaded = false;
    }

    static fromDetailedData(data: HubDetailedData): Hub {
        const hub = new Hub({...data}, data.categories, data.member_ids)
        hub.detailedLoaded = true;
        return hub;
    }
}

export {Hub};
export type { Category, HubBasicData, CategoryData, HubChannelData, HubDetailedData };

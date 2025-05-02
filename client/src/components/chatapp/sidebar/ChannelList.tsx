import { useEffect } from "react";
import { Action, appGetFocusHub, useApp } from "../AppProvider";
import HubCategory from "./HubCategory";
import fetchData from "../../../services/api";
import { HubDetailedData } from "../../../models/hub";

const ChannelList = () => {
    const { app, dispatch } = useApp();
    const hub = appGetFocusHub(app);
    const categories = Array.from(hub?.categories.values() || [])
        .sort((a, b) => a.position - b.position);

    useEffect(() => {
        if (hub?.detailedLoaded === false) {
            fetchData(`/api/hubs/${hub.id}`)
                .then((json) => {
                    const hubData: HubDetailedData = json;
                    dispatch({
                        type: Action.ADD_DETAILED_HUB,
                        payload: hubData,
                    });
                })
        }
    }, [hub]);

    return (
        <>
            {categories.map((category => (
                <li key={category.id}>
                    <HubCategory category={category}/>
                </li>
            )))}
        </>
    );
};

export default ChannelList;

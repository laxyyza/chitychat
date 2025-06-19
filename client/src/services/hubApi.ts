import { Dispatch } from "react";
import { Action, DispatchAction } from "../components/chatapp/AppProvider";
import { HubDetailedData } from "../models/hub";
import fetchData from "./api";

export const getHubDetails = (hubID: number, dispatch: Dispatch<DispatchAction>) => {
    fetchData(`/api/hubs/${hubID}`)
        .then((json) => {
            const hubData: HubDetailedData = json;
            dispatch({
                type: Action.ADD_DETAILED_HUB,
                payload: hubData,
            });
        })
};

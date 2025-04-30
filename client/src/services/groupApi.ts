import { Action, DispatchAction } from "../components/chatapp/AppProvider";
import { GroupProps } from "../models/group";
import fetchData from "./api";

export const fetchGroup = (groupID: number, dispatch: React.Dispatch<DispatchAction>) => {
    fetchData(`/api/groups/${groupID}`).then((resp) => {
        const group: GroupProps = resp.group;
        dispatch({
            type: Action.ADD_GROUPS,
            payload: [group]
        });
    })
}

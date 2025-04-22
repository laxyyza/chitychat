
export const isDev = process.env.NODE_ENV === 'development';

export const baseUrl = isDev ? "https://localhost:8080" : window.location.origin;

const fetchData = async (
    url: string, 
    method: string = 'GET', 
    body?: any, 
    headers: Record<string, string> = { 
        'Content-Type': 'application/json' 
    }
) => {
    const opts: RequestInit = {
        method,
        headers,
        credentials: "include",
    };

    if (method !== 'GET' && body) {
        opts.body = JSON.stringify(body);
    }

    const resp = await fetch(baseUrl + url, opts);

    if (!resp.ok) {
        const data = await resp.json();
        if (data.error)
            throw data.error;
        else 
            throw `${resp.status} ${resp.statusText}`;
    }

    const contentType = resp.headers.get('Content-Type');
    if (contentType === "application/json")
    {
        const json = await resp.json();
        return json;
    }
    else
        return resp.body;
};

export default fetchData;

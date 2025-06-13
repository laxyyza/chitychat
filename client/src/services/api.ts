export const isDev = process.env.NODE_ENV === 'development';

export const baseUrl = isDev ? "https://localhost:8080" : window.location.origin;

const doFetchData = async (
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

    return await fetch(baseUrl + url, opts);
}

const validJson = (text: string): Boolean => {
    try {
        JSON.parse(text);
        return true;
    } catch (e) {
        return false;
    }
};

const throwError = async (resp: Response) => {
    const text = await resp.text();
    if (validJson(text)) {
        const json = JSON.parse(text);
        if (json.error) {
            throw `${resp.status} ${resp.statusText}: ${json.error}`;
        }
    }

    throw `${resp.status} ${resp.statusText}`;
}

const fetchData = async (
    url: string,
    method: string = 'GET',
    body?: any,
    headers: Record<string, string> = {
        'Content-Type': 'application/json'
    }
) => {
    let resp = await doFetchData(url, method, body, headers);

    if (!resp.ok) {
        if (resp.status === 401 && url !== '/api/auth/remember') {
            // Most likely because session expired, try to get session again.
            const remember_resp = await doFetchData('/api/auth/remember');
            if (remember_resp.ok) {
                resp = await doFetchData(url, method, body, headers);
            } else {
                await throwError(resp);
            }
        } else {
            await throwError(resp);
        }
    }

    const contentType = resp.headers.get('Content-Type');
    if (contentType === "application/json") {
        const json = await resp.json();
        return json;
    }
    else
        return resp.body;
};

export default fetchData;

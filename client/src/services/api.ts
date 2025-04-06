
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
    try {
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
            throw new Error(`HTTP ${method} error: ${resp.status}`);
        }

        return await resp.json();
    } catch (error) {
        console.error('Fetch error:', error);
        return error;
    }
};

export default fetchData;

/**
 * e2e.test.js — Teste end-to-end Sera Inteligenta
 * Rulare: cd etapa10 && npm test
 */
const axios = require('axios');

const BASE = process.env.API_URL || 'http://localhost:3000';
let accessToken, refreshToken;

describe('Autentificare', () => {
    test('Login cu admin/Admin1234!', async () => {
        const r = await axios.post(`${BASE}/api/auth/login`, {username:'admin',password:'Admin1234!'});
        expect(r.status).toBe(200);
        expect(r.data.access_token).toBeTruthy();
        accessToken  = r.data.access_token;
        refreshToken = r.data.refresh_token;
    });
    test('Login gresit returneaza 401', async () => {
        await expect(axios.post(`${BASE}/api/auth/login`,{username:'admin',password:'gresit'}))
            .rejects.toMatchObject({response:{status:401}});
    });
    test('Refresh token functioneaza', async () => {
        const r = await axios.post(`${BASE}/api/auth/refresh`,{refresh_token:refreshToken});
        expect(r.status).toBe(200);
        expect(r.data.access_token).toBeTruthy();
        accessToken = r.data.access_token;
    });
});

describe('Dashboard', () => {
    test('GET /api/dashboard returneaza date', async () => {
        const r = await axios.get(`${BASE}/api/dashboard`,
            {headers:{Authorization:`Bearer ${accessToken}`}});
        expect(r.status).toBe(200);
        expect(r.data).toHaveProperty('device');
        expect(r.data).toHaveProperty('active_alerts');
    });
});

describe('Senzori', () => {
    test('GET /api/sensors/latest', async () => {
        const r = await axios.get(`${BASE}/api/sensors/latest`,
            {headers:{Authorization:`Bearer ${accessToken}`}});
        expect(r.status).toBe(200);
        expect(r.data).toHaveProperty('data');
    });
    test('GET /api/sensors/stats', async () => {
        const r = await axios.get(`${BASE}/api/sensors/stats?period=24h`,
            {headers:{Authorization:`Bearer ${accessToken}`}});
        expect(r.status).toBe(200);
    });
    test('GET /api/sensors/history cu interval 1h', async () => {
        const r = await axios.get(`${BASE}/api/sensors/history?interval=1h`,
            {headers:{Authorization:`Bearer ${accessToken}`}});
        expect(r.status).toBe(200);
        expect(r.data).toHaveProperty('view_used');
    });
});

describe('Alerte', () => {
    test('GET /api/alerts/rules returneaza reguli default', async () => {
        const r = await axios.get(`${BASE}/api/alerts/rules`,
            {headers:{Authorization:`Bearer ${accessToken}`}});
        expect(r.status).toBe(200);
        expect(r.data.data.length).toBeGreaterThan(0);
    });
});

describe('Securitate', () => {
    test('Fara token returneaza 401', async () => {
        await expect(axios.get(`${BASE}/api/dashboard`))
            .rejects.toMatchObject({response:{status:401}});
    });
    test('Token fals returneaza 401', async () => {
        await expect(axios.get(`${BASE}/api/dashboard`,
            {headers:{Authorization:'Bearer token_fals_12345'}}))
            .rejects.toMatchObject({response:{status:401}});
    });
    test('Health check public', async () => {
        const r = await axios.get(`${BASE}/health`);
        expect(r.status).toBe(200);
        expect(r.data.status).toBe('ok');
    });
});

/**
 * api.js — Client HTTP centralizat pentru aplicația Sera
 * ─────────────────────────────────────────────────────────────────────────────
 * Gestioneaza: Bearer token pe fiecare request, refresh automat la 401,
 * persistarea sesiunii in SecureStore (criptat de OS).
 * Token access (15 min) in memorie; refresh (30 zile) in SecureStore.
 */
import axios from 'axios';
import * as SecureStore from 'expo-secure-store';

const BASE_URL = 'https://app.sera.md/api';   // serverul de productie

let accessToken = null;
let refreshToken = null;
const client = axios.create({ baseURL: BASE_URL, timeout: 15000 });

client.interceptors.request.use((config) => {
  if (accessToken) config.headers.Authorization = `Bearer ${accessToken}`;
  return config;
});

let refreshing = null;
client.interceptors.response.use(
  (res) => res,
  async (error) => {
    const original = error.config;
    if (error.response?.status === 401 && !original._retry && refreshToken) {
      original._retry = true;
      try {
        refreshing = refreshing || axios.post(`${BASE_URL}/auth/refresh`,
          { refresh_token: refreshToken });
        const { data } = await refreshing;
        refreshing = null;
        await setSession(data.access_token, data.refresh_token);
        original.headers.Authorization = `Bearer ${data.access_token}`;
        return client(original);
      } catch (e) {
        refreshing = null;
        await clearSession();
        throw e;
      }
    }
    return Promise.reject(error);
  }
);

export async function setSession(access, refresh) {
  accessToken = access; refreshToken = refresh;
  await SecureStore.setItemAsync('refresh_token', refresh);
}
export async function restoreSession() {
  refreshToken = await SecureStore.getItemAsync('refresh_token');
  if (!refreshToken) return false;
  const { data } = await axios.post(`${BASE_URL}/auth/refresh`,
    { refresh_token: refreshToken });
  await setSession(data.access_token, data.refresh_token);
  return true;
}
export async function clearSession() {
  accessToken = refreshToken = null;
  await SecureStore.deleteItemAsync('refresh_token');
}

export const api = {
  login:       (u, p)       => client.post('/auth/login', { username: u, password: p }),
  dashboard:   (device)     => client.get('/dashboard', { params: { device } }),
  devices:     ()           => client.get('/devices'),
  history:     (device, iv) => client.get('/sensors/history', { params: { device, interval: iv } }),
  command:     (body)       => client.post('/actuators/command', body),
  schedules:   ()           => client.get('/schedules'),
  addSchedule: (body)       => client.post('/schedules', body),
  triggerPump: (device, s)  => client.post('/schedules/trigger', { device_id: device, duration_s: s }),
  alertRules:  ()           => client.get('/alerts/rules'),
  alertHistory:()           => client.get('/alerts/history'),
  claim:       (d, c, name) => client.post('/claim', { device_id: d, claim_code: c, name }),
  registerPush:(token)      => client.post('/devices/push-token', { token }),
};
export default client;

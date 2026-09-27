/**
 * push.js — Notificari push pentru alerte (Expo Push / FCM)
 * ─────────────────────────────────────────────────────────────────────────────
 * La pornirea aplicatiei: cere permisiune, obtine Expo push token, il trimite
 * la backend (/devices/push-token). Backend-ul il foloseste in alert engine
 * pentru a notifica utilizatorul cand o regula se declanseaza, chiar daca
 * aplicatia e inchisa.
 */
import * as Notifications from 'expo-notifications';
import * as Device from 'expo-device';
import { api } from './api';

// Afiseaza notificarea si cand aplicatia e in prim-plan
Notifications.setNotificationHandler({
  handleNotification: async () => ({
    shouldShowAlert: true, shouldPlaySound: true, shouldSetBadge: true,
  }),
});

export async function registerForPush() {
  if (!Device.isDevice) return null;   // emulatoarele nu primesc push

  const { status: existing } = await Notifications.getPermissionsAsync();
  let status = existing;
  if (existing !== 'granted') {
    status = (await Notifications.requestPermissionsAsync()).status;
  }
  if (status !== 'granted') return null;

  // Android: canal pentru alerte critice (sunet + vibratie)
  if (Device.osName === 'Android') {
    await Notifications.setNotificationChannelAsync('alerts', {
      name: 'Alerte sera',
      importance: Notifications.AndroidImportance.HIGH,
      vibrationPattern: [0, 250, 250, 250],
    });
  }

  const token = (await Notifications.getExpoPushTokenAsync()).data;
  try { await api.registerPush(token); } catch (e) { console.warn('push reg', e.message); }
  return token;
}

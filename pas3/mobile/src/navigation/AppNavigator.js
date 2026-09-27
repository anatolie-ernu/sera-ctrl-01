/**
 * AppNavigator.js — Fluxul de ecrane si restaurarea sesiunii la pornire
 * ─────────────────────────────────────────────────────────────────────────────
 * La pornire incearca restoreSession(): daca reuseste → Dashboard, altfel → Login.
 * Dupa login, daca utilizatorul nu are nicio sera → ClaimScreen (onboarding).
 */
import React, { useState, useEffect } from 'react';
import { View, ActivityIndicator } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { restoreSession, api } from '../services/api';
import { registerForPush } from '../services/push';

import LoginScreen     from '../screens/LoginScreen';
import ClaimScreen     from '../screens/ClaimScreen';
import DashboardScreen from '../screens/DashboardScreen';

const Stack = createNativeStackNavigator();

export default function AppNavigator() {
  const [ready, setReady] = useState(false);
  const [initial, setInitial] = useState('Login');

  useEffect(() => {
    (async () => {
      try {
        if (await restoreSession()) {
          await registerForPush();
          // Decide onboarding vs dashboard pe baza numarului de dispozitive
          const r = await api.devices();
          setInitial(r.data.data?.length ? 'Dashboard' : 'Claim');
        }
      } catch (e) { /* sesiune expirata → ramane Login */ }
      setReady(true);
    })();
  }, []);

  if (!ready) return (
    <View style={{ flex: 1, justifyContent: 'center' }}>
      <ActivityIndicator size="large" color="#1f6e3a" />
    </View>
  );

  return (
    <NavigationContainer>
      <Stack.Navigator initialRouteName={initial}
        screenOptions={{ headerStyle: { backgroundColor: '#1f6e3a' }, headerTintColor: '#fff' }}>
        <Stack.Screen name="Login"     component={LoginScreen}     options={{ headerShown: false }} />
        <Stack.Screen name="Claim"     component={ClaimScreen}     options={{ title: 'Adauga o sera' }} />
        <Stack.Screen name="Dashboard" component={DashboardScreen} options={{ title: 'Sera mea' }} />
      </Stack.Navigator>
    </NavigationContainer>
  );
}

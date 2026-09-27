/**
 * LoginScreen.js — Autentificare. La succes salveaza sesiunea si merge mai departe.
 */
import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { api, setSession } from '../services/api';
import { registerForPush } from '../services/push';

export default function LoginScreen({ navigation }) {
  const [u, setU] = useState('');
  const [p, setP] = useState('');
  const [busy, setBusy] = useState(false);

  const login = async () => {
    setBusy(true);
    try {
      const { data } = await api.login(u, p);
      await setSession(data.access_token, data.refresh_token);
      await registerForPush();
      const dv = await api.devices();
      navigation.replace(dv.data.data?.length ? 'Dashboard' : 'Claim');
    } catch (e) {
      Alert.alert('Autentificare esuata', e.response?.data?.error || e.message);
    } finally { setBusy(false); }
  };

  return (
    <View style={st.c}>
      <Text style={st.logo}>🌿 Sera</Text>
      <TextInput style={st.in} placeholder="Utilizator sau email" autoCapitalize="none"
        value={u} onChangeText={setU} />
      <TextInput style={st.in} placeholder="Parola" secureTextEntry
        value={p} onChangeText={setP} />
      <TouchableOpacity style={st.btn} onPress={login} disabled={busy}>
        <Text style={st.btnT}>{busy ? 'Se conecteaza...' : 'Autentificare'}</Text>
      </TouchableOpacity>
    </View>
  );
}
const st = StyleSheet.create({
  c: { flex: 1, justifyContent: 'center', padding: 28, backgroundColor: '#f4f7f4' },
  logo: { fontSize: 40, fontWeight: '800', color: '#1f6e3a', textAlign: 'center', marginBottom: 32 },
  in: { backgroundColor: '#fff', borderRadius: 10, padding: 14, fontSize: 16, marginBottom: 12, borderWidth: 1, borderColor: '#ddd' },
  btn: { backgroundColor: '#1f6e3a', padding: 16, borderRadius: 10, marginTop: 8 },
  btnT: { color: '#fff', textAlign: 'center', fontSize: 16, fontWeight: '700' },
});

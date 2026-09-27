/**
 * DashboardScreen.js — Ecranul principal: stare live + control manual
 * ─────────────────────────────────────────────────────────────────────────────
 * Afiseaza ultima citire (temp/umid/sol), starea actuatoarelor cu butoane
 * de comutare, si statistici 24h. Reimprospatare la 30s (pull-to-refresh manual).
 */
import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, ScrollView, RefreshControl,
         TouchableOpacity, Alert } from 'react-native';
import { api } from '../services/api';

export default function DashboardScreen({ route }) {
  const device = route.params?.device || 'SERA_001';
  const [data, setData] = useState(null);
  const [refreshing, setRefreshing] = useState(false);

  const load = useCallback(async () => {
    try { const r = await api.dashboard(device); setData(r.data); }
    catch (e) { console.warn('dashboard', e.message); }
  }, [device]);

  // Reimprospatare automata la 30s
  useEffect(() => {
    load();
    const t = setInterval(load, 30000);
    return () => clearInterval(t);
  }, [load]);

  const onRefresh = async () => { setRefreshing(true); await load(); setRefreshing(false); };

  // Trimite o comanda catre dispozitiv si reimprospateaza
  const sendCmd = async (type, action, params = {}) => {
    try {
      await api.command({ device_id: device, type, action, params });
      Alert.alert('Comanda trimisa', `${type}: ${action}`);
      setTimeout(load, 1500);   // lasa ESP32 sa execute + raporteze heartbeat
    } catch (e) {
      Alert.alert('Eroare', e.response?.data?.error || e.message);
    }
  };

  if (!data) return <View style={s.center}><Text>Se incarca...</Text></View>;

  const latest = data.latest || {};
  const online = data.device?.online;
  const st = data.latest?.state || {};

  return (
    <ScrollView style={s.bg}
      refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}>
      {/* Status conectare */}
      <View style={[s.badge, { backgroundColor: online ? '#1f6e3a' : '#c0392b' }]}>
        <Text style={s.badgeText}>{online ? '● Online' : '○ Offline'}  ·  {data.device?.name || device}</Text>
      </View>

      {/* Carduri senzori */}
      <View style={s.row}>
        <Metric label="Temperatura" value={latest.temperature} unit="°C" />
        <Metric label="Umiditate"   value={latest.humidity}    unit="%" />
        <Metric label="Sol"         value={latest.soil_pct}    unit="%" />
      </View>

      {/* Statistici 24h */}
      {data.stats_24h && (
        <View style={s.card}>
          <Text style={s.cardTitle}>Ultimele 24h</Text>
          <Text style={s.statLine}>Temp: min {data.stats_24h.tmin}° · med {data.stats_24h.tavg}° · max {data.stats_24h.tmax}°</Text>
          <Text style={s.statLine}>Citiri: {data.stats_24h.readings}</Text>
        </View>
      )}

      {/* Control manual */}
      <Text style={s.section}>Control manual</Text>
      <Control label="Ferestre" on={st.windows}
        onPress={() => sendCmd('windows', st.windows ? 'CLOSE' : 'OPEN')} />
      <Control label="Ventilatoare" on={st.fan1}
        onPress={() => sendCmd('fan', st.fan1 ? 'OFF' : 'ON', { fan: 0 })} />
      <Control label="Irigare 60s" on={st.pump}
        onPress={() => api.triggerPump(device, 60).then(() => setTimeout(load, 1500))} />

      {/* Alerte active */}
      {data.active_alerts?.length > 0 && (
        <View style={[s.card, { borderColor: '#c0392b', borderWidth: 1 }]}>
          <Text style={[s.cardTitle, { color: '#c0392b' }]}>⚠ Alerte active</Text>
          {data.active_alerts.map((a, i) => <Text key={i} style={s.statLine}>{a.message}</Text>)}
        </View>
      )}
    </ScrollView>
  );
}

const Metric = ({ label, value, unit }) => (
  <View style={s.metric}>
    <Text style={s.metricVal}>{value != null ? Number(value).toFixed(1) : '—'}</Text>
    <Text style={s.metricUnit}>{unit}</Text>
    <Text style={s.metricLbl}>{label}</Text>
  </View>
);
const Control = ({ label, on, onPress }) => (
  <TouchableOpacity style={[s.ctrl, { backgroundColor: on ? '#1f6e3a' : '#e8e8e8' }]} onPress={onPress}>
    <Text style={[s.ctrlText, { color: on ? '#fff' : '#333' }]}>{label}</Text>
    <Text style={[s.ctrlState, { color: on ? '#cde' : '#999' }]}>{on ? 'PORNIT' : 'OPRIT'}</Text>
  </TouchableOpacity>
);

const s = StyleSheet.create({
  bg: { flex: 1, backgroundColor: '#f4f7f4', padding: 14 },
  center: { flex: 1, justifyContent: 'center', alignItems: 'center' },
  badge: { padding: 10, borderRadius: 8, marginBottom: 12 },
  badgeText: { color: '#fff', fontWeight: '600', textAlign: 'center' },
  row: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  metric: { flex: 1, backgroundColor: '#fff', borderRadius: 12, padding: 14, marginHorizontal: 4, alignItems: 'center' },
  metricVal: { fontSize: 28, fontWeight: '700', color: '#1f6e3a' },
  metricUnit: { fontSize: 13, color: '#888', marginTop: -4 },
  metricLbl: { fontSize: 12, color: '#666', marginTop: 6 },
  card: { backgroundColor: '#fff', borderRadius: 12, padding: 14, marginBottom: 12 },
  cardTitle: { fontWeight: '700', marginBottom: 6, color: '#333' },
  statLine: { color: '#555', fontSize: 13, marginVertical: 2 },
  section: { fontWeight: '700', fontSize: 16, marginVertical: 10, color: '#333' },
  ctrl: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
          padding: 16, borderRadius: 12, marginBottom: 8 },
  ctrlText: { fontSize: 16, fontWeight: '600' },
  ctrlState: { fontSize: 13, fontWeight: '600' },
});

/**
 * ClaimScreen.js — Inrolarea unei sere noi prin scanarea QR-ului de pe eticheta
 * ─────────────────────────────────────────────────────────────────────────────
 * Pasi ghidati: 1) scaneaza QR → device_id + claim_code; 2) /api/claim →
 * dispozitivul intra in cont; 3) ghidare conectare la AP "SERA-XXXXXX" pt WiFi.
 * QR payload: https://app.sera.md/claim?d=SERA-A1B2C3&c=A1B2C3D4
 */
import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert, Linking } from 'react-native';
import { CameraView, useCameraPermissions } from 'expo-camera';
import { api } from '../services/api';

export default function ClaimScreen({ navigation }) {
  const [permission, requestPermission] = useCameraPermissions();
  const [scanned, setScanned] = useState(false);
  const [busy, setBusy] = useState(false);

  if (!permission?.granted) {
    return (
      <View style={styles.center}>
        <Text style={styles.info}>Avem nevoie de camera pentru a scana codul QR de pe sera dvs.</Text>
        <TouchableOpacity style={styles.btn} onPress={requestPermission}>
          <Text style={styles.btnText}>Permite accesul la camera</Text>
        </TouchableOpacity>
      </View>
    );
  }

  const onScan = async ({ data }) => {
    if (scanned || busy) return;
    setScanned(true);
    try {
      const url = new URL(data);
      const deviceId  = url.searchParams.get('d');
      const claimCode = url.searchParams.get('c');
      if (!deviceId || !claimCode) throw new Error('Cod QR nerecunoscut');
      setBusy(true);
      await api.claim(deviceId, claimCode, 'Sera mea');
      setBusy(false);
      Alert.alert('Sera adaugata!',
        `Dispozitivul ${deviceId} este acum in contul dvs.\n\nPasul urmator: conectati-l la WiFi.`,
        [{ text: 'Configureaza WiFi', onPress: showWifiGuide },
         { text: 'Mai tarziu', onPress: () => navigation.replace('Dashboard') }]);
    } catch (e) {
      setBusy(false);
      Alert.alert('Eroare', e.response?.data?.error || e.message,
        [{ text: 'Reincearca', onPress: () => setScanned(false) }]);
    }
  };

  const showWifiGuide = () => {
    Alert.alert('Configurare WiFi',
      '1. Deschideti Setari WiFi\n2. Conectati-va la reteaua "SERA-..."\n' +
      '3. Se deschide automat pagina de configurare\n4. Introduceti WiFi-ul de acasa si salvati',
      [{ text: 'Deschide Setari', onPress: () => Linking.openSettings() },
       { text: 'Gata', onPress: () => navigation.replace('Dashboard') }]);
  };

  return (
    <View style={styles.container}>
      <CameraView style={StyleSheet.absoluteFill}
        barcodeScannerSettings={{ barcodeTypes: ['qr'] }}
        onBarcodeScanned={scanned ? undefined : onScan} />
      <View style={styles.overlay}>
        <View style={styles.frame} />
        <Text style={styles.hint}>{busy ? 'Se inroleaza...' : 'Scanati codul QR de pe eticheta serei'}</Text>
      </View>
    </View>
  );
}
const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#000' },
  center: { flex: 1, justifyContent: 'center', alignItems: 'center', padding: 32 },
  overlay: { ...StyleSheet.absoluteFillObject, justifyContent: 'center', alignItems: 'center' },
  frame: { width: 240, height: 240, borderWidth: 3, borderColor: '#1f6e3a', borderRadius: 16 },
  hint: { color: '#fff', fontSize: 16, marginTop: 24, textAlign: 'center', paddingHorizontal: 24 },
  info: { fontSize: 16, textAlign: 'center', marginBottom: 24, color: '#333' },
  btn: { backgroundColor: '#1f6e3a', paddingVertical: 14, paddingHorizontal: 28, borderRadius: 10 },
  btnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
});

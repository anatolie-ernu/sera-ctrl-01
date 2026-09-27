# Aplicație Mobilă — Sera Inteligentă

React Native (Expo) — un singur cod pentru Android și iOS.

## Pornire
```bash
npm install
npx expo start
```

## Build producție
```bash
eas build -p android --profile production
eas build -p ios --profile production
```

## Flux utilizator
1. Login → autentificare JWT
2. Claim QR → scanare etichetă dispozitiv nou
3. Dashboard → date live, control manual, alerte

#!/usr/bin/env bash
# security_harden.sh — Hardening server Linux pentru productie
set -euo pipefail
GREEN='\033[0;32m';CYAN='\033[0;36m';NC='\033[0m'
[[ $EUID -ne 0 ]] && echo "Rulati cu sudo" && exit 1
section() { echo -e "${CYAN}══════ $* ══════${NC}"; }
log()     { echo -e "${GREEN}[✔] $*${NC}"; }

section "SSH Hardening"
SSH_CONF=/etc/ssh/sshd_config
cp "$SSH_CONF" "${SSH_CONF}.bak"
sed -i 's/^#*PermitRootLogin.*/PermitRootLogin no/'    "$SSH_CONF"
sed -i 's/^#*PasswordAuthentication.*/PasswordAuthentication no/' "$SSH_CONF"
sed -i 's/^#*X11Forwarding.*/X11Forwarding no/'        "$SSH_CONF"
sed -i 's/^#*MaxAuthTries.*/MaxAuthTries 3/'            "$SSH_CONF"
systemctl restart sshd
log "SSH hardening aplicat"

section "Fail2ban pentru SSH + Docker"
cat > /etc/fail2ban/jail.local << 'F2B'
[DEFAULT]
bantime  = 3600
findtime = 600
maxretry = 5

[sshd]
enabled = true
port    = ssh
filter  = sshd
logpath = /var/log/auth.log
maxretry = 3
bantime  = 86400
F2B
systemctl restart fail2ban
log "Fail2ban configurat"

section "Limite sistem"
cat >> /etc/security/limits.conf << 'LIM'
* soft nofile 65536
* hard nofile 65536
LIM
log "Limite aplicate"

section "Backup automat cron"
cat > /etc/cron.d/greenhouse-backup << 'CRON'
# Backup TimescaleDB zilnic la 03:00
0 3 * * * root /opt/greenhouse/scripts/backup.sh >> /var/log/greenhouse-backup.log 2>&1
# Curatare loguri vechi (>30 zile) saptamanal
0 4 * * 0 root find /opt/greenhouse/etapa3/backend/logs -name "*.log" -mtime +30 -delete
CRON
chmod 644 /etc/cron.d/greenhouse-backup
log "Cron backup configurat"

section "Docker security"
cat > /etc/docker/daemon.json << 'DOCK'
{
  "log-driver": "json-file",
  "log-opts": {"max-size": "10m", "max-file": "5"},
  "live-restore": true,
  "userland-proxy": false,
  "no-new-privileges": true
}
DOCK
systemctl restart docker
log "Docker security options aplicate"

section "GATA — Server hardened!"

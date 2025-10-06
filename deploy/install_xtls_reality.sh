#!/bin/bash

# XTLS-Reality-Vision Quantum VPN Deployment Script
# Supports: Ubuntu 20.04+, Debian 11+, CentOS 8+

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuration
INSTALL_DIR="/opt/xtls-reality"
CONFIG_DIR="/etc/xtls-reality"
LOG_DIR="/var/log/xtls-reality"
SYSTEMD_DIR="/etc/systemd/system"

# Server type
SERVER_TYPE="${1:-entry}" # entry or exit

echo -e "${GREEN}=== XTLS-Reality-Vision Quantum VPN Installation ===${NC}"

# Check OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VERSION=$VERSION_ID
else
    echo -e "${RED}Cannot detect OS${NC}"
    exit 1
fi

# Check root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Please run as root${NC}"
    exit 1
fi

# Install dependencies
echo -e "${YELLOW}Installing dependencies...${NC}"

case $OS in
    ubuntu|debian)
        apt-get update
        apt-get install -y \
            build-essential \
            cmake \
            git \
            libsodium-dev \
            libssl-dev \
            libevent-dev \
            pkg-config \
            python3-pip \
            curl \
            wget \
            htop \
            iftop \
            vnstat \
            net-tools
        ;;
    centos|rhel|fedora)
        yum install -y epel-release
        yum groupinstall -y "Development Tools"
        yum install -y \
            cmake3 \
            git \
            libsodium-devel \
            openssl-devel \
            libevent-devel \
            python3-pip \
            curl \
            wget \
            htop \
            iftop \
            vnstat \
            net-tools
        ;;
    *)
        echo -e "${RED}Unsupported OS: $OS${NC}"
        exit 1
        ;;
esac

# Create directories
echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p "$INSTALL_DIR" "$CONFIG_DIR" "$LOG_DIR"

# Clone repository
echo -e "${YELLOW}Cloning repository...${NC}"
cd /tmp
if [ -d "xtls-reality-quantum" ]; then
    rm -rf xtls-reality-quantum
fi
git clone https://github.com/yourusername/xtls-reality-quantum.git || {
    # If repo doesn't exist, create from local files
    mkdir -p xtls-reality-quantum
    cp -r /root/xtls-reality/* xtls-reality-quantum/ 2>/dev/null || true
}

# Build
echo -e "${YELLOW}Building XTLS-Reality server...${NC}"
cd xtls-reality-quantum
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Install
echo -e "${YELLOW}Installing binaries...${NC}"
cp xtls-reality-server "$INSTALL_DIR/"
cp -r ../configs/* "$CONFIG_DIR/"
cp -r ../scripts/* "$INSTALL_DIR/"

# Generate keys
echo -e "${YELLOW}Generating cryptographic keys...${NC}"
cd "$INSTALL_DIR"

# Generate X25519 key pair
openssl genpkey -algorithm X25519 -out private.pem
openssl pkey -in private.pem -pubout -out public.pem

# Convert to base64
PRIVATE_KEY=$(openssl pkey -in private.pem -text | grep -A 3 "priv:" | tail -3 | tr -d ' \n:' | xxd -r -p | base64)
PUBLIC_KEY=$(openssl pkey -in public.pem -pubin -text | grep -A 3 "pub:" | tail -3 | tr -d ' \n:' | xxd -r -p | base64)

# Generate short ID
SHORT_ID=$(openssl rand -hex 4)

# Configure based on server type
echo -e "${YELLOW}Configuring $SERVER_TYPE server...${NC}"

if [ "$SERVER_TYPE" == "entry" ]; then
    # Entry server configuration (Russia)
    cat > "$CONFIG_DIR/config.yaml" <<EOF
server:
  type: entry
  listen: 0.0.0.0:443
  protocol: tcp

reality:
  server_name: www.microsoft.com
  private_key: "$PRIVATE_KEY"
  public_key: "$PUBLIC_KEY"
  short_id: "$SHORT_ID"
  
quantum:
  enabled: true
  strength: 256
  qber_threshold: 0.11
  
adaptive:
  enabled: true
  target_profile: https
  russian_whitelist: true
  services:
    - yandex.ru
    - mail.ru
    - vk.com
    - sberbank.ru
    - gosuslugi.ru
    
vision:
  enabled: true
  threshold: 1024
  
chain:
  next_hop: "EXIT_SERVER_IP:443"
  
monitoring:
  enabled: true
  port: 9090
EOF

    # Russian IP whitelist updater
    cat > "$INSTALL_DIR/update_whitelist.sh" <<'EOF'
#!/bin/bash
# Update Russian IP whitelist

WHITELIST_FILE="/etc/xtls-reality/russian_ips.txt"

# Download fresh Russian IP ranges
curl -s https://www.ipdeny.com/ipblocks/data/countries/ru.zone > "$WHITELIST_FILE.tmp"

# Add known Russian service IPs
for domain in yandex.ru mail.ru vk.com sberbank.ru gosuslugi.ru; do
    dig +short $domain >> "$WHITELIST_FILE.tmp"
done

# Sort and deduplicate
sort -u "$WHITELIST_FILE.tmp" > "$WHITELIST_FILE"
rm "$WHITELIST_FILE.tmp"

# Reload server
systemctl reload xtls-reality
EOF
    chmod +x "$INSTALL_DIR/update_whitelist.sh"
    
    # Set up cron for whitelist updates
    echo "0 */6 * * * $INSTALL_DIR/update_whitelist.sh" | crontab -

elif [ "$SERVER_TYPE" == "exit" ]; then
    # Exit server configuration (Netherlands)
    cat > "$CONFIG_DIR/config.yaml" <<EOF
server:
  type: exit
  listen: 0.0.0.0:443
  protocol: tcp

reality:
  server_name: www.cloudflare.com
  private_key: "$PRIVATE_KEY"
  public_key: "$PUBLIC_KEY"
  short_id: "$SHORT_ID"
  
quantum:
  enabled: true
  strength: 256
  
vision:
  enabled: true
  threshold: 1024
  
outbound:
  interface: eth0
  mark: 0xFF
  
monitoring:
  enabled: true
  port: 9090
EOF
fi

# Create systemd service
echo -e "${YELLOW}Creating systemd service...${NC}"
cat > "$SYSTEMD_DIR/xtls-reality.service" <<EOF
[Unit]
Description=XTLS-Reality-Vision Quantum VPN Server
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$INSTALL_DIR
ExecStart=$INSTALL_DIR/xtls-reality-server -c $CONFIG_DIR/config.yaml
Restart=always
RestartSec=10
StandardOutput=append:$LOG_DIR/server.log
StandardError=append:$LOG_DIR/error.log

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=$LOG_DIR

[Install]
WantedBy=multi-user.target
EOF

# Configure firewall
echo -e "${YELLOW}Configuring firewall...${NC}"

# Detect firewall
if command -v ufw &> /dev/null; then
    ufw allow 443/tcp
    ufw allow 9090/tcp
    ufw --force enable
elif command -v firewall-cmd &> /dev/null; then
    firewall-cmd --permanent --add-port=443/tcp
    firewall-cmd --permanent --add-port=9090/tcp
    firewall-cmd --reload
elif command -v iptables &> /dev/null; then
    iptables -A INPUT -p tcp --dport 443 -j ACCEPT
    iptables -A INPUT -p tcp --dport 9090 -j ACCEPT
    iptables-save > /etc/iptables/rules.v4 2>/dev/null || true
fi

# Optimize network settings
echo -e "${YELLOW}Optimizing network settings...${NC}"
cat >> /etc/sysctl.conf <<EOF

# XTLS-Reality Optimizations
net.ipv4.tcp_fastopen = 3
net.ipv4.tcp_congestion_control = bbr
net.core.default_qdisc = fq
net.ipv4.tcp_slow_start_after_idle = 0
net.ipv4.tcp_mtu_probing = 1
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.ip_forward = 1
EOF

sysctl -p

# Enable BBR
if ! lsmod | grep -q bbr; then
    modprobe tcp_bbr
    echo "tcp_bbr" >> /etc/modules-load.d/modules.conf
fi

# Install monitoring stack
echo -e "${YELLOW}Installing monitoring...${NC}"

# Prometheus node exporter
wget -q https://github.com/prometheus/node_exporter/releases/latest/download/node_exporter-1.6.1.linux-amd64.tar.gz
tar xzf node_exporter-*.tar.gz
cp node_exporter-*/node_exporter /usr/local/bin/
rm -rf node_exporter-*

cat > "$SYSTEMD_DIR/node_exporter.service" <<EOF
[Unit]
Description=Prometheus Node Exporter
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/node_exporter
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# Start services
echo -e "${YELLOW}Starting services...${NC}"
systemctl daemon-reload
systemctl enable xtls-reality
systemctl enable node_exporter
systemctl start xtls-reality
systemctl start node_exporter

# Show configuration
echo -e "${GREEN}=== Installation Complete ===${NC}"
echo
echo -e "${YELLOW}Server Type:${NC} $SERVER_TYPE"
echo -e "${YELLOW}Public Key:${NC} $PUBLIC_KEY"
echo -e "${YELLOW}Short ID:${NC} $SHORT_ID"
echo
echo -e "${YELLOW}Config:${NC} $CONFIG_DIR/config.yaml"
echo -e "${YELLOW}Logs:${NC} $LOG_DIR/"
echo
echo -e "${GREEN}Service Status:${NC}"
systemctl status xtls-reality --no-pager

# Generate client config
echo
echo -e "${YELLOW}Client Configuration:${NC}"
cat > "$CONFIG_DIR/client.json" <<EOF
{
  "server": "$(curl -s ifconfig.me)",
  "port": 443,
  "publicKey": "$PUBLIC_KEY",
  "shortId": "$SHORT_ID",
  "serverName": "$([ "$SERVER_TYPE" == "entry" ] && echo "www.microsoft.com" || echo "www.cloudflare.com")"
}
EOF

cat "$CONFIG_DIR/client.json"

echo
echo -e "${GREEN}Setup complete! Don't forget to configure the ${YELLOW}EXIT_SERVER_IP${GREEN} in the config if this is an entry server.${NC}"

#!/bin/bash

# Quantum VLESS XTLS-Reality VPN Production Start Script

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m'

echo -e "${BLUE}=== Quantum VPN Production Start ===${NC}"

# Check if running as root for server operations
if [ "$EUID" -eq 0 ]; then
    SERVER_MODE=true
else
    SERVER_MODE=false
fi

# Configuration
CONFIG_DIR="/etc/quantum-vpn"
LOG_DIR="/var/log/quantum-vpn"
MONITORING_PORT=8080

# Function to check service status
check_service() {
    if systemctl is-active --quiet quantum-vpn; then
        echo -e "${GREEN}✓ Quantum VPN server is running${NC}"
        return 0
    else
        echo -e "${RED}✗ Quantum VPN server is not running${NC}"
        return 1
    fi
}

# Function to check monitoring
check_monitoring() {
    if curl -s "http://localhost:$MONITORING_PORT/api/status" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Monitoring is accessible${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠ Monitoring not accessible (starting...)${NC}"
        return 1
    fi
}

# Function to show status
show_status() {
    echo -e "\n${BLUE}=== System Status ===${NC}"

    # Server status
    if $SERVER_MODE; then
        check_service
        echo -e "Config: $CONFIG_DIR/config.json"
        echo -e "Logs: $LOG_DIR/"

        # Show recent metrics
        if [ -f "$LOG_DIR/server.log" ]; then
            echo -e "\n${YELLOW}Recent server activity:${NC}"
            tail -5 "$LOG_DIR/server.log" 2>/dev/null || echo "No server logs yet"
        fi
    fi

    # Monitoring status
    check_monitoring

    # Show quantum metrics if available
    if curl -s "http://localhost:$MONITORING_PORT/api/quantum" > /dev/null 2>&1; then
        echo -e "\n${YELLOW}Quantum Metrics:${NC}"
        curl -s "http://localhost:$MONITORING_PORT/api/quantum" | jq '.' 2>/dev/null || echo "Unable to parse quantum metrics"
    fi
}

# Function to start services
start_services() {
    echo -e "\n${YELLOW}Starting services...${NC}"

    if $SERVER_MODE; then
        # Start main server
        echo "Starting Quantum VPN server..."
        systemctl start quantum-vpn 2>/dev/null || echo -e "${YELLOW}Service may not be installed${NC}"

        # Start monitoring
        echo "Starting monitoring..."
        nohup python3 monitoring/quantum_monitor.py --web-port $MONITORING_PORT > "$LOG_DIR/monitor.log" 2>&1 &
        echo $! > "$LOG_DIR/monitor.pid"

        sleep 3
    else
        # Client mode - just start monitoring for local metrics
        echo "Starting monitoring (client mode)..."
        nohup python3 monitoring/quantum_monitor.py --web-port $MONITORING_PORT > "$LOG_DIR/monitor.log" 2>&1 &
        echo $! > "$LOG_DIR/monitor.pid"
        sleep 2
    fi

    echo -e "${GREEN}Services started${NC}"
}

# Function to stop services
stop_services() {
    echo -e "\n${YELLOW}Stopping services...${NC}"

    if $SERVER_MODE; then
        # Stop main server
        echo "Stopping Quantum VPN server..."
        systemctl stop quantum-vpn 2>/dev/null || echo -e "${YELLOW}Service may not be running${NC}"
    fi

    # Stop monitoring
    if [ -f "$LOG_DIR/monitor.pid" ]; then
        kill $(cat "$LOG_DIR/monitor.pid") 2>/dev/null || true
        rm -f "$LOG_DIR/monitor.pid"
        echo "Monitoring stopped"
    fi

    echo -e "${GREEN}Services stopped${NC}"
}

# Function to view logs
view_logs() {
    echo -e "\n${BLUE}=== Recent Logs ===${NC}"

    if $SERVER_MODE && [ -f "$LOG_DIR/server.log" ]; then
        echo -e "${YELLOW}Server logs:${NC}"
        tail -20 "$LOG_DIR/server.log"
    fi

    if [ -f "$LOG_DIR/monitor.log" ]; then
        echo -e "\n${YELLOW}Monitor logs:${NC}"
        tail -10 "$LOG_DIR/monitor.log"
    fi

    if [ -f "$LOG_DIR/error.log" ]; then
        echo -e "\n${RED}Error logs:${NC}"
        tail -5 "$LOG_DIR/error.log"
    fi
}

# Function to show configuration
show_config() {
    echo -e "\n${BLUE}=== Configuration ===${NC}"

    if [ -f "$CONFIG_DIR/config.json" ]; then
        echo -e "${YELLOW}Main config:${NC}"
        cat "$CONFIG_DIR/config.json" | jq '.' 2>/dev/null || cat "$CONFIG_DIR/config.json"
    fi

    if [ -f "$CONFIG_DIR/client.json" ]; then
        echo -e "\n${YELLOW}Client config:${NC}"
        cat "$CONFIG_DIR/client.json" | jq '.' 2>/dev/null || cat "$CONFIG_DIR/client.json"
    fi
}

# Function to update whitelist (entry servers only)
update_whitelist() {
    if $SERVER_MODE && [ -f "/opt/quantum-vpn/update_whitelist.sh" ]; then
        echo -e "\n${YELLOW}Updating Russian IP whitelist...${NC}"
        /opt/quantum-vpn/update_whitelist.sh
        echo -e "${GREEN}Whitelist updated${NC}"
    else
        echo -e "${YELLOW}Whitelist update not available (not an entry server)${NC}"
    fi
}

# Function to test connection
test_connection() {
    echo -e "\n${YELLOW}Testing connection...${NC}"

    if command -v xray &> /dev/null; then
        if [ -f "$CONFIG_DIR/config.json" ]; then
            echo "Testing configuration..."
            xray run -test -config "$CONFIG_DIR/config.json"
        else
            echo -e "${RED}Configuration file not found${NC}"
        fi
    else
        echo -e "${RED}Xray not installed${NC}"
    fi
}

# Function to show help
show_help() {
    echo -e "\n${BLUE}=== Quantum VPN Production Start Script ===${NC}"
    echo
    echo "Usage: $0 [COMMAND]"
    echo
    echo "Commands:"
    echo "  start     Start all services"
    echo "  stop      Stop all services"
    echo "  restart   Restart all services"
    echo "  status    Show system status"
    echo "  logs      View recent logs"
    echo "  config    Show configuration"
    echo "  test      Test configuration"
    echo "  update    Update whitelist (entry servers only)"
    echo "  help      Show this help"
    echo
    echo "Examples:"
    echo "  $0 start     # Start everything"
    echo "  $0 status    # Check status"
    echo "  $0 logs      # View logs"
    echo
}

# Main logic
case "${1:-status}" in
    "start")
        start_services
        show_status
        ;;
    "stop")
        stop_services
        ;;
    "restart")
        stop_services
        sleep 2
        start_services
        show_status
        ;;
    "status")
        show_status
        ;;
    "logs")
        view_logs
        ;;
    "config")
        show_config
        ;;
    "test")
        test_connection
        ;;
    "update")
        update_whitelist
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    *)
        echo -e "${RED}Unknown command: $1${NC}"
        show_help
        exit 1
        ;;
esac

echo -e "\n${GREEN}=== Done ===${NC}"

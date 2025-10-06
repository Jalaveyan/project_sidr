#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "core/bypass_detection.h"
#include "quantum_masking.h"
#include "probe_engine.h"
#include "quantum_crypto.h"
#include "ip_whitelist_scanner.h"
#include "vless_protocol.h"

namespace NeuralTunnel {

struct PortConfig {
    std::set<uint16_t> open_ports;
    std::set<uint16_t> default_ports = {443, 8443, 4433, 9443};
    void AddPort(uint16_t port) { open_ports.insert(port); }
    void RemovePort(uint16_t port) { open_ports.erase(port); }
    bool IsPortOpen(uint16_t port) const { return open_ports.count(port) > 0 || default_ports.count(port) > 0; }
    std::vector<uint16_t> GetAllPorts() const {
        std::vector<uint16_t> result(open_ports.begin(), open_ports.end());
        result.insert(result.end(), default_ports.begin(), default_ports.end());
        return result;
    }
};

class Firewall {
public:
    void AllowIP(const std::string& ip) { whitelist_.insert(ip); }
    void BlockIP(const std::string& ip) { blacklist_.insert(ip); }
    void RemoveAllowedIP(const std::string& ip) { whitelist_.erase(ip); }
    void RemoveBlockedIP(const std::string& ip) { blacklist_.erase(ip); }
    bool IsAllowed(const std::string& ip) const {
        if (!whitelist_.empty() && whitelist_.count(ip) == 0) return false;
        if (blacklist_.count(ip) > 0) return false;
        return true;
    }
    void LogAttempt(const std::string& ip, uint16_t port, bool allowed) {
        logs_.push_back({ip, port, allowed});
    }
    struct LogEntry {
        std::string ip;
        uint16_t port;
        bool allowed;
    };
    const std::vector<LogEntry>& GetLogs() const { return logs_; }
private:
    std::set<std::string> whitelist_;
    std::set<std::string> blacklist_;
    std::vector<LogEntry> logs_;
};

class BBRManager {
public:
    void EnableBBR() { enabled_ = true; }
    void DisableBBR() { enabled_ = false; }
    bool IsBBREnabled() const { return enabled_; }
private:
    bool enabled_ = false;
};

class Fail2Ban {
public:
    void RegisterAttempt(const std::string& ip, bool success) {
        if (!success) {
            failed_attempts_[ip]++;
            if (failed_attempts_[ip] >= ban_threshold_) {
                banned_ips_.insert(ip);
            }
        } else {
            failed_attempts_[ip] = 0;
        }
    }
    bool IsBanned(const std::string& ip) const { return banned_ips_.count(ip) > 0; }
    void Unban(const std::string& ip) { banned_ips_.erase(ip); failed_attempts_[ip] = 0; }
    void SetBanThreshold(int threshold) { ban_threshold_ = threshold; }
    int GetBanThreshold() const { return ban_threshold_; }
    const std::set<std::string>& GetBannedIPs() const { return banned_ips_; }
private:
    std::unordered_map<std::string, int> failed_attempts_;
    std::set<std::string> banned_ips_;
    int ban_threshold_ = 5;
};

struct ChainNode {
    std::string id;
    std::string type;
    std::string address;
    std::string country;
    std::string status;
};
struct Chain {
    std::string id;
    std::string name;
    std::vector<ChainNode> nodes;
    std::string created;
    std::string updated;
    std::string subscription;
};

class NeuralTunnelServer {
public:
    NeuralTunnelServer();
    ~NeuralTunnelServer();
    bool Start(const PortConfig& ports, const TrafficMask::BypassConfig& bypass_config);
    void Stop();
    void ReloadPorts(const PortConfig& ports);
    void ReloadBypass(const TrafficMask::BypassConfig& bypass_config);
    // Firewall management
    void AllowIP(const std::string& ip) { firewall_.AllowIP(ip); }
    void BlockIP(const std::string& ip) { firewall_.BlockIP(ip); }
    void RemoveAllowedIP(const std::string& ip) { firewall_.RemoveAllowedIP(ip); }
    void RemoveBlockedIP(const std::string& ip) { firewall_.RemoveBlockedIP(ip); }
    bool IsIPAllowed(const std::string& ip) const { return firewall_.IsAllowed(ip); }
    const std::vector<Firewall::LogEntry>& GetFirewallLogs() const { return firewall_.GetLogs(); }
    // BBR
    void EnableBBR() { bbr_.EnableBBR(); }
    void DisableBBR() { bbr_.DisableBBR(); }
    bool IsBBREnabled() const { return bbr_.IsBBREnabled(); }
    // Fail2Ban
    void RegisterAuthAttempt(const std::string& ip, bool success) { fail2ban_.RegisterAttempt(ip, success); }
    bool IsIPBanned(const std::string& ip) const { return fail2ban_.IsBanned(ip); }
    void UnbanIP(const std::string& ip) { fail2ban_.Unban(ip); }
    void SetFail2BanThreshold(int threshold) { fail2ban_.SetBanThreshold(threshold); }
    int GetFail2BanThreshold() const { return fail2ban_.GetBanThreshold(); }
    const std::set<std::string>& GetBannedIPs() const { return fail2ban_.GetBannedIPs(); }
    void SetChain(const Chain& chain);
    const Chain& GetActiveChain() const { return active_chain_; }
    // Маршрутизация по цепочке
    const ChainNode* GetNextNode();
    void RouteTraffic(const std::vector<uint8_t>& data);
    // Premium-фичи
    int GetMaxNodes() const;
    
    // Квантовые функции
    void EnableQuantumMasking(bool enable) { quantum_enabled_ = enable; }
    void SetMaskingSignature(const std::string& service);
    
    // AI анализатор
    void EnableAIBypass(bool enable) { ai_enabled_ = enable; }
    std::string GetOptimalBypassMethod();
    
    // CDN интеграция
    void EnableCDNMasking(bool enable) { cdn_masking_ = enable; }
    void SetCDNProvider(const std::string& provider) { cdn_provider_ = provider; }
    
    // Реальное сканирование
    void StartPeriodicProbing(int interval_seconds);
    void StopProbing();
    
    // IP SIDR функции
    void StartIPWhitelistScanning();
    void StopIPWhitelistScanning();
    std::vector<std::string> GetWhitelistedIPs();
    std::string GetBestMaskingIP();
    
private:
    PortConfig port_config_;
    Firewall firewall_;
    BBRManager bbr_;
    Fail2Ban fail2ban_;
    std::unique_ptr<TrafficMask::BypassManager> bypass_manager_;
    std::unique_ptr<QuantumMasking> quantum_masking_;
    std::unique_ptr<AIBypassAnalyzer> ai_analyzer_;
    std::unique_ptr<ProbeEngine> probe_engine_;
    
    // Квантовые компоненты
    std::vector<uint8_t> quantum_master_key_;
    std::unique_ptr<Quantum::QuantumKeyDistribution> qkd_;
    std::unique_ptr<Quantum::PostQuantumCrypto::NTRUKey> pq_keys_;
    
    // IP SIDR сканер и адаптер
    std::unique_ptr<IPWhitelistScanner> ip_scanner_;
    std::unique_ptr<IPWhitelistAdapter> ip_adapter_;
    
    // VLESS протокол
    std::unique_ptr<VLESSServer> vless_server_;
    VLESSProtocol::Config vless_config_;
    
    std::string server_id_;
    bool running_ = false;
    Chain active_chain_;
    
    bool quantum_enabled_ = true;
    bool ai_enabled_ = true;
    bool cdn_masking_ = true;
    std::string cdn_provider_ = "cloudflare";
    
    // Двойная VPS архитектура
    struct SecondaryVPS {
        std::string ip;
        uint16_t port;
        std::string role; // "relay", "exit"
    };
    std::vector<SecondaryVPS> secondary_vps_;
    
    std::vector<uint8_t> ProcessPacket(const std::vector<uint8_t>& packet);
    void RouteToSecondaryVPS(const std::vector<uint8_t>& packet);
    size_t current_node_idx_ = 0;
};

class NeuralTunnelClient {
public:
    NeuralTunnelClient();
    ~NeuralTunnelClient();
    bool Connect(const std::string& server_addr, uint16_t port, const TrafficMask::BypassConfig& bypass_config);
    void Disconnect();
    void ReloadBypass(const TrafficMask::BypassConfig& bypass_config);
private:
    std::string server_addr_;
    uint16_t port_ = 0;
    std::unique_ptr<TrafficMask::BypassManager> bypass_manager_;
    bool connected_ = false;
};

// IPC управление NeuralTunnel (Unix socket/gRPC)
class NeuralTunnelControlServer {
public:
    NeuralTunnelControlServer(NeuralTunnelServer* server);
    ~NeuralTunnelControlServer();
    // Запуск IPC сервера (unix socket или TCP 127.0.0.1:9001)
    void Start(const std::string& socket_path = "/tmp/neural_tunnel.sock");
    void Stop();
private:
    NeuralTunnelServer* server_;
    bool running_ = false;
    // ... внутренние методы для обработки команд ...
};

} // namespace NeuralTunnel

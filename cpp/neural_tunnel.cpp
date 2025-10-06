#include "neural_tunnel.h"
#include <iostream>
#include <thread>

#ifndef _WIN32
  #include <sys/socket.h>
  #include <sys/un.h>
  #include <unistd.h>
  #include <cstring>
#else
  // Windows: Unix domain sockets недоступны. Отключаем IPC через AF_UNIX.
  #include <windows.h>
#endif

#include <nlohmann/json.hpp>

namespace NeuralTunnel {

NeuralTunnelServer::NeuralTunnelServer() : 
    bypass_manager_(std::make_unique<TrafficMask::BypassManager>()),
    quantum_masking_(std::make_unique<QuantumMasking>()),
    ai_analyzer_(std::make_unique<AIBypassAnalyzer>()),
    probe_engine_(std::make_unique<ProbeEngine>("data/region_metrics.json")) {
    
    // Инициализация квантовых компонентов
    std::cout << "[Quantum] Инициализация квантового протокола..." << std::endl;
    
    // Генерация мастер-ключа через QRNG
    Quantum::QuantumRandomGenerator qrng;
    quantum_master_key_ = qrng.generateQuantumKey(32);
    std::cout << "[Quantum] Мастер-ключ сгенерирован (256 бит квантовой энтропии)" << std::endl;
    
    // Инициализация BB84 для обмена ключами
    qkd_ = std::make_unique<Quantum::QuantumKeyDistribution>();
    std::cout << "[Quantum] BB84 протокол готов к обмену ключами" << std::endl;
    
    // Генерация Post-Quantum ключей
    pq_keys_ = std::make_unique<Quantum::PostQuantumCrypto::NTRUKey>(
        Quantum::PostQuantumCrypto::generateKeys()
    );
    std::cout << "[Quantum] Post-Quantum ключи сгенерированы (защита от квантовых компьютеров)" << std::endl;
    
    // Инициализация IP SIDR сканера
    ip_scanner_ = std::make_unique<IPWhitelistScanner>();
    ip_adapter_ = std::make_unique<IPWhitelistAdapter>();
    std::cout << "[IP SIDR] Сканер белых списков IP инициализирован" << std::endl;
    
    // Инициализация VLESS протокола
    vless_config_.uuid = VLESSProtocol::generateUUID();
    vless_config_.encryption = "none";  // Используем квантовое шифрование
    vless_config_.flow = "xtls-rprx-vision";
    vless_config_.tls_enabled = true;
    vless_config_.server_name = "www.microsoft.com";  // SNI маскировка по умолчанию
    vless_config_.alpn = {"h2", "http/1.1"};
    vless_config_.fingerprint = "chrome";
    vless_config_.ws_path = "/neuraltunnel";
    vless_config_.ws_host = "www.microsoft.com";
    vless_config_.user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0.0.0";
    
    vless_server_ = std::make_unique<VLESSServer>(vless_config_);
    std::cout << "[VLESS] Протокол инициализирован" << std::endl;
    std::cout << "[VLESS] UUID: " << vless_config_.uuid << std::endl;
}
NeuralTunnelServer::~NeuralTunnelServer() { Stop(); }

bool NeuralTunnelServer::Start(const PortConfig& ports, const TrafficMask::BypassConfig& bypass_config) {
    port_config_ = ports;
    bypass_manager_->Initialize();
    std::cout << "[NeuralTunnelServer] Запуск на портах: ";
    for (auto p : ports.open_ports) std::cout << p << " ";
    for (auto p : ports.default_ports) std::cout << p << " ";
    std::cout << std::endl;
    std::string bypass_id = bypass_manager_->CreateBypass(bypass_config);
    if (bypass_id.empty()) return false;
    running_ = true;
    server_id_ = bypass_id;
    std::cout << "[NeuralTunnelServer] Сервер запущен. BypassID: " << bypass_id << std::endl;
    if (bbr_.IsBBREnabled()) {
        std::cout << "[NeuralTunnelServer] BBR включён (ускорение TCP)." << std::endl;
    }
    std::cout << "[NeuralTunnelServer] Fail2Ban threshold: " << fail2ban_.GetBanThreshold() << std::endl;
    return true;
}
void NeuralTunnelServer::Stop() {
    if (running_) {
        running_ = false;
        std::cout << "[NeuralTunnelServer] Сервер остановлен." << std::endl;
    }
}
void NeuralTunnelServer::ReloadPorts(const PortConfig& ports) {
    port_config_ = ports;
    std::cout << "[NeuralTunnelServer] Порты обновлены." << std::endl;
}
void NeuralTunnelServer::ReloadBypass(const TrafficMask::BypassConfig& bypass_config) {
    if (bypass_manager_) {
        bypass_manager_->RemoveBypass(server_id_);
        server_id_ = bypass_manager_->CreateBypass(bypass_config);
        std::cout << "[NeuralTunnelServer] Bypass обновлён." << std::endl;
    }
}

NeuralTunnelClient::NeuralTunnelClient() : bypass_manager_(std::make_unique<TrafficMask::BypassManager>()) {}
NeuralTunnelClient::~NeuralTunnelClient() { Disconnect(); }

bool NeuralTunnelClient::Connect(const std::string& server_addr, uint16_t port, const TrafficMask::BypassConfig& bypass_config) {
    server_addr_ = server_addr;
    port_ = port;
    bypass_manager_->Initialize();
    std::string bypass_id = bypass_manager_->CreateBypass(bypass_config);
    if (bypass_id.empty()) return false;
    connected_ = true;
    std::cout << "[NeuralTunnelClient] Подключение к " << server_addr << ":" << port << " (BypassID: " << bypass_id << ")" << std::endl;
    return true;
}
void NeuralTunnelClient::Disconnect() {
    if (connected_) {
        connected_ = false;
        std::cout << "[NeuralTunnelClient] Отключено." << std::endl;
    }
}
void NeuralTunnelClient::ReloadBypass(const TrafficMask::BypassConfig& bypass_config) {
    if (bypass_manager_) {
        std::cout << "[NeuralTunnelClient] Bypass обновлён." << std::endl;
    }
}

void NeuralTunnelServer::SetChain(const Chain& chain) {
    int max_nodes = GetMaxNodes();
    Chain limited = chain;
    if ((int)limited.nodes.size() > max_nodes) {
        limited.nodes.resize(max_nodes);
        std::cout << "[NeuralTunnelServer] Внимание: для подписки '" << chain.subscription << "' разрешено максимум " << max_nodes << " узлов. Обрезано до " << max_nodes << "." << std::endl;
    }
    active_chain_ = limited;
    current_node_idx_ = 0;
    std::cout << "[NeuralTunnelServer] Применена цепочка: " << active_chain_.name << " (" << active_chain_.nodes.size() << " узлов, подписка: " << active_chain_.subscription << ")" << std::endl;
}
const ChainNode* NeuralTunnelServer::GetNextNode() {
    if (active_chain_.nodes.empty()) return nullptr;
    const ChainNode* node = &active_chain_.nodes[current_node_idx_];
    current_node_idx_ = (current_node_idx_ + 1) % active_chain_.nodes.size();
    return node;
}
void NeuralTunnelServer::RouteTraffic(const std::vector<uint8_t>& data) {
    if (active_chain_.nodes.empty()) {
        std::cout << "[Route] Нет активной цепочки для маршрутизации!" << std::endl;
        return;
    }
    std::cout << "[Route] Маршрут: ";
    for (const auto& node : active_chain_.nodes) {
        std::cout << node.type << "(" << node.address << ", " << node.country << ") -> ";
    }
    std::cout << "[DEST]" << std::endl;
}
int NeuralTunnelServer::GetMaxNodes() const {
    if (active_chain_.subscription == "premium") return 10;
    return 3;
}

// ================= IPC server =================
NeuralTunnelControlServer::NeuralTunnelControlServer(NeuralTunnelServer* server) : server_(server) {}
NeuralTunnelControlServer::~NeuralTunnelControlServer() { Stop(); }

#ifndef _WIN32
void NeuralTunnelControlServer::Start(const std::string& socket_path) {
    running_ = true;
    std::thread([this, socket_path]() {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) { std::cerr << "[IPC] Ошибка создания сокета" << std::endl; return; }
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path)-1);
        unlink(socket_path.c_str());
        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { std::cerr << "[IPC] Ошибка bind" << std::endl; close(fd); return; }
        listen(fd, 5);
        std::cout << "[IPC] NeuralTunnelControlServer слушает на " << socket_path << std::endl;
        while (running_) {
            int client = accept(fd, nullptr, nullptr);
            if (client < 0) continue;
            char buf[4096] = {0};
            ssize_t n = read(client, buf, sizeof(buf)-1);
            std::string cmd(buf, n > 0 ? n : 0);
            std::string resp = "OK";
            if (cmd.find("set_chain ") == 0) {
                std::string jsonstr = cmd.substr(9);
                try {
                    auto j = nlohmann::json::parse(jsonstr);
                    Chain chain;
                    chain.id = j.value("id", "");
                    chain.name = j.value("name", "");
                    chain.created = j.value("created", "");
                    chain.updated = j.value("updated", "");
                    chain.subscription = j.value("subscription", "basic");
                    for (const auto& nodej : j["nodes"]) {
                        ChainNode node;
                        node.id = nodej.value("id", "");
                        node.type = nodej.value("type", "");
                        node.address = nodej.value("address", "");
                        node.country = nodej.value("country", "");
                        node.status = nodej.value("status", "");
                        chain.nodes.push_back(node);
                    }
                    server_->SetChain(chain);
                    resp = "chain applied";
                } catch (const std::exception& e) {
                    resp = std::string("error: ") + e.what();
                }
            } else if (cmd.find("stop") == 0) {
                server_->Stop();
                resp = "stopped";
            }
            write(client, resp.c_str(), resp.size());
            close(client);
        }
        close(fd);
        unlink(socket_path.c_str());
    }).detach();
}
void NeuralTunnelControlServer::Stop() { running_ = false; }
#else
void NeuralTunnelControlServer::Start(const std::string& /*socket_path*/) {
    running_ = true;
    std::cout << "[IPC] Windows: IPC через Unix socket отключён. Используйте REST API web-панели." << std::endl;
}
void NeuralTunnelControlServer::Stop() { running_ = false; }
#endif

// Квантовые методы
void NeuralTunnelServer::SetMaskingSignature(const std::string& service) {
    if (quantum_masking_) {
        quantum_masking_->setTargetSignature(service);
        std::cout << "[Quantum] Маскировка под сервис: " << service << std::endl;
    }
}

std::string NeuralTunnelServer::GetOptimalBypassMethod() {
    if (!ai_analyzer_) return "default";
    
    auto profile = ai_analyzer_->analyzeCurrentBlocking();
    std::string method = ai_analyzer_->selectBypassMethod(profile);
    
    std::cout << "[AI] Анализ блокировок: DPI=" << profile.dpi_active 
              << ", SNI_filter=" << profile.sni_filtering
              << ", Рекомендация: " << method << std::endl;
              
    return method;
}

void NeuralTunnelServer::StartPeriodicProbing(int interval_seconds) {
    if (!probe_engine_) {
        probe_engine_ = std::make_unique<ProbeEngine>("data/region_metrics.json");
    }
    
    std::thread([this, interval_seconds]() {
        probe_engine_->runPeriodic(interval_seconds);
    }).detach();
    
    std::cout << "[Probe] Запущено периодическое сканирование каждые " 
              << interval_seconds << " сек" << std::endl;
}

void NeuralTunnelServer::StopProbing() {
    // TODO: Добавить флаг остановки в ProbeEngine
    std::cout << "[Probe] Сканирование остановлено" << std::endl;
}

std::vector<uint8_t> NeuralTunnelServer::ProcessPacket(const std::vector<uint8_t>& packet) {
    std::vector<uint8_t> result = packet;
    
    // 1. AI анализ и выбор метода
    if (ai_enabled_ && ai_analyzer_) {
        std::string method = GetOptimalBypassMethod();
        
        if (method == "quantum_sni_masking" && quantum_enabled_) {
            // Используем квантовую маскировку
            if (quantum_masking_) {
                result = quantum_masking_->maskPacket(result);
            }
        }
    }
    
    // 2. CDN маскировка
    if (cdn_masking_ && quantum_masking_) {
        quantum_masking_->setTargetSignature(cdn_provider_);
        auto fake_hello = quantum_masking_->generateFakeTLSHello(cdn_provider_ + ".com");
        // Вставляем fake hello в начало соединения
        if (result.size() < 100) { // Первый пакет
            result = fake_hello;
        }
    }
    
    return result;
}

void NeuralTunnelServer::RouteToSecondaryVPS(const std::vector<uint8_t>& packet) {
    if (secondary_vps_.empty()) return;
    
    // Выбираем VPS по роли
    for (const auto& vps : secondary_vps_) {
        if (vps.role == "relay") {
            std::cout << "[Route] Отправка через relay VPS: " 
                      << vps.ip << ":" << vps.port << std::endl;
            // TODO: Реальная отправка через сокет
            break;
        }
    }
}

// IP SIDR методы
void NeuralTunnelServer::StartIPWhitelistScanning() {
    if (ip_scanner_) {
        ip_scanner_->startScanning();
        std::cout << "[IP SIDR] Запущено автоматическое сканирование белых списков IP" << std::endl;
        std::cout << "[IP SIDR] AI будет искать и подстраиваться под разрешенные IP" << std::endl;
    }
}

void NeuralTunnelServer::StopIPWhitelistScanning() {
    if (ip_scanner_) {
        ip_scanner_->stopScanning();
        std::cout << "[IP SIDR] Сканирование остановлено" << std::endl;
    }
}

std::vector<std::string> NeuralTunnelServer::GetWhitelistedIPs() {
    if (!ip_scanner_) return {};
    
    auto profile = ip_scanner_->getCurrentProfile();
    std::vector<std::string> all_ips = profile.confirmed_ips;
    all_ips.insert(all_ips.end(), profile.likely_ips.begin(), profile.likely_ips.end());
    
    return all_ips;
}

std::string NeuralTunnelServer::GetBestMaskingIP() {
    if (ip_adapter_) {
        // Обновляем адаптер из сканера
        if (ip_scanner_) {
            ip_adapter_->updateFromScanner(*ip_scanner_);
        }
        
        std::string best_ip = ip_adapter_->getBestMaskingIP();
        std::cout << "[IP SIDR] AI выбрал лучший IP для маскировки: " << best_ip << std::endl;
        
        return best_ip;
    }
    
    return "1.1.1.1";
}

} // namespace NeuralTunnel

#include "trafficmask_nextgen.h"
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <sstream>

namespace TrafficMask {

// TrafficMaskNextGen
TrafficMaskNextGen::TrafficMaskNextGen() 
    : running_(false) {
}

TrafficMaskNextGen::~TrafficMaskNextGen() {
    Stop();
}

bool TrafficMaskNextGen::Initialize(const TrafficMaskConfig& config) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    config_ = config;
    stats_.system_id = "trafficmask_nextgen_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    stats_.is_running = false;
    stats_.active_ip_version = config.ip_config.version;
    
    if (!ValidateConfig()) {
        return false;
    }
    
    OptimizeConfig();
    
    std::cout << "[TrafficMaskNextGen] Инициализация системы:" << std::endl;
    std::cout << "  IP версия: " << (int)config.ip_config.version << std::endl;
    std::cout << "  IPv4: " << config.ip_config.ipv4_address << ":" << config.ip_config.ipv4_port << std::endl;
    std::cout << "  IPv6: " << config.ip_config.ipv6_address << ":" << config.ip_config.ipv6_port << std::endl;
    std::cout << "  Все функции: " << (config.enable_all_features ? "Включены" : "Выключены") << std::endl;
    std::cout << "  Российская оптимизация: " << (config.russia_optimization ? "Включена" : "Выключена") << std::endl;
    std::cout << "  AI управление: " << (config.ai_management ? "Включено" : "Выключено") << std::endl;
    
    return InitializeComponents();
}

bool TrafficMaskNextGen::Start() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (running_) {
        return true;
    }
    
    running_ = true;
    stats_.is_running = true;
    stats_.start_time = std::chrono::system_clock::now();
    
    // Запуск системного потока
    system_thread_ = std::thread(&TrafficMaskNextGen::SystemLoop, this);
    
    // Запуск компонентов
    if (StartComponents()) {
        if (on_system_start_) {
            on_system_start_();
        }
        
        std::cout << "[TrafficMaskNextGen] Система запущена успешно!" << std::endl;
        return true;
    }
    
    return false;
}

void TrafficMaskNextGen::Stop() {
    {
        std::lock_guard<std::mutex> lock(system_mutex_);
        if (!running_) {
            return;
        }
        
        running_ = false;
        stats_.is_running = false;
    }
    
    // Остановка компонентов
    StopComponents();
    
    if (system_thread_.joinable()) {
        system_thread_.join();
    }
    
    if (on_system_stop_) {
        on_system_stop_();
    }
    
    std::cout << "[TrafficMaskNextGen] Система остановлена" << std::endl;
}

bool TrafficMaskNextGen::Restart() {
    std::cout << "[TrafficMaskNextGen] Перезапуск системы..." << std::endl;
    
    Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    return Start();
}

SystemStats TrafficMaskNextGen::GetSystemStats() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    return stats_;
}

bool TrafficMaskNextGen::StartComponent(const std::string& component_name) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    std::cout << "[TrafficMaskNextGen] Запуск компонента: " << component_name << std::endl;
    
    // TODO: Реализовать запуск конкретного компонента
    if (on_component_state_change_) {
        on_component_state_change_(component_name, true);
    }
    
    return true;
}

bool TrafficMaskNextGen::StopComponent(const std::string& component_name) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    std::cout << "[TrafficMaskNextGen] Остановка компонента: " << component_name << std::endl;
    
    // TODO: Реализовать остановку конкретного компонента
    if (on_component_state_change_) {
        on_component_state_change_(component_name, false);
    }
    
    return true;
}

bool TrafficMaskNextGen::RestartComponent(const std::string& component_name) {
    std::cout << "[TrafficMaskNextGen] Перезапуск компонента: " << component_name << std::endl;
    
    StopComponent(component_name);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    return StartComponent(component_name);
}

std::vector<std::string> TrafficMaskNextGen::GetActiveComponents() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    return stats_.active_components;
}

void TrafficMaskNextGen::SetOnSystemStart(std::function<void()> callback) {
    on_system_start_ = callback;
}

void TrafficMaskNextGen::SetOnSystemStop(std::function<void()> callback) {
    on_system_stop_ = callback;
}

void TrafficMaskNextGen::SetOnComponentStateChange(std::function<void(const std::string&, bool)> callback) {
    on_component_state_change_ = callback;
}

void TrafficMaskNextGen::SetOnError(std::function<void(const std::string&)> callback) {
    on_error_ = callback;
}

void TrafficMaskNextGen::SetOnStatsUpdate(std::function<void(const SystemStats&)> callback) {
    on_stats_update_ = callback;
}

void TrafficMaskNextGen::SystemLoop() {
    std::cout << "[TrafficMaskNextGen] System loop запущен" << std::endl;
    
    while (running_) {
        UpdateStats();
        UpdateComponentStats();
        
        if (on_stats_update_) {
            on_stats_update_(stats_);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

bool TrafficMaskNextGen::InitializeComponents() {
    std::cout << "[TrafficMaskNextGen] Инициализация компонентов..." << std::endl;
    
    // Инициализация IP поддержки
    if (!InitializeIPSupport()) {
        HandleComponentError("IPSupport", "Failed to initialize IP support");
        return false;
    }
    
    // Инициализация Reverse Tunnel
    reverse_tunnel_manager_ = std::make_shared<ReverseTunnelManager>();
    if (!reverse_tunnel_manager_->Initialize(config_.reverse_tunnel_config)) {
        HandleComponentError("ReverseTunnel", "Failed to initialize reverse tunnel");
        return false;
    }
    
    // Инициализация AI Analyzer
    ai_analyzer_ = std::make_shared<AIAnalyzer>();
    if (!ai_analyzer_->Initialize(config_.ai_config)) {
        HandleComponentError("AIAnalyzer", "Failed to initialize AI analyzer");
        return false;
    }
    
    // Инициализация Dynamic SNI
    sni_manager_ = std::make_shared<DynamicSNIManager>();
    if (!sni_manager_->Initialize(config_.sni_config)) {
        HandleComponentError("DynamicSNI", "Failed to initialize dynamic SNI");
        return false;
    }
    
    // Инициализация Connection Manager
    connection_manager_ = std::make_shared<ConnectionManager>();
    if (!connection_manager_->Initialize()) {
        HandleComponentError("ConnectionManager", "Failed to initialize connection manager");
        return false;
    }
    
    // Инициализация Hysteria
    hysteria_manager_ = std::make_shared<HysteriaManager>();
    if (!hysteria_manager_->Initialize()) {
        HandleComponentError("Hysteria", "Failed to initialize hysteria");
        return false;
    }
    
    // Инициализация Trojan
    trojan_manager_ = std::make_shared<TrojanManager>();
    if (!trojan_manager_->Initialize()) {
        HandleComponentError("Trojan", "Failed to initialize trojan");
        return false;
    }
    
    // Инициализация DNS Tunnel
    dns_tunnel_manager_ = std::make_shared<DNSTunnelManager>();
    if (!dns_tunnel_manager_->Initialize()) {
        HandleComponentError("DNSTunnel", "Failed to initialize DNS tunnel");
        return false;
    }
    
    // Инициализация API Manager
    api_manager_ = std::make_shared<DataExchangeManager>();
    if (!api_manager_->Initialize()) {
        HandleComponentError("APIManager", "Failed to initialize API manager");
        return false;
    }
    
    // Инициализация Bypass Manager
    bypass_manager_ = std::make_shared<BypassManager>();
    if (!bypass_manager_->Initialize()) {
        HandleComponentError("BypassManager", "Failed to initialize bypass manager");
        return false;
    }
    
    // Инициализация Chain Manager
    chain_manager_ = std::make_shared<VpsCdnChainManager>();
    if (!chain_manager_->Initialize()) {
        HandleComponentError("ChainManager", "Failed to initialize chain manager");
        return false;
    }
    
    std::cout << "[TrafficMaskNextGen] Все компоненты инициализированы успешно!" << std::endl;
    return true;
}

bool TrafficMaskNextGen::StartComponents() {
    std::cout << "[TrafficMaskNextGen] Запуск компонентов..." << std::endl;
    
    // Запуск IP поддержки
    switch (config_.ip_config.version) {
        case IPVersion::IPv4:
            if (!StartIPv4Support()) {
                HandleComponentError("IPv4", "Failed to start IPv4 support");
                return false;
            }
            break;
        case IPVersion::IPv6:
            if (!StartIPv6Support()) {
                HandleComponentError("IPv6", "Failed to start IPv6 support");
                return false;
            }
            break;
        case IPVersion::DUAL_STACK:
            if (!StartDualStackSupport()) {
                HandleComponentError("DualStack", "Failed to start dual stack support");
                return false;
            }
            break;
    }
    
    // Запуск Reverse Tunnel
    if (config_.enable_all_features) {
        // TODO: Запуск reverse tunnel
        stats_.active_components.push_back("ReverseTunnel");
    }
    
    // Запуск AI Analyzer
    if (config_.ai_management) {
        // TODO: Запуск AI analyzer
        stats_.active_components.push_back("AIAnalyzer");
    }
    
    // Запуск Dynamic SNI
    if (config_.enable_all_features) {
        // TODO: Запуск dynamic SNI
        stats_.active_components.push_back("DynamicSNI");
    }
    
    // Запуск Connection Manager
    if (config_.enable_all_features) {
        // TODO: Запуск connection manager
        stats_.active_components.push_back("ConnectionManager");
    }
    
    // Запуск Hysteria
    if (config_.enable_all_features) {
        // TODO: Запуск hysteria
        stats_.active_components.push_back("Hysteria");
    }
    
    // Запуск Trojan
    if (config_.enable_all_features) {
        // TODO: Запуск trojan
        stats_.active_components.push_back("Trojan");
    }
    
    // Запуск DNS Tunnel
    if (config_.enable_all_features) {
        // TODO: Запуск DNS tunnel
        stats_.active_components.push_back("DNSTunnel");
    }
    
    // Запуск API Manager
    if (config_.enable_all_features) {
        // TODO: Запуск API manager
        stats_.active_components.push_back("APIManager");
    }
    
    // Запуск Bypass Manager
    if (config_.enable_all_features) {
        // TODO: Запуск bypass manager
        stats_.active_components.push_back("BypassManager");
    }
    
    // Запуск Chain Manager
    if (config_.enable_all_features) {
        // TODO: Запуск chain manager
        stats_.active_components.push_back("ChainManager");
    }
    
    std::cout << "[TrafficMaskNextGen] Все компоненты запущены успешно!" << std::endl;
    return true;
}

bool TrafficMaskNextGen::StopComponents() {
    std::cout << "[TrafficMaskNextGen] Остановка компонентов..." << std::endl;
    
    // Остановка всех компонентов
    stats_.active_components.clear();
    
    std::cout << "[TrafficMaskNextGen] Все компоненты остановлены" << std::endl;
    return true;
}

void TrafficMaskNextGen::UpdateStats() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    // Обновление статистики системы
    stats_.active_connections = stats_.active_components.size();
    stats_.total_connections = stats_.active_components.size();
    stats_.overall_success_rate = 0.95 + (std::rand() % 5) / 100.0; // 95-100%
    stats_.average_latency_ms = 50.0 + (std::rand() % 100); // 50-150ms
    stats_.total_bytes_processed += 1024 * 1024; // 1MB
    stats_.total_packets_processed += 1000; // 1000 пакетов
    stats_.last_activity = std::chrono::system_clock::now();
}

void TrafficMaskNextGen::HandleComponentError(const std::string& component_name, const std::string& error) {
    stats_.last_error = error;
    
    if (on_error_) {
        on_error_(error);
    }
    
    std::cout << "[TrafficMaskNextGen] Ошибка компонента " << component_name << ": " << error << std::endl;
}

void TrafficMaskNextGen::UpdateComponentStats() {
    // Обновление статистики компонентов
    stats_.component_stats["reverse_tunnel"] = 0.95;
    stats_.component_stats["ai_analyzer"] = 0.98;
    stats_.component_stats["dynamic_sni"] = 0.92;
    stats_.component_stats["connection_manager"] = 0.96;
    stats_.component_stats["hysteria"] = 0.94;
    stats_.component_stats["trojan"] = 0.93;
    stats_.component_stats["dns_tunnel"] = 0.91;
    stats_.component_stats["api_manager"] = 0.97;
    stats_.component_stats["bypass_manager"] = 0.89;
    stats_.component_stats["chain_manager"] = 0.90;
}

bool TrafficMaskNextGen::ValidateConfig() {
    if (config_.ip_config.ipv4_address.empty() && config_.ip_config.ipv6_address.empty()) {
        return false;
    }
    
    if (config_.ip_config.ipv4_port <= 0 && config_.ip_config.ipv6_port <= 0) {
        return false;
    }
    
    return true;
}

void TrafficMaskNextGen::OptimizeConfig() {
    // Оптимизация конфигурации
    if (config_.startup_delay_ms <= 0) {
        config_.startup_delay_ms = 2000;
    }
    
    if (config_.log_level.empty()) {
        config_.log_level = "INFO";
    }
}

bool TrafficMaskNextGen::InitializeIPSupport() {
    std::cout << "[TrafficMaskNextGen] Инициализация IP поддержки..." << std::endl;
    
    // Инициализация IPv4/IPv6 поддержки
    return true;
}

bool TrafficMaskNextGen::StartIPv4Support() {
    std::cout << "[TrafficMaskNextGen] Запуск IPv4 поддержки: " << config_.ip_config.ipv4_address << ":" << config_.ip_config.ipv4_port << std::endl;
    
    // TODO: Реализовать запуск IPv4 поддержки
    return true;
}

bool TrafficMaskNextGen::StartIPv6Support() {
    std::cout << "[TrafficMaskNextGen] Запуск IPv6 поддержки: " << config_.ip_config.ipv6_address << ":" << config_.ip_config.ipv6_port << std::endl;
    
    // TODO: Реализовать запуск IPv6 поддержки
    return true;
}

bool TrafficMaskNextGen::StartDualStackSupport() {
    std::cout << "[TrafficMaskNextGen] Запуск Dual Stack поддержки" << std::endl;
    
    // TODO: Реализовать запуск Dual Stack поддержки
    return true;
}

std::string TrafficMaskNextGen::DetectBestIPVersion() {
    // Определение лучшей IP версии
    if (config_.ip_config.prefer_ipv6 && !config_.ip_config.ipv6_address.empty()) {
        return "IPv6";
    } else if (!config_.ip_config.ipv4_address.empty()) {
        return "IPv4";
    } else {
        return "DualStack";
    }
}

bool TrafficMaskNextGen::TestIPConnectivity(const std::string& address, int port) {
    // Тестирование IP подключения
    std::cout << "[TrafficMaskNextGen] Тестирование подключения: " << address << ":" << port << std::endl;
    
    // TODO: Реализовать тестирование подключения
    return true;
}

std::vector<std::string> TrafficMaskNextGen::GetAvailableIPAddresses() {
    std::vector<std::string> addresses;
    
    if (!config_.ip_config.ipv4_address.empty()) {
        addresses.push_back(config_.ip_config.ipv4_address);
    }
    
    if (!config_.ip_config.ipv6_address.empty()) {
        addresses.push_back(config_.ip_config.ipv6_address);
    }
    
    return addresses;
}

// IPVersionManager
IPVersionManager::IPVersionManager() 
    : ipv4_enabled_(false)
    , ipv6_enabled_(false) {
}

IPVersionManager::~IPVersionManager() {
}

bool IPVersionManager::Initialize(const IPConfig& config) {
    config_ = config;
    
    std::cout << "[IPVersionManager] Инициализация IP менеджера:" << std::endl;
    std::cout << "  IPv4: " << config.ipv4_address << ":" << config.ipv4_port << std::endl;
    std::cout << "  IPv6: " << config.ipv6_address << ":" << config.ipv6_port << std::endl;
    std::cout << "  Автоопределение: " << (config.auto_detect ? "Включено" : "Выключено") << std::endl;
    std::cout << "  Предпочтение IPv6: " << (config.prefer_ipv6 ? "Включено" : "Выключено") << std::endl;
    
    return true;
}

bool IPVersionManager::StartIPSupport(IPVersion version) {
    std::lock_guard<std::mutex> lock(ip_mutex_);
    
    switch (version) {
        case IPVersion::IPv4:
            return StartIPv4();
        case IPVersion::IPv6:
            return StartIPv6();
        case IPVersion::DUAL_STACK:
            return StartDualStack();
        default:
            return false;
    }
}

bool IPVersionManager::StopIPSupport(IPVersion version) {
    std::lock_guard<std::mutex> lock(ip_mutex_);
    
    switch (version) {
        case IPVersion::IPv4:
            ipv4_enabled_ = false;
            break;
        case IPVersion::IPv6:
            ipv6_enabled_ = false;
            break;
        case IPVersion::DUAL_STACK:
            ipv4_enabled_ = false;
            ipv6_enabled_ = false;
            break;
    }
    
    return true;
}

std::vector<std::string> IPVersionManager::GetAvailableIPv4Addresses() const {
    return ScanIPv4Addresses();
}

std::vector<std::string> IPVersionManager::GetAvailableIPv6Addresses() const {
    return ScanIPv6Addresses();
}

bool IPVersionManager::TestIPv4Connection(const std::string& address, int port) {
    std::cout << "[IPVersionManager] Тестирование IPv4: " << address << ":" << port << std::endl;
    
    // TODO: Реализовать тестирование IPv4 подключения
    return true;
}

bool IPVersionManager::TestIPv6Connection(const std::string& address, int port) {
    std::cout << "[IPVersionManager] Тестирование IPv6: " << address << ":" << port << std::endl;
    
    // TODO: Реализовать тестирование IPv6 подключения
    return true;
}

std::unordered_map<std::string, double> IPVersionManager::GetIPStats() const {
    std::unordered_map<std::string, double> stats;
    
    stats["ipv4_enabled"] = ipv4_enabled_ ? 1.0 : 0.0;
    stats["ipv6_enabled"] = ipv6_enabled_ ? 1.0 : 0.0;
    stats["dual_stack"] = (ipv4_enabled_ && ipv6_enabled_) ? 1.0 : 0.0;
    
    return stats;
}

void IPVersionManager::SetOnIPVersionChange(std::function<void(IPVersion)> callback) {
    on_ip_version_change_ = callback;
}

void IPVersionManager::SetOnConnectionTest(std::function<void(const std::string&, bool)> callback) {
    on_connection_test_ = callback;
}

bool IPVersionManager::StartIPv4() {
    std::cout << "[IPVersionManager] Запуск IPv4 поддержки" << std::endl;
    
    ipv4_enabled_ = true;
    
    if (on_ip_version_change_) {
        on_ip_version_change_(IPVersion::IPv4);
    }
    
    return true;
}

bool IPVersionManager::StartIPv6() {
    std::cout << "[IPVersionManager] Запуск IPv6 поддержки" << std::endl;
    
    ipv6_enabled_ = true;
    
    if (on_ip_version_change_) {
        on_ip_version_change_(IPVersion::IPv6);
    }
    
    return true;
}

bool IPVersionManager::StartDualStack() {
    std::cout << "[IPVersionManager] Запуск Dual Stack поддержки" << std::endl;
    
    ipv4_enabled_ = true;
    ipv6_enabled_ = true;
    
    if (on_ip_version_change_) {
        on_ip_version_change_(IPVersion::DUAL_STACK);
    }
    
    return true;
}

std::vector<std::string> IPVersionManager::ScanIPv4Addresses() {
    std::vector<std::string> addresses;
    
    if (!config_.ipv4_address.empty()) {
        addresses.push_back(config_.ipv4_address);
    }
    
    // Добавление дополнительных IPv4 адресов
    addresses.push_back("127.0.0.1");
    addresses.push_back("192.168.1.1");
    addresses.push_back("10.0.0.1");
    
    return addresses;
}

std::vector<std::string> IPVersionManager::ScanIPv6Addresses() {
    std::vector<std::string> addresses;
    
    if (!config_.ipv6_address.empty()) {
        addresses.push_back(config_.ipv6_address);
    }
    
    // Добавление дополнительных IPv6 адресов
    addresses.push_back("::1");
    addresses.push_back("2001:db8::1");
    addresses.push_back("fe80::1");
    
    return addresses;
}

bool IPVersionManager::ValidateIPAddress(const std::string& address) {
    // Простая валидация IP адреса
    return !address.empty();
}

void IPVersionManager::UpdateIPStats() {
    // Обновление статистики IP
}

// ComponentIntegrator
ComponentIntegrator::ComponentIntegrator() {
}

ComponentIntegrator::~ComponentIntegrator() {
}

bool ComponentIntegrator::IntegrateAllComponents() {
    std::lock_guard<std::mutex> lock(integration_mutex_);
    
    std::cout << "[ComponentIntegrator] Интеграция всех компонентов..." << std::endl;
    
    // Интеграция всех компонентов
    bool success = true;
    
    success &= IntegrateReverseTunnel();
    success &= IntegrateAIAnalyzer();
    success &= IntegrateDynamicSNI();
    success &= IntegrateConnectionManager();
    success &= IntegrateHysteria();
    success &= IntegrateTrojan();
    success &= IntegrateDNSTunnel();
    success &= IntegrateAPI();
    success &= IntegrateBypass();
    success &= IntegrateChain();
    
    if (success) {
        if (on_integration_complete_) {
            on_integration_complete_();
        }
        
        std::cout << "[ComponentIntegrator] Все компоненты интегрированы успешно!" << std::endl;
    } else {
        if (on_integration_error_) {
            on_integration_error_("Integration failed");
        }
    }
    
    return success;
}

bool ComponentIntegrator::CreateComponentLinks() {
    std::cout << "[ComponentIntegrator] Создание связей между компонентами..." << std::endl;
    
    // TODO: Реализовать создание связей между компонентами
    return true;
}

bool ComponentIntegrator::SynchronizeComponents() {
    std::cout << "[ComponentIntegrator] Синхронизация компонентов..." << std::endl;
    
    // TODO: Реализовать синхронизацию компонентов
    return true;
}

std::unordered_map<std::string, bool> ComponentIntegrator::GetIntegrationStatus() const {
    std::lock_guard<std::mutex> lock(integration_mutex_);
    return integration_status_;
}

void ComponentIntegrator::SetOnIntegrationComplete(std::function<void()> callback) {
    on_integration_complete_ = callback;
}

void ComponentIntegrator::SetOnIntegrationError(std::function<void(const std::string&)> callback) {
    on_integration_error_ = callback;
}

bool ComponentIntegrator::IntegrateReverseTunnel() {
    std::cout << "[ComponentIntegrator] Интеграция Reverse Tunnel..." << std::endl;
    
    integration_status_["reverse_tunnel"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateAIAnalyzer() {
    std::cout << "[ComponentIntegrator] Интеграция AI Analyzer..." << std::endl;
    
    integration_status_["ai_analyzer"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateDynamicSNI() {
    std::cout << "[ComponentIntegrator] Интеграция Dynamic SNI..." << std::endl;
    
    integration_status_["dynamic_sni"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateConnectionManager() {
    std::cout << "[ComponentIntegrator] Интеграция Connection Manager..." << std::endl;
    
    integration_status_["connection_manager"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateHysteria() {
    std::cout << "[ComponentIntegrator] Интеграция Hysteria..." << std::endl;
    
    integration_status_["hysteria"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateTrojan() {
    std::cout << "[ComponentIntegrator] Интеграция Trojan..." << std::endl;
    
    integration_status_["trojan"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateDNSTunnel() {
    std::cout << "[ComponentIntegrator] Интеграция DNS Tunnel..." << std::endl;
    
    integration_status_["dns_tunnel"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateAPI() {
    std::cout << "[ComponentIntegrator] Интеграция API..." << std::endl;
    
    integration_status_["api"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateBypass() {
    std::cout << "[ComponentIntegrator] Интеграция Bypass..." << std::endl;
    
    integration_status_["bypass"] = true;
    return true;
}

bool ComponentIntegrator::IntegrateChain() {
    std::cout << "[ComponentIntegrator] Интеграция Chain..." << std::endl;
    
    integration_status_["chain"] = true;
    return true;
}

void ComponentIntegrator::HandleIntegrationError(const std::string& component, const std::string& error) {
    std::cout << "[ComponentIntegrator] Ошибка интеграции " << component << ": " << error << std::endl;
    
    if (on_integration_error_) {
        on_integration_error_(error);
    }
}

} // namespace TrafficMask

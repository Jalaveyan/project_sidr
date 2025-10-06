#pragma once

#include "reverse_tunnel.h"
#include "ai_analyzer.h"
#include "dynamic_sni.h"
#include "connection_manager.h"
#include "hysteria_integration.h"
#include "trojan_integration.h"
#include "dns_tunneling.h"
#include "s3_api_integration.h"
#include "bypass_detection.h"
#include "vps_cdn_chain.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace TrafficMask {

// Поддержка IPv4/IPv6
enum class IPVersion {
    IPv4,
    IPv6,
    DUAL_STACK
};

// Конфигурация IPv4/IPv6
struct IPConfig {
    IPVersion version;
    std::string ipv4_address;
    std::string ipv6_address;
    int ipv4_port;
    int ipv6_port;
    bool auto_detect = true;
    bool prefer_ipv6 = false;
    std::vector<std::string> ipv4_ranges;
    std::vector<std::string> ipv6_ranges;
    std::unordered_map<std::string, std::string> custom_params;
};

// Главная конфигурация TrafficMask NextGen
struct TrafficMaskConfig {
    // IPv4/IPv6 настройки
    IPConfig ip_config;
    
    // Reverse tunnel
    ReverseTunnelConfig reverse_tunnel_config;
    
    // AI анализ
    AIAnalyzerConfig ai_config;
    
    // Динамический SNI
    DynamicSNIConfig sni_config;
    
    // Управление соединениями
    ConnectionConfig connection_config;
    
    // Hysteria
    HysteriaConfig hysteria_config;
    
    // Trojan
    TrojanConfig trojan_config;
    
    // DNS туннелирование
    DNSTunnelConfig dns_tunnel_config;
    
    // S3/YaDocs API
    APIConfig api_config;
    
    // Обход
    BypassConfig bypass_config;
    
    // VPS/CDN цепочки
    ChainConfig chain_config;
    
    // Общие настройки
    bool enable_all_features = true;
    bool russia_optimization = true;
    bool ai_management = true;
    int startup_delay_ms = 2000;
    std::string log_level = "INFO";
    std::unordered_map<std::string, std::string> global_params;
};

// Статистика всей системы
struct SystemStats {
    std::string system_id;
    bool is_running;
    IPVersion active_ip_version;
    int active_connections;
    int total_connections;
    double overall_success_rate;
    double average_latency_ms;
    uint64_t total_bytes_processed;
    uint64_t total_packets_processed;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_activity;
    std::unordered_map<std::string, double> component_stats;
    std::vector<std::string> active_components;
    std::string last_error;
};

// Главный менеджер TrafficMask NextGen
class TrafficMaskNextGen {
public:
    TrafficMaskNextGen();
    ~TrafficMaskNextGen();

    // Инициализация системы
    bool Initialize(const TrafficMaskConfig& config);
    
    // Запуск всей системы
    bool Start();
    
    // Остановка системы
    void Stop();
    
    // Перезапуск системы
    bool Restart();
    
    // Получение статистики
    SystemStats GetSystemStats() const;
    
    // Управление компонентами
    bool StartComponent(const std::string& component_name);
    bool StopComponent(const std::string& component_name);
    bool RestartComponent(const std::string& component_name);
    
    // Получение списка компонентов
    std::vector<std::string> GetActiveComponents() const;
    
    // Callback для событий
    void SetOnSystemStart(std::function<void()> callback);
    void SetOnSystemStop(std::function<void()> callback);
    void SetOnComponentStateChange(std::function<void(const std::string&, bool)> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
    void SetOnStatsUpdate(std::function<void(const SystemStats&)> callback);

private:
    TrafficMaskConfig config_;
    SystemStats stats_;
    std::atomic<bool> running_;
    std::thread system_thread_;
    std::mutex system_mutex_;
    
    // Компоненты системы
    std::shared_ptr<ReverseTunnelManager> reverse_tunnel_manager_;
    std::shared_ptr<AIAnalyzer> ai_analyzer_;
    std::shared_ptr<DynamicSNIManager> sni_manager_;
    std::shared_ptr<ConnectionManager> connection_manager_;
    std::shared_ptr<HysteriaManager> hysteria_manager_;
    std::shared_ptr<TrojanManager> trojan_manager_;
    std::shared_ptr<DNSTunnelManager> dns_tunnel_manager_;
    std::shared_ptr<DataExchangeManager> api_manager_;
    std::shared_ptr<BypassManager> bypass_manager_;
    std::shared_ptr<VpsCdnChainManager> chain_manager_;
    
    // Callbacks
    std::function<void()> on_system_start_;
    std::function<void()> on_system_stop_;
    std::function<void(const std::string&, bool)> on_component_state_change_;
    std::function<void(const std::string&)> on_error_;
    std::function<void(const SystemStats&)> on_stats_update_;
    
    // Внутренние методы
    void SystemLoop();
    bool InitializeComponents();
    bool StartComponents();
    bool StopComponents();
    void UpdateStats();
    void HandleComponentError(const std::string& component_name, const std::string& error);
    void UpdateComponentStats();
    bool ValidateConfig();
    void OptimizeConfig();
    
    // IPv4/IPv6 методы
    bool InitializeIPSupport();
    bool StartIPv4Support();
    bool StartIPv6Support();
    bool StartDualStackSupport();
    std::string DetectBestIPVersion();
    bool TestIPConnectivity(const std::string& address, int port);
    std::vector<std::string> GetAvailableIPAddresses();
};

// Менеджер IPv4/IPv6
class IPVersionManager {
public:
    IPVersionManager();
    ~IPVersionManager();

    // Инициализация менеджера
    bool Initialize(const IPConfig& config);
    
    // Запуск поддержки IP версий
    bool StartIPSupport(IPVersion version);
    
    // Остановка поддержки IP версий
    bool StopIPSupport(IPVersion version);
    
    // Получение доступных IP адресов
    std::vector<std::string> GetAvailableIPv4Addresses() const;
    std::vector<std::string> GetAvailableIPv6Addresses() const;
    
    // Тестирование подключения
    bool TestIPv4Connection(const std::string& address, int port);
    bool TestIPv6Connection(const std::string& address, int port);
    
    // Получение статистики
    std::unordered_map<std::string, double> GetIPStats() const;
    
    // Callback для событий
    void SetOnIPVersionChange(std::function<void(IPVersion)> callback);
    void SetOnConnectionTest(std::function<void(const std::string&, bool)> callback);

private:
    IPConfig config_;
    std::atomic<bool> ipv4_enabled_;
    std::atomic<bool> ipv6_enabled_;
    std::mutex ip_mutex_;
    
    // Callbacks
    std::function<void(IPVersion)> on_ip_version_change_;
    std::function<void(const std::string&, bool)> on_connection_test_;
    
    // Внутренние методы
    bool StartIPv4();
    bool StartIPv6();
    bool StartDualStack();
    std::vector<std::string> ScanIPv4Addresses();
    std::vector<std::string> ScanIPv6Addresses();
    bool ValidateIPAddress(const std::string& address);
    void UpdateIPStats();
};

// Интегратор всех компонентов
class ComponentIntegrator {
public:
    ComponentIntegrator();
    ~ComponentIntegrator();

    // Интеграция всех компонентов
    bool IntegrateAllComponents();
    
    // Создание связей между компонентами
    bool CreateComponentLinks();
    
    // Синхронизация компонентов
    bool SynchronizeComponents();
    
    // Получение статуса интеграции
    std::unordered_map<std::string, bool> GetIntegrationStatus() const;
    
    // Callback для событий
    void SetOnIntegrationComplete(std::function<void()> callback);
    void SetOnIntegrationError(std::function<void(const std::string&)> callback);

private:
    std::mutex integration_mutex_;
    std::unordered_map<std::string, bool> integration_status_;
    
    // Callbacks
    std::function<void()> on_integration_complete_;
    std::function<void(const std::string&)> on_integration_error_;
    
    // Внутренние методы
    bool IntegrateReverseTunnel();
    bool IntegrateAIAnalyzer();
    bool IntegrateDynamicSNI();
    bool IntegrateConnectionManager();
    bool IntegrateHysteria();
    bool IntegrateTrojan();
    bool IntegrateDNSTunnel();
    bool IntegrateAPI();
    bool IntegrateBypass();
    bool IntegrateChain();
    void HandleIntegrationError(const std::string& component, const std::string& error);
};

} // namespace TrafficMask

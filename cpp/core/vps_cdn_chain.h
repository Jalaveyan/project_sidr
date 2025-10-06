#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <queue>

namespace TrafficMask {

// Типы узлов в цепочке
enum class ChainNodeType {
    VPS,           // VPS сервер
    CDN,           // CDN узел
    PROXY,         // Прокси сервер
    TUNNEL,        // Туннельный узел
    BRIDGE         // Мостовой узел
};

// Состояние узла
enum class NodeState {
    OFFLINE,       // Недоступен
    CONNECTING,    // Подключение
    ONLINE,        // Онлайн
    DEGRADED,      // Деградированная работа
    ERROR          // Ошибка
};

// Конфигурация узла
struct ChainNodeConfig {
    std::string node_id;
    std::string endpoint;
    int port;
    ChainNodeType type;
    std::string region;
    std::string provider;
    int priority = 1;
    double weight = 1.0;
    int timeout_ms = 30000;
    int retry_count = 3;
    bool auto_failover = true;
    std::unordered_map<std::string, std::string> custom_params;
};

// Статистика узла
struct NodeStats {
    std::string node_id;
    NodeState state;
    double latency_ms;
    double bandwidth_mbps;
    double success_rate;
    uint64_t bytes_transferred;
    int connection_count;
    std::chrono::system_clock::time_point last_activity;
    std::string last_error;
    std::unordered_map<std::string, double> custom_metrics;
};

// Конфигурация цепочки
struct ChainConfig {
    std::string chain_id;
    std::string name;
    std::vector<ChainNodeConfig> nodes;
    bool auto_optimization = true;
    bool load_balancing = true;
    bool failover_enabled = true;
    int max_retries = 3;
    double health_check_interval = 30.0; // секунды
    std::string optimization_strategy = "ai_driven";
    std::unordered_map<std::string, std::string> global_params;
};

// Статистика цепочки
struct ChainStats {
    std::string chain_id;
    bool is_active;
    int active_nodes;
    int total_nodes;
    double average_latency;
    double total_bandwidth;
    double overall_success_rate;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_optimization;
    std::vector<NodeStats> node_statistics;
};

// Менеджер цепочки VPS/CDN
class VpsCdnChainManager {
public:
    VpsCdnChainManager();
    ~VpsCdnChainManager();

    // Инициализация менеджера
    bool Initialize();
    
    // Создание цепочки
    std::string CreateChain(const ChainConfig& config);
    
    // Удаление цепочки
    bool RemoveChain(const std::string& chain_id);
    
    // Запуск цепочки
    bool StartChain(const std::string& chain_id);
    
    // Остановка цепочки
    bool StopChain(const std::string& chain_id);
    
    // Получение статистики цепочки
    ChainStats GetChainStats(const std::string& chain_id) const;
    
    // Получение всех цепочек
    std::vector<std::string> GetAllChains() const;
    
    // Оптимизация цепочки
    bool OptimizeChain(const std::string& chain_id);
    
    // Переключение на резервную цепочку
    bool SwitchToBackupChain(const std::string& chain_id);
    
    // Callback для событий
    void SetOnChainStateChange(std::function<void(const std::string&, bool)> callback);
    void SetOnNodeStateChange(std::function<void(const std::string&, const std::string&, NodeState)> callback);
    void SetOnOptimizationComplete(std::function<void(const std::string&, const ChainStats&)> callback);

private:
    std::unordered_map<std::string, ChainConfig> chains_;
    std::unordered_map<std::string, ChainStats> chain_stats_;
    std::unordered_map<std::string, std::vector<std::string>> chain_nodes_;
    std::mutex chains_mutex_;
    
    std::atomic<bool> running_;
    std::thread optimization_thread_;
    std::thread monitoring_thread_;
    
    // Callbacks
    std::function<void(const std::string&, bool)> on_chain_state_change_;
    std::function<void(const std::string&, const std::string&, NodeState)> on_node_state_change_;
    std::function<void(const std::string&, const ChainStats&)> on_optimization_complete_;
    
    // Внутренние методы
    void OptimizationLoop();
    void MonitoringLoop();
    bool ValidateChainConfig(const ChainConfig& config);
    bool StartChainNodes(const std::string& chain_id);
    bool StopChainNodes(const std::string& chain_id);
    void UpdateChainStats(const std::string& chain_id);
    std::string SelectBestNode(const std::string& chain_id);
    bool PerformFailover(const std::string& chain_id, const std::string& failed_node);
    ChainStats CalculateChainStats(const std::string& chain_id);
};

// Оптимизатор цепочки
class ChainOptimizer {
public:
    ChainOptimizer();
    ~ChainOptimizer();

    // Оптимизация цепочки
    ChainConfig OptimizeChain(const ChainConfig& config, const ChainStats& stats);
    
    // Выбор лучших узлов
    std::vector<ChainNodeConfig> SelectBestNodes(const std::vector<ChainNodeConfig>& nodes, int count);
    
    // Анализ производительности
    double AnalyzePerformance(const ChainStats& stats);
    
    // Рекомендации по улучшению
    std::vector<std::string> GetOptimizationRecommendations(const ChainStats& stats);

private:
    std::mutex optimizer_mutex_;
    
    // Методы оптимизации
    double CalculateNodeScore(const ChainNodeConfig& node, const NodeStats& stats);
    std::vector<ChainNodeConfig> SortNodesByScore(const std::vector<ChainNodeConfig>& nodes, const std::vector<NodeStats>& stats);
    bool ShouldReplaceNode(const NodeStats& node_stats);
    ChainNodeConfig FindReplacementNode(const std::vector<ChainNodeConfig>& available_nodes, const NodeStats& current_stats);
};

// Монитор цепочки
class ChainMonitor {
public:
    ChainMonitor();
    ~ChainMonitor();

    // Мониторинг цепочки
    void StartMonitoring(const std::string& chain_id);
    void StopMonitoring(const std::string& chain_id);
    
    // Проверка здоровья узлов
    bool CheckNodeHealth(const std::string& node_id);
    
    // Получение метрик
    std::unordered_map<std::string, double> GetNodeMetrics(const std::string& node_id);
    
    // Callback для событий
    void SetOnNodeHealthChange(std::function<void(const std::string&, bool)> callback);
    void SetOnPerformanceAlert(std::function<void(const std::string&, const std::string&)> callback);

private:
    std::unordered_set<std::string> monitored_chains_;
    std::mutex monitor_mutex_;
    std::thread monitor_thread_;
    std::atomic<bool> running_;
    
    // Callbacks
    std::function<void(const std::string&, bool)> on_node_health_change_;
    std::function<void(const std::string&, const std::string&)> on_performance_alert_;
    
    // Внутренние методы
    void MonitorLoop();
    bool PerformHealthCheck(const std::string& node_id);
    std::unordered_map<std::string, double> CollectNodeMetrics(const std::string& node_id);
    void AnalyzePerformanceTrends(const std::string& chain_id);
};

// Менеджер резервных цепочек
class BackupChainManager {
public:
    BackupChainManager();
    ~BackupChainManager();

    // Добавление резервной цепочки
    bool AddBackupChain(const std::string& primary_chain_id, const std::string& backup_chain_id);
    
    // Удаление резервной цепочки
    bool RemoveBackupChain(const std::string& primary_chain_id);
    
    // Переключение на резервную цепочку
    bool SwitchToBackup(const std::string& primary_chain_id);
    
    // Возврат на основную цепочку
    bool SwitchToPrimary(const std::string& primary_chain_id);
    
    // Получение резервных цепочек
    std::vector<std::string> GetBackupChains(const std::string& primary_chain_id) const;

private:
    std::unordered_map<std::string, std::vector<std::string>> backup_chains_;
    std::unordered_map<std::string, std::string> active_backups_;
    std::mutex backup_mutex_;
    
    // Внутренние методы
    bool ValidateBackupChain(const std::string& backup_chain_id);
    void NotifyBackupSwitch(const std::string& primary_chain_id, const std::string& backup_chain_id);
};

// Интеграция с российскими сервисами
class RussiaServiceIntegration {
public:
    RussiaServiceIntegration();
    ~RussiaServiceIntegration();

    // Адаптация цепочки для России
    ChainConfig AdaptChainForRussia(const ChainConfig& config);
    
    // Добавление российских узлов
    std::vector<ChainNodeConfig> AddRussiaNodes(const std::vector<ChainNodeConfig>& nodes);
    
    // Оптимизация для российских провайдеров
    bool OptimizeForRussiaProviders(const std::string& chain_id);
    
    // Получение российских CDN
    std::vector<ChainNodeConfig> GetRussiaCDNNodes();

private:
    std::vector<std::string> russia_providers_;
    std::vector<std::string> russia_regions_;
    std::vector<std::string> russia_cdn_providers_;
    
    // Методы адаптации
    ChainNodeConfig CreateRussiaNode(const std::string& provider, const std::string& region);
    bool IsRussiaProvider(const std::string& provider);
    std::string SelectBestRussiaRegion();
};

} // namespace TrafficMask

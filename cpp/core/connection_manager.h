#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <queue>

namespace TrafficMask {

// Типы соединений
enum class ConnectionType {
    TCP,
    UDP,
    WEBSOCKET,
    HTTP2,
    QUIC,
    HYSTERIA,
    TROJAN
};

// Состояние соединения
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    ERROR,
    TERMINATED
};

// Приоритет соединения
enum class ConnectionPriority {
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    CRITICAL = 4
};

// Конфигурация соединения
struct ConnectionConfig {
    std::string endpoint;
    int port;
    ConnectionType type;
    ConnectionPriority priority;
    int timeout_ms = 30000;
    int retry_count = 3;
    int retry_delay_ms = 1000;
    bool auto_reconnect = true;
    bool ai_management = true;
    std::string encryption_key;
    std::unordered_map<std::string, std::string> custom_params;
};

// Статистика соединения
struct ConnectionStats {
    std::string connection_id;
    ConnectionState state;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_activity;
    uint64_t bytes_sent = 0;
    uint64_t bytes_received = 0;
    uint64_t packets_sent = 0;
    uint64_t packets_received = 0;
    double latency_ms = 0.0;
    double success_rate = 0.0;
    int reconnect_count = 0;
    std::string last_error;
};

// AI агент для управления соединениями
class ConnectionAIAgent {
public:
    ConnectionAIAgent();
    ~ConnectionAIAgent();

    // Анализ состояния соединения
    double AnalyzeConnectionHealth(const ConnectionStats& stats);
    
    // Рекомендация действий для соединения
    std::vector<std::string> RecommendActions(const ConnectionStats& stats);
    
    // Оптимизация параметров соединения
    ConnectionConfig OptimizeConnection(const ConnectionConfig& config, const ConnectionStats& stats);
    
    // Предсказание проблем соединения
    bool PredictConnectionFailure(const ConnectionStats& stats);
    
    // Обновление модели на основе результатов
    void UpdateModel(const std::string& connection_id, bool success, double performance);

private:
    std::unordered_map<std::string, double> connection_performance_;
    std::unordered_map<std::string, int> connection_failure_count_;
    std::mutex model_mutex_;
    
    double CalculateHealthScore(const ConnectionStats& stats);
    std::vector<std::string> GenerateRecommendations(double health_score);
    ConnectionConfig ApplyOptimizations(const ConnectionConfig& config, double health_score);
};

// Менеджер соединений
class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager();

    // Инициализация
    bool Initialize();
    
    // Создание соединения
    std::string CreateConnection(const ConnectionConfig& config);
    
    // Удаление соединения
    bool RemoveConnection(const std::string& connection_id);
    
    // Получение соединения
    std::shared_ptr<ConnectionStats> GetConnection(const std::string& connection_id);
    
    // Получение всех соединений
    std::vector<std::shared_ptr<ConnectionStats>> GetAllConnections();
    
    // Управление соединением
    bool Connect(const std::string& connection_id);
    bool Disconnect(const std::string& connection_id);
    bool Reconnect(const std::string& connection_id);
    
    // AI управление
    void EnableAIManagement(bool enable);
    void SetAIAgent(std::shared_ptr<ConnectionAIAgent> agent);
    
    // Callback для событий
    void SetOnConnectionStateChange(std::function<void(const std::string&, ConnectionState)> callback);
    void SetOnConnectionError(std::function<void(const std::string&, const std::string&)> callback);
    void SetOnAIAnalysis(std::function<void(const std::string&, const std::vector<std::string>&)> callback);

private:
    std::unordered_map<std::string, std::shared_ptr<ConnectionStats>> connections_;
    std::unordered_map<std::string, ConnectionConfig> connection_configs_;
    std::mutex connections_mutex_;
    
    std::shared_ptr<ConnectionAIAgent> ai_agent_;
    std::atomic<bool> ai_management_enabled_;
    std::thread ai_worker_thread_;
    std::atomic<bool> running_;
    
    // Callbacks
    std::function<void(const std::string&, ConnectionState)> on_state_change_;
    std::function<void(const std::string&, const std::string&)> on_error_;
    std::function<void(const std::string&, const std::vector<std::string>&)> on_ai_analysis_;
    
    // Внутренние методы
    void AIWorkerLoop();
    void UpdateConnectionStats(const std::string& connection_id, const ConnectionStats& stats);
    bool PerformConnection(const std::string& connection_id);
    bool PerformDisconnection(const std::string& connection_id);
    bool PerformReconnection(const std::string& connection_id);
    void HandleConnectionError(const std::string& connection_id, const std::string& error);
    void ProcessAIAnalysis(const std::string& connection_id);
};

// Пул соединений
class ConnectionPool {
public:
    ConnectionPool();
    ~ConnectionPool();

    // Инициализация пула
    bool Initialize(int min_connections = 5, int max_connections = 50);
    
    // Получение соединения из пула
    std::string GetConnection();
    
    // Возврат соединения в пул
    void ReturnConnection(const std::string& connection_id);
    
    // Создание новых соединений
    void CreateConnections(int count);
    
    // Очистка неиспользуемых соединений
    void CleanupIdleConnections();
    
    // Получение статистики пула
    std::unordered_map<std::string, int> GetPoolStatistics();

private:
    std::queue<std::string> available_connections_;
    std::unordered_set<std::string> used_connections_;
    std::mutex pool_mutex_;
    
    int min_connections_;
    int max_connections_;
    std::atomic<int> current_connections_;
    
    std::string CreateNewConnection();
    void RemoveConnection(const std::string& connection_id);
    bool IsConnectionIdle(const std::string& connection_id);
};

// Менеджер цепочки соединений (VPS/CDN)
class ConnectionChainManager {
public:
    ConnectionChainManager();
    ~ConnectionChainManager();

    // Создание цепочки соединений
    std::string CreateChain(const std::vector<ConnectionConfig>& chain_config);
    
    // Получение цепочки
    std::vector<std::string> GetChain(const std::string& chain_id);
    
    // Управление цепочкой
    bool StartChain(const std::string& chain_id);
    bool StopChain(const std::string& chain_id);
    bool SwitchChain(const std::string& chain_id, const std::string& new_chain_id);
    
    // Автоматическое переключение
    void EnableAutoSwitch(bool enable);
    void SetSwitchThreshold(double threshold);

private:
    std::unordered_map<std::string, std::vector<std::string>> chains_;
    std::unordered_map<std::string, std::string> active_chains_;
    std::mutex chains_mutex_;
    
    bool auto_switch_enabled_;
    double switch_threshold_;
    
    std::string SelectBestChain();
    bool EvaluateChainPerformance(const std::string& chain_id);
    void PerformChainSwitch(const std::string& from_chain, const std::string& to_chain);
};

} // namespace TrafficMask

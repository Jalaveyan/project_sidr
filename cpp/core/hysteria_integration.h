#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace TrafficMask {

// Конфигурация Hysteria
struct HysteriaConfig {
    std::string server_address;
    int server_port;
    std::string auth_key;
    std::string obfs_password;
    int bandwidth_mbps = 100;
    int mtu = 1200;
    bool fast_open = true;
    bool congestion_control = true;
    std::string congestion_algorithm = "bbr";
    int timeout_seconds = 30;
    int retry_count = 3;
    bool auto_reconnect = true;
    std::unordered_map<std::string, std::string> custom_params;
};

// Статистика Hysteria
struct HysteriaStats {
    std::string connection_id;
    bool is_connected;
    double upload_speed_mbps;
    double download_speed_mbps;
    uint64_t bytes_uploaded;
    uint64_t bytes_downloaded;
    double latency_ms;
    double packet_loss_rate;
    int reconnect_count;
    std::string last_error;
    std::chrono::system_clock::time_point last_activity;
};

// Hysteria клиент
class HysteriaClient {
public:
    HysteriaClient();
    ~HysteriaClient();

    // Инициализация с конфигурацией
    bool Initialize(const HysteriaConfig& config);
    
    // Подключение к серверу
    bool Connect();
    
    // Отключение от сервера
    void Disconnect();
    
    // Отправка данных
    bool SendData(const std::vector<uint8_t>& data);
    
    // Получение данных
    std::vector<uint8_t> ReceiveData();
    
    // Получение статистики
    HysteriaStats GetStats() const;
    
    // Callback для событий
    void SetOnConnect(std::function<void()> callback);
    void SetOnDisconnect(std::function<void()> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
    void SetOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback);

private:
    HysteriaConfig config_;
    HysteriaStats stats_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::mutex stats_mutex_;
    
    // Callbacks
    std::function<void()> on_connect_;
    std::function<void()> on_disconnect_;
    std::function<void(const std::string&)> on_error_;
    std::function<void(const std::vector<uint8_t>&)> on_data_received_;
    
    // Внутренние методы
    void WorkerLoop();
    bool EstablishConnection();
    void UpdateStats();
    void HandleError(const std::string& error);
};

// Hysteria сервер
class HysteriaServer {
public:
    HysteriaServer();
    ~HysteriaServer();

    // Инициализация сервера
    bool Initialize(const HysteriaConfig& config);
    
    // Запуск сервера
    bool Start();
    
    // Остановка сервера
    void Stop();
    
    // Получение статистики сервера
    std::unordered_map<std::string, HysteriaStats> GetClientStats() const;
    
    // Callback для событий
    void SetOnClientConnect(std::function<void(const std::string&)> callback);
    void SetOnClientDisconnect(std::function<void(const std::string&)> callback);
    void SetOnDataReceived(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback);

private:
    HysteriaConfig config_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::unordered_map<std::string, HysteriaStats> client_stats_;
    std::mutex clients_mutex_;
    
    // Callbacks
    std::function<void(const std::string&)> on_client_connect_;
    std::function<void(const std::string&)> on_client_disconnect_;
    std::function<void(const std::string&, const std::vector<uint8_t>&)> on_data_received_;
    
    // Внутренние методы
    void ServerLoop();
    void HandleClient(const std::string& client_id);
    void UpdateClientStats(const std::string& client_id, const HysteriaStats& stats);
};

// Менеджер Hysteria соединений
class HysteriaManager {
public:
    HysteriaManager();
    ~HysteriaManager();

    // Инициализация менеджера
    bool Initialize();
    
    // Создание клиента
    std::string CreateClient(const HysteriaConfig& config);
    
    // Создание сервера
    std::string CreateServer(const HysteriaConfig& config);
    
    // Управление соединениями
    bool StartClient(const std::string& client_id);
    bool StopClient(const std::string& client_id);
    bool StartServer(const std::string& server_id);
    bool StopServer(const std::string& server_id);
    
    // Получение статистики
    HysteriaStats GetClientStats(const std::string& client_id) const;
    std::unordered_map<std::string, HysteriaStats> GetServerStats(const std::string& server_id) const;
    
    // Управление полосой пропускания
    bool SetBandwidth(const std::string& connection_id, int bandwidth_mbps);
    
    // Оптимизация параметров
    bool OptimizeConnection(const std::string& connection_id);

private:
    std::unordered_map<std::string, std::shared_ptr<HysteriaClient>> clients_;
    std::unordered_map<std::string, std::shared_ptr<HysteriaServer>> servers_;
    std::mutex clients_mutex_;
    std::mutex servers_mutex_;
    
    // Внутренние методы
    std::string GenerateConnectionId();
    bool ValidateConfig(const HysteriaConfig& config);
    void OptimizeConfig(HysteriaConfig& config);
};

// Интеграция с TrafficMask
class HysteriaTrafficMaskIntegration {
public:
    HysteriaTrafficMaskIntegration();
    ~HysteriaTrafficMaskIntegration();

    // Интеграция с reverse tunnel
    bool IntegrateWithReverseTunnel(const std::string& hysteria_connection_id);
    
    // Интеграция с AI анализом
    bool IntegrateWithAIAnalysis(const std::string& hysteria_connection_id);
    
    // Интеграция с динамическим SNI
    bool IntegrateWithDynamicSNI(const std::string& hysteria_connection_id);
    
    // Адаптация для российских сервисов
    bool AdaptForRussiaServices(const std::string& hysteria_connection_id);
    
    // Получение метрик для AI
    std::unordered_map<std::string, double> GetAIMetrics(const std::string& hysteria_connection_id);

private:
    std::shared_ptr<HysteriaManager> hysteria_manager_;
    std::mutex integration_mutex_;
    
    // Методы адаптации
    HysteriaConfig AdaptConfigForRussia(const HysteriaConfig& config);
    bool ApplyRussiaOptimizations(const std::string& connection_id);
    std::unordered_map<std::string, double> ExtractMetrics(const HysteriaStats& stats);
};

} // namespace TrafficMask

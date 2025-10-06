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

namespace TrafficMask {

// Конфигурация Trojan
struct TrojanConfig {
    std::string server_address;
    int server_port;
    std::string password;
    std::string method = "aes-256-gcm";
    std::string obfs = "tls";
    std::string obfs_param;
    std::string sni;
    std::string alpn;
    std::string path = "/";
    bool insecure = false;
    int timeout_seconds = 30;
    int retry_count = 3;
    bool auto_reconnect = true;
    std::unordered_map<std::string, std::string> custom_params;
};

// Статистика Trojan
struct TrojanStats {
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
    std::unordered_map<std::string, double> custom_metrics;
};

// Trojan клиент
class TrojanClient {
public:
    TrojanClient();
    ~TrojanClient();

    // Инициализация с конфигурацией
    bool Initialize(const TrojanConfig& config);
    
    // Подключение к серверу
    bool Connect();
    
    // Отключение от сервера
    void Disconnect();
    
    // Отправка данных
    bool SendData(const std::vector<uint8_t>& data);
    
    // Получение данных
    std::vector<uint8_t> ReceiveData();
    
    // Получение статистики
    TrojanStats GetStats() const;
    
    // Callback для событий
    void SetOnConnect(std::function<void()> callback);
    void SetOnDisconnect(std::function<void()> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
    void SetOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback);

private:
    TrojanConfig config_;
    TrojanStats stats_;
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
    bool ValidateConfig();
};

// Trojan сервер
class TrojanServer {
public:
    TrojanServer();
    ~TrojanServer();

    // Инициализация сервера
    bool Initialize(const TrojanConfig& config);
    
    // Запуск сервера
    bool Start();
    
    // Остановка сервера
    void Stop();
    
    // Получение статистики сервера
    std::unordered_map<std::string, TrojanStats> GetClientStats() const;
    
    // Callback для событий
    void SetOnClientConnect(std::function<void(const std::string&)> callback);
    void SetOnClientDisconnect(std::function<void(const std::string&)> callback);
    void SetOnDataReceived(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback);

private:
    TrojanConfig config_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::unordered_map<std::string, TrojanStats> client_stats_;
    std::mutex clients_mutex_;
    
    // Callbacks
    std::function<void(const std::string&)> on_client_connect_;
    std::function<void(const std::string&)> on_client_disconnect_;
    std::function<void(const std::string&, const std::vector<uint8_t>&)> on_data_received_;
    
    // Внутренние методы
    void ServerLoop();
    void HandleClient(const std::string& client_id);
    void UpdateClientStats(const std::string& client_id, const TrojanStats& stats);
};

// Менеджер Trojan соединений
class TrojanManager {
public:
    TrojanManager();
    ~TrojanManager();

    // Инициализация менеджера
    bool Initialize();
    
    // Создание клиента
    std::string CreateClient(const TrojanConfig& config);
    
    // Создание сервера
    std::string CreateServer(const TrojanConfig& config);
    
    // Управление соединениями
    bool StartClient(const std::string& client_id);
    bool StopClient(const std::string& client_id);
    bool StartServer(const std::string& server_id);
    bool StopServer(const std::string& server_id);
    
    // Получение статистики
    TrojanStats GetClientStats(const std::string& client_id) const;
    std::unordered_map<std::string, TrojanStats> GetServerStats(const std::string& server_id) const;
    
    // Управление паролем
    bool ChangePassword(const std::string& connection_id, const std::string& new_password);
    
    // Оптимизация соединения
    bool OptimizeConnection(const std::string& connection_id);

private:
    std::unordered_map<std::string, std::shared_ptr<TrojanClient>> clients_;
    std::unordered_map<std::string, std::shared_ptr<TrojanServer>> servers_;
    std::mutex clients_mutex_;
    std::mutex servers_mutex_;
    
    // Внутренние методы
    std::string GenerateConnectionId();
    bool ValidateConfig(const TrojanConfig& config);
    void OptimizeConfig(TrojanConfig& config);
};

// Интеграция Trojan с TrafficMask
class TrojanTrafficMaskIntegration {
public:
    TrojanTrafficMaskIntegration();
    ~TrojanTrafficMaskIntegration();

    // Интеграция с reverse tunnel
    bool IntegrateWithReverseTunnel(const std::string& trojan_connection_id);
    
    // Интеграция с AI анализом
    bool IntegrateWithAIAnalysis(const std::string& trojan_connection_id);
    
    // Интеграция с динамическим SNI
    bool IntegrateWithDynamicSNI(const std::string& trojan_connection_id);
    
    // Адаптация для российских сервисов
    bool AdaptForRussiaServices(const std::string& trojan_connection_id);
    
    // Получение метрик для AI
    std::unordered_map<std::string, double> GetAIMetrics(const std::string& trojan_connection_id);
    
    // Создание Trojan конфигурации для России
    TrojanConfig CreateRussiaTrojanConfig();

private:
    std::shared_ptr<TrojanManager> trojan_manager_;
    std::mutex integration_mutex_;
    
    // Методы адаптации
    TrojanConfig AdaptConfigForRussia(const TrojanConfig& config);
    bool ApplyRussiaOptimizations(const std::string& connection_id);
    std::unordered_map<std::string, double> ExtractMetrics(const TrojanStats& stats);
    std::string GenerateRussiaPassword();
    std::string SelectRussiaSNI();
};

// Trojan обфускация
class TrojanObfuscation {
public:
    TrojanObfuscation();
    ~TrojanObfuscation();

    // Обфускация трафика
    std::vector<uint8_t> ObfuscateData(const std::vector<uint8_t>& data);
    
    // Деобфускация трафика
    std::vector<uint8_t> DeobfuscateData(const std::vector<uint8_t>& data);
    
    // Генерация обфускации
    std::string GenerateObfuscationKey();
    
    // Анализ обфускации
    double AnalyzeObfuscationQuality(const std::vector<uint8_t>& data);

private:
    std::string obfuscation_key_;
    std::mutex obfuscation_mutex_;
    
    // Методы обфускации
    std::vector<uint8_t> ApplyXOR(const std::vector<uint8_t>& data, const std::string& key);
    std::vector<uint8_t> ApplyBase64(const std::vector<uint8_t>& data);
    std::vector<uint8_t> ApplyCompression(const std::vector<uint8_t>& data);
    std::string GenerateRandomKey();
};

// Trojan шифрование
class TrojanEncryption {
public:
    TrojanEncryption();
    ~TrojanEncryption();

    // Шифрование данных
    std::vector<uint8_t> EncryptData(const std::vector<uint8_t>& data, const std::string& password);
    
    // Расшифровка данных
    std::vector<uint8_t> DecryptData(const std::vector<uint8_t>& data, const std::string& password);
    
    // Генерация ключа
    std::string GenerateEncryptionKey();
    
    // Валидация пароля
    bool ValidatePassword(const std::string& password);

private:
    std::mutex encryption_mutex_;
    
    // Методы шифрования
    std::vector<uint8_t> ApplyAES256GCM(const std::vector<uint8_t>& data, const std::string& key);
    std::vector<uint8_t> ApplyChaCha20Poly1305(const std::vector<uint8_t>& data, const std::string& key);
    std::string DeriveKey(const std::string& password);
    std::string GenerateRandomIV();
};

} // namespace TrafficMask

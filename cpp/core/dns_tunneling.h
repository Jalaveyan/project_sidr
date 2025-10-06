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

// Типы DNS туннелирования
enum class DNSTunnelType {
    TXT_RECORD,     // TXT записи
    A_RECORD,       // A записи
    AAAA_RECORD,    // AAAA записи
    CNAME_RECORD,   // CNAME записи
    MX_RECORD,      // MX записи
    MIXED           // Смешанный тип
};

// Состояние DNS туннеля
enum class DNSTunnelState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    ERROR,
    BLOCKED
};

// Конфигурация DNS туннеля
struct DNSTunnelConfig {
    std::string domain;
    std::string dns_server;
    DNSTunnelType tunnel_type;
    int chunk_size = 64;
    int max_retries = 3;
    int timeout_seconds = 30;
    bool compression = true;
    bool encryption = true;
    std::string encryption_key;
    std::string obfuscation_method;
    bool auto_reconnect = true;
    std::unordered_map<std::string, std::string> custom_params;
};

// Статистика DNS туннеля
struct DNSTunnelStats {
    std::string tunnel_id;
    DNSTunnelState state;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t queries_sent;
    uint64_t queries_received;
    double success_rate;
    double average_latency_ms;
    int reconnect_count;
    std::string last_error;
    std::chrono::system_clock::time_point last_activity;
    std::unordered_map<std::string, double> custom_metrics;
};

// DNS туннель клиент
class DNSTunnelClient {
public:
    DNSTunnelClient();
    ~DNSTunnelClient();

    // Инициализация с конфигурацией
    bool Initialize(const DNSTunnelConfig& config);
    
    // Подключение к туннелю
    bool Connect();
    
    // Отключение от туннеля
    void Disconnect();
    
    // Отправка данных через DNS
    bool SendData(const std::vector<uint8_t>& data);
    
    // Получение данных через DNS
    std::vector<uint8_t> ReceiveData();
    
    // Получение статистики
    DNSTunnelStats GetStats() const;
    
    // Callback для событий
    void SetOnConnect(std::function<void()> callback);
    void SetOnDisconnect(std::function<void()> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
    void SetOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback);

private:
    DNSTunnelConfig config_;
    DNSTunnelStats stats_;
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
    bool EstablishTunnel();
    void UpdateStats();
    void HandleError(const std::string& error);
    bool ValidateConfig();
    
    // DNS операции
    bool SendDNSQuery(const std::string& query);
    std::string ReceiveDNSResponse();
    std::vector<std::string> ChunkData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> ReassembleData(const std::vector<std::string>& chunks);
    std::string EncodeData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecodeData(const std::string& encoded);
};

// DNS туннель сервер
class DNSTunnelServer {
public:
    DNSTunnelServer();
    ~DNSTunnelServer();

    // Инициализация сервера
    bool Initialize(const DNSTunnelConfig& config);
    
    // Запуск сервера
    bool Start();
    
    // Остановка сервера
    void Stop();
    
    // Получение статистики сервера
    std::unordered_map<std::string, DNSTunnelStats> GetClientStats() const;
    
    // Callback для событий
    void SetOnClientConnect(std::function<void(const std::string&)> callback);
    void SetOnClientDisconnect(std::function<void(const std::string&)> callback);
    void SetOnDataReceived(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback);

private:
    DNSTunnelConfig config_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::unordered_map<std::string, DNSTunnelStats> client_stats_;
    std::mutex clients_mutex_;
    
    // Callbacks
    std::function<void(const std::string&)> on_client_connect_;
    std::function<void(const std::string&)> on_client_disconnect_;
    std::function<void(const std::string&, const std::vector<uint8_t>&)> on_data_received_;
    
    // Внутренние методы
    void ServerLoop();
    void HandleClient(const std::string& client_id);
    void UpdateClientStats(const std::string& client_id, const DNSTunnelStats& stats);
    void ProcessDNSQuery(const std::string& query);
    std::string GenerateDNSResponse(const std::string& query);
};

// Менеджер DNS туннелей
class DNSTunnelManager {
public:
    DNSTunnelManager();
    ~DNSTunnelManager();

    // Инициализация менеджера
    bool Initialize();
    
    // Создание клиента
    std::string CreateClient(const DNSTunnelConfig& config);
    
    // Создание сервера
    std::string CreateServer(const DNSTunnelConfig& config);
    
    // Управление туннелями
    bool StartTunnel(const std::string& tunnel_id);
    bool StopTunnel(const std::string& tunnel_id);
    
    // Получение статистики
    DNSTunnelStats GetTunnelStats(const std::string& tunnel_id) const;
    
    // Оптимизация туннеля
    bool OptimizeTunnel(const std::string& tunnel_id);

private:
    std::unordered_map<std::string, std::shared_ptr<DNSTunnelClient>> clients_;
    std::unordered_map<std::string, std::shared_ptr<DNSTunnelServer>> servers_;
    std::mutex clients_mutex_;
    std::mutex servers_mutex_;
    
    // Внутренние методы
    std::string GenerateTunnelId();
    bool ValidateConfig(const DNSTunnelConfig& config);
    void OptimizeConfig(DNSTunnelConfig& config);
};

// DNS обфускация
class DNSObfuscation {
public:
    DNSObfuscation();
    ~DNSObfuscation();

    // Обфускация DNS запроса
    std::string ObfuscateQuery(const std::string& query);
    
    // Деобфускация DNS запроса
    std::string DeobfuscateQuery(const std::string& obfuscated_query);
    
    // Генерация случайного поддомена
    std::string GenerateRandomSubdomain();
    
    // Анализ качества обфускации
    double AnalyzeObfuscationQuality(const std::string& query);

private:
    std::string obfuscation_key_;
    std::mutex obfuscation_mutex_;
    
    // Методы обфускации
    std::string ApplyBase32Encoding(const std::string& data);
    std::string ApplyBase64Encoding(const std::string& data);
    std::string ApplyHexEncoding(const std::string& data);
    std::string ApplyRandomPadding(const std::string& data);
    std::string GenerateRandomString(int length);
};

// DNS шифрование
class DNSEncryption {
public:
    DNSEncryption();
    ~DNSEncryption();

    // Шифрование DNS данных
    std::string EncryptData(const std::string& data, const std::string& key);
    
    // Расшифровка DNS данных
    std::string DecryptData(const std::string& encrypted_data, const std::string& key);
    
    // Генерация ключа шифрования
    std::string GenerateEncryptionKey();
    
    // Валидация ключа
    bool ValidateKey(const std::string& key);

private:
    std::mutex encryption_mutex_;
    
    // Методы шифрования
    std::string ApplyXOR(const std::string& data, const std::string& key);
    std::string ApplyAES(const std::string& data, const std::string& key);
    std::string ApplyChaCha20(const std::string& data, const std::string& key);
    std::string DeriveKey(const std::string& password);
    std::string GenerateRandomIV();
};

// Интеграция DNS туннелирования с TrafficMask
class DNSTunnelTrafficMaskIntegration {
public:
    DNSTunnelTrafficMaskIntegration();
    ~DNSTunnelTrafficMaskIntegration();

    // Интеграция с reverse tunnel
    bool IntegrateWithReverseTunnel(const std::string& dns_tunnel_id);
    
    // Интеграция с AI анализом
    bool IntegrateWithAIAnalysis(const std::string& dns_tunnel_id);
    
    // Интеграция с динамическим SNI
    bool IntegrateWithDynamicSNI(const std::string& dns_tunnel_id);
    
    // Адаптация для российских сервисов
    bool AdaptForRussiaServices(const std::string& dns_tunnel_id);
    
    // Получение метрик для AI
    std::unordered_map<std::string, double> GetAIMetrics(const std::string& dns_tunnel_id);
    
    // Создание DNS туннеля для России
    DNSTunnelConfig CreateRussiaDNSTunnelConfig();

private:
    std::shared_ptr<DNSTunnelManager> dns_tunnel_manager_;
    std::mutex integration_mutex_;
    
    // Методы адаптации
    DNSTunnelConfig AdaptConfigForRussia(const DNSTunnelConfig& config);
    bool ApplyRussiaOptimizations(const std::string& tunnel_id);
    std::unordered_map<std::string, double> ExtractMetrics(const DNSTunnelStats& stats);
    std::string SelectRussiaDomain();
    std::string SelectRussiaDNSServer();
};

} // namespace TrafficMask

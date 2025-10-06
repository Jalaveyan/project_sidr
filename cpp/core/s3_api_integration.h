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

// Типы API для обмена данными
enum class DataExchangeType {
    S3,           // Amazon S3
    YADOCS,       // Yandex Documents
    EXCEL,        // Excel файлы
    GOOGLE_DRIVE, // Google Drive
    DROPBOX,      // Dropbox
    ONEDRIVE      // OneDrive
};

// Состояние API соединения
enum class APIState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR,
    RATE_LIMITED
};

// Конфигурация API
struct APIConfig {
    DataExchangeType type;
    std::string endpoint;
    std::string access_key;
    std::string secret_key;
    std::string bucket_name;
    std::string region;
    int timeout_seconds = 30;
    int max_retries = 3;
    bool encryption = true;
    std::string encryption_key;
    std::unordered_map<std::string, std::string> custom_params;
};

// Статистика API
struct APIStats {
    std::string api_id;
    APIState state;
    uint64_t bytes_uploaded;
    uint64_t bytes_downloaded;
    uint64_t requests_sent;
    uint64_t requests_received;
    double success_rate;
    double average_latency_ms;
    int error_count;
    std::string last_error;
    std::chrono::system_clock::time_point last_activity;
    std::unordered_map<std::string, double> custom_metrics;
};

// S3 API клиент
class S3APIClient {
public:
    S3APIClient();
    ~S3APIClient();

    // Инициализация с конфигурацией
    bool Initialize(const APIConfig& config);
    
    // Подключение к API
    bool Connect();
    
    // Отключение от API
    void Disconnect();
    
    // Загрузка файла
    bool UploadFile(const std::string& file_path, const std::string& remote_path);
    
    // Скачивание файла
    bool DownloadFile(const std::string& remote_path, const std::string& local_path);
    
    // Загрузка данных
    bool UploadData(const std::vector<uint8_t>& data, const std::string& remote_path);
    
    // Скачивание данных
    std::vector<uint8_t> DownloadData(const std::string& remote_path);
    
    // Получение статистики
    APIStats GetStats() const;
    
    // Callback для событий
    void SetOnConnect(std::function<void()> callback);
    void SetOnDisconnect(std::function<void()> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
    void SetOnUploadComplete(std::function<void(const std::string&)> callback);
    void SetOnDownloadComplete(std::function<void(const std::string&)> callback);

private:
    APIConfig config_;
    APIStats stats_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::mutex stats_mutex_;
    
    // Callbacks
    std::function<void()> on_connect_;
    std::function<void()> on_disconnect_;
    std::function<void(const std::string&)> on_error_;
    std::function<void(const std::string&)> on_upload_complete_;
    std::function<void(const std::string&)> on_download_complete_;
    
    // Внутренние методы
    void WorkerLoop();
    bool EstablishConnection();
    void UpdateStats();
    void HandleError(const std::string& error);
    bool ValidateConfig();
    
    // API операции
    bool PerformUpload(const std::string& remote_path, const std::vector<uint8_t>& data);
    std::vector<uint8_t> PerformDownload(const std::string& remote_path);
    std::string GenerateSignedURL(const std::string& path);
    bool CheckRateLimit();
};

// Yandex Documents API клиент
class YaDocsAPIClient {
public:
    YaDocsAPIClient();
    ~YaDocsAPIClient();

    // Инициализация с конфигурацией
    bool Initialize(const APIConfig& config);
    
    // Подключение к API
    bool Connect();
    
    // Отключение от API
    void Disconnect();
    
    // Создание документа
    std::string CreateDocument(const std::string& title, const std::string& content);
    
    // Обновление документа
    bool UpdateDocument(const std::string& document_id, const std::string& content);
    
    // Получение документа
    std::string GetDocument(const std::string& document_id);
    
    // Удаление документа
    bool DeleteDocument(const std::string& document_id);
    
    // Получение статистики
    APIStats GetStats() const;
    
    // Callback для событий
    void SetOnConnect(std::function<void()> callback);
    void SetOnDisconnect(std::function<void()> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
    void SetOnDocumentCreated(std::function<void(const std::string&)> callback);
    void SetOnDocumentUpdated(std::function<void(const std::string&)> callback);

private:
    APIConfig config_;
    APIStats stats_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::mutex stats_mutex_;
    
    // Callbacks
    std::function<void()> on_connect_;
    std::function<void()> on_disconnect_;
    std::function<void(const std::string&)> on_error_;
    std::function<void(const std::string&)> on_document_created_;
    std::function<void(const std::string&)> on_document_updated_;
    
    // Внутренние методы
    void WorkerLoop();
    bool EstablishConnection();
    void UpdateStats();
    void HandleError(const std::string& error);
    bool ValidateConfig();
    
    // API операции
    std::string PerformCreateDocument(const std::string& title, const std::string& content);
    bool PerformUpdateDocument(const std::string& document_id, const std::string& content);
    std::string PerformGetDocument(const std::string& document_id);
    bool PerformDeleteDocument(const std::string& document_id);
    std::string GenerateAuthToken();
};

// Excel API клиент
class ExcelAPIClient {
public:
    ExcelAPIClient();
    ~ExcelAPIClient();

    // Инициализация с конфигурацией
    bool Initialize(const APIConfig& config);
    
    // Подключение к API
    bool Connect();
    
    // Отключение от API
    void Disconnect();
    
    // Создание Excel файла
    std::string CreateExcelFile(const std::string& filename, const std::vector<std::vector<std::string>>& data);
    
    // Чтение Excel файла
    std::vector<std::vector<std::string>> ReadExcelFile(const std::string& file_id);
    
    // Обновление Excel файла
    bool UpdateExcelFile(const std::string& file_id, const std::vector<std::vector<std::string>>& data);
    
    // Получение статистики
    APIStats GetStats() const;
    
    // Callback для событий
    void SetOnConnect(std::function<void()> callback);
    void SetOnDisconnect(std::function<void()> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
    void SetOnFileCreated(std::function<void(const std::string&)> callback);
    void SetOnFileUpdated(std::function<void(const std::string&)> callback);

private:
    APIConfig config_;
    APIStats stats_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::mutex stats_mutex_;
    
    // Callbacks
    std::function<void()> on_connect_;
    std::function<void()> on_disconnect_;
    std::function<void(const std::string&)> on_error_;
    std::function<void(const std::string&)> on_file_created_;
    std::function<void(const std::string&)> on_file_updated_;
    
    // Внутренние методы
    void WorkerLoop();
    bool EstablishConnection();
    void UpdateStats();
    void HandleError(const std::string& error);
    bool ValidateConfig();
    
    // API операции
    std::string PerformCreateExcelFile(const std::string& filename, const std::vector<std::vector<std::string>>& data);
    std::vector<std::vector<std::string>> PerformReadExcelFile(const std::string& file_id);
    bool PerformUpdateExcelFile(const std::string& file_id, const std::vector<std::vector<std::string>>& data);
    std::string ConvertDataToExcel(const std::vector<std::vector<std::string>>& data);
    std::vector<std::vector<std::string>> ConvertExcelToData(const std::string& excel_content);
};

// Менеджер API для обмена данными
class DataExchangeManager {
public:
    DataExchangeManager();
    ~DataExchangeManager();

    // Инициализация менеджера
    bool Initialize();
    
    // Создание API клиента
    std::string CreateAPIClient(const APIConfig& config);
    
    // Удаление API клиента
    bool RemoveAPIClient(const std::string& client_id);
    
    // Управление API клиентами
    bool StartAPIClient(const std::string& client_id);
    bool StopAPIClient(const std::string& client_id);
    
    // Получение статистики
    APIStats GetAPIClientStats(const std::string& client_id) const;
    
    // Оптимизация API клиента
    bool OptimizeAPIClient(const std::string& client_id);

private:
    std::unordered_map<std::string, std::shared_ptr<S3APIClient>> s3_clients_;
    std::unordered_map<std::string, std::shared_ptr<YaDocsAPIClient>> yadocs_clients_;
    std::unordered_map<std::string, std::shared_ptr<ExcelAPIClient>> excel_clients_;
    std::mutex clients_mutex_;
    
    // Внутренние методы
    std::string GenerateClientId();
    bool ValidateAPIConfig(const APIConfig& config);
    void OptimizeAPIConfig(APIConfig& config);
};

// Интеграция API с TrafficMask
class APITrafficMaskIntegration {
public:
    APITrafficMaskIntegration();
    ~APITrafficMaskIntegration();

    // Интеграция с reverse tunnel
    bool IntegrateWithReverseTunnel(const std::string& api_client_id);
    
    // Интеграция с AI анализом
    bool IntegrateWithAIAnalysis(const std::string& api_client_id);
    
    // Интеграция с динамическим SNI
    bool IntegrateWithDynamicSNI(const std::string& api_client_id);
    
    // Адаптация для российских сервисов
    bool AdaptForRussiaServices(const std::string& api_client_id);
    
    // Получение метрик для AI
    std::unordered_map<std::string, double> GetAIMetrics(const std::string& api_client_id);
    
    // Создание API конфигурации для России
    APIConfig CreateRussiaAPIConfig(DataExchangeType type);

private:
    std::shared_ptr<DataExchangeManager> api_manager_;
    std::mutex integration_mutex_;
    
    // Методы адаптации
    APIConfig AdaptConfigForRussia(const APIConfig& config);
    bool ApplyRussiaOptimizations(const std::string& client_id);
    std::unordered_map<std::string, double> ExtractMetrics(const APIStats& stats);
    std::string SelectRussiaEndpoint(DataExchangeType type);
    std::string SelectRussiaRegion();
};

} // namespace TrafficMask

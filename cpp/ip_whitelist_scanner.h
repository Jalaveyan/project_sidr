#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace NeuralTunnel {

// Результат сканирования IP
struct IPScanResult {
    std::string ip;
    std::string subnet;  // CIDR
    bool is_whitelisted;
    bool is_accessible;
    double response_time_ms;
    std::string service_name;  // Google, Cloudflare, etc
    std::string country;
    int success_count;
    int total_attempts;
};

// AI-анализатор белых списков IP
class AIWhitelistAnalyzer {
public:
    struct WhitelistProfile {
        std::vector<std::string> confirmed_ips;      // Точно работают
        std::vector<std::string> confirmed_subnets;  // Подсети
        std::vector<std::string> likely_ips;         // Вероятно работают
        std::vector<std::string> blocked_ips;        // Заблокированы
        std::map<std::string, double> ip_scores;     // IP -> вероятность работы
        double confidence;
    };
    
    AIWhitelistAnalyzer();
    
    // Анализ результатов сканирования
    WhitelistProfile analyzeResults(const std::vector<IPScanResult>& results);
    
    // Предсказание: будет ли IP работать
    double predictIPSuccess(const std::string& ip);
    
    // Обучение на результатах
    void learnFromAttempt(const std::string& ip, bool success);
    
    // Рекомендация лучших IP для использования
    std::vector<std::string> recommendBestIPs(int count = 10);
    
    // Определение паттернов в белых списках
    std::vector<std::string> detectIPPatterns();
    
private:
    std::map<std::string, std::pair<int, int>> ip_history_;  // IP -> (success, total)
    std::mutex history_mutex_;
    
    // AI модель
    struct NeuralModel {
        std::vector<std::vector<double>> weights;
        std::vector<double> biases;
    } model_;
    
    void trainModel();
    std::vector<double> extractIPFeatures(const std::string& ip);
    double calculateIPScore(const std::string& ip);
};

// Сканер белых списков IP
class IPWhitelistScanner {
public:
    IPWhitelistScanner();
    ~IPWhitelistScanner();
    
    // Запуск сканирования
    void startScanning();
    void stopScanning();
    bool isScanning() const { return scanning_; }
    
    // Настройка целей сканирования
    void addTargetSubnet(const std::string& subnet);  // Например: "8.8.8.0/24"
    void addKnownService(const std::string& service, const std::string& ip_range);
    
    // Получение результатов
    std::vector<IPScanResult> getResults() const;
    AIWhitelistAnalyzer::WhitelistProfile getCurrentProfile() const;
    
    // Экспорт результатов
    void exportToFile(const std::string& filename);
    void exportToJSON(const std::string& filename);
    
    // Callback для обновлений
    void setOnNewIPFound(std::function<void(const IPScanResult&)> callback);
    
private:
    std::atomic<bool> scanning_;
    std::thread scanner_thread_;
    std::vector<IPScanResult> results_;
    std::unique_ptr<AIWhitelistAnalyzer> ai_analyzer_;
    mutable std::mutex results_mutex_;
    
    std::function<void(const IPScanResult&)> on_new_ip_callback_;
    
    // Известные сервисы с их IP диапазонами
    std::map<std::string, std::vector<std::string>> known_services_;
    
    void scannerLoop();
    IPScanResult scanIP(const std::string& ip);
    bool testIPConnectivity(const std::string& ip, int port = 443);
    std::string identifyService(const std::string& ip);
    std::vector<std::string> expandSubnet(const std::string& subnet);
    
    // Умное сканирование
    void initializeKnownServices();
    std::vector<std::string> generateCandidateIPs();
};

// Адаптер для использования белых списков IP
class IPWhitelistAdapter {
public:
    IPWhitelistAdapter();
    
    // Получить лучший IP для маскировки
    std::string getBestMaskingIP();
    
    // Получить IP для конкретного сервиса
    std::string getIPForService(const std::string& service);
    
    // Обновить список из сканера
    void updateFromScanner(const IPWhitelistScanner& scanner);
    
    // Применить белый список к соединению
    bool applyWhitelistRouting(const std::string& destination_ip);
    
private:
    std::vector<std::string> whitelist_ips_;
    std::map<std::string, std::string> service_to_ip_;
    std::unique_ptr<AIWhitelistAnalyzer> ai_;
};

} // namespace NeuralTunnel

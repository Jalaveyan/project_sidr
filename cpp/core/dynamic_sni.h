#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

namespace TrafficMask {

// Стратегии смены SNI
enum class SNIStrategy {
    RANDOM,           // Случайная смена
    ROUND_ROBIN,      // По кругу
    ADAPTIVE,         // Адаптивная на основе анализа
    AI_DRIVEN,        // AI-управляемая
    FALLBACK          // Резервная стратегия
};

// Состояние SNI
enum class SNIState {
    ACTIVE,           // Активный SNI
    SCANNING,         // Сканирование новых SNI
    SWITCHING,        // Переключение SNI
    FALLBACK,         // Резервный SNI
    ERROR             // Ошибка
};

// Конфигурация динамического SNI
struct DynamicSNIConfig {
    std::vector<std::string> sni_pool;           // Пул SNI для использования
    std::vector<std::string> russia_domains;    // Российские домены
    std::vector<std::string> fallback_domains;  // Резервные домены
    
    SNIStrategy default_strategy = SNIStrategy::AI_DRIVEN;
    int switch_interval_ms = 30000;             // Интервал смены SNI
    int scan_interval_ms = 60000;               // Интервал сканирования
    int max_retries = 3;                        // Максимальное количество попыток
    bool auto_scan = true;                      // Автоматическое сканирование
    bool ai_analysis = true;                    // AI анализ SNI
    double success_threshold = 0.8;              // Порог успешности
};

// Результат сканирования SNI
struct SNIScanResult {
    std::string sni;
    bool is_accessible;
    double response_time_ms;
    double success_rate;
    std::string error_message;
    std::chrono::system_clock::time_point timestamp;
};

// Менеджер динамического SNI
class DynamicSNIManager {
public:
    DynamicSNIManager();
    ~DynamicSNIManager();

    // Инициализация с конфигурацией
    bool Initialize(const DynamicSNIConfig& config);
    
    // Запуск менеджера
    bool Start();
    
    // Остановка менеджера
    void Stop();
    
    // Получение текущего SNI
    std::string GetCurrentSNI() const;
    
    // Принудительная смена SNI
    bool SwitchSNI(const std::string& new_sni);
    
    // Автоматическая смена SNI
    bool AutoSwitchSNI();
    
    // Сканирование новых SNI
    std::vector<SNIScanResult> ScanNewSNI();
    
    // Получение статистики
    std::unordered_map<std::string, double> GetSNIStatistics() const;
    
    // Callback для событий
    void SetOnSNIChange(std::function<void(const std::string&)> callback);
    void SetOnScanResult(std::function<void(const std::vector<SNIScanResult>&)> callback);
    void SetOnStateChange(std::function<void(SNIState)> callback);

private:
    DynamicSNIConfig config_;
    std::string current_sni_;
    SNIState state_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::thread scanner_thread_;
    std::mutex state_mutex_;
    std::mutex sni_mutex_;
    
    // Статистика SNI
    std::unordered_map<std::string, double> sni_statistics_;
    std::unordered_set<std::string> failed_sni_;
    
    // Callbacks
    std::function<void(const std::string&)> on_sni_change_;
    std::function<void(const std::vector<SNIScanResult>&)> on_scan_result_;
    std::function<void(SNIState)> on_state_change_;
    
    // Внутренние методы
    void WorkerLoop();
    void ScannerLoop();
    bool PerformSNISwitch(const std::string& new_sni);
    std::string SelectNextSNI();
    SNIScanResult TestSNI(const std::string& sni);
    void UpdateStatistics(const std::string& sni, bool success, double response_time);
    void SetState(SNIState new_state);
    
    // AI анализ SNI
    double AnalyzeSNIEffectiveness(const std::string& sni);
    std::string GetBestSNI();
    bool ShouldSwitchSNI();
};

// Сканер SNI
class SNIScanner {
public:
    SNIScanner();
    ~SNIScanner();

    // Сканирование SNI
    std::vector<SNIScanResult> ScanSNI(const std::vector<std::string>& sni_list);
    
    // Сканирование российских доменов
    std::vector<SNIScanResult> ScanRussiaDomains();
    
    // Сканирование с AI анализом
    std::vector<SNIScanResult> ScanWithAIAnalysis(const std::vector<std::string>& sni_list);

private:
    SNIScanResult TestSingleSNI(const std::string& sni);
    bool IsSNIAccessible(const std::string& sni);
    double MeasureResponseTime(const std::string& sni);
    std::string GetErrorForSNI(const std::string& sni);
};

// AI анализатор SNI
class SNIAIAnalyzer {
public:
    SNIAIAnalyzer();
    ~SNIAIAnalyzer();

    // Анализ эффективности SNI
    double AnalyzeEffectiveness(const std::string& sni);
    
    // Рекомендация лучшего SNI
    std::string RecommendBestSNI(const std::vector<std::string>& available_sni);
    
    // Анализ риска обнаружения
    double AssessDetectionRisk(const std::string& sni);
    
    // Обновление модели на основе результатов
    void UpdateModel(const std::string& sni, bool success, double performance);

private:
    std::unordered_map<std::string, double> sni_performance_;
    std::unordered_map<std::string, int> sni_usage_count_;
    std::mutex model_mutex_;
    
    double CalculateSNIScore(const std::string& sni);
    void UpdateSNIPerformance(const std::string& sni, double score);
};

} // namespace TrafficMask

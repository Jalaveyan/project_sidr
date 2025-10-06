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

// Типы обхода
enum class BypassType {
    SNI,           // SNI обход
    IP_SIDR,       // IP SIDR обход
    MIXED,         // Смешанный обход
    ADAPTIVE,      // Адаптивный обход
    AI_DRIVEN      // AI-управляемый обход
};

// Состояние обхода
enum class BypassState {
    INACTIVE,      // Неактивен
    SCANNING,      // Сканирование
    ACTIVE,        // Активен
    OPTIMIZING,    // Оптимизация
    FAILED,        // Неудачен
    BLOCKED        // Заблокирован
};

// Конфигурация обхода
struct BypassConfig {
    BypassType bypass_type;
    std::vector<std::string> sni_domains;
    std::vector<std::string> ip_ranges;
    bool auto_detection = true;
    bool ai_optimization = true;
    int detection_interval_ms = 5000;
    int optimization_interval_ms = 30000;
    double success_threshold = 0.8;
    double failure_threshold = 0.3;
    std::unordered_map<std::string, std::string> custom_params;
};

// Статистика обхода
struct BypassStats {
    std::string bypass_id;
    BypassType current_type;
    BypassState state;
    double success_rate;
    double average_latency_ms;
    uint64_t packets_processed;
    uint64_t packets_blocked;
    int detection_count;
    int optimization_count;
    std::string last_error;
    std::chrono::system_clock::time_point last_activity;
    std::unordered_map<std::string, double> custom_metrics;
};

// Детектор типа обхода
class BypassDetector {
public:
    BypassDetector();
    ~BypassDetector();

    // Инициализация детектора
    bool Initialize(const BypassConfig& config);
    
    // Определение типа обхода
    BypassType DetectBypassType(const std::vector<uint8_t>& packet_data);
    
    // Анализ эффективности обхода
    double AnalyzeBypassEffectiveness(BypassType bypass_type);
    
    // Рекомендация лучшего типа обхода
    BypassType RecommendBestBypassType();
    
    // Получение статистики
    BypassStats GetStats() const;
    
    // Callback для событий
    void SetOnBypassTypeDetected(std::function<void(BypassType)> callback);
    void SetOnBypassOptimized(std::function<void(BypassType)> callback);
    void SetOnBypassFailed(std::function<void(BypassType, const std::string&)> callback);

private:
    BypassConfig config_;
    BypassStats stats_;
    std::atomic<bool> running_;
    std::thread detection_thread_;
    mutable std::mutex stats_mutex_;
    
    // Callbacks
    std::function<void(BypassType)> on_bypass_type_detected_;
    std::function<void(BypassType)> on_bypass_optimized_;
    std::function<void(BypassType, const std::string&)> on_bypass_failed_;
    
    // Внутренние методы
    void DetectionLoop();
    BypassType AnalyzePacket(const std::vector<uint8_t>& packet_data);
    double CalculateSNIScore(const std::vector<uint8_t>& packet_data);
    double CalculateIPSIDRScore(const std::vector<uint8_t>& packet_data);
    bool IsSNIPacket(const std::vector<uint8_t>& packet_data);
    bool IsIPSIDRPacket(const std::vector<uint8_t>& packet_data);
    void UpdateStats(BypassType bypass_type, bool success);
    void HandleBypassFailure(BypassType bypass_type, const std::string& error);
};

// Оптимизатор обхода
class BypassOptimizer {
public:
    BypassOptimizer();
    ~BypassOptimizer();

    // Инициализация оптимизатора
    bool Initialize(const BypassConfig& config);
    
    // Оптимизация обхода
    BypassType OptimizeBypass(BypassType current_type, const BypassStats& stats);
    
    // Анализ производительности
    double AnalyzePerformance(BypassType bypass_type, const BypassStats& stats);
    
    // Рекомендации по улучшению
    std::vector<std::string> GetOptimizationRecommendations(BypassType bypass_type, const BypassStats& stats);
    
    // Получение статистики
    BypassStats GetStats() const;
    
    // Callback для событий
    void SetOnOptimizationComplete(std::function<void(BypassType)> callback);
    void SetOnOptimizationFailed(std::function<void(const std::string&)> callback);

private:
    BypassConfig config_;
    BypassStats stats_;
    std::atomic<bool> running_;
    std::thread optimization_thread_;
    mutable std::mutex stats_mutex_;
    
    // Callbacks
    std::function<void(BypassType)> on_optimization_complete_;
    std::function<void(const std::string&)> on_optimization_failed_;
    
    // Внутренние методы
    void OptimizationLoop();
    BypassType SelectBestBypassType(const BypassStats& stats);
    double CalculateBypassScore(BypassType bypass_type, const BypassStats& stats);
    bool ShouldSwitchBypassType(BypassType current_type, BypassType recommended_type);
    void UpdateStats(BypassType bypass_type, bool success);
    void HandleOptimizationFailure(const std::string& error);
};

// Менеджер обхода
class BypassManager {
public:
    BypassManager();
    ~BypassManager();

    // Инициализация менеджера
    bool Initialize();
    
    // Создание обхода
    std::string CreateBypass(const BypassConfig& config);
    
    // Удаление обхода
    bool RemoveBypass(const std::string& bypass_id);
    
    // Управление обходом
    bool StartBypass(const std::string& bypass_id);
    bool StopBypass(const std::string& bypass_id);
    bool OptimizeBypass(const std::string& bypass_id);
    
    // Получение статистики
    BypassStats GetBypassStats(const std::string& bypass_id) const;
    
    // Получение всех обходов
    std::vector<std::string> GetAllBypasses() const;
    
    // Callback для событий
    void SetOnBypassStateChange(std::function<void(const std::string&, BypassState)> callback);
    void SetOnBypassTypeChange(std::function<void(const std::string&, BypassType)> callback);
    void SetOnBypassOptimized(std::function<void(const std::string&, BypassType)> callback);

private:
    std::unordered_map<std::string, std::shared_ptr<BypassDetector>> detectors_;
    std::unordered_map<std::string, std::shared_ptr<BypassOptimizer>> optimizers_;
    std::unordered_map<std::string, BypassConfig> bypass_configs_;
    mutable std::mutex bypasses_mutex_;
    
    // Callbacks
    std::function<void(const std::string&, BypassState)> on_bypass_state_change_;
    std::function<void(const std::string&, BypassType)> on_bypass_type_change_;
    std::function<void(const std::string&, BypassType)> on_bypass_optimized_;
    
    // Внутренние методы
    std::string GenerateBypassId();
    bool ValidateBypassConfig(const BypassConfig& config);
    void OptimizeBypassConfig(BypassConfig& config);
    void HandleBypassStateChange(const std::string& bypass_id, BypassState new_state);
    void HandleBypassTypeChange(const std::string& bypass_id, BypassType new_type);
    void HandleBypassOptimized(const std::string& bypass_id, BypassType optimized_type);
};

// Интеграция обхода с TrafficMask
class BypassTrafficMaskIntegration {
public:
    BypassTrafficMaskIntegration();
    ~BypassTrafficMaskIntegration();

    // Интеграция с reverse tunnel
    bool IntegrateWithReverseTunnel(const std::string& bypass_id);
    
    // Интеграция с AI анализом
    bool IntegrateWithAIAnalysis(const std::string& bypass_id);
    
    // Интеграция с динамическим SNI
    bool IntegrateWithDynamicSNI(const std::string& bypass_id);
    
    // Адаптация для российских сервисов
    bool AdaptForRussiaServices(const std::string& bypass_id);
    
    // Получение метрик для AI
    std::unordered_map<std::string, double> GetAIMetrics(const std::string& bypass_id);
    
    // Создание конфигурации обхода для России
    BypassConfig CreateRussiaBypassConfig();

private:
    std::shared_ptr<BypassManager> bypass_manager_;
    std::mutex integration_mutex_;
    
    // Методы адаптации
    BypassConfig AdaptConfigForRussia(const BypassConfig& config);
    bool ApplyRussiaOptimizations(const std::string& bypass_id);
    std::unordered_map<std::string, double> ExtractMetrics(const BypassStats& stats);
    std::vector<std::string> GetRussiaSNIDomains();
    std::vector<std::string> GetRussiaIPRanges();
    BypassType SelectBestRussiaBypassType();
};

} // namespace TrafficMask

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

namespace TrafficMask {

// Типы анализа
enum class AnalysisType {
    ENTROPY,           // Анализ энтропии
    MAC_IP_CORRELATION, // Корреляция MAC/IP
    CDN_DETECTION,     // Обнаружение CDN
    DPI_RISK,          // Оценка риска DPI
    PROTOCOL_FINGERPRINT, // Отпечаток протокола
    TRAFFIC_PATTERN,   // Паттерн трафика
    SNI_ANALYSIS,      // Анализ SNI
    IP_SIDR_ANALYSIS   // Анализ IP SIDR
};

// Результат AI анализа
struct AIAnalysisResult {
    double confidence;              // Уверенность в результате (0-1)
    double risk_score;              // Оценка риска (0-1)
    std::string recommendation;    // Рекомендация
    std::vector<std::string> actions; // Рекомендуемые действия
    std::unordered_map<std::string, double> metrics; // Дополнительные метрики
    std::chrono::system_clock::time_point timestamp;
};

// Конфигурация AI анализатора
struct AIAnalyzerConfig {
    bool enable_entropy_analysis = true;
    bool enable_mac_ip_analysis = true;
    bool enable_cdn_detection = true;
    bool enable_dpi_risk_assessment = true;
    bool enable_protocol_fingerprinting = true;
    bool enable_traffic_pattern_analysis = true;
    bool enable_sni_analysis = true;
    bool enable_ip_sidr_analysis = true;
    
    double risk_threshold = 0.7;        // Порог риска для срабатывания
    double confidence_threshold = 0.8;   // Минимальная уверенность
    int analysis_interval_ms = 2000;    // Интервал анализа
    int history_size = 100;             // Размер истории для анализа
};

// AI анализатор для сетевого трафика
class AIAnalyzer {
public:
    AIAnalyzer();
    ~AIAnalyzer();

    // Инициализация с конфигурацией
    bool Initialize(const AIAnalyzerConfig& config);
    
    // Анализ пакета данных
    AIAnalysisResult AnalyzePacket(const std::vector<uint8_t>& packet_data);
    
    // Анализ потока данных
    AIAnalysisResult AnalyzeStream(const std::vector<std::vector<uint8_t>>& stream_data);
    
    // Анализ SNI
    AIAnalysisResult AnalyzeSNI(const std::string& sni);
    
    // Анализ IP SIDR
    AIAnalysisResult AnalyzeIPSIDR(const std::string& source_ip, const std::string& dest_ip);
    
    // Получение истории анализа
    std::vector<AIAnalysisResult> GetAnalysisHistory() const;
    
    // Очистка истории
    void ClearHistory();
    
    // Callback для результатов анализа
    void SetOnAnalysisResult(std::function<void(AIAnalysisResult)> callback);

private:
    AIAnalyzerConfig config_;
    std::vector<AIAnalysisResult> analysis_history_;
    std::mutex history_mutex_;
    std::function<void(AIAnalysisResult)> on_analysis_result_;
    
    // Методы анализа
    double AnalyzeEntropy(const std::vector<uint8_t>& data);
    double AnalyzeMACIPCorrelation(const std::vector<uint8_t>& data);
    double DetectCDNTraffic(const std::vector<uint8_t>& data);
    double AssessDPIRisk(const std::vector<uint8_t>& data);
    double AnalyzeProtocolFingerprint(const std::vector<uint8_t>& data);
    double AnalyzeTrafficPattern(const std::vector<std::vector<uint8_t>>& stream);
    double AnalyzeSNIPattern(const std::string& sni);
    double AnalyzeIPSIDRPattern(const std::string& source_ip, const std::string& dest_ip);
    
    // Вспомогательные методы
    std::string GenerateRecommendation(const AIAnalysisResult& result);
    std::vector<std::string> GenerateActions(const AIAnalysisResult& result);
    void AddToHistory(const AIAnalysisResult& result);
    double CalculateConfidence(const std::unordered_map<std::string, double>& metrics);
};

// Менеджер AI анализаторов
class AIAnalyzerManager {
public:
    AIAnalyzerManager();
    ~AIAnalyzerManager();

    // Добавление анализатора
    void AddAnalyzer(const std::string& name, std::shared_ptr<AIAnalyzer> analyzer);
    
    // Получение анализатора
    std::shared_ptr<AIAnalyzer> GetAnalyzer(const std::string& name);
    
    // Запуск всех анализаторов
    void StartAllAnalyzers();
    
    // Остановка всех анализаторов
    void StopAllAnalyzers();
    
    // Агрегированный анализ
    AIAnalysisResult AggregateAnalysis(const std::vector<uint8_t>& data);

private:
    std::unordered_map<std::string, std::shared_ptr<AIAnalyzer>> analyzers_;
    std::mutex analyzers_mutex_;
};

} // namespace TrafficMask

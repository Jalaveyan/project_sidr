#include "ai_analyzer.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <mutex>

namespace TrafficMask {

AIAnalyzer::AIAnalyzer() {
}

AIAnalyzer::~AIAnalyzer() {
}

bool AIAnalyzer::Initialize(const AIAnalyzerConfig& config) {
    config_ = config;
    analysis_history_.clear();
    
    std::cout << "[AIAnalyzer] Инициализация AI анализатора:" << std::endl;
    std::cout << "  Энтропия: " << (config.enable_entropy_analysis ? "Включен" : "Выключен") << std::endl;
    std::cout << "  MAC/IP: " << (config.enable_mac_ip_analysis ? "Включен" : "Выключен") << std::endl;
    std::cout << "  CDN: " << (config.enable_cdn_detection ? "Включен" : "Выключен") << std::endl;
    std::cout << "  DPI риск: " << (config.enable_dpi_risk_assessment ? "Включен" : "Выключен") << std::endl;
    std::cout << "  Порог риска: " << config.risk_threshold << std::endl;
    
    return true;
}

AIAnalysisResult AIAnalyzer::AnalyzePacket(const std::vector<uint8_t>& packet_data) {
    AIAnalysisResult result;
    result.timestamp = std::chrono::system_clock::now();
    
    std::unordered_map<std::string, double> metrics;
    
    // Анализ энтропии
    if (config_.enable_entropy_analysis) {
        metrics["entropy"] = AnalyzeEntropy(packet_data);
    }
    
    // Анализ MAC/IP корреляции
    if (config_.enable_mac_ip_analysis) {
        metrics["mac_ip_correlation"] = AnalyzeMACIPCorrelation(packet_data);
    }
    
    // Обнаружение CDN
    if (config_.enable_cdn_detection) {
        metrics["cdn_detection"] = DetectCDNTraffic(packet_data);
    }
    
    // Оценка риска DPI
    if (config_.enable_dpi_risk_assessment) {
        metrics["dpi_risk"] = AssessDPIRisk(packet_data);
    }
    
    // Анализ отпечатка протокола
    if (config_.enable_protocol_fingerprinting) {
        metrics["protocol_fingerprint"] = AnalyzeProtocolFingerprint(packet_data);
    }
    
    result.metrics = metrics;
    result.confidence = CalculateConfidence(metrics);
    result.risk_score = metrics.count("dpi_risk") ? metrics["dpi_risk"] : 0.0;
    result.recommendation = GenerateRecommendation(result);
    result.actions = GenerateActions(result);
    
    // Добавление в историю
    AddToHistory(result);
    
    // Вызов callback
    if (on_analysis_result_) {
        on_analysis_result_(result);
    }
    
    return result;
}

AIAnalysisResult AIAnalyzer::AnalyzeStream(const std::vector<std::vector<uint8_t>>& stream_data) {
    AIAnalysisResult result;
    result.timestamp = std::chrono::system_clock::now();
    
    std::unordered_map<std::string, double> metrics;
    
    // Анализ паттерна трафика
    if (config_.enable_traffic_pattern_analysis) {
        metrics["traffic_pattern"] = AnalyzeTrafficPattern(stream_data);
    }
    
    // Агрегированный анализ всех пакетов
    for (const auto& packet : stream_data) {
        auto packet_result = AnalyzePacket(packet);
        for (const auto& metric : packet_result.metrics) {
            metrics[metric.first] += metric.second;
        }
    }
    
    // Нормализация метрик
    for (auto& metric : metrics) {
        metric.second /= stream_data.size();
    }
    
    result.metrics = metrics;
    result.confidence = CalculateConfidence(metrics);
    result.risk_score = metrics.count("dpi_risk") ? metrics["dpi_risk"] : 0.0;
    result.recommendation = GenerateRecommendation(result);
    result.actions = GenerateActions(result);
    
    AddToHistory(result);
    
    if (on_analysis_result_) {
        on_analysis_result_(result);
    }
    
    return result;
}

AIAnalysisResult AIAnalyzer::AnalyzeSNI(const std::string& sni) {
    AIAnalysisResult result;
    result.timestamp = std::chrono::system_clock::now();
    
    std::unordered_map<std::string, double> metrics;
    
    if (config_.enable_sni_analysis) {
        metrics["sni_analysis"] = AnalyzeSNIPattern(sni);
    }
    
    result.metrics = metrics;
    result.confidence = CalculateConfidence(metrics);
    result.risk_score = metrics.count("sni_analysis") ? (1.0 - metrics["sni_analysis"]) : 0.0;
    result.recommendation = GenerateRecommendation(result);
    result.actions = GenerateActions(result);
    
    AddToHistory(result);
    
    if (on_analysis_result_) {
        on_analysis_result_(result);
    }
    
    return result;
}

AIAnalysisResult AIAnalyzer::AnalyzeIPSIDR(const std::string& source_ip, const std::string& dest_ip) {
    AIAnalysisResult result;
    result.timestamp = std::chrono::system_clock::now();
    
    std::unordered_map<std::string, double> metrics;
    
    if (config_.enable_ip_sidr_analysis) {
        metrics["ip_sidr_analysis"] = AnalyzeIPSIDRPattern(source_ip, dest_ip);
    }
    
    result.metrics = metrics;
    result.confidence = CalculateConfidence(metrics);
    result.risk_score = metrics.count("ip_sidr_analysis") ? (1.0 - metrics["ip_sidr_analysis"]) : 0.0;
    result.recommendation = GenerateRecommendation(result);
    result.actions = GenerateActions(result);
    
    AddToHistory(result);
    
    if (on_analysis_result_) {
        on_analysis_result_(result);
    }
    
    return result;
}

std::vector<AIAnalysisResult> AIAnalyzer::GetAnalysisHistory() const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    return analysis_history_;
}

void AIAnalyzer::ClearHistory() {
    std::lock_guard<std::mutex> lock(history_mutex_);
    analysis_history_.clear();
}

void AIAnalyzer::SetOnAnalysisResult(std::function<void(AIAnalysisResult)> callback) {
    on_analysis_result_ = callback;
}

double AIAnalyzer::AnalyzeEntropy(const std::vector<uint8_t>& data) {
    if (data.empty()) return 0.0;
    
    std::unordered_map<uint8_t, int> frequency;
    for (uint8_t byte : data) {
        frequency[byte]++;
    }
    
    double entropy = 0.0;
    double data_size = data.size();
    
    for (const auto& pair : frequency) {
        double probability = pair.second / data_size;
        if (probability > 0) {
            entropy -= probability * std::log2(probability);
        }
    }
    
    // Нормализация к диапазону 0-1
    return std::min(entropy / 8.0, 1.0);
}

double AIAnalyzer::AnalyzeMACIPCorrelation(const std::vector<uint8_t>& data) {
    // Симуляция анализа корреляции MAC/IP
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

double AIAnalyzer::DetectCDNTraffic(const std::vector<uint8_t>& data) {
    // Симуляция обнаружения CDN трафика
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

double AIAnalyzer::AssessDPIRisk(const std::vector<uint8_t>& data) {
    // Симуляция оценки риска DPI
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

double AIAnalyzer::AnalyzeProtocolFingerprint(const std::vector<uint8_t>& data) {
    // Симуляция анализа отпечатка протокола
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

double AIAnalyzer::AnalyzeTrafficPattern(const std::vector<std::vector<uint8_t>>& stream) {
    if (stream.empty()) return 0.0;
    
    // Анализ паттернов в потоке данных
    double pattern_score = 0.0;
    
    for (size_t i = 1; i < stream.size(); ++i) {
        if (stream[i].size() == stream[i-1].size()) {
            pattern_score += 0.1;
        }
    }
    
    return std::min(pattern_score / stream.size(), 1.0);
}

double AIAnalyzer::AnalyzeSNIPattern(const std::string& sni) {
    // Анализ паттерна SNI
    if (sni.empty()) return 0.0;
    
    // Проверка на российские домены
    std::vector<std::string> russia_domains = {
        "vk.com", "mail.ru", "yandex.ru", "ok.ru", "rambler.ru"
    };
    
    for (const auto& domain : russia_domains) {
        if (sni.find(domain) != std::string::npos) {
            return 0.9; // Высокий балл для российских доменов
        }
    }
    
    return 0.5; // Средний балл для других доменов
}

double AIAnalyzer::AnalyzeIPSIDRPattern(const std::string& source_ip, const std::string& dest_ip) {
    // Анализ паттерна IP SIDR
    // Российские IP диапазоны
    std::vector<std::string> russia_ip_ranges = {
        "77.88.8.8", "94.100.180.200", "87.240.190.72", "81.19.70.1"
    };
    
    for (const auto& ip : russia_ip_ranges) {
        if (source_ip == ip || dest_ip == ip) {
            return 0.9; // Высокий балл для российских IP
        }
    }
    
    return 0.5; // Средний балл для других IP
}

std::string AIAnalyzer::GenerateRecommendation(const AIAnalysisResult& result) {
    if (result.risk_score > config_.risk_threshold) {
        return "ВЫСОКИЙ_РИСК_DPI";
    } else if (result.confidence < config_.confidence_threshold) {
        return "НИЗКАЯ_УВЕРЕННОСТЬ";
    } else {
        return "НОРМАЛЬНОЕ_СОСТОЯНИЕ";
    }
}

std::vector<std::string> AIAnalyzer::GenerateActions(const AIAnalysisResult& result) {
    std::vector<std::string> actions;
    
    if (result.risk_score > config_.risk_threshold) {
        actions.push_back("SWITCH_ROLES");
        actions.push_back("CHANGE_SNI");
        actions.push_back("USE_FALLBACK");
    } else if (result.confidence < config_.confidence_threshold) {
        actions.push_back("CONTINUE_MONITORING");
    } else {
        actions.push_back("MAINTAIN_CURRENT_STATE");
    }
    
    return actions;
}

void AIAnalyzer::AddToHistory(const AIAnalysisResult& result) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    analysis_history_.push_back(result);
    
    // Ограничение размера истории
    if (analysis_history_.size() > config_.history_size) {
        analysis_history_.erase(analysis_history_.begin());
    }
}

double AIAnalyzer::CalculateConfidence(const std::unordered_map<std::string, double>& metrics) {
    if (metrics.empty()) return 0.0;
    
    double total_confidence = 0.0;
    int count = 0;
    
    for (const auto& metric : metrics) {
        total_confidence += metric.second;
        count++;
    }
    
    return count > 0 ? total_confidence / count : 0.0;
}

// AIAnalyzerManager
AIAnalyzerManager::AIAnalyzerManager() {
}

AIAnalyzerManager::~AIAnalyzerManager() {
}

void AIAnalyzerManager::AddAnalyzer(const std::string& name, std::shared_ptr<AIAnalyzer> analyzer) {
    std::lock_guard<std::mutex> lock(analyzers_mutex_);
    analyzers_[name] = analyzer;
}

std::shared_ptr<AIAnalyzer> AIAnalyzerManager::GetAnalyzer(const std::string& name) {
    std::lock_guard<std::mutex> lock(analyzers_mutex_);
    auto it = analyzers_.find(name);
    return (it != analyzers_.end()) ? it->second : nullptr;
}

void AIAnalyzerManager::StartAllAnalyzers() {
    std::lock_guard<std::mutex> lock(analyzers_mutex_);
    std::cout << "[AIAnalyzerManager] Запуск " << analyzers_.size() << " анализаторов" << std::endl;
}

void AIAnalyzerManager::StopAllAnalyzers() {
    std::lock_guard<std::mutex> lock(analyzers_mutex_);
    std::cout << "[AIAnalyzerManager] Остановка всех анализаторов" << std::endl;
}

AIAnalysisResult AIAnalyzerManager::AggregateAnalysis(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(analyzers_mutex_);
    
    AIAnalysisResult aggregated_result;
    aggregated_result.timestamp = std::chrono::system_clock::now();
    
    std::unordered_map<std::string, double> aggregated_metrics;
    double total_confidence = 0.0;
    int analyzer_count = 0;
    
    for (const auto& pair : analyzers_) {
        auto result = pair.second->AnalyzePacket(data);
        
        for (const auto& metric : result.metrics) {
            aggregated_metrics[metric.first] += metric.second;
        }
        
        total_confidence += result.confidence;
        analyzer_count++;
    }
    
    // Нормализация метрик
    for (auto& metric : aggregated_metrics) {
        metric.second /= analyzer_count;
    }
    
    aggregated_result.metrics = aggregated_metrics;
    aggregated_result.confidence = analyzer_count > 0 ? total_confidence / analyzer_count : 0.0;
    aggregated_result.risk_score = aggregated_metrics.count("dpi_risk") ? aggregated_metrics["dpi_risk"] : 0.0;
    
    return aggregated_result;
}

} // namespace TrafficMask

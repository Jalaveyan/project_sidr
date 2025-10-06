#include "bypass_detection.h"
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>

namespace TrafficMask {

// BypassDetector
BypassDetector::BypassDetector() 
    : running_(false) {
}

BypassDetector::~BypassDetector() {
    if (running_) {
        running_ = false;
        if (detection_thread_.joinable()) {
            detection_thread_.join();
        }
    }
}

bool BypassDetector::Initialize(const BypassConfig& config) {
    config_ = config;
    stats_.bypass_id = "bypass_detector_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    stats_.current_type = BypassType::SNI;
    stats_.state = BypassState::INACTIVE;
    
    std::cout << "[BypassDetector] Инициализация детектора:" << std::endl;
    std::cout << "  Тип обхода: " << (int)config.bypass_type << std::endl;
    std::cout << "  SNI домены: " << config.sni_domains.size() << std::endl;
    std::cout << "  IP диапазоны: " << config.ip_ranges.size() << std::endl;
    std::cout << "  Автоопределение: " << (config.auto_detection ? "Включено" : "Выключено") << std::endl;
    std::cout << "  AI оптимизация: " << (config.ai_optimization ? "Включено" : "Выключено") << std::endl;
    
    return true;
}

BypassType BypassDetector::DetectBypassType(const std::vector<uint8_t>& packet_data) {
    if (packet_data.empty()) {
        return BypassType::SNI;
    }
    
    // Анализ пакета для определения типа обхода
    double sni_score = CalculateSNIScore(packet_data);
    double ip_sidr_score = CalculateIPSIDRScore(packet_data);
    
    std::cout << "[BypassDetector] Анализ пакета: SNI=" << sni_score << ", IP_SIDR=" << ip_sidr_score << std::endl;
    
    if (sni_score > ip_sidr_score) {
        return BypassType::SNI;
    } else if (ip_sidr_score > sni_score) {
        return BypassType::IP_SIDR;
    } else {
        return BypassType::MIXED;
    }
}

double BypassDetector::AnalyzeBypassEffectiveness(BypassType bypass_type) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Анализ эффективности обхода
    double effectiveness = 0.0;
    
    switch (bypass_type) {
        case BypassType::SNI:
            effectiveness = 0.8 + (std::rand() % 20) / 100.0; // 80-100%
            break;
        case BypassType::IP_SIDR:
            effectiveness = 0.75 + (std::rand() % 25) / 100.0; // 75-100%
            break;
        case BypassType::MIXED:
            effectiveness = 0.85 + (std::rand() % 15) / 100.0; // 85-100%
            break;
        case BypassType::ADAPTIVE:
            effectiveness = 0.9 + (std::rand() % 10) / 100.0; // 90-100%
            break;
        case BypassType::AI_DRIVEN:
            effectiveness = 0.95 + (std::rand() % 5) / 100.0; // 95-100%
            break;
    }
    
    return effectiveness;
}

BypassType BypassDetector::RecommendBestBypassType() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Рекомендация лучшего типа обхода
    BypassType best_type = BypassType::SNI;
    double best_score = 0.0;
    
    for (int type = 0; type < 5; ++type) {
        BypassType current_type = static_cast<BypassType>(type);
        double score = AnalyzeBypassEffectiveness(current_type);
        
        if (score > best_score) {
            best_score = score;
            best_type = current_type;
        }
    }
    
    std::cout << "[BypassDetector] Рекомендуемый тип обхода: " << (int)best_type << " (балл: " << best_score << ")" << std::endl;
    
    return best_type;
}

BypassStats BypassDetector::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void BypassDetector::SetOnBypassTypeDetected(std::function<void(BypassType)> callback) {
    on_bypass_type_detected_ = callback;
}

void BypassDetector::SetOnBypassOptimized(std::function<void(BypassType)> callback) {
    on_bypass_optimized_ = callback;
}

void BypassDetector::SetOnBypassFailed(std::function<void(BypassType, const std::string&)> callback) {
    on_bypass_failed_ = callback;
}

void BypassDetector::DetectionLoop() {
    std::cout << "[BypassDetector] Detection loop запущен" << std::endl;
    
    while (running_) {
        // Симуляция обнаружения типа обхода
        std::vector<uint8_t> test_packet(1024);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::generate(test_packet.begin(), test_packet.end(), [&gen]() { return gen() % 256; });
        
        BypassType detected_type = DetectBypassType(test_packet);
        
        if (detected_type != stats_.current_type) {
            stats_.current_type = detected_type;
            stats_.detection_count++;
            
            if (on_bypass_type_detected_) {
                on_bypass_type_detected_(detected_type);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.detection_interval_ms));
    }
}

BypassType BypassDetector::AnalyzePacket(const std::vector<uint8_t>& packet_data) {
    if (IsSNIPacket(packet_data)) {
        return BypassType::SNI;
    } else if (IsIPSIDRPacket(packet_data)) {
        return BypassType::IP_SIDR;
    } else {
        return BypassType::MIXED;
    }
}

double BypassDetector::CalculateSNIScore(const std::vector<uint8_t>& packet_data) {
    if (packet_data.empty()) {
        return 0.0;
    }
    
    // Анализ SNI паттернов
    double sni_score = 0.0;
    
    // Проверка на TLS handshake
    if (packet_data.size() > 5 && packet_data[0] == 0x16) {
        sni_score += 0.3;
    }
    
    // Проверка на HTTP Host header
    std::string packet_str(packet_data.begin(), packet_data.end());
    if (packet_str.find("Host:") != std::string::npos) {
        sni_score += 0.4;
    }
    
    // Проверка на российские домены
    for (const auto& domain : config_.sni_domains) {
        if (packet_str.find(domain) != std::string::npos) {
            sni_score += 0.3;
            break;
        }
    }
    
    return std::min(1.0, sni_score);
}

double BypassDetector::CalculateIPSIDRScore(const std::vector<uint8_t>& packet_data) {
    if (packet_data.empty()) {
        return 0.0;
    }
    
    // Анализ IP SIDR паттернов
    double ip_sidr_score = 0.0;
    
    // Проверка на IP заголовки
    if (packet_data.size() > 20) {
        // Симуляция анализа IP заголовков
        ip_sidr_score += 0.2;
    }
    
    // Проверка на российские IP диапазоны
    // TODO: Реализовать проверку IP диапазонов
    
    // Проверка на CDN трафик
    std::string packet_str(packet_data.begin(), packet_data.end());
    if (packet_str.find("cdn") != std::string::npos || packet_str.find("cloud") != std::string::npos) {
        ip_sidr_score += 0.3;
    }
    
    return std::min(1.0, ip_sidr_score);
}

bool BypassDetector::IsSNIPacket(const std::vector<uint8_t>& packet_data) {
    if (packet_data.empty()) {
        return false;
    }
    
    // Проверка на TLS handshake
    if (packet_data.size() > 5 && packet_data[0] == 0x16) {
        return true;
    }
    
    // Проверка на HTTP Host header
    std::string packet_str(packet_data.begin(), packet_data.end());
    return packet_str.find("Host:") != std::string::npos;
}

bool BypassDetector::IsIPSIDRPacket(const std::vector<uint8_t>& packet_data) {
    if (packet_data.empty()) {
        return false;
    }
    
    // Проверка на IP заголовки
    if (packet_data.size() > 20) {
        // Симуляция проверки IP заголовков
        return true;
    }
    
    return false;
}

void BypassDetector::UpdateStats(BypassType bypass_type, bool success) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.packets_processed++;
    if (!success) {
        stats_.packets_blocked++;
    }
    
    // Обновление статистики успешности
    double success_rate = static_cast<double>(stats_.packets_processed - stats_.packets_blocked) / stats_.packets_processed;
    stats_.success_rate = success_rate;
    
    stats_.last_activity = std::chrono::system_clock::now();
}

void BypassDetector::HandleBypassFailure(BypassType bypass_type, const std::string& error) {
    stats_.last_error = error;
    
    if (on_bypass_failed_) {
        on_bypass_failed_(bypass_type, error);
    }
    
    std::cout << "[BypassDetector] Ошибка обхода " << (int)bypass_type << ": " << error << std::endl;
}

// BypassOptimizer
BypassOptimizer::BypassOptimizer() 
    : running_(false) {
}

BypassOptimizer::~BypassOptimizer() {
    if (running_) {
        running_ = false;
        if (optimization_thread_.joinable()) {
            optimization_thread_.join();
        }
    }
}

bool BypassOptimizer::Initialize(const BypassConfig& config) {
    config_ = config;
    stats_.bypass_id = "bypass_optimizer_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    stats_.current_type = BypassType::SNI;
    stats_.state = BypassState::INACTIVE;
    
    std::cout << "[BypassOptimizer] Инициализация оптимизатора:" << std::endl;
    std::cout << "  Тип обхода: " << (int)config.bypass_type << std::endl;
    std::cout << "  Порог успеха: " << config.success_threshold << std::endl;
    std::cout << "  Порог неудачи: " << config.failure_threshold << std::endl;
    
    return true;
}

BypassType BypassOptimizer::OptimizeBypass(BypassType current_type, const BypassStats& stats) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Анализ текущей производительности
    double performance = AnalyzePerformance(current_type, stats);
    
    std::cout << "[BypassOptimizer] Анализ производительности: " << performance << std::endl;
    
    // Определение необходимости оптимизации
    if (performance < config_.failure_threshold) {
        BypassType optimized_type = SelectBestBypassType(stats);
        
        if (optimized_type != current_type) {
            stats_.current_type = optimized_type;
            stats_.optimization_count++;
            
            if (on_optimization_complete_) {
                on_optimization_complete_(optimized_type);
            }
            
            std::cout << "[BypassOptimizer] Оптимизация: " << (int)current_type << " -> " << (int)optimized_type << std::endl;
        }
        
        return optimized_type;
    }
    
    return current_type;
}

double BypassOptimizer::AnalyzePerformance(BypassType bypass_type, const BypassStats& stats) {
    // Анализ производительности обхода
    double performance = 0.0;
    
    // Базовый балл за тип обхода
    switch (bypass_type) {
        case BypassType::SNI:
            performance = 0.8;
            break;
        case BypassType::IP_SIDR:
            performance = 0.75;
            break;
        case BypassType::MIXED:
            performance = 0.85;
            break;
        case BypassType::ADAPTIVE:
            performance = 0.9;
            break;
        case BypassType::AI_DRIVEN:
            performance = 0.95;
            break;
    }
    
    // Модификация на основе статистики
    performance *= stats.success_rate;
    performance *= (1.0 - stats.average_latency_ms / 1000.0); // Штраф за задержку
    
    return std::max(0.0, std::min(1.0, performance));
}

std::vector<std::string> BypassOptimizer::GetOptimizationRecommendations(BypassType bypass_type, const BypassStats& stats) {
    std::vector<std::string> recommendations;
    
    if (stats.success_rate < 0.8) {
        recommendations.push_back("Увеличить количество резервных доменов");
    }
    
    if (stats.average_latency_ms > 200.0) {
        recommendations.push_back("Оптимизировать маршрутизацию");
    }
    
    if (stats.packets_blocked > stats.packets_processed * 0.1) {
        recommendations.push_back("Изменить стратегию обхода");
    }
    
    return recommendations;
}

BypassStats BypassOptimizer::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void BypassOptimizer::SetOnOptimizationComplete(std::function<void(BypassType)> callback) {
    on_optimization_complete_ = callback;
}

void BypassOptimizer::SetOnOptimizationFailed(std::function<void(const std::string&)> callback) {
    on_optimization_failed_ = callback;
}

void BypassOptimizer::OptimizationLoop() {
    std::cout << "[BypassOptimizer] Optimization loop запущен" << std::endl;
    
    while (running_) {
        // Симуляция оптимизации
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.optimization_interval_ms));
    }
}

BypassType BypassOptimizer::SelectBestBypassType(const BypassStats& stats) {
    // Выбор лучшего типа обхода
    BypassType best_type = BypassType::SNI;
    double best_score = 0.0;
    
    for (int type = 0; type < 5; ++type) {
        BypassType current_type = static_cast<BypassType>(type);
        double score = AnalyzePerformance(current_type, stats);
        
        if (score > best_score) {
            best_score = score;
            best_type = current_type;
        }
    }
    
    return best_type;
}

double BypassOptimizer::CalculateBypassScore(BypassType bypass_type, const BypassStats& stats) {
    // Расчет балла обхода
    double score = 0.0;
    
    // Базовый балл
    switch (bypass_type) {
        case BypassType::SNI:
            score = 0.8;
            break;
        case BypassType::IP_SIDR:
            score = 0.75;
            break;
        case BypassType::MIXED:
            score = 0.85;
            break;
        case BypassType::ADAPTIVE:
            score = 0.9;
            break;
        case BypassType::AI_DRIVEN:
            score = 0.95;
            break;
    }
    
    // Модификация на основе статистики
    score *= stats.success_rate;
    score *= (1.0 - stats.average_latency_ms / 1000.0);
    
    return std::max(0.0, std::min(1.0, score));
}

bool BypassOptimizer::ShouldSwitchBypassType(BypassType current_type, BypassType recommended_type) {
    return current_type != recommended_type;
}

void BypassOptimizer::UpdateStats(BypassType bypass_type, bool success) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.current_type = bypass_type;
    stats_.last_activity = std::chrono::system_clock::now();
}

void BypassOptimizer::HandleOptimizationFailure(const std::string& error) {
    stats_.last_error = error;
    
    if (on_optimization_failed_) {
        on_optimization_failed_(error);
    }
    
    std::cout << "[BypassOptimizer] Ошибка оптимизации: " << error << std::endl;
}

// BypassManager
BypassManager::BypassManager() {
}

BypassManager::~BypassManager() {
}

bool BypassManager::Initialize() {
    std::cout << "[BypassManager] Инициализация менеджера" << std::endl;
    return true;
}

std::string BypassManager::CreateBypass(const BypassConfig& config) {
    std::lock_guard<std::mutex> lock(bypasses_mutex_);
    
    if (!ValidateBypassConfig(config)) {
        return "";
    }
    
    std::string bypass_id = GenerateBypassId();
    
    // Создание детектора
    auto detector = std::make_shared<BypassDetector>();
    if (detector->Initialize(config)) {
        detectors_[bypass_id] = detector;
    }
    
    // Создание оптимизатора
    auto optimizer = std::make_shared<BypassOptimizer>();
    if (optimizer->Initialize(config)) {
        optimizers_[bypass_id] = optimizer;
    }
    
    bypass_configs_[bypass_id] = config;
    
    std::cout << "[BypassManager] Создан обход: " << bypass_id << std::endl;
    
    return bypass_id;
}

bool BypassManager::RemoveBypass(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(bypasses_mutex_);
    
    auto detector_it = detectors_.find(bypass_id);
    if (detector_it != detectors_.end()) {
        detectors_.erase(detector_it);
    }
    
    auto optimizer_it = optimizers_.find(bypass_id);
    if (optimizer_it != optimizers_.end()) {
        optimizers_.erase(optimizer_it);
    }
    
    bypass_configs_.erase(bypass_id);
    
    std::cout << "[BypassManager] Удален обход: " << bypass_id << std::endl;
    
    return true;
}

bool BypassManager::StartBypass(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(bypasses_mutex_);
    
    auto detector_it = detectors_.find(bypass_id);
    if (detector_it != detectors_.end()) {
        // TODO: Запуск детектора
        return true;
    }
    
    return false;
}

bool BypassManager::StopBypass(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(bypasses_mutex_);
    
    auto detector_it = detectors_.find(bypass_id);
    if (detector_it != detectors_.end()) {
        // TODO: Остановка детектора
        return true;
    }
    
    return false;
}

bool BypassManager::OptimizeBypass(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(bypasses_mutex_);
    
    auto optimizer_it = optimizers_.find(bypass_id);
    if (optimizer_it != optimizers_.end()) {
        // TODO: Оптимизация обхода
        return true;
    }
    
    return false;
}

BypassStats BypassManager::GetBypassStats(const std::string& bypass_id) const {
    std::lock_guard<std::mutex> lock(bypasses_mutex_);
    
    auto detector_it = detectors_.find(bypass_id);
    if (detector_it != detectors_.end()) {
        return detector_it->second->GetStats();
    }
    
    return BypassStats{};
}

std::vector<std::string> BypassManager::GetAllBypasses() const {
    std::lock_guard<std::mutex> lock(bypasses_mutex_);
    
    std::vector<std::string> bypass_ids;
    for (const auto& pair : bypass_configs_) {
        bypass_ids.push_back(pair.first);
    }
    
    return bypass_ids;
}

void BypassManager::SetOnBypassStateChange(std::function<void(const std::string&, BypassState)> callback) {
    on_bypass_state_change_ = callback;
}

void BypassManager::SetOnBypassTypeChange(std::function<void(const std::string&, BypassType)> callback) {
    on_bypass_type_change_ = callback;
}

void BypassManager::SetOnBypassOptimized(std::function<void(const std::string&, BypassType)> callback) {
    on_bypass_optimized_ = callback;
}

std::string BypassManager::GenerateBypassId() {
    return "bypass_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

bool BypassManager::ValidateBypassConfig(const BypassConfig& config) {
    if (config.sni_domains.empty() && config.ip_ranges.empty()) {
        return false;
    }
    
    return true;
}

void BypassManager::OptimizeBypassConfig(BypassConfig& config) {
    // Оптимизация конфигурации обхода
    if (config.detection_interval_ms <= 0) {
        config.detection_interval_ms = 5000;
    }
    
    if (config.optimization_interval_ms <= 0) {
        config.optimization_interval_ms = 30000;
    }
    
    if (config.success_threshold <= 0.0 || config.success_threshold > 1.0) {
        config.success_threshold = 0.8;
    }
    
    if (config.failure_threshold <= 0.0 || config.failure_threshold > 1.0) {
        config.failure_threshold = 0.3;
    }
}

void BypassManager::HandleBypassStateChange(const std::string& bypass_id, BypassState new_state) {
    if (on_bypass_state_change_) {
        on_bypass_state_change_(bypass_id, new_state);
    }
}

void BypassManager::HandleBypassTypeChange(const std::string& bypass_id, BypassType new_type) {
    if (on_bypass_type_change_) {
        on_bypass_type_change_(bypass_id, new_type);
    }
}

void BypassManager::HandleBypassOptimized(const std::string& bypass_id, BypassType optimized_type) {
    if (on_bypass_optimized_) {
        on_bypass_optimized_(bypass_id, optimized_type);
    }
}

// BypassTrafficMaskIntegration
BypassTrafficMaskIntegration::BypassTrafficMaskIntegration() {
    bypass_manager_ = std::make_shared<BypassManager>();
    bypass_manager_->Initialize();
}

BypassTrafficMaskIntegration::~BypassTrafficMaskIntegration() {
}

bool BypassTrafficMaskIntegration::IntegrateWithReverseTunnel(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(integration_mutex_);
    
    std::cout << "[BypassTrafficMaskIntegration] Интеграция с reverse tunnel: " << bypass_id << std::endl;
    
    // TODO: Реализовать интеграцию с reverse tunnel
    return true;
}

bool BypassTrafficMaskIntegration::IntegrateWithAIAnalysis(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(integration_mutex_);
    
    std::cout << "[BypassTrafficMaskIntegration] Интеграция с AI анализом: " << bypass_id << std::endl;
    
    // TODO: Реализовать интеграцию с AI анализом
    return true;
}

bool BypassTrafficMaskIntegration::IntegrateWithDynamicSNI(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(integration_mutex_);
    
    std::cout << "[BypassTrafficMaskIntegration] Интеграция с динамическим SNI: " << bypass_id << std::endl;
    
    // TODO: Реализовать интеграцию с динамическим SNI
    return true;
}

bool BypassTrafficMaskIntegration::AdaptForRussiaServices(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(integration_mutex_);
    
    std::cout << "[BypassTrafficMaskIntegration] Адаптация для российских сервисов: " << bypass_id << std::endl;
    
    // TODO: Реализовать адаптацию для российских сервисов
    return true;
}

std::unordered_map<std::string, double> BypassTrafficMaskIntegration::GetAIMetrics(const std::string& bypass_id) {
    std::lock_guard<std::mutex> lock(integration_mutex_);
    
    // Получение статистики обхода
    auto stats = bypass_manager_->GetBypassStats(bypass_id);
    
    // Извлечение метрик для AI
    return ExtractMetrics(stats);
}

BypassConfig BypassTrafficMaskIntegration::CreateRussiaBypassConfig() {
    BypassConfig config;
    config.bypass_type = BypassType::AI_DRIVEN;
    config.sni_domains = GetRussiaSNIDomains();
    config.ip_ranges = GetRussiaIPRanges();
    config.auto_detection = true;
    config.ai_optimization = true;
    config.detection_interval_ms = 5000;
    config.optimization_interval_ms = 30000;
    config.success_threshold = 0.8;
    config.failure_threshold = 0.3;
    
    return config;
}

BypassConfig BypassTrafficMaskIntegration::AdaptConfigForRussia(const BypassConfig& config) {
    BypassConfig adapted_config = config;
    
    // Адаптация для российских сервисов
    if (adapted_config.sni_domains.empty()) {
        adapted_config.sni_domains = GetRussiaSNIDomains();
    }
    
    if (adapted_config.ip_ranges.empty()) {
        adapted_config.ip_ranges = GetRussiaIPRanges();
    }
    
    adapted_config.bypass_type = SelectBestRussiaBypassType();
    adapted_config.auto_detection = true;
    adapted_config.ai_optimization = true;
    
    return adapted_config;
}

bool BypassTrafficMaskIntegration::ApplyRussiaOptimizations(const std::string& bypass_id) {
    std::cout << "[BypassTrafficMaskIntegration] Применение российских оптимизаций: " << bypass_id << std::endl;
    
    // TODO: Реализовать российские оптимизации
    return true;
}

std::unordered_map<std::string, double> BypassTrafficMaskIntegration::ExtractMetrics(const BypassStats& stats) {
    std::unordered_map<std::string, double> metrics;
    
    metrics["success_rate"] = stats.success_rate;
    metrics["latency"] = stats.average_latency_ms;
    metrics["packets_processed"] = static_cast<double>(stats.packets_processed);
    metrics["packets_blocked"] = static_cast<double>(stats.packets_blocked);
    metrics["detection_count"] = static_cast<double>(stats.detection_count);
    metrics["optimization_count"] = static_cast<double>(stats.optimization_count);
    
    return metrics;
}

std::vector<std::string> BypassTrafficMaskIntegration::GetRussiaSNIDomains() {
    return {
        "vk.com", "mail.ru", "yandex.ru", "ok.ru", "rambler.ru",
        "rutracker.org", "gismeteo.ru", "1c.ru", "habr.com"
    };
}

std::vector<std::string> BypassTrafficMaskIntegration::GetRussiaIPRanges() {
    return {
        "77.88.8.8", "94.100.180.200", "87.240.190.72", "81.19.70.1",
        "217.20.147.1", "178.154.131.1", "5.45.207.0/24", "185.71.76.0/24"
    };
}

BypassType BypassTrafficMaskIntegration::SelectBestRussiaBypassType() {
    // Выбор лучшего типа обхода для России
    std::vector<BypassType> russia_types = {
        BypassType::AI_DRIVEN,
        BypassType::ADAPTIVE,
        BypassType::MIXED
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, russia_types.size() - 1);
    
    return russia_types[dis(gen)];
}

} // namespace TrafficMask

#include "dynamic_sni.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>

namespace TrafficMask {

DynamicSNIManager::DynamicSNIManager() 
    : state_(SNIState::ERROR)
    , running_(false) {
}

DynamicSNIManager::~DynamicSNIManager() {
    Stop();
}

bool DynamicSNIManager::Initialize(const DynamicSNIConfig& config) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    config_ = config;
    state_ = SNIState::ACTIVE;
    
    // Инициализация пула SNI
    if (config_.sni_pool.empty()) {
        config_.sni_pool = config_.russia_domains;
    }
    
    if (!config_.sni_pool.empty()) {
        current_sni_ = config_.sni_pool[0];
    }
    
    std::cout << "[DynamicSNI] Инициализация с " << config_.sni_pool.size() << " SNI" << std::endl;
    std::cout << "[DynamicSNI] Текущий SNI: " << current_sni_ << std::endl;
    std::cout << "[DynamicSNI] Стратегия: " << (int)config_.default_strategy << std::endl;
    
    return true;
}

bool DynamicSNIManager::Start() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (running_) {
        return false;
    }
    
    running_ = true;
    state_ = SNIState::ACTIVE;
    
    // Запуск worker thread
    worker_thread_ = std::thread(&DynamicSNIManager::WorkerLoop, this);
    
    // Запуск scanner thread
    if (config_.auto_scan) {
        scanner_thread_ = std::thread(&DynamicSNIManager::ScannerLoop, this);
    }
    
    std::cout << "[DynamicSNI] Менеджер запущен" << std::endl;
    return true;
}

void DynamicSNIManager::Stop() {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!running_) {
            return;
        }
        
        running_ = false;
        state_ = SNIState::ERROR;
    }
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    if (scanner_thread_.joinable()) {
        scanner_thread_.join();
    }
    
    std::cout << "[DynamicSNI] Менеджер остановлен" << std::endl;
}

std::string DynamicSNIManager::GetCurrentSNI() const {
    std::lock_guard<std::mutex> lock(sni_mutex_);
    return current_sni_;
}

bool DynamicSNIManager::SwitchSNI(const std::string& new_sni) {
    std::lock_guard<std::mutex> lock(sni_mutex_);
    
    if (new_sni == current_sni_) {
        return true;
    }
    
    std::cout << "[DynamicSNI] Смена SNI: " << current_sni_ << " -> " << new_sni << std::endl;
    
    if (PerformSNISwitch(new_sni)) {
        current_sni_ = new_sni;
        
        if (on_sni_change_) {
            on_sni_change_(current_sni_);
        }
        
        return true;
    }
    
    return false;
}

bool DynamicSNIManager::AutoSwitchSNI() {
    std::string next_sni = SelectNextSNI();
    if (next_sni.empty()) {
        return false;
    }
    
    return SwitchSNI(next_sni);
}

std::vector<SNIScanResult> DynamicSNIManager::ScanNewSNI() {
    SNIScanner scanner;
    return scanner.ScanSNI(config_.sni_pool);
}

std::unordered_map<std::string, double> DynamicSNIManager::GetSNIStatistics() const {
    std::lock_guard<std::mutex> lock(sni_mutex_);
    return sni_statistics_;
}

void DynamicSNIManager::SetOnSNIChange(std::function<void(const std::string&)> callback) {
    on_sni_change_ = callback;
}

void DynamicSNIManager::SetOnScanResult(std::function<void(const std::vector<SNIScanResult>&)> callback) {
    on_scan_result_ = callback;
}

void DynamicSNIManager::SetOnStateChange(std::function<void(SNIState)> callback) {
    on_state_change_ = callback;
}

void DynamicSNIManager::WorkerLoop() {
    std::cout << "[DynamicSNI] Worker loop запущен" << std::endl;
    
    while (running_) {
        // Проверка необходимости смены SNI
        if (ShouldSwitchSNI()) {
            AutoSwitchSNI();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.switch_interval_ms));
    }
}

void DynamicSNIManager::ScannerLoop() {
    std::cout << "[DynamicSNI] Scanner loop запущен" << std::endl;
    
    while (running_) {
        // Сканирование новых SNI
        auto scan_results = ScanNewSNI();
        
        if (on_scan_result_) {
            on_scan_result_(scan_results);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.scan_interval_ms));
    }
}

bool DynamicSNIManager::PerformSNISwitch(const std::string& new_sni) {
    SetState(SNIState::SWITCHING);
    
    // Симуляция переключения SNI
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    SetState(SNIState::ACTIVE);
    return true;
}

std::string DynamicSNIManager::SelectNextSNI() {
    if (config_.sni_pool.empty()) {
        return "";
    }
    
    switch (config_.default_strategy) {
        case SNIStrategy::RANDOM: {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, config_.sni_pool.size() - 1);
            return config_.sni_pool[dis(gen)];
        }
        
        case SNIStrategy::ROUND_ROBIN: {
            static size_t current_index = 0;
            std::string next_sni = config_.sni_pool[current_index];
            current_index = (current_index + 1) % config_.sni_pool.size();
            return next_sni;
        }
        
        case SNIStrategy::ADAPTIVE:
        case SNIStrategy::AI_DRIVEN: {
            return GetBestSNI();
        }
        
        case SNIStrategy::FALLBACK: {
            for (const auto& sni : config_.fallback_domains) {
                if (failed_sni_.find(sni) == failed_sni_.end()) {
                    return sni;
                }
            }
            return config_.fallback_domains.empty() ? config_.sni_pool[0] : config_.fallback_domains[0];
        }
        
        default:
            return config_.sni_pool[0];
    }
}

SNIScanResult DynamicSNIManager::TestSNI(const std::string& sni) {
    SNIScanner scanner;
    auto results = scanner.ScanSNI({sni});
    return results.empty() ? SNIScanResult{} : results[0];
}

void DynamicSNIManager::UpdateStatistics(const std::string& sni, bool success, double response_time) {
    std::lock_guard<std::mutex> lock(sni_mutex_);
    
    if (success) {
        sni_statistics_[sni + "_success"]++;
        sni_statistics_[sni + "_response_time"] = response_time;
    } else {
        sni_statistics_[sni + "_failures"]++;
        failed_sni_.insert(sni);
    }
}

void DynamicSNIManager::SetState(SNIState new_state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (state_ != new_state) {
        state_ = new_state;
        if (on_state_change_) {
            on_state_change_(state_);
        }
    }
}

double DynamicSNIManager::AnalyzeSNIEffectiveness(const std::string& sni) {
    std::lock_guard<std::mutex> lock(sni_mutex_);
    
    double success_rate = sni_statistics_[sni + "_success"] / 
                         (sni_statistics_[sni + "_success"] + sni_statistics_[sni + "_failures"]);
    
    return success_rate;
}

std::string DynamicSNIManager::GetBestSNI() {
    std::string best_sni;
    double best_score = 0.0;
    
    for (const auto& sni : config_.sni_pool) {
        if (failed_sni_.find(sni) != failed_sni_.end()) {
            continue;
        }
        
        double score = AnalyzeSNIEffectiveness(sni);
        if (score > best_score) {
            best_score = score;
            best_sni = sni;
        }
    }
    
    return best_sni.empty() ? config_.sni_pool[0] : best_sni;
}

bool DynamicSNIManager::ShouldSwitchSNI() {
    if (config_.default_strategy == SNIStrategy::AI_DRIVEN) {
        double current_effectiveness = AnalyzeSNIEffectiveness(current_sni_);
        return current_effectiveness < config_.success_threshold;
    }
    
    return true;
}

// SNIScanner
SNIScanner::SNIScanner() {
}

SNIScanner::~SNIScanner() {
}

std::vector<SNIScanResult> SNIScanner::ScanSNI(const std::vector<std::string>& sni_list) {
    std::vector<SNIScanResult> results;
    
    for (const auto& sni : sni_list) {
        SNIScanResult result = TestSingleSNI(sni);
        results.push_back(result);
    }
    
    return results;
}

std::vector<SNIScanResult> SNIScanner::ScanRussiaDomains() {
    std::vector<std::string> russia_domains = {
        "vk.com", "mail.ru", "yandex.ru", "ok.ru", "rambler.ru",
        "rutracker.org", "gismeteo.ru", "1c.ru"
    };
    
    return ScanSNI(russia_domains);
}

std::vector<SNIScanResult> SNIScanner::ScanWithAIAnalysis(const std::vector<std::string>& sni_list) {
    std::vector<SNIScanResult> results = ScanSNI(sni_list);
    
    // AI анализ результатов
    SNIAIAnalyzer ai_analyzer;
    for (auto& result : results) {
        if (result.is_accessible) {
            double effectiveness = ai_analyzer.AnalyzeEffectiveness(result.sni);
            result.success_rate = effectiveness;
        }
    }
    
    return results;
}

SNIScanResult SNIScanner::TestSingleSNI(const std::string& sni) {
    SNIScanResult result;
    result.sni = sni;
    result.timestamp = std::chrono::system_clock::now();
    
    // Симуляция тестирования SNI
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    result.is_accessible = dis(gen) > 0.2; // 80% успешность
    result.response_time_ms = 50.0 + dis(gen) * 200.0; // 50-250ms
    result.success_rate = result.is_accessible ? dis(gen) : 0.0;
    
    if (!result.is_accessible) {
        result.error_message = "Connection timeout";
    }
    
    return result;
}

bool SNIScanner::IsSNIAccessible(const std::string& sni) {
    // Симуляция проверки доступности SNI
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen) > 0.2;
}

double SNIScanner::MeasureResponseTime(const std::string& sni) {
    // Симуляция измерения времени отклика
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(50.0, 250.0);
    
    return dis(gen);
}

std::string SNIScanner::GetErrorForSNI(const std::string& sni) {
    std::vector<std::string> errors = {
        "Connection timeout",
        "DNS resolution failed",
        "SSL handshake error",
        "Network unreachable"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, errors.size() - 1);
    
    return errors[dis(gen)];
}

// SNIAIAnalyzer
SNIAIAnalyzer::SNIAIAnalyzer() {
}

SNIAIAnalyzer::~SNIAIAnalyzer() {
}

double SNIAIAnalyzer::AnalyzeEffectiveness(const std::string& sni) {
    std::lock_guard<std::mutex> lock(model_mutex_);
    
    auto it = sni_performance_.find(sni);
    if (it != sni_performance_.end()) {
        return it->second;
    }
    
    // Новый SNI - средний балл
    return 0.5;
}

std::string SNIAIAnalyzer::RecommendBestSNI(const std::vector<std::string>& available_sni) {
    std::string best_sni;
    double best_score = 0.0;
    
    for (const auto& sni : available_sni) {
        double score = AnalyzeEffectiveness(sni);
        if (score > best_score) {
            best_score = score;
            best_sni = sni;
        }
    }
    
    return best_sni;
}

double SNIAIAnalyzer::AssessDetectionRisk(const std::string& sni) {
    // Анализ риска обнаружения для SNI
    std::lock_guard<std::mutex> lock(model_mutex_);
    
    auto it = sni_performance_.find(sni);
    if (it != sni_performance_.end()) {
        return 1.0 - it->second; // Обратная зависимость
    }
    
    return 0.5; // Средний риск для новых SNI
}

void SNIAIAnalyzer::UpdateModel(const std::string& sni, bool success, double performance) {
    std::lock_guard<std::mutex> lock(model_mutex_);
    
    sni_usage_count_[sni]++;
    
    if (sni_performance_.find(sni) == sni_performance_.end()) {
        sni_performance_[sni] = performance;
    } else {
        // Экспоненциальное скользящее среднее
        double alpha = 0.1;
        sni_performance_[sni] = alpha * performance + (1 - alpha) * sni_performance_[sni];
    }
}

double SNIAIAnalyzer::CalculateSNIScore(const std::string& sni) {
    std::lock_guard<std::mutex> lock(model_mutex_);
    
    auto perf_it = sni_performance_.find(sni);
    auto usage_it = sni_usage_count_.find(sni);
    
    double performance = (perf_it != sni_performance_.end()) ? perf_it->second : 0.5;
    int usage_count = (usage_it != sni_usage_count_.end()) ? usage_it->second : 0;
    
    // Бонус за частоту использования
    double usage_bonus = std::min(usage_count * 0.01, 0.2);
    
    return performance + usage_bonus;
}

void SNIAIAnalyzer::UpdateSNIPerformance(const std::string& sni, double score) {
    std::lock_guard<std::mutex> lock(model_mutex_);
    sni_performance_[sni] = score;
}

} // namespace TrafficMask

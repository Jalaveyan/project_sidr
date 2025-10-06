#include "reverse_tunnel.h"
#include <iostream>
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace TrafficMask {

ReverseTunnelManager::ReverseTunnelManager() 
    : state_(TunnelState::DISCONNECTED)
    , role_(TunnelRole::INITIATOR)
    , running_(false) {
}

ReverseTunnelManager::~ReverseTunnelManager() {
    StopTunnel();
}

bool ReverseTunnelManager::Initialize(const ReverseTunnelConfig& config) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    config_ = config;
    state_ = TunnelState::DISCONNECTED;
    
    std::cout << "[ReverseTunnel] Инициализация с конфигурацией:" << std::endl;
    std::cout << "  Local: " << config.local_endpoint << std::endl;
    std::cout << "  Remote: " << config.remote_endpoint << std::endl;
    std::cout << "  API: " << config.api_endpoint << std::endl;
    std::cout << "  AI Analysis: " << (config.ai_analysis ? "Включен" : "Выключен") << std::endl;
    
    return true;
}

bool ReverseTunnelManager::StartTunnel() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (running_) {
        return false;
    }
    
    running_ = true;
    state_ = TunnelState::CONNECTING;
    
    if (on_state_change_) {
        on_state_change_(state_);
    }
    
    // Запуск worker thread
    worker_thread_ = std::thread(&ReverseTunnelManager::WorkerLoop, this);
    
    std::cout << "[ReverseTunnel] Запуск reverse tunnel..." << std::endl;
    return true;
}

void ReverseTunnelManager::StopTunnel() {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!running_) {
            return;
        }
        
        running_ = false;
        state_ = TunnelState::DISCONNECTED;
    }
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    if (on_state_change_) {
        on_state_change_(state_);
    }
    
    std::cout << "[ReverseTunnel] Туннель остановлен" << std::endl;
}

bool ReverseTunnelManager::SwitchRoles() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (state_ != TunnelState::CONNECTED) {
        return false;
    }
    
    state_ = TunnelState::ROLE_SWITCHING;
    if (on_state_change_) {
        on_state_change_(state_);
    }
    
    std::cout << "[ReverseTunnel] Смена ролей: " 
              << (role_ == TunnelRole::INITIATOR ? "INITIATOR" : "RECEIVER")
              << " -> ";
    
    // Смена роли
    role_ = (role_ == TunnelRole::INITIATOR) ? TunnelRole::RECEIVER : TunnelRole::INITIATOR;
    
    std::cout << (role_ == TunnelRole::INITIATOR ? "INITIATOR" : "RECEIVER") << std::endl;
    
    if (on_role_switch_) {
        on_role_switch_(role_);
    }
    
    state_ = TunnelState::ACTIVE;
    if (on_state_change_) {
        on_state_change_(state_);
    }
    
    return true;
}

AIAnalysisResult ReverseTunnelManager::AnalyzeTraffic() {
    return PerformAIAnalysis();
}

void ReverseTunnelManager::SetOnStateChange(std::function<void(TunnelState)> callback) {
    on_state_change_ = callback;
}

void ReverseTunnelManager::SetOnRoleSwitch(std::function<void(TunnelRole)> callback) {
    on_role_switch_ = callback;
}

void ReverseTunnelManager::SetOnAIAnalysis(std::function<void(AIAnalysisResult)> callback) {
    on_ai_analysis_ = callback;
}

void ReverseTunnelManager::WorkerLoop() {
    std::cout << "[ReverseTunnel] Worker loop запущен" << std::endl;
    
    // Установка соединения
    if (!EstablishConnection()) {
        std::cerr << "[ReverseTunnel] Ошибка установки соединения" << std::endl;
        return;
    }
    
    // Ожидание перед сменой ролей
    if (config_.auto_switch) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.role_switch_delay_ms));
        SwitchRoles();
    }
    
    // Основной цикл с AI анализом
    while (running_) {
        // AI анализ каждые 2 секунды
        if (config_.ai_analysis) {
            AIAnalysisResult analysis = PerformAIAnalysis();
            
            if (on_ai_analysis_) {
                on_ai_analysis_(analysis);
            }
            
            // Автоматические действия на основе AI анализа
            if (analysis.switch_roles) {
                std::cout << "[ReverseTunnel] AI рекомендует смену ролей" << std::endl;
                SwitchRoles();
            }
            
            if (analysis.change_sni) {
                std::cout << "[ReverseTunnel] AI рекомендует смену SNI" << std::endl;
                // TODO: Реализовать смену SNI
            }
            
            if (analysis.use_fallback) {
                std::cout << "[ReverseTunnel] AI активирует резервный механизм" << std::endl;
                // TODO: Активировать резервный механизм
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

bool ReverseTunnelManager::EstablishConnection() {
    std::cout << "[ReverseTunnel] Установка соединения..." << std::endl;
    
    // Симуляция установки соединения
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_ = TunnelState::CONNECTED;
    }
    
    if (on_state_change_) {
        on_state_change_(state_);
    }
    
    std::cout << "[ReverseTunnel] Соединение установлено" << std::endl;
    return true;
}

bool ReverseTunnelManager::PerformRoleSwitch() {
    return SwitchRoles();
}

AIAnalysisResult ReverseTunnelManager::PerformAIAnalysis() {
    AIAnalysisResult result;
    
    // Генерация случайных данных для симуляции
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Анализ энтропии
    result.entropy_score = dis(gen);
    
    // Анализ корреляции MAC/IP
    result.mac_ip_correlation = dis(gen);
    
    // Обнаружение CDN
    result.cdn_detection_score = dis(gen);
    
    // Оценка риска DPI
    result.dpi_risk_score = dis(gen);
    
    // Логика принятия решений на основе анализа
    if (result.dpi_risk_score > 0.7) {
        result.recommended_action = "SWITCH_ROLES";
        result.switch_roles = true;
    } else if (result.cdn_detection_score > 0.8) {
        result.recommended_action = "CHANGE_SNI";
        result.change_sni = true;
    } else if (result.entropy_score < 0.3) {
        result.recommended_action = "USE_FALLBACK";
        result.use_fallback = true;
    } else {
        result.recommended_action = "CONTINUE";
    }
    
    std::cout << "[ReverseTunnel] AI анализ: " << result.recommended_action 
              << " (DPI риск: " << result.dpi_risk_score << ")" << std::endl;
    
    return result;
}

double ReverseTunnelManager::CalculateEntropy(const std::vector<uint8_t>& data) {
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
    
    return entropy;
}

double ReverseTunnelManager::AnalyzeMACIPCorrelation() {
    // Симуляция анализа корреляции MAC/IP
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

double ReverseTunnelManager::DetectCDNTraffic() {
    // Симуляция обнаружения CDN трафика
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

double ReverseTunnelManager::AssessDPIRisk() {
    // Симуляция оценки риска DPI
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen);
}

bool ReverseTunnelManager::SendDataToAPI(const std::string& data) {
    // TODO: Реализовать отправку данных через S3/YaDocs API
    std::cout << "[ReverseTunnel] Отправка данных в API: " << data.size() << " байт" << std::endl;
    return true;
}

std::string ReverseTunnelManager::ReceiveDataFromAPI() {
    // TODO: Реализовать получение данных через S3/YaDocs API
    std::cout << "[ReverseTunnel] Получение данных из API" << std::endl;
    return "received_data";
}

// Фабрика
std::unique_ptr<ReverseTunnelManager> ReverseTunnelFactory::CreateTunnel(
    const ReverseTunnelConfig& config) {
    
    auto tunnel = std::make_unique<ReverseTunnelManager>();
    if (!tunnel->Initialize(config)) {
        return nullptr;
    }
    return tunnel;
}

std::unique_ptr<ReverseTunnelManager> ReverseTunnelFactory::CreateAITunnel(
    const ReverseTunnelConfig& config) {
    
    auto tunnel = std::make_unique<ReverseTunnelManager>();
    if (!tunnel->Initialize(config)) {
        return nullptr;
    }
    return tunnel;
}

} // namespace TrafficMask

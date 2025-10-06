#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace TrafficMask {

// Роли в reverse tunnel
enum class TunnelRole {
    INITIATOR,    // Инициатор соединения
    RECEIVER,     // Принимающая сторона
    SWITCHED      // После смены ролей
};

// Состояние туннеля
enum class TunnelState {
    CONNECTING,
    CONNECTED,
    ROLE_SWITCHING,
    ACTIVE,
    DISCONNECTED,
    ERROR
};

// Конфигурация reverse tunnel
struct ReverseTunnelConfig {
    std::string local_endpoint;
    std::string remote_endpoint;
    std::string api_endpoint;        // Для обмена данными (S3/YaDocs)
    std::string encryption_key;
    int role_switch_delay_ms = 5000;  // Задержка перед сменой ролей
    bool auto_switch = true;          // Автоматическая смена ролей
    bool ai_analysis = true;          // AI анализ трафика
};

// AI анализ трафика
struct AIAnalysisResult {
    double entropy_score;           // Энтропия пакетов
    double mac_ip_correlation;     // Корреляция MAC/IP
    double cdn_detection_score;    // Обнаружение CDN
    double dpi_risk_score;         // Риск DPI обнаружения
    std::string recommended_action; // Рекомендуемое действие
    bool switch_roles;             // Нужна ли смена ролей
    bool change_sni;               // Нужна ли смена SNI
    bool use_fallback;             // Использовать резервный механизм
};

// Reverse Tunnel Manager
class ReverseTunnelManager {
public:
    ReverseTunnelManager();
    ~ReverseTunnelManager();

    // Инициализация с конфигурацией
    bool Initialize(const ReverseTunnelConfig& config);
    
    // Запуск reverse tunnel
    bool StartTunnel();
    
    // Остановка туннеля
    void StopTunnel();
    
    // Смена ролей (клиент <-> сервер)
    bool SwitchRoles();
    
    // AI анализ текущего трафика
    AIAnalysisResult AnalyzeTraffic();
    
    // Получение текущего состояния
    TunnelState GetState() const { return state_; }
    TunnelRole GetRole() const { return role_; }
    
    // Callback для событий
    void SetOnStateChange(std::function<void(TunnelState)> callback);
    void SetOnRoleSwitch(std::function<void(TunnelRole)> callback);
    void SetOnAIAnalysis(std::function<void(AIAnalysisResult)> callback);

private:
    ReverseTunnelConfig config_;
    TunnelState state_;
    TunnelRole role_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::mutex state_mutex_;
    
    // Callbacks
    std::function<void(TunnelState)> on_state_change_;
    std::function<void(TunnelRole)> on_role_switch_;
    std::function<void(AIAnalysisResult)> on_ai_analysis_;
    
    // Внутренние методы
    void WorkerLoop();
    bool EstablishConnection();
    bool PerformRoleSwitch();
    AIAnalysisResult PerformAIAnalysis();
    bool SendDataToAPI(const std::string& data);
    std::string ReceiveDataFromAPI();
    
    // Анализ пакетов для AI
    double CalculateEntropy(const std::vector<uint8_t>& data);
    double AnalyzeMACIPCorrelation();
    double DetectCDNTraffic();
    double AssessDPIRisk();
};

// Фабрика для создания reverse tunnel
class ReverseTunnelFactory {
public:
    static std::unique_ptr<ReverseTunnelManager> CreateTunnel(
        const ReverseTunnelConfig& config);
    
    // Создание туннеля с AI анализом
    static std::unique_ptr<ReverseTunnelManager> CreateAITunnel(
        const ReverseTunnelConfig& config);
};

} // namespace TrafficMask

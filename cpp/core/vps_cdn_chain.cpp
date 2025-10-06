#include "vps_cdn_chain.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>

namespace TrafficMask {

// VpsCdnChainManager
VpsCdnChainManager::VpsCdnChainManager() 
    : running_(false) {
}

VpsCdnChainManager::~VpsCdnChainManager() {
    if (running_) {
        running_ = false;
        if (optimization_thread_.joinable()) {
            optimization_thread_.join();
        }
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
    }
}

bool VpsCdnChainManager::Initialize() {
    running_ = true;
    optimization_thread_ = std::thread(&VpsCdnChainManager::OptimizationLoop, this);
    monitoring_thread_ = std::thread(&VpsCdnChainManager::MonitoringLoop, this);
    
    std::cout << "[VpsCdnChainManager] Инициализация завершена" << std::endl;
    return true;
}

std::string VpsCdnChainManager::CreateChain(const ChainConfig& config) {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    if (!ValidateChainConfig(config)) {
        return "";
    }
    
    std::string chain_id = config.chain_id;
    if (chain_id.empty()) {
        chain_id = "chain_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }
    
    chains_[chain_id] = config;
    
    // Инициализация статистики
    ChainStats stats;
    stats.chain_id = chain_id;
    stats.is_active = false;
    stats.active_nodes = 0;
    stats.total_nodes = config.nodes.size();
    stats.created_at = std::chrono::system_clock::now();
    chain_stats_[chain_id] = stats;
    
    // Сохранение узлов цепочки
    std::vector<std::string> node_ids;
    for (const auto& node : config.nodes) {
        node_ids.push_back(node.node_id);
    }
    chain_nodes_[chain_id] = node_ids;
    
    std::cout << "[VpsCdnChainManager] Создана цепочка: " << chain_id 
              << " (" << config.nodes.size() << " узлов)" << std::endl;
    
    return chain_id;
}

bool VpsCdnChainManager::RemoveChain(const std::string& chain_id) {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    auto it = chains_.find(chain_id);
    if (it != chains_.end()) {
        chains_.erase(it);
        chain_stats_.erase(chain_id);
        chain_nodes_.erase(chain_id);
        
        std::cout << "[VpsCdnChainManager] Удалена цепочка: " << chain_id << std::endl;
        return true;
    }
    
    return false;
}

bool VpsCdnChainManager::StartChain(const std::string& chain_id) {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    auto it = chains_.find(chain_id);
    if (it == chains_.end()) {
        return false;
    }
    
    if (StartChainNodes(chain_id)) {
        chain_stats_[chain_id].is_active = true;
        
        if (on_chain_state_change_) {
            on_chain_state_change_(chain_id, true);
        }
        
        std::cout << "[VpsCdnChainManager] Запущена цепочка: " << chain_id << std::endl;
        return true;
    }
    
    return false;
}

bool VpsCdnChainManager::StopChain(const std::string& chain_id) {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    auto it = chains_.find(chain_id);
    if (it == chains_.end()) {
        return false;
    }
    
    if (StopChainNodes(chain_id)) {
        chain_stats_[chain_id].is_active = false;
        
        if (on_chain_state_change_) {
            on_chain_state_change_(chain_id, false);
        }
        
        std::cout << "[VpsCdnChainManager] Остановлена цепочка: " << chain_id << std::endl;
        return true;
    }
    
    return false;
}

ChainStats VpsCdnChainManager::GetChainStats(const std::string& chain_id) const {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    auto it = chain_stats_.find(chain_id);
    return (it != chain_stats_.end()) ? it->second : ChainStats{};
}

std::vector<std::string> VpsCdnChainManager::GetAllChains() const {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    std::vector<std::string> chain_ids;
    for (const auto& pair : chains_) {
        chain_ids.push_back(pair.first);
    }
    
    return chain_ids;
}

bool VpsCdnChainManager::OptimizeChain(const std::string& chain_id) {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    auto chain_it = chains_.find(chain_id);
    auto stats_it = chain_stats_.find(chain_id);
    
    if (chain_it == chains_.end() || stats_it == chain_stats_.end()) {
        return false;
    }
    
    ChainOptimizer optimizer;
    ChainConfig optimized_config = optimizer.OptimizeChain(chain_it->second, stats_it->second);
    
    // Применение оптимизированной конфигурации
    chains_[chain_id] = optimized_config;
    chain_stats_[chain_id].last_optimization = std::chrono::system_clock::now();
    
    if (on_optimization_complete_) {
        on_optimization_complete_(chain_id, chain_stats_[chain_id]);
    }
    
    std::cout << "[VpsCdnChainManager] Оптимизирована цепочка: " << chain_id << std::endl;
    return true;
}

bool VpsCdnChainManager::SwitchToBackupChain(const std::string& chain_id) {
    std::lock_guard<std::mutex> lock(chains_mutex_);
    
    // TODO: Реализовать переключение на резервную цепочку
    std::cout << "[VpsCdnChainManager] Переключение на резервную цепочку: " << chain_id << std::endl;
    return true;
}

void VpsCdnChainManager::SetOnChainStateChange(std::function<void(const std::string&, bool)> callback) {
    on_chain_state_change_ = callback;
}

void VpsCdnChainManager::SetOnNodeStateChange(std::function<void(const std::string&, const std::string&, NodeState)> callback) {
    on_node_state_change_ = callback;
}

void VpsCdnChainManager::SetOnOptimizationComplete(std::function<void(const std::string&, const ChainStats&)> callback) {
    on_optimization_complete_ = callback;
}

void VpsCdnChainManager::OptimizationLoop() {
    std::cout << "[VpsCdnChainManager] Optimization loop запущен" << std::endl;
    
    while (running_) {
        std::lock_guard<std::mutex> lock(chains_mutex_);
        
        for (const auto& pair : chains_) {
            const std::string& chain_id = pair.first;
            const auto& config = pair.second;
            
            if (config.auto_optimization) {
                OptimizeChain(chain_id);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

void VpsCdnChainManager::MonitoringLoop() {
    std::cout << "[VpsCdnChainManager] Monitoring loop запущен" << std::endl;
    
    while (running_) {
        std::lock_guard<std::mutex> lock(chains_mutex_);
        
        for (const auto& pair : chains_) {
            const std::string& chain_id = pair.first;
            UpdateChainStats(chain_id);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

bool VpsCdnChainManager::ValidateChainConfig(const ChainConfig& config) {
    if (config.nodes.empty()) {
        return false;
    }
    
    for (const auto& node : config.nodes) {
        if (node.endpoint.empty() || node.port <= 0) {
            return false;
        }
    }
    
    return true;
}

bool VpsCdnChainManager::StartChainNodes(const std::string& chain_id) {
    auto it = chains_.find(chain_id);
    if (it == chains_.end()) {
        return false;
    }
    
    const auto& config = it->second;
    int active_count = 0;
    
    for (const auto& node : config.nodes) {
        // Симуляция запуска узла
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        NodeState node_state = NodeState::ONLINE;
        if (on_node_state_change_) {
            on_node_state_change_(chain_id, node.node_id, node_state);
        }
        
        active_count++;
    }
    
    chain_stats_[chain_id].active_nodes = active_count;
    return true;
}

bool VpsCdnChainManager::StopChainNodes(const std::string& chain_id) {
    auto it = chains_.find(chain_id);
    if (it == chains_.end()) {
        return false;
    }
    
    const auto& config = it->second;
    
    for (const auto& node : config.nodes) {
        // Симуляция остановки узла
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        NodeState node_state = NodeState::OFFLINE;
        if (on_node_state_change_) {
            on_node_state_change_(chain_id, node.node_id, node_state);
        }
    }
    
    chain_stats_[chain_id].active_nodes = 0;
    return true;
}

void VpsCdnChainManager::UpdateChainStats(const std::string& chain_id) {
    auto it = chain_stats_.find(chain_id);
    if (it == chain_stats_.end()) {
        return;
    }
    
    ChainStats& stats = it->second;
    
    // Обновление статистики
    stats.average_latency = 50.0 + (std::rand() % 100); // 50-150ms
    stats.total_bandwidth = 100.0 + (std::rand() % 500); // 100-600 Mbps
    stats.overall_success_rate = 0.8 + (std::rand() % 20) / 100.0; // 80-100%
    
    // Обновление статистики узлов
    stats.node_statistics.clear();
    for (const auto& node_id : chain_nodes_[chain_id]) {
        NodeStats node_stats;
        node_stats.node_id = node_id;
        node_stats.state = NodeState::ONLINE;
        node_stats.latency_ms = 30.0 + (std::rand() % 100);
        node_stats.bandwidth_mbps = 50.0 + (std::rand() % 200);
        node_stats.success_rate = 0.85 + (std::rand() % 15) / 100.0;
        node_stats.bytes_transferred = std::rand() % 1000000;
        node_stats.connection_count = 1 + (std::rand() % 10);
        node_stats.last_activity = std::chrono::system_clock::now();
        
        stats.node_statistics.push_back(node_stats);
    }
}

std::string VpsCdnChainManager::SelectBestNode(const std::string& chain_id) {
    auto it = chain_stats_.find(chain_id);
    if (it == chain_stats_.end()) {
        return "";
    }
    
    const auto& stats = it->second;
    if (stats.node_statistics.empty()) {
        return "";
    }
    
    // Выбор узла с лучшей производительностью
    auto best_node = std::max_element(stats.node_statistics.begin(), stats.node_statistics.end(),
        [](const NodeStats& a, const NodeStats& b) {
            return a.success_rate < b.success_rate;
        });
    
    return best_node->node_id;
}

bool VpsCdnChainManager::PerformFailover(const std::string& chain_id, const std::string& failed_node) {
    std::cout << "[VpsCdnChainManager] Failover для узла " << failed_node << " в цепочке " << chain_id << std::endl;
    
    // TODO: Реализовать failover логику
    return true;
}

ChainStats VpsCdnChainManager::CalculateChainStats(const std::string& chain_id) {
    auto it = chain_stats_.find(chain_id);
    if (it == chain_stats_.end()) {
        return ChainStats{};
    }
    
    return it->second;
}

// ChainOptimizer
ChainOptimizer::ChainOptimizer() {
}

ChainOptimizer::~ChainOptimizer() {
}

ChainConfig ChainOptimizer::OptimizeChain(const ChainConfig& config, const ChainStats& stats) {
    std::lock_guard<std::mutex> lock(optimizer_mutex_);
    
    ChainConfig optimized_config = config;
    
    // Анализ производительности
    double performance = AnalyzePerformance(stats);
    
    if (performance < 0.7) {
        // Оптимизация узлов
        optimized_config.nodes = SelectBestNodes(config.nodes, config.nodes.size());
        
        // Улучшение параметров
        for (auto& node : optimized_config.nodes) {
            if (node.timeout_ms < 30000) {
                node.timeout_ms = 30000;
            }
            if (node.retry_count < 3) {
                node.retry_count = 3;
            }
        }
    }
    
    std::cout << "[ChainOptimizer] Оптимизация цепочки " << config.chain_id 
              << " (производительность: " << performance << ")" << std::endl;
    
    return optimized_config;
}

std::vector<ChainNodeConfig> ChainOptimizer::SelectBestNodes(const std::vector<ChainNodeConfig>& nodes, int count) {
    std::vector<ChainNodeConfig> best_nodes = nodes;
    
    // Сортировка по приоритету и весу
    std::sort(best_nodes.begin(), best_nodes.end(),
        [](const ChainNodeConfig& a, const ChainNodeConfig& b) {
            if (a.priority != b.priority) {
                return a.priority > b.priority;
            }
            return a.weight > b.weight;
        });
    
    if (count < best_nodes.size()) {
        best_nodes.resize(count);
    }
    
    return best_nodes;
}

double ChainOptimizer::AnalyzePerformance(const ChainStats& stats) {
    if (stats.node_statistics.empty()) {
        return 0.0;
    }
    
    double total_success_rate = 0.0;
    double total_latency = 0.0;
    
    for (const auto& node : stats.node_statistics) {
        total_success_rate += node.success_rate;
        total_latency += node.latency_ms;
    }
    
    double avg_success_rate = total_success_rate / stats.node_statistics.size();
    double avg_latency = total_latency / stats.node_statistics.size();
    
    // Расчет общего балла производительности
    double performance_score = avg_success_rate * 0.7 + (1.0 - avg_latency / 1000.0) * 0.3;
    
    return std::max(0.0, std::min(1.0, performance_score));
}

std::vector<std::string> ChainOptimizer::GetOptimizationRecommendations(const ChainStats& stats) {
    std::vector<std::string> recommendations;
    
    if (stats.overall_success_rate < 0.8) {
        recommendations.push_back("Увеличить количество резервных узлов");
    }
    
    if (stats.average_latency > 200.0) {
        recommendations.push_back("Оптимизировать маршрутизацию");
    }
    
    if (stats.total_bandwidth < 100.0) {
        recommendations.push_back("Увеличить пропускную способность");
    }
    
    return recommendations;
}

double ChainOptimizer::CalculateNodeScore(const ChainNodeConfig& node, const NodeStats& stats) {
    double score = 0.0;
    
    // Балл за приоритет
    score += node.priority * 0.3;
    
    // Балл за вес
    score += node.weight * 0.2;
    
    // Балл за производительность
    score += stats.success_rate * 0.3;
    
    // Балл за задержку (обратная зависимость)
    score += (1.0 - stats.latency_ms / 1000.0) * 0.2;
    
    return std::max(0.0, std::min(1.0, score));
}

std::vector<ChainNodeConfig> ChainOptimizer::SortNodesByScore(const std::vector<ChainNodeConfig>& nodes, const std::vector<NodeStats>& stats) {
    std::vector<std::pair<ChainNodeConfig, double>> scored_nodes;
    
    for (size_t i = 0; i < nodes.size() && i < stats.size(); ++i) {
        double score = CalculateNodeScore(nodes[i], stats[i]);
        scored_nodes.push_back({nodes[i], score});
    }
    
    std::sort(scored_nodes.begin(), scored_nodes.end(),
        [](const std::pair<ChainNodeConfig, double>& a, const std::pair<ChainNodeConfig, double>& b) {
            return a.second > b.second;
        });
    
    std::vector<ChainNodeConfig> sorted_nodes;
    for (const auto& pair : scored_nodes) {
        sorted_nodes.push_back(pair.first);
    }
    
    return sorted_nodes;
}

bool ChainOptimizer::ShouldReplaceNode(const NodeStats& node_stats) {
    return node_stats.success_rate < 0.5 || node_stats.latency_ms > 500.0;
}

ChainNodeConfig ChainOptimizer::FindReplacementNode(const std::vector<ChainNodeConfig>& available_nodes, const NodeStats& current_stats) {
    if (available_nodes.empty()) {
        return ChainNodeConfig{};
    }
    
    // Поиск узла с лучшими характеристиками
    auto best_node = std::max_element(available_nodes.begin(), available_nodes.end(),
        [&current_stats](const ChainNodeConfig& a, const ChainNodeConfig& b) {
            return a.priority < b.priority;
        });
    
    return *best_node;
}

// ChainMonitor
ChainMonitor::ChainMonitor() 
    : running_(false) {
}

ChainMonitor::~ChainMonitor() {
    if (running_) {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
}

void ChainMonitor::StartMonitoring(const std::string& chain_id) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    monitored_chains_.insert(chain_id);
    
    if (!running_) {
        running_ = true;
        monitor_thread_ = std::thread(&ChainMonitor::MonitorLoop, this);
    }
    
    std::cout << "[ChainMonitor] Начат мониторинг цепочки: " << chain_id << std::endl;
}

void ChainMonitor::StopMonitoring(const std::string& chain_id) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    monitored_chains_.erase(chain_id);
    
    std::cout << "[ChainMonitor] Остановлен мониторинг цепочки: " << chain_id << std::endl;
}

bool ChainMonitor::CheckNodeHealth(const std::string& node_id) {
    return PerformHealthCheck(node_id);
}

std::unordered_map<std::string, double> ChainMonitor::GetNodeMetrics(const std::string& node_id) {
    return CollectNodeMetrics(node_id);
}

void ChainMonitor::SetOnNodeHealthChange(std::function<void(const std::string&, bool)> callback) {
    on_node_health_change_ = callback;
}

void ChainMonitor::SetOnPerformanceAlert(std::function<void(const std::string&, const std::string&)> callback) {
    on_performance_alert_ = callback;
}

void ChainMonitor::MonitorLoop() {
    std::cout << "[ChainMonitor] Monitor loop запущен" << std::endl;
    
    while (running_) {
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        
        for (const auto& chain_id : monitored_chains_) {
            AnalyzePerformanceTrends(chain_id);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

bool ChainMonitor::PerformHealthCheck(const std::string& node_id) {
    // Симуляция проверки здоровья узла
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    bool is_healthy = dis(gen) > 0.1; // 90% вероятность здорового состояния
    
    if (on_node_health_change_) {
        on_node_health_change_(node_id, is_healthy);
    }
    
    return is_healthy;
}

std::unordered_map<std::string, double> ChainMonitor::CollectNodeMetrics(const std::string& node_id) {
    std::unordered_map<std::string, double> metrics;
    
    // Симуляция сбора метрик
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    metrics["latency"] = 30.0 + dis(gen) * 100.0;
    metrics["bandwidth"] = 50.0 + dis(gen) * 200.0;
    metrics["success_rate"] = 0.8 + dis(gen) * 0.2;
    metrics["cpu_usage"] = dis(gen);
    metrics["memory_usage"] = dis(gen);
    
    return metrics;
}

void ChainMonitor::AnalyzePerformanceTrends(const std::string& chain_id) {
    // Анализ трендов производительности
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    if (dis(gen) < 0.05) { // 5% вероятность предупреждения
        if (on_performance_alert_) {
            on_performance_alert_(chain_id, "Низкая производительность");
        }
    }
}

// BackupChainManager
BackupChainManager::BackupChainManager() {
}

BackupChainManager::~BackupChainManager() {
}

bool BackupChainManager::AddBackupChain(const std::string& primary_chain_id, const std::string& backup_chain_id) {
    std::lock_guard<std::mutex> lock(backup_mutex_);
    
    if (!ValidateBackupChain(backup_chain_id)) {
        return false;
    }
    
    backup_chains_[primary_chain_id].push_back(backup_chain_id);
    
    std::cout << "[BackupChainManager] Добавлена резервная цепочка " << backup_chain_id 
              << " для " << primary_chain_id << std::endl;
    
    return true;
}

bool BackupChainManager::RemoveBackupChain(const std::string& primary_chain_id) {
    std::lock_guard<std::mutex> lock(backup_mutex_);
    
    backup_chains_.erase(primary_chain_id);
    active_backups_.erase(primary_chain_id);
    
    std::cout << "[BackupChainManager] Удалена резервная цепочка для " << primary_chain_id << std::endl;
    
    return true;
}

bool BackupChainManager::SwitchToBackup(const std::string& primary_chain_id) {
    std::lock_guard<std::mutex> lock(backup_mutex_);
    
    auto it = backup_chains_.find(primary_chain_id);
    if (it == backup_chains_.end() || it->second.empty()) {
        return false;
    }
    
    std::string backup_chain_id = it->second[0]; // Выбор первой резервной цепочки
    active_backups_[primary_chain_id] = backup_chain_id;
    
    NotifyBackupSwitch(primary_chain_id, backup_chain_id);
    
    std::cout << "[BackupChainManager] Переключение на резервную цепочку " << backup_chain_id 
              << " для " << primary_chain_id << std::endl;
    
    return true;
}

bool BackupChainManager::SwitchToPrimary(const std::string& primary_chain_id) {
    std::lock_guard<std::mutex> lock(backup_mutex_);
    
    active_backups_.erase(primary_chain_id);
    
    std::cout << "[BackupChainManager] Возврат на основную цепочку " << primary_chain_id << std::endl;
    
    return true;
}

std::vector<std::string> BackupChainManager::GetBackupChains(const std::string& primary_chain_id) const {
    std::lock_guard<std::mutex> lock(backup_mutex_);
    
    auto it = backup_chains_.find(primary_chain_id);
    return (it != backup_chains_.end()) ? it->second : std::vector<std::string>();
}

bool BackupChainManager::ValidateBackupChain(const std::string& backup_chain_id) {
    // TODO: Реализовать валидацию резервной цепочки
    return !backup_chain_id.empty();
}

void BackupChainManager::NotifyBackupSwitch(const std::string& primary_chain_id, const std::string& backup_chain_id) {
    std::cout << "[BackupChainManager] Уведомление о переключении: " << primary_chain_id 
              << " -> " << backup_chain_id << std::endl;
}

// RussiaServiceIntegration
RussiaServiceIntegration::RussiaServiceIntegration() {
    russia_providers_ = {"Yandex", "Mail.ru", "VK", "Rambler", "OK.ru"};
    russia_regions_ = {"Moscow", "StPetersburg", "Novosibirsk", "Ekaterinburg", "Kazan"};
    russia_cdn_providers_ = {"Yandex CDN", "Mail.ru CDN", "VK CDN", "Rambler CDN"};
}

RussiaServiceIntegration::~RussiaServiceIntegration() {
}

ChainConfig RussiaServiceIntegration::AdaptChainForRussia(const ChainConfig& config) {
    ChainConfig adapted_config = config;
    
    // Добавление российских узлов
    adapted_config.nodes = AddRussiaNodes(config.nodes);
    
    // Адаптация параметров для России
    for (auto& node : adapted_config.nodes) {
        if (node.region.empty()) {
            node.region = SelectBestRussiaRegion();
        }
        if (node.provider.empty()) {
            node.provider = "Russia Provider";
        }
    }
    
    std::cout << "[RussiaServiceIntegration] Адаптация цепочки для России: " << config.chain_id << std::endl;
    
    return adapted_config;
}

std::vector<ChainNodeConfig> RussiaServiceIntegration::AddRussiaNodes(const std::vector<ChainNodeConfig>& nodes) {
    std::vector<ChainNodeConfig> enhanced_nodes = nodes;
    
    // Добавление российских CDN узлов
    for (const auto& cdn_provider : russia_cdn_providers_) {
        ChainNodeConfig russia_node = CreateRussiaNode(cdn_provider, SelectBestRussiaRegion());
        enhanced_nodes.push_back(russia_node);
    }
    
    return enhanced_nodes;
}

bool RussiaServiceIntegration::OptimizeForRussiaProviders(const std::string& chain_id) {
    std::cout << "[RussiaServiceIntegration] Оптимизация для российских провайдеров: " << chain_id << std::endl;
    
    // TODO: Реализовать оптимизацию для российских провайдеров
    return true;
}

std::vector<ChainNodeConfig> RussiaServiceIntegration::GetRussiaCDNNodes() {
    std::vector<ChainNodeConfig> cdn_nodes;
    
    for (const auto& cdn_provider : russia_cdn_providers_) {
        ChainNodeConfig cdn_node = CreateRussiaNode(cdn_provider, SelectBestRussiaRegion());
        cdn_nodes.push_back(cdn_node);
    }
    
    return cdn_nodes;
}

ChainNodeConfig RussiaServiceIntegration::CreateRussiaNode(const std::string& provider, const std::string& region) {
    ChainNodeConfig node;
    node.node_id = "russia_" + provider + "_" + region;
    node.endpoint = "russia-" + region + ".example.com";
    node.port = 443;
    node.type = ChainNodeType::CDN;
    node.region = region;
    node.provider = provider;
    node.priority = 2;
    node.weight = 1.0;
    node.timeout_ms = 30000;
    node.retry_count = 3;
    node.auto_failover = true;
    
    return node;
}

bool RussiaServiceIntegration::IsRussiaProvider(const std::string& provider) {
    return std::find(russia_providers_.begin(), russia_providers_.end(), provider) != russia_providers_.end();
}

std::string RussiaServiceIntegration::SelectBestRussiaRegion() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, russia_regions_.size() - 1);
    
    return russia_regions_[dis(gen)];
}

} // namespace TrafficMask

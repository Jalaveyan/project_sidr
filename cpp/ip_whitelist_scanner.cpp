#include "ip_whitelist_scanner.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace NeuralTunnel {

// AI Analyzer
AIWhitelistAnalyzer::AIWhitelistAnalyzer() {
    // Инициализация простой нейросети
    model_.weights.resize(3, std::vector<double>(4, 0.5));
    model_.biases.resize(3, 0.1);
}

AIWhitelistAnalyzer::WhitelistProfile AIWhitelistAnalyzer::analyzeResults(
    const std::vector<IPScanResult>& results) {
    
    WhitelistProfile profile;
    profile.confidence = 0.0;
    
    for (const auto& result : results) {
        if (result.is_accessible && result.success_count > 0) {
            double success_rate = (double)result.success_count / result.total_attempts;
            
            if (success_rate > 0.9) {
                profile.confirmed_ips.push_back(result.ip);
                if (!result.subnet.empty()) {
                    profile.confirmed_subnets.push_back(result.subnet);
                }
            } else if (success_rate > 0.5) {
                profile.likely_ips.push_back(result.ip);
            } else {
                profile.blocked_ips.push_back(result.ip);
            }
            
            profile.ip_scores[result.ip] = success_rate;
        }
    }
    
    // Рассчитываем уверенность
    if (!results.empty()) {
        int accessible = 0;
        for (const auto& r : results) {
            if (r.is_accessible) accessible++;
        }
        profile.confidence = (double)accessible / results.size();
    }
    
    std::cout << "[AI Whitelist] Анализ завершен:" << std::endl;
    std::cout << "  Подтвержденных IP: " << profile.confirmed_ips.size() << std::endl;
    std::cout << "  Вероятных IP: " << profile.likely_ips.size() << std::endl;
    std::cout << "  Заблокированных: " << profile.blocked_ips.size() << std::endl;
    std::cout << "  Уверенность: " << (profile.confidence * 100) << "%" << std::endl;
    
    return profile;
}

double AIWhitelistAnalyzer::predictIPSuccess(const std::string& ip) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    auto it = ip_history_.find(ip);
    if (it != ip_history_.end()) {
        int success = it->second.first;
        int total = it->second.second;
        return total > 0 ? (double)success / total : 0.5;
    }
    
    // Для новых IP используем эвристику
    auto features = extractIPFeatures(ip);
    return calculateIPScore(ip);
}

void AIWhitelistAnalyzer::learnFromAttempt(const std::string& ip, bool success) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    if (ip_history_.find(ip) == ip_history_.end()) {
        ip_history_[ip] = {0, 0};
    }
    
    if (success) {
        ip_history_[ip].first++;
    }
    ip_history_[ip].second++;
    
    // Обучаем модель каждые 100 попыток
    static int total_attempts = 0;
    total_attempts++;
    if (total_attempts % 100 == 0) {
        trainModel();
    }
}

std::vector<std::string> AIWhitelistAnalyzer::recommendBestIPs(int count) {
    std::vector<std::pair<std::string, double>> scored_ips;
    
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        for (const auto& [ip, stats] : ip_history_) {
            if (stats.second > 0) {
                double score = (double)stats.first / stats.second;
                scored_ips.push_back({ip, score});
            }
        }
    }
    
    // Сортируем по score
    std::sort(scored_ips.begin(), scored_ips.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::string> best_ips;
    for (int i = 0; i < std::min(count, (int)scored_ips.size()); i++) {
        best_ips.push_back(scored_ips[i].first);
    }
    
    return best_ips;
}

std::vector<double> AIWhitelistAnalyzer::extractIPFeatures(const std::string& ip) {
    std::vector<double> features;
    
    // Парсим IP
    unsigned int octets[4] = {0};
    sscanf(ip.c_str(), "%u.%u.%u.%u", &octets[0], &octets[1], &octets[2], &octets[3]);
    
    // Признаки:
    features.push_back(octets[0] / 255.0);  // Первый октет
    features.push_back(octets[1] / 255.0);  // Второй октет
    features.push_back((octets[0] == 8 || octets[0] == 1) ? 1.0 : 0.0);  // Google/Cloudflare
    features.push_back((octets[0] >= 77 && octets[0] <= 95) ? 1.0 : 0.0);  // Российские диапазоны
    
    return features;
}

double AIWhitelistAnalyzer::calculateIPScore(const std::string& ip) {
    auto features = extractIPFeatures(ip);
    
    // Эвристика для российских IP
    double score = 0.5;
    
    // Яндекс (высокий приоритет)
    if (ip.find("77.88.") == 0 || ip.find("5.255.") == 0 || ip.find("87.250.") == 0) {
        score = 0.95;
    }
    // Mail.ru, VK (высокий приоритет)
    else if (ip.find("94.100.") == 0 || ip.find("87.240.") == 0) {
        score = 0.90;
    }
    // Сбербанк, Госуслуги (средний приоритет)
    else if (ip.find("195.161.") == 0 || ip.find("188.254.") == 0) {
        score = 0.85;
    }
    // Wildberries, Ozon (средний приоритет)
    else if (ip.find("178.154.") == 0 || ip.find("185.179.") == 0) {
        score = 0.80;
    }
    // Ростелеком (низкий приоритет, но доступен)
    else if (ip.find("212.48.") == 0 || ip.find("213.234.") == 0) {
        score = 0.75;
    }
    
    return score;
}

void AIWhitelistAnalyzer::trainModel() {
    // Простое обучение на истории
    std::cout << "[AI] Обучение модели на " << ip_history_.size() << " примерах" << std::endl;
}

std::vector<std::string> AIWhitelistAnalyzer::detectIPPatterns() {
    std::vector<std::string> patterns;
    
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    // Анализируем первые октеты успешных IP
    std::map<int, int> first_octet_counts;
    for (const auto& [ip, stats] : ip_history_) {
        if (stats.first > 0) {  // Если были успехи
            unsigned int first_octet;
            sscanf(ip.c_str(), "%u.", &first_octet);
            first_octet_counts[first_octet]++;
        }
    }
    
    // Находим популярные диапазоны
    for (const auto& [octet, count] : first_octet_counts) {
        if (count >= 3) {
            patterns.push_back(std::to_string(octet) + ".0.0.0/8");
        }
    }
    
    return patterns;
}

// IP Whitelist Scanner
IPWhitelistScanner::IPWhitelistScanner() : scanning_(false) {
    ai_analyzer_ = std::make_unique<AIWhitelistAnalyzer>();
    initializeKnownServices();
}

IPWhitelistScanner::~IPWhitelistScanner() {
    stopScanning();
}

void IPWhitelistScanner::initializeKnownServices() {
    // Только российские сервисы и их IP диапазоны
    
    // Яндекс
    known_services_["Yandex"] = {
        "77.88.8.0/24",      // Яндекс DNS
        "5.255.255.0/24",    // Яндекс
        "87.250.250.0/24",   // Яндекс
        "213.180.193.0/24",  // Яндекс
        "213.180.204.0/24"   // Яндекс
    };
    
    // Mail.ru
    known_services_["Mail.ru"] = {
        "94.100.180.0/24",   // Mail.ru
        "217.69.139.0/24",   // Mail.ru
        "95.163.0.0/16"      // Mail.ru Group
    };
    
    // VK
    known_services_["VK"] = {
        "87.240.190.0/24",   // VK
        "95.142.192.0/20",   // VK
        "93.186.224.0/20"    // VK
    };
    
    // Одноклассники
    known_services_["OK.ru"] = {
        "217.20.147.0/24",   // OK.ru
        "217.20.151.0/24"    // OK.ru
    };
    
    // Rambler
    known_services_["Rambler"] = {
        "81.19.70.0/24",     // Rambler
        "81.19.72.0/24"      // Rambler
    };
    
    // Сбербанк
    known_services_["Sberbank"] = {
        "195.161.0.0/16",    // Сбербанк
        "194.186.0.0/16"     // Сбербанк
    };
    
    // Ростелеком
    known_services_["Rostelecom"] = {
        "212.48.0.0/16",     // Ростелеком
        "213.234.0.0/16",    // Ростелеком
        "178.176.0.0/16"     // Ростелеком
    };
    
    // Wildberries
    known_services_["Wildberries"] = {
        "178.154.131.0/24",  // Wildberries
        "185.71.76.0/24"     // Wildberries
    };
    
    // Ozon
    known_services_["Ozon"] = {
        "185.179.189.0/24",  // Ozon
        "91.203.4.0/24"      // Ozon
    };
    
    // Госуслуги
    known_services_["Gosuslugi"] = {
        "188.254.0.0/16",    // Госуслуги
        "194.67.0.0/16"      // Госуслуги
    };
    
    std::cout << "[IP Scanner] Инициализировано " << known_services_.size() 
              << " российских сервисов для сканирования" << std::endl;
}

void IPWhitelistScanner::startScanning() {
    if (scanning_) return;
    
    scanning_ = true;
    scanner_thread_ = std::thread(&IPWhitelistScanner::scannerLoop, this);
    
    std::cout << "[IP Scanner] Запущено сканирование белых списков IP" << std::endl;
}

void IPWhitelistScanner::stopScanning() {
    if (!scanning_) return;
    
    scanning_ = false;
    if (scanner_thread_.joinable()) {
        scanner_thread_.join();
    }
    
    std::cout << "[IP Scanner] Сканирование остановлено" << std::endl;
}

void IPWhitelistScanner::scannerLoop() {
    while (scanning_) {
        // Генерируем кандидатов для сканирования
        auto candidates = generateCandidateIPs();
        
        std::cout << "[IP Scanner] Сканирование " << candidates.size() << " IP адресов..." << std::endl;
        
        for (const auto& ip : candidates) {
            if (!scanning_) break;
            
            auto result = scanIP(ip);
            
            {
                std::lock_guard<std::mutex> lock(results_mutex_);
                results_.push_back(result);
                
                // Ограничиваем размер результатов
                if (results_.size() > 10000) {
                    results_.erase(results_.begin());
                }
            }
            
            // Обучаем AI
            ai_analyzer_->learnFromAttempt(result.ip, result.is_accessible);
            
            // Callback
            if (result.is_accessible && on_new_ip_callback_) {
                on_new_ip_callback_(result);
            }
            
            // Небольшая задержка между проверками
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Пауза между циклами сканирования
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

IPScanResult IPWhitelistScanner::scanIP(const std::string& ip) {
    IPScanResult result;
    result.ip = ip;
    result.service_name = identifyService(ip);
    result.total_attempts = 3;
    result.success_count = 0;
    
    // Пробуем подключиться несколько раз
    double total_time = 0;
    for (int i = 0; i < result.total_attempts; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        bool success = testIPConnectivity(ip, 443);
        auto end = std::chrono::high_resolution_clock::now();
        
        if (success) {
            result.success_count++;
            total_time += std::chrono::duration<double, std::milli>(end - start).count();
        }
    }
    
    result.is_accessible = result.success_count > 0;
    result.response_time_ms = result.success_count > 0 ? 
        total_time / result.success_count : 0;
    result.is_whitelisted = result.is_accessible;
    
    return result;
}

bool IPWhitelistScanner::testIPConnectivity(const std::string& ip, int port) {
#ifdef _WIN32
    static bool wsa_initialized = false;
    if (!wsa_initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        wsa_initialized = true;
    }
#endif
    
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return false;
    
    // Non-blocking
    u_long mode = 1;
#ifdef _WIN32
    ioctlsocket(sock, FIONBIO, &mode);
#else
    fcntl(sock, F_SETFL, O_NONBLOCK);
#endif
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    // Wait for connection with timeout
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);
    
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    int result = select(sock + 1, NULL, &writeSet, NULL, &timeout);
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    return result > 0;
}

std::string IPWhitelistScanner::identifyService(const std::string& ip) {
    // Идентификация российских сервисов по IP
    if (ip.find("77.88.") == 0 || ip.find("5.255.") == 0 || ip.find("87.250.") == 0) return "Yandex";
    if (ip.find("94.100.") == 0 || ip.find("217.69.") == 0 || ip.find("95.163.") == 0) return "Mail.ru";
    if (ip.find("87.240.") == 0 || ip.find("95.142.") == 0 || ip.find("93.186.") == 0) return "VK";
    if (ip.find("217.20.") == 0) return "OK.ru";
    if (ip.find("81.19.") == 0) return "Rambler";
    if (ip.find("195.161.") == 0 || ip.find("194.186.") == 0) return "Sberbank";
    if (ip.find("212.48.") == 0 || ip.find("213.234.") == 0 || ip.find("178.176.") == 0) return "Rostelecom";
    if (ip.find("178.154.") == 0 || ip.find("185.71.") == 0) return "Wildberries";
    if (ip.find("185.179.") == 0 || ip.find("91.203.") == 0) return "Ozon";
    if (ip.find("188.254.") == 0 || ip.find("194.67.") == 0) return "Gosuslugi";
    
    return "Unknown";
}

std::vector<std::string> IPWhitelistScanner::generateCandidateIPs() {
    std::vector<std::string> candidates;
    
    // Генерируем IP из известных сервисов
    for (const auto& [service, subnets] : known_services_) {
        for (const auto& subnet : subnets) {
            auto ips = expandSubnet(subnet);
            // Берем первые 5 IP из каждой подсети
            for (int i = 0; i < std::min(5, (int)ips.size()); i++) {
                candidates.push_back(ips[i]);
            }
        }
    }
    
    return candidates;
}

std::vector<std::string> IPWhitelistScanner::expandSubnet(const std::string& subnet) {
    std::vector<std::string> ips;
    
    // Простое расширение для /24 подсетей
    size_t slash_pos = subnet.find('/');
    if (slash_pos == std::string::npos) {
        ips.push_back(subnet);
        return ips;
    }
    
    std::string base = subnet.substr(0, slash_pos);
    int prefix = std::stoi(subnet.substr(slash_pos + 1));
    
    if (prefix == 24) {
        // Для /24 генерируем .1, .2, .8, .53, .80, .443
        std::vector<int> common_hosts = {1, 2, 8, 53, 80, 443};
        size_t last_dot = base.rfind('.');
        std::string network = base.substr(0, last_dot + 1);
        
        for (int host : common_hosts) {
            ips.push_back(network + std::to_string(host));
        }
    }
    
    return ips;
}

std::vector<IPScanResult> IPWhitelistScanner::getResults() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    return results_;
}

AIWhitelistAnalyzer::WhitelistProfile IPWhitelistScanner::getCurrentProfile() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    return ai_analyzer_->analyzeResults(results_);
}

void IPWhitelistScanner::exportToJSON(const std::string& filename) {
    std::lock_guard<std::mutex> lock(results_mutex_);
    
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"whitelist_ips\": [\n";
    
    bool first = true;
    for (const auto& result : results_) {
        if (result.is_accessible) {
            if (!first) file << ",\n";
            first = false;
            
            file << "    {\n";
            file << "      \"ip\": \"" << result.ip << "\",\n";
            file << "      \"service\": \"" << result.service_name << "\",\n";
            file << "      \"response_time\": " << result.response_time_ms << ",\n";
            file << "      \"success_rate\": " << ((double)result.success_count / result.total_attempts) << "\n";
            file << "    }";
        }
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
    
    std::cout << "[IP Scanner] Экспортировано в " << filename << std::endl;
}

// IP Whitelist Adapter
IPWhitelistAdapter::IPWhitelistAdapter() {
    ai_ = std::make_unique<AIWhitelistAnalyzer>();
}

std::string IPWhitelistAdapter::getBestMaskingIP() {
    if (whitelist_ips_.empty()) {
        return "1.1.1.1";  // Fallback to Cloudflare
    }
    
    // Используем AI для выбора лучшего IP
    auto best_ips = ai_->recommendBestIPs(1);
    return best_ips.empty() ? whitelist_ips_[0] : best_ips[0];
}

std::string IPWhitelistAdapter::getIPForService(const std::string& service) {
    auto it = service_to_ip_.find(service);
    if (it != service_to_ip_.end()) {
        return it->second;
    }
    
    return getBestMaskingIP();
}

void IPWhitelistAdapter::updateFromScanner(const IPWhitelistScanner& scanner) {
    auto profile = scanner.getCurrentProfile();
    
    whitelist_ips_.clear();
    whitelist_ips_ = profile.confirmed_ips;
    
    // Добавляем вероятные IP
    whitelist_ips_.insert(whitelist_ips_.end(), 
        profile.likely_ips.begin(), profile.likely_ips.end());
    
    std::cout << "[IP Adapter] Обновлен белый список: " 
              << whitelist_ips_.size() << " IP" << std::endl;
}

bool IPWhitelistAdapter::applyWhitelistRouting(const std::string& destination_ip) {
    // Проверяем, находится ли destination в белом списке
    for (const auto& whitelisted : whitelist_ips_) {
        if (destination_ip == whitelisted) {
            std::cout << "[IP Adapter] Маршрутизация через белый список: " 
                      << destination_ip << std::endl;
            return true;
        }
    }
    
    // Если нет - используем маскировку
    std::string masking_ip = getBestMaskingIP();
    std::cout << "[IP Adapter] Маскировка под: " << masking_ip << std::endl;
    
    return true;
}

} // namespace NeuralTunnel

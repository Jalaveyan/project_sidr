#include "core/trafficmask_nextgen.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

using namespace TrafficMask;

// Глобальная переменная для graceful shutdown
std::atomic<bool> g_running(true);

// Обработчик сигналов
void signalHandler(int signal) {
    std::cout << "\n[Main] Получен сигнал " << signal << ", завершение работы..." << std::endl;
    g_running = false;
}

// Создание конфигурации для России
TrafficMaskConfig createRussiaConfig() {
    TrafficMaskConfig config;
    
    // IPv4/IPv6 настройки
    config.ip_config.version = IPVersion::DUAL_STACK;
    config.ip_config.ipv4_address = "0.0.0.0";
    config.ip_config.ipv6_address = "::";
    config.ip_config.ipv4_port = 443;
    config.ip_config.ipv6_port = 443;
    config.ip_config.auto_detect = true;
    config.ip_config.prefer_ipv6 = false;
    config.ip_config.ipv4_ranges = {"77.88.8.8", "94.100.180.200", "87.240.190.72"};
    config.ip_config.ipv6_ranges = {"2001:db8::1", "fe80::1"};
    
    // Reverse tunnel
    config.reverse_tunnel_config.local_endpoint = "127.0.0.1:8080";
    config.reverse_tunnel_config.remote_endpoint = "russia.example.com:443";
    config.reverse_tunnel_config.api_endpoint = "https://api.russia.example.com";
    config.reverse_tunnel_config.encryption_key = "russia_key_2024";
    config.reverse_tunnel_config.role_switch_delay_ms = 5000;
    config.reverse_tunnel_config.auto_switch = true;
    config.reverse_tunnel_config.ai_analysis = true;
    
    // AI анализ
    config.ai_config.enable_entropy_analysis = true;
    config.ai_config.enable_mac_ip_analysis = true;
    config.ai_config.enable_cdn_detection = true;
    config.ai_config.enable_dpi_risk_assessment = true;
    config.ai_config.enable_protocol_fingerprinting = true;
    config.ai_config.enable_traffic_pattern_analysis = true;
    config.ai_config.enable_sni_analysis = true;
    config.ai_config.enable_ip_sidr_analysis = true;
    config.ai_config.risk_threshold = 0.7;
    config.ai_config.confidence_threshold = 0.8;
    config.ai_config.analysis_interval_ms = 2000;
    config.ai_config.history_size = 100;
    
    // Динамический SNI
    config.sni_config.sni_pool = {"vk.com", "mail.ru", "yandex.ru", "ok.ru", "rambler.ru"};
    config.sni_config.russia_domains = {"vk.com", "mail.ru", "yandex.ru", "ok.ru", "rambler.ru", "rutracker.org", "gismeteo.ru", "1c.ru"};
    config.sni_config.fallback_domains = {"google.com", "microsoft.com", "amazon.com"};
    config.sni_config.default_strategy = SNIStrategy::AI_DRIVEN;
    config.sni_config.switch_interval_ms = 30000;
    config.sni_config.scan_interval_ms = 60000;
    config.sni_config.max_retries = 3;
    config.sni_config.auto_scan = true;
    config.sni_config.ai_analysis = true;
    config.sni_config.success_threshold = 0.8;
    
    // Управление соединениями
    config.connection_config.endpoint = "russia.example.com";
    config.connection_config.port = 443;
    config.connection_config.type = ConnectionType::TCP;
    config.connection_config.priority = ConnectionPriority::HIGH;
    config.connection_config.timeout_ms = 30000;
    config.connection_config.retry_count = 3;
    config.connection_config.retry_delay_ms = 1000;
    config.connection_config.auto_reconnect = true;
    config.connection_config.ai_management = true;
    config.connection_config.encryption_key = "russia_conn_key_2024";
    
    // Hysteria
    config.hysteria_config.server_address = "russia.example.com";
    config.hysteria_config.server_port = 443;
    config.hysteria_config.auth_key = "russia_hysteria_key_2024";
    config.hysteria_config.obfs_password = "russia_obfs_2024";
    config.hysteria_config.bandwidth_mbps = 100;
    config.hysteria_config.mtu = 1200;
    config.hysteria_config.fast_open = true;
    config.hysteria_config.congestion_control = true;
    config.hysteria_config.congestion_algorithm = "bbr";
    config.hysteria_config.timeout_seconds = 30;
    config.hysteria_config.retry_count = 3;
    config.hysteria_config.auto_reconnect = true;
    
    // Trojan
    config.trojan_config.server_address = "russia.example.com";
    config.trojan_config.server_port = 443;
    config.trojan_config.password = "russia_trojan_pass_2024";
    config.trojan_config.method = "aes-256-gcm";
    config.trojan_config.obfs = "tls";
    config.trojan_config.obfs_param = "russia_obfs_param";
    config.trojan_config.sni = "vk.com";
    config.trojan_config.alpn = "h2,http/1.1";
    config.trojan_config.path = "/";
    config.trojan_config.insecure = false;
    config.trojan_config.timeout_seconds = 30;
    config.trojan_config.retry_count = 3;
    config.trojan_config.auto_reconnect = true;
    
    // DNS туннелирование
    config.dns_tunnel_config.domain = "yandex.ru";
    config.dns_tunnel_config.dns_server = "8.8.8.8";
    config.dns_tunnel_config.tunnel_type = DNSTunnelType::TXT_RECORD;
    config.dns_tunnel_config.chunk_size = 64;
    config.dns_tunnel_config.max_retries = 3;
    config.dns_tunnel_config.timeout_seconds = 30;
    config.dns_tunnel_config.compression = true;
    config.dns_tunnel_config.encryption = true;
    config.dns_tunnel_config.encryption_key = "russia_dns_key_2024";
    config.dns_tunnel_config.obfuscation_method = "base32";
    config.dns_tunnel_config.auto_reconnect = true;
    
    // S3/YaDocs API
    config.api_config.type = DataExchangeType::YADOCS;
    config.api_config.endpoint = "docs.yandex.ru";
    config.api_config.access_key = "russia_api_key_2024";
    config.api_config.secret_key = "russia_secret_key_2024";
    config.api_config.bucket_name = "russia-bucket";
    config.api_config.region = "ru-east-1";
    config.api_config.timeout_seconds = 30;
    config.api_config.max_retries = 3;
    config.api_config.encryption = true;
    config.api_config.encryption_key = "russia_api_encryption_2024";
    
    // Обход
    config.bypass_config.bypass_type = BypassType::AI_DRIVEN;
    config.bypass_config.sni_domains = {"vk.com", "mail.ru", "yandex.ru", "ok.ru", "rambler.ru"};
    config.bypass_config.ip_ranges = {"77.88.8.8", "94.100.180.200", "87.240.190.72", "81.19.70.1"};
    config.bypass_config.auto_detection = true;
    config.bypass_config.ai_optimization = true;
    config.bypass_config.detection_interval_ms = 5000;
    config.bypass_config.optimization_interval_ms = 30000;
    config.bypass_config.success_threshold = 0.8;
    config.bypass_config.failure_threshold = 0.3;
    
    // VPS/CDN цепочки
    config.chain_config.chain_id = "russia_chain_2024";
    config.chain_config.name = "Russia Traffic Chain";
    config.chain_config.auto_optimization = true;
    config.chain_config.load_balancing = true;
    config.chain_config.failover_enabled = true;
    config.chain_config.max_retries = 3;
    config.chain_config.health_check_interval = 30.0;
    config.chain_config.optimization_strategy = "ai_driven";
    
    // Общие настройки
    config.enable_all_features = true;
    config.russia_optimization = true;
    config.ai_management = true;
    config.startup_delay_ms = 2000;
    config.log_level = "INFO";
    
    return config;
}

int main() {
    std::cout << "🚀 TrafficMask NextGen - Новое поколение обхода DPI" << std::endl;
    std::cout << "🇷🇺 Российская адаптация с AI-управлением" << std::endl;
    std::cout << "🌐 Поддержка IPv4/IPv6 Dual Stack" << std::endl;
    std::cout << "⚡ Агрессивные методы обхода" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // Установка обработчиков сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Создание конфигурации
    TrafficMaskConfig config = createRussiaConfig();
    
    // Создание и инициализация системы
    TrafficMaskNextGen system;
    
    std::cout << "[Main] Инициализация TrafficMask NextGen..." << std::endl;
    if (!system.Initialize(config)) {
        std::cerr << "[Main] Ошибка инициализации системы!" << std::endl;
        return 1;
    }
    
    // Установка callback'ов
    system.SetOnSystemStart([]() {
        std::cout << "[Main] ✅ Система запущена успешно!" << std::endl;
    });
    
    system.SetOnSystemStop([]() {
        std::cout << "[Main] ⏹️ Система остановлена" << std::endl;
    });
    
    system.SetOnComponentStateChange([](const std::string& component, bool state) {
        std::cout << "[Main] 🔄 Компонент " << component << ": " << (state ? "Запущен" : "Остановлен") << std::endl;
    });
    
    system.SetOnError([](const std::string& error) {
        std::cerr << "[Main] ❌ Ошибка: " << error << std::endl;
    });
    
    system.SetOnStatsUpdate([](const SystemStats& stats) {
        std::cout << "[Main] 📊 Статистика: " << stats.active_connections << " соединений, "
                  << "Успешность: " << (stats.overall_success_rate * 100) << "%, "
                  << "Задержка: " << stats.average_latency_ms << "ms" << std::endl;
    });
    
    // Запуск системы
    std::cout << "[Main] Запуск TrafficMask NextGen..." << std::endl;
    if (!system.Start()) {
        std::cerr << "[Main] Ошибка запуска системы!" << std::endl;
        return 1;
    }
    
    // Основной цикл
    std::cout << "[Main] Система работает. Нажмите Ctrl+C для остановки..." << std::endl;
    
    while (g_running) {
        // Получение статистики
        SystemStats stats = system.GetSystemStats();
        
        // Вывод статистики каждые 10 секунд
        static auto last_stats_time = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time).count() >= 10) {
            std::cout << "\n[Main] 📊 Статистика системы:" << std::endl;
            std::cout << "  🔗 Активных соединений: " << stats.active_connections << std::endl;
            std::cout << "  📈 Общая успешность: " << (stats.overall_success_rate * 100) << "%" << std::endl;
            std::cout << "  ⏱️ Средняя задержка: " << stats.average_latency_ms << "ms" << std::endl;
            std::cout << "  📦 Обработано байт: " << (stats.total_bytes_processed / 1024 / 1024) << "MB" << std::endl;
            std::cout << "  📊 Обработано пакетов: " << stats.total_packets_processed << std::endl;
            std::cout << "  🌐 IP версия: " << (int)stats.active_ip_version << std::endl;
            std::cout << "  🧩 Активных компонентов: " << stats.active_components.size() << std::endl;
            
            if (!stats.active_components.empty()) {
                std::cout << "  📋 Компоненты: ";
                for (size_t i = 0; i < stats.active_components.size(); ++i) {
                    std::cout << stats.active_components[i];
                    if (i < stats.active_components.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << std::endl;
            }
            
            last_stats_time = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Остановка системы
    std::cout << "\n[Main] Остановка TrafficMask NextGen..." << std::endl;
    system.Stop();
    
    std::cout << "[Main] 👋 TrafficMask NextGen завершен" << std::endl;
    return 0;
}

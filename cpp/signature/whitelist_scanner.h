#pragma once

#include "trafficmask.h"
#include <unordered_set>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>

namespace TrafficMask {

// Класс для сканирования разрешенных IP в белом списке
class WhitelistScanner {
public:
    WhitelistScanner();
    ~WhitelistScanner();
    
    // Основные методы
    bool Initialize(const std::string& config_path);
    void StartScanning();
    void StopScanning();
    bool IsScanning() const { return is_scanning_; }
    
    // Управление белым списком
    void AddWhitelistIp(const std::string& ip);
    void RemoveWhitelistIp(const std::string& ip);
    void UpdateWhitelistIp(const std::string& old_ip, const std::string& new_ip);
    
    // Проверка IP
    bool IsIpWhitelisted(const std::string& ip) const;
    bool ShouldMaskIp(const std::string& ip) const;
    
    // Получение статистики
    size_t GetWhitelistSize() const { return whitelist_ips_.size(); }
    size_t GetScanningErrors() const { return scanning_errors_.load(); }
    size_t GetLastScanTime() const { return last_scan_time_.load(); }
    
    // Callback для уведомлений
    void SetIpDiscoveredCallback(std::function<void(const std::string&)> callback) {
        ip_discovered_callback_ = callback;
    }
    
private:
    std::unordered_set<std::string> whitelist_ips_;
    mutable std::mutex whitelist_mutex_;
    
    std::atomic<bool> is_scanning_;
    std::atomic<bool> should_stop_;
    std::atomic<size_t> scanning_errors_;
    std::atomic<size_t> last_scan_time_;
    
    std::function<void(const std::string&)> ip_discovered_callback_;
    std::vector<std::thread> scanning_threads_;
    
    std::vector<std::string> scanning_ranges_;
    std::chrono::seconds scan_interval_;
    size_t max_threads_;
    
    // Методы сканирования
    void ScanWorkerThread();
    void ScanIpRange(const std::string& ip_range);
    bool ScanSingleIp(const std::string& ip);
    void ValidateAndAddIp(const std::string& ip);
    
    // Утилиты
    std::vector<std::string> ParseIpRange(const std::string& range);
    bool IsValidIp(const std::string& ip) const;
    std::string GenerateRandomIp() const;
    
    // Конфигурация
    bool LoadConfiguration(const std::string& config_path);
};

// Процессор для маскировки на основе белого списка
class WhitelistBasedMasker : public BaseSignatureProcessor {
public:
    WhitelistBasedMasker() : BaseSignatureProcessor("whitelist_based_masker") {
        scanner_ = std::make_unique<WhitelistScanner>();
        AddKeyword("IP");
        AddKeyword("address");
        AddPattern("\\d+\\.\\d+\\.\\d+\\.\\d+");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive()) return false;
        
        return ApplyWhitelistMasking(packet);
    }
    
    // Управление сканером
    bool InitializeScanner(const std::string& config_path) {
        return scanner_->Initialize(config_path);
    }
    
    void StartScanner() {
        scanner_->StartScanning();
    }
    
    void StopScanner() {
        scanner_->StopScanning();
    }
    
    // Управление белым списком
    void AddToWhitelist(const std::string& ip) {
        scanner_->AddWhitelistIp(ip);
    }
    
    void RemoveFromWhitelist(const std::string& ip) {
        scanner_->RemoveWhitelistIp(ip);
    }
    
    // Статистика
    size_t GetWhitelistSize() const {
        return scanner_->GetWhitelistSize();
    }
    
    bool IsIpWhitelisted(const std::string& ip) const {
        return scanner_->IsIpWhitelisted(ip);
    }
    
private:
    std::unique_ptr<WhitelistScanner> scanner_;
    
    bool ApplyWhitelistMasking(Packet& packet) {
        // Извлекаем IP адреса из пакета
        std::vector<std::string> ips = ExtractIpsFromPacket(packet.data);
        
        bool masked = false;
        for (const std::string& ip : ips) {
            if (!scanner_->IsIpWhitelisted(ip)) {
                if (MaskIpInPacket(packet.data, ip)) {
                    masked = true;
                }
            }
        }
        
        return masked;
    }
    
    std::vector<std::string> ExtractIpsFromPacket(const ByteArray& data) {
        std::vector<std::string> ips;
        std::string content(data.begin(), data.end());
        
        // Простой regex для поиска IP адресов
        std::regex ip_pattern(R"(\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b)");
        
        std::sregex_iterator begin(content.begin(), content.end(), ip_pattern);
        std::sregex_iterator end;
        
        for (auto it = begin; it != end; ++it) {
            ips.push_back(it->str());
        }
        
        return ips;
    }
    
    bool MaskIpInPacket(ByteArray& data, const std::string& ip) {
        std::string content(data.begin(), data.end());
        std::string original_content = content;
        
        // Заменяем неразрешенный IP на случайный из белого списка
        std::string masked_ip = GenerateMaskedIpFromWhitelist();
        
        size_t pos = content.find(ip);
        if (pos != std::string::npos) {
            content.replace(pos, ip.length(), masked_ip);
            data.assign(content.begin(), content.end());
            return original_content != content;
        }
        
        return false;
    }
    
    std::string GenerateMaskedIpFromWhitelist() const {
        // Возвращаем случайный IP из пула разрешенных
        static std::vector<std::string> fallback_ips = {
            "77.88.8.8",   // Yandex DNS
            "13.13.13.13", // Mail.ru
            "46.46.46.46", // Rambler
            "31.31.31.31"  // VK
        };
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, fallback_ips.size() - 1);
        
        return fallback_ips[dis(gen)];
    }
};

// Автономный сканер разрешенных IP
class StandaloneIpScanner {
public:
    StandaloneIpScanner();
    ~StandaloneIpScanner();
    
    // Конфигурация и запуск
    bool Initialize(const std::vector<std::string>& ip_ranges);
    void StartScanning();
    void StopScanning();
    
    // Получение результатов
    std::vector<std::string> GetDiscoveredIps() const;
    std::unordered_set<std::string> GetWhitelist() const;
    
    // Экспорт результатов
    bool ExportToFile(const std::string& file_path) const;
    bool ImportFromFile(const std::string& file_path);
    
private:
    std::unordered_set<std::string> whitelist_ips_;
    std::vector<std::string> discovered_ips_;
    std::vector<std::string> scanning_ranges_;
    
    std::atomic<bool> is_scanning_;
    std::atomic<bool> should_stop_;
    
    std::vector<std::thread> worker_threads_;
    mutable std::mutex whitelist_mutex_;
    mutable std::mutex discovered_mutex_;
    
    void ScanWorker();
    void ScanIpRange(const std::string& range);
    bool TestIpConnectivity(const std::string& ip);
    void AddDiscoveredIp(const std::string& ip);
};

} // namespace TrafficMask

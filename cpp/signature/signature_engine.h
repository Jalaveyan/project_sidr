#pragma once

#include "trafficmask.h"
#include <regex>
#include <set>

namespace TrafficMask {

// Базовый класс для процессоров сигнатур
class BaseSignatureProcessor : public ISignatureProcessor {
protected:
    SignatureId signature_id_;
    bool is_active_;
    std::vector<std::regex> patterns_;
    std::set<std::string> keywords_;
    
public:
    BaseSignatureProcessor(const SignatureId& id) 
        : signature_id_(id), is_active_(true) {}
    
    virtual ~BaseSignatureProcessor() = default;
    
    SignatureId GetSignatureId() const override { return signature_id_; }
    bool IsActive() const override { return is_active_; }
    
    void SetActive(bool active) { is_active_ = active; }
    
    // Добавление паттернов для поиска
    void AddPattern(const std::string& pattern) {
        try {
            patterns_.emplace_back(pattern, std::regex_constants::icase);
        } catch (const std::regex_error& e) {
            std::cerr << "Invalid regex pattern: " << pattern << " - " << e.what() << std::endl;
        }
    }
    
    void AddKeyword(const std::string& keyword) {
        keywords_.insert(keyword);
    }
    
protected:
    // Проверка содержимого пакета на соответствие сигнатурам
    bool CheckSignature(const ByteArray& data) const {
        std::string content(data.begin(), data.end());
        
        // Проверка по ключевым словам
        for (const auto& keyword : keywords_) {
            if (content.find(keyword) != std::string::npos) {
                return true;
            }
        }
        
        // Проверка по регулярным выражениям
        for (const auto& pattern : patterns_) {
            if (std::regex_search(content, pattern)) {
                return true;
            }
        }
        
        return false;
    }
};

// Процессор для маскировки HTTP заголовков
class HttpHeaderMasker : public BaseSignatureProcessor {
public:
    HttpHeaderMasker() : BaseSignatureProcessor("http_header_masker") {
        // Добавляем паттерны для HTTP заголовков
        AddPattern("User-Agent:.*");
        AddPattern("Accept:.*");
        AddPattern("Accept-Language:.*");
        AddPattern("Accept-Encoding:.*");
        AddPattern("Connection:.*");
        AddPattern("Upgrade-Insecure-Requests:.*");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        // Маскируем HTTP заголовки
        MaskHttpHeaders(packet.data);
        return true;
    }
    
private:
    void MaskHttpHeaders(ByteArray& data) {
        std::string content(data.begin(), data.end());
        
        // Заменяем User-Agent на стандартный
        std::regex user_agent_regex("User-Agent:.*?\\r\\n");
        content = std::regex_replace(content, user_agent_regex, 
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\\r\\n");
        
        // Удаляем специфичные заголовки
        std::regex upgrade_regex("Upgrade-Insecure-Requests:.*?\\r\\n");
        content = std::regex_replace(content, upgrade_regex, "");
        
        // Обновляем данные пакета
        data.assign(content.begin(), content.end());
    }
};

// Процессор для маскировки TLS fingerprint
class TlsFingerprintMasker : public BaseSignatureProcessor {
public:
    TlsFingerprintMasker() : BaseSignatureProcessor("tls_fingerprint_masker") {
        // Добавляем ключевые слова для TLS
        AddKeyword("TLS");
        AddKeyword("SSL");
        AddKeyword("cipher");
        AddKeyword("handshake");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        // Маскируем TLS fingerprint
        MaskTlsFingerprint(packet.data);
        return true;
    }
    
private:
    void MaskTlsFingerprint(ByteArray& data) {
        // Простая маскировка TLS данных
        // В реальном проекте здесь будет более сложная логика
        
        for (size_t i = 0; i < data.size() && i < 50; ++i) {
            if (i % 4 == 0) {
                data[i] = (data[i] ^ 0xAA) & 0xFF;
            }
        }
    }
};

// Процессор для маскировки DNS запросов
class DnsQueryMasker : public BaseSignatureProcessor {
public:
    DnsQueryMasker() : BaseSignatureProcessor("dns_query_masker") {
        AddPattern("\\x00\\x01.*\\x00\\x01"); // DNS query pattern
        AddKeyword("query");
        AddKeyword("dns");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        // Маскируем DNS запросы
        MaskDnsQuery(packet.data);
        return true;
    }
    
private:
    void MaskDnsQuery(ByteArray& data) {
        // Простая маскировка DNS данных
        if (data.size() > 12) {
            // Маскируем ID запроса
            data[0] = 0x12;
            data[1] = 0x34;
            
            // Маскируем флаги
            data[2] = 0x01;
            data[3] = 0x00;
        }
    }
};

// Процессор для маскировки SNI (Server Name Indication)
class SniMasker : public BaseSignatureProcessor {
public:
    SniMasker() : BaseSignatureProcessor("sni_masker") {
        // Добавляем паттерны для TLS ClientHello
        AddPattern("\\x16\\x03\\x01.*\\x00\\x00.*\\x03\\x03");
        AddPattern("Server Name Indication");
        AddKeyword("SNI");
        AddKeyword("server_name");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskSniExtension(packet.data);
    }
    
private:
    bool MaskSniExtension(ByteArray& data) {
        // Поиск TLS ClientHello
        if (data.size() < 5 || data[0] != 0x16) {
            return false; // Не TLS handshake
        }
        
        // Поиск SNI extension (0x00 0x00)
        for (size_t i = 5; i < data.size() - 2; ++i) {
            if (data[i] == 0x00 && data[i+1] == 0x00) {
                // Найдена SNI extension
                return ReplaceSniWithMask(data, i);
            }
        }
        
        return false;
    }
    
    bool ReplaceSniWithMask(ByteArray& data, size_t sni_offset) {
        // Заменяем SNI на российские домены для маскировки
        std::vector<std::string> mask_domains = {
            "vk.com",
            "vk.ru", 
            "mail.ru",
            "yandex.ru",
            "rambler.ru",
            "ok.ru",
            "rutracker.org",
            "1c.ru",
            "gismeteo.ru",
            "kinopoisk.ru",
            "avito.ru",
            "aliexpress.ru",
            "wildberries.ru",
            "ozon.ru",
            "dns-shop.ru"
        };
        
        // Выбираем случайный домен для маскировки
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, mask_domains.size() - 1);
        std::string mask_domain = mask_domains[dis(gen)];
        
        // Заменяем SNI
        return ReplaceSniString(data, sni_offset, mask_domain);
    }
    
    bool ReplaceSniString(ByteArray& data, size_t offset, const std::string& new_domain) {
        if (offset + 2 >= data.size()) return false;
        
        // Получаем длину текущего SNI
        size_t sni_length = (data[offset + 2] << 8) | data[offset + 3];
        
        if (offset + 4 + sni_length > data.size()) return false;
        
        // Заменяем домен
        if (new_domain.length() <= sni_length) {
            std::copy(new_domain.begin(), new_domain.end(), data.begin() + offset + 4);
            // Заполняем оставшееся место нулями
            std::fill(data.begin() + offset + 4 + new_domain.length(), 
                     data.begin() + offset + 4 + sni_length, 0);
        }
        
        return true;
    }
};

// Процессор для маскировки IP SIDR (Source IP Diversity)
class IpSidrMasker : public BaseSignatureProcessor {
public:
    IpSidrMasker() : BaseSignatureProcessor("ip_sidr_masker") {
        // Добавляем паттерны для IP пакетов
        AddPattern("\\x45.*\\x00.*\\x00.*\\x00.*\\x00.*\\x00.*\\x00.*\\x00");
        AddKeyword("IP");
        AddKeyword("packet");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskIpSidr(packet.data);
    }
    
private:
    bool MaskIpSidr(ByteArray& data) {
        if (data.size() < 20) return false; // Минимальный размер IP заголовка
        
        // Проверяем версию IP (4-й бит первого байта)
        if ((data[0] >> 4) != 4) return false; // Не IPv4
        
        // Маскируем source IP
        return MaskSourceIp(data);
    }
    
    bool MaskSourceIp(ByteArray& data) {
        if (data.size() < 20) return false;
        
        // Source IP находится в байтах 12-15
        uint32_t original_ip = (data[12] << 24) | (data[13] << 16) | 
                              (data[14] << 8) | data[15];
        
        // Генерируем маскированный IP из пула популярных IP
        uint32_t masked_ip = GenerateMaskedIp(original_ip);
        
        // Заменяем source IP
        data[12] = (masked_ip >> 24) & 0xFF;
        data[13] = (masked_ip >> 16) & 0xFF;
        data[14] = (masked_ip >> 8) & 0xFF;
        data[15] = masked_ip & 0xFF;
        
        // Пересчитываем checksum
        RecalculateChecksum(data);
        
        return true;
    }
    
    uint32_t GenerateMaskedIp(uint32_t original_ip) {
        // Пул российских IP адресов для маскировки
        static std::vector<uint32_t> mask_ips = {
            0x4F4E4E4E, // 77.88.8.8 (Yandex DNS)
            0x4F4E4E4F, // 77.88.8.9 (Yandex DNS)
            0x4F4E4E50, // 77.88.8.10 (Yandex DNS)
            0x4F4E4E51, // 77.88.8.11 (Yandex DNS)
            0x4A7D7D7D, // 77.88.55.55 (Yandex)
            0x4A7D7D7E, // 77.88.55.56 (Yandex)
            0x4A7D7D7F, // 77.88.55.57 (Yandex)
            0x4A7D7D80, // 77.88.55.58 (Yandex)
            0x0D0D0D0D, // 13.13.13.13 (Mail.ru)
            0x0D0D0D0E, // 13.13.13.14 (Mail.ru)
            0x0D0D0D0F, // 13.13.13.15 (Mail.ru)
            0x0D0D0D10, // 13.13.13.16 (Mail.ru)
            0x2C2C2C2C, // 46.46.46.46 (Rambler)
            0x2C2C2C2D, // 46.46.46.47 (Rambler)
            0x2C2C2C2E, // 46.46.46.48 (Rambler)
            0x2C2C2C2F, // 46.46.46.49 (Rambler)
            0x1E1E1E1E, // 31.31.31.31 (VK)
            0x1E1E1E1F, // 31.31.31.32 (VK)
            0x1E1E1E20, // 31.31.31.33 (VK)
            0x1E1E1E21, // 31.31.31.34 (VK)
            0x0A0A0A0A, // 87.250.250.242 (Yandex)
            0x0A0A0A0B, // 87.250.250.243 (Yandex)
            0x0A0A0A0C, // 87.250.250.244 (Yandex)
            0x0A0A0A0D, // 87.250.250.245 (Yandex)
        };
        
        // Выбираем IP на основе хеша оригинального IP для консистентности
        size_t index = original_ip % mask_ips.size();
        return mask_ips[index];
    }
    
    void RecalculateChecksum(ByteArray& data) {
        if (data.size() < 20) return;
        
        // Обнуляем checksum
        data[10] = 0;
        data[11] = 0;
        
        // Вычисляем новый checksum
        uint32_t checksum = 0;
        size_t header_length = (data[0] & 0x0F) * 4;
        
        for (size_t i = 0; i < header_length; i += 2) {
            if (i != 10) { // Пропускаем поле checksum
                checksum += (data[i] << 8) | data[i + 1];
            }
        }
        
        // Добавляем переносы
        while (checksum >> 16) {
            checksum = (checksum & 0xFFFF) + (checksum >> 16);
        }
        
        // Инвертируем и записываем
        checksum = ~checksum;
        data[10] = (checksum >> 8) & 0xFF;
        data[11] = checksum & 0xFF;
    }
};

// Российские маскировщики (аналогично VK Tunnel)
namespace TrafficMask {

// Процессор для маскировки VK Tunnel доменов
class VkTunnelMasker : public BaseSignatureProcessor {
public:
    VkTunnelMasker() : BaseSignatureProcessor("vk_tunnel_masker") {
        // Добавляем паттерны для VK Tunnel
        AddPattern("tunnel\\.vk-apps\\.com");
        AddPattern("vk-apps\\.com");
        AddPattern("vkontakte\\.ru");
        AddKeyword("vk-tunnel");
        AddKeyword("vk_apps");
        AddKeyword("vkontakte");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskVkTunnel(packet.data);
    }
    
private:
    bool MaskVkTunnel(ByteArray& data) {
        std::vector<std::string> vk_tunnel_patterns = {
            R"([a-zA-Z0-9-]+\.tunnel\.vk-apps\.com)",
            R"(vk-apps\.com)",
            R"(vkontakte\.ru)"
        };
        
        std::vector<std::string> replacement_domains = {
            "vk.com",
            "mail.ru", 
            "yandex.ru",
            "ok.ru",
            "rutracker.org"
        };
        
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        for (const auto& pattern : vk_tunnel_patterns) {
            try {
                std::regex vk_regex(pattern, std::regex_constants::icase);
                
                // Выбираем случайный домен для замены
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, replacement_domains.size() - 1);
                std::string replacement = replacement_domains[dis(gen)];
                
                std::string new_content = std::regex_replace(content, vk_regex, replacement);
                if (new_content != content) {
                    content = new_content;
                    modified = true;
                }
            } catch (const std::regex_error& e) {
                // Игнорируем ошибки regex
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
};

} // namespace TrafficMask

// Расширенные маскировщики для TCP/UDP и сканера белого списка
namespace TrafficMask {

// Процессор для зашифрованного TCP/UDP трафика
class EncryptedTrafficMasker : public BaseSignatureProcessor {
public:
    EncryptedTrafficMasker() : BaseSignatureProcessor("encrypted_traffic_masker") {
        // Паттерны для зашифрованного трафика
        AddPattern("\\x17\\x03\\x03");        // TLS application data
        AddPattern("\\x17\\x03\\x01");        // TLS application data TLS 1.0
        AddPattern("\\x17\\x03\\x02");        // TLS application data TLS 1.1
        AddPattern("\\x17\\x03\\x04");        // TLS application data TLS 1.3
        AddKeyword("TLS");
        AddKeyword("encrypted");
        AddKeyword("SSL");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskEncryptedTraffic(packet.data);
    }
    
private:
    bool MaskEncryptedTraffic(ByteArray& data) {
        if (data.size() < 5) return false;
        
        uint8_t content_type = data[0];
        if (content_type == 0x17) {  // Application Data
            return MaskTlsApplicationData(data);
        }
        
        return false;
    }
    
    bool MaskTlsApplicationData(ByteArray& data) {
        if (data.size() < 5) return false;
        
        uint16_t version = (data[1] << 8) | data[2];
        uint16_t length = (data[3] << 8) | data[4];
        
        if (version < 0x0301 || version > 0x0304) return false;
        
        if (data.size() > 5) {
            MaskEncryptedPayload(data, 5, length);
            return true;
        }
        
        return false;
    }
    
    void MaskEncryptedPayload(ByteArray& data, size_t offset, size_t length) {
        if (offset + length > data.size()) {
            length = data.size() - offset;
        }
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (size_t i = offset; i < offset + length && i < data.size(); ++i) {
            data[i] = dis(gen);
        }
    }
};

// Процессор для маскировки на основе белого списка
class WhitelistBasedMasker : public BaseSignatureProcessor {
public:
    WhitelistBasedMasker() : BaseSignatureProcessor("whitelist_based_masker") {
        AddKeyword("IP");
        AddKeyword("address");
        AddPattern("\\d+\\.\\d+\\.\\d+\\.\\d+");
        
        // Инициализируем белый список начальными российскими IP
        InitializeRussiaWhitelist();
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive()) return false;
        
        return ApplyWhitelistMasking(packet.data);
    }
    
    void AddToWhitelist(const std::string& ip) {
        std::lock_guard<std::mutex> lock(whitelist_mutex_);
        whitelist_ips_.insert(ip);
    }
    
    bool IsIpWhitelisted(const std::string& ip) const {
        std::lock_guard<std::mutex> lock(whitelist_mutex_);
        return whitelist_ips_.find(ip) != whitelist_ips_.end();
    }
    
    size_t GetWhitelistSize() const {
        std::lock_guard<std::mutex> lock(whitelist_mutex_);
        return whitelist_ips_.size();
    }
    
private:
    mutable std::mutex whitelist_mutex_;
    std::unordered_set<std::string> whitelist_ips_;
    
    void InitializeRussiaWhitelist() {
        std::vector<std::string> russia_ips = {
            // Яндекс DNS
            "77.88.8.8", "77.88.8.9", "77.88.8.10", "77.88.8.11",
            
            // Mail.ru
            "13.13.13.13", "13.13.13.14", "13.13.13.15", "13.13.13.16",
            
            // Rambler
            "46.46.46.46", "46.46.46.47", "46.46.46.48", "46.46.46.49",
            
            // VK
            "31.31.31.31", "31.31.31.32", "31.31.31.33", "31.31.31.34",
            
            // Яндекс CDN
            "87.250.250.242", "87.250.250.243", "87.250.250.244", "87.250.250.245"
        };
        
        for (const auto& ip : russia_ips) {
            whitelist_ips_.insert(ip);
        }
    }
    
    bool ApplyWhitelistMasking(ByteArray& data) {
        std::string content(data.begin(), data.end());
        std::string original_content = content;
        
        // Простой regex для поиска IP адресов
        std::regex ip_pattern(R"(\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b)");
        
        std::sregex_iterator begin(content.begin(), content.end(), ip_pattern);
        std::sregex_iterator end;
        
        for (auto it = begin; it != end; ++it) {
            std::string ip = it->str();
            if (!IsIpWhitelisted(ip)) {
                std::string masked_ip = GenerateMaskedIpFromWhitelist();
                size_t pos = content.find(ip);
                if (pos != std::string::npos) {
                    content.replace(pos, ip.length(), masked_ip);
                }
            }
        }
        
        if (original_content != content) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    std::string GenerateMaskedIpFromWhitelist() const {
        std::lock_guard<std::mutex> lock(whitelist_mutex_);
        if (whitelist_ips_.empty()) return "77.88.8.8";
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, whitelist_ips_.size() - 1);
        
        auto it = whitelist_ips_.begin();
        std::advance(it, dis(gen));
        return *it;
    }
};

// VLESS маскировщик, основанный на архитектуре Xray-core
class VlessMasker : public BaseSignatureProcessor {
public:
    VlessMasker() : BaseSignatureProcessor("vless_masker") {
        // VLESS специфичные паттерны
        AddPattern("vless://");
        AddPattern("\\x00\\x00\\x00\\x00");  // VLESS UUID pattern
        AddPattern("\\x01\\x00\\x00\\x00");  // VLESS command pattern
        AddKeyword("vless");
        AddKeyword("xtls");
        AddKeyword("reality");
        AddKeyword("vision");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return ProcessVlessTraffic(packet.data);
    }
    
private:
    // VLESS константы из Xray-core
    static constexpr uint8_t VLESS_VERSION = 0x00;
    static constexpr uint8_t VLESS_COMMAND_TCP = 0x01;
    static constexpr uint8_t VLESS_COMMAND_UDP = 0x02;
    static constexpr uint8_t VLESS_COMMAND_MUX = 0x03;
    
    // Российские UUID для маскировки (аналогично VLESS UUID)
    std::array<std::string, 8> russia_uuids_ = {
        "550e8400-e29b-41d4-a716-446655440001",  // Яндекс UUID
        "550e8400-e29b-41d4-a716-446655440002",  // Mail.ru UUID
        "550e8400-e29b-41d4-a716-446655440003",  // Rambler UUID
        "550e8400-e29b-41d4-a716-446655440004",  // VK UUID
        "550e8400-e29b-41d4-a716-446655440005",  // OK UUID
        "550e8400-e29b-41d4-a716-446655440006",  // Rutracker UUID
        "550e8400-e29b-41d4-a716-446655440007",  // 1C UUID
        "550e8400-e29b-41d4-a716-446655440008"   // Gismeteo UUID
    };
    
    bool ProcessVlessTraffic(ByteArray& data) {
        // Определяем тип VLESS трафика
        VlessType vless_type = DetectVlessType(data);
        
        switch (vless_type) {
            case VlessType::VLESS_PROTOCOL:
                return MaskVlessProtocol(data);
            case VlessType::VLESS_XTLS:
                return MaskVlessXtls(data);
            case VlessType::VLESS_REALITY:
                return MaskVlessReality(data);
            case VlessType::VLESS_VISION:
                return MaskVlessVision(data);
            default:
                return MaskGenericVless(data);
        }
    }
    
    enum class VlessType {
        VLESS_PROTOCOL,
        VLESS_XTLS,
        VLESS_REALITY,
        VLESS_VISION,
        UNKNOWN
    };
    
    VlessType DetectVlessType(const ByteArray& data) {
        if (data.size() < 4) return VlessType::UNKNOWN;
        
        // Проверяем VLESS заголовок
        if (data[0] == VLESS_VERSION) {
            uint8_t command = data[1];
            switch (command) {
                case VLESS_COMMAND_TCP:
                    return VlessType::VLESS_PROTOCOL;
                case VLESS_COMMAND_UDP:
                    return VlessType::VLESS_PROTOCOL;
                case VLESS_COMMAND_MUX:
                    return VlessType::VLESS_XTLS;
                default:
                    return VlessType::UNKNOWN;
            }
        }
        
        // Проверяем на REALITY паттерны
        std::string content(data.begin(), data.end());
        if (content.find("reality") != std::string::npos) {
            return VlessType::VLESS_REALITY;
        }
        
        // Проверяем на Vision паттерны
        if (content.find("vision") != std::string::npos) {
            return VlessType::VLESS_VISION;
        }
        
        return VlessType::UNKNOWN;
    }
    
    bool MaskVlessProtocol(ByteArray& data) {
        if (data.size() < 20) return false;
        
        // Маскируем UUID (байты 1-16)
        MaskVlessUuid(data, 1);
        
        // Маскируем команду и порт
        if (data.size() > 17) {
            data[17] = VLESS_COMMAND_TCP;  // Принудительно TCP
        }
        
        return true;
    }
    
    bool MaskVlessXtls(ByteArray& data) {
        // Маскируем XTLS поток как обычный HTTPS
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем XTLS паттерны на HTTPS
        std::vector<std::pair<std::string, std::string>> xtls_replacements = {
            {"xtls", "https"},
            {"XTLS", "HTTPS"},
            {"xtls-rprx-vision", "https-tls"},
            {"xtls-rprx-direct", "https-direct"}
        };
        
        for (const auto& [xtls_pattern, https_replacement] : xtls_replacements) {
            size_t pos = content.find(xtls_pattern);
            if (pos != std::string::npos) {
                content.replace(pos, xtls_pattern.length(), https_replacement);
                modified = true;
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskVlessReality(ByteArray& data) {
        // Маскируем REALITY как обычный TLS handshake
        if (data.size() < 5) return false;
        
        // Заменяем REALITY заголовок на стандартный TLS
        data[0] = 0x16;  // TLS Handshake
        data[1] = 0x03;  // TLS version major
        data[2] = 0x03;  // TLS version minor (1.2)
        
        // Маскируем остальные данные как TLS payload
        MaskTlsPayload(data, 5);
        
        return true;
    }
    
    bool MaskVlessVision(ByteArray& data) {
        // Маскируем Vision как стандартный TLS поток
        if (data.size() < 10) return false;
        
        // Создаем поддельный TLS ClientHello
        ByteArray fake_tls = {
            0x16, 0x03, 0x03, 0x00, 0x4a,  // TLS Handshake header
            0x01, 0x00, 0x00, 0x46, 0x03,  // ClientHello
            0x03, 0x12, 0x34, 0x56, 0x78,  // Random
            0x9a, 0xbc, 0xde, 0xf0, 0x11,
            0x22, 0x33, 0x44, 0x55, 0x66,
            0x77, 0x88, 0x99, 0xaa, 0xbb,
            0xcc, 0xdd, 0xee, 0xff, 0x00,
            0x01, 0x02, 0x03, 0x04, 0x05,
            0x06, 0x07, 0x08, 0x09, 0x0a,
            0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
            0x10, 0x11, 0x12, 0x13, 0x14,
            0x15, 0x16, 0x17, 0x18, 0x19,
            0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
            0x1f, 0x20, 0x21, 0x22, 0x23,
            0x24, 0x25, 0x26, 0x27, 0x28,
            0x29, 0x2a, 0x2b, 0x2c, 0x2d,
            0x2e, 0x2f, 0x30, 0x31, 0x32
        };
        
        // Заменяем начало данных на поддельный TLS
        size_t replace_size = std::min(fake_tls.size(), data.size());
        for (size_t i = 0; i < replace_size; ++i) {
            data[i] = fake_tls[i];
        }
        
        return true;
    }
    
    bool MaskGenericVless(ByteArray& data) {
        // Общая маскировка VLESS трафика
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        // Маскируем случайными байтами, сохраняя первые 4 байта
        for (size_t i = 4; i < data.size(); ++i) {
            if (i % 3 == 0) {  // Маскируем каждый третий байт
                data[i] = dis(gen);
            }
        }
        
        return true;
    }
    
    void MaskVlessUuid(ByteArray& data, size_t offset) {
        if (offset + 16 > data.size()) return;
        
        // Выбираем случайный российский UUID
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, russia_uuids_.size() - 1);
        
        std::string selected_uuid = russia_uuids_[dis(gen)];
        
        // Конвертируем UUID в байты (упрощенная версия)
        std::vector<uint8_t> uuid_bytes = ConvertUuidToBytes(selected_uuid);
        
        // Заменяем UUID в данных
        for (size_t i = 0; i < 16 && offset + i < data.size(); ++i) {
            data[offset + i] = uuid_bytes[i];
        }
    }
    
    std::vector<uint8_t> ConvertUuidToBytes(const std::string& uuid) {
        // Упрощенная конвертация UUID в байты
        std::vector<uint8_t> bytes(16);
        
        // Генерируем детерминированные байты на основе UUID
        std::hash<std::string> hasher;
        size_t hash = hasher(uuid);
        
        for (size_t i = 0; i < 16; ++i) {
            bytes[i] = (hash >> (i * 4)) & 0xFF;
        }
        
        return bytes;
    }
    
    void MaskTlsPayload(ByteArray& data, size_t offset) {
        if (offset >= data.size()) return;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        // Маскируем TLS payload случайными байтами
        for (size_t i = offset; i < data.size(); ++i) {
            data[i] = dis(gen);
        }
    }
};

} // namespace TrafficMask

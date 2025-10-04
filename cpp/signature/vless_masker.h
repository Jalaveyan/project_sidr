#pragma once

#include "trafficmask.h"
#include <unordered_map>
#include <vector>
#include <random>
#include <array>

namespace TrafficMask {

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
        if (ContainsRealityPattern(data)) {
            return VlessType::VLESS_REALITY;
        }
        
        // Проверяем на Vision паттерны
        if (ContainsVisionPattern(data)) {
            return VlessType::VLESS_VISION;
        }
        
        return VlessType::UNKNOWN;
    }
    
    bool ContainsRealityPattern(const ByteArray& data) {
        std::string content(data.begin(), data.end());
        return content.find("reality") != std::string::npos ||
               content.find("REALITY") != std::string::npos ||
               content.find("xtls-rprx-vision") != std::string::npos;
    }
    
    bool ContainsVisionPattern(const ByteArray& data) {
        std::string content(data.begin(), data.end());
        return content.find("vision") != std::string::npos ||
               content.find("VISION") != std::string::npos ||
               content.find("xtls-rprx-vision") != std::string::npos;
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

// VLESS-совместимый прокси маскировщик
class VlessProxyMasker : public BaseSignatureProcessor {
public:
    VlessProxyMasker() : BaseSignatureProcessor("vless_proxy_masker") {
        AddPattern("vless://.*@.*:.*");
        AddPattern("\\x00\\x00\\x00\\x00.*\\x01\\x00\\x00\\x00");
        AddKeyword("proxy");
        AddKeyword("socks");
        AddKeyword("http");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskVlessProxy(packet.data);
    }
    
private:
    bool MaskVlessProxy(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем VLESS прокси на российские сервисы
        std::vector<std::pair<std::::string, std::string>> proxy_replacements = {
            {"vless://", "https://"},
            {"@", "@mail.ru:"},
            {":443", ":443"},
            {":80", ":80"},
            {"?type=tcp", "?type=https"},
            {"&security=tls", "&security=tls"},
            {"&path=/", "&path=/api/"}
        };
        
        for (const auto& [vless_pattern, russia_replacement] : proxy_replacements) {
            size_t pos = content.find(vless_pattern);
            if (pos != std::string::npos) {
                content.replace(pos, vless_pattern.length(), russia_replacement);
                modified = true;
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

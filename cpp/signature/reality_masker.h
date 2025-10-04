#pragma once

#include "trafficmask.h"
#include <unordered_map>
#include <vector>
#include <random>
#include <array>

namespace TrafficMask {

// REALITY маскировщик, основанный на Xray-core REALITY
class RealityMasker : public BaseSignatureProcessor {
public:
    RealityMasker() : BaseSignatureProcessor("reality_masker") {
        // REALITY специфичные паттерны
        AddPattern("reality");
        AddPattern("REALITY");
        AddPattern("xtls-rprx-vision");
        AddPattern("xtls-rprx-direct");
        AddPattern("\\x17\\x03\\x03.*\\x00\\x00\\x00\\x00");
        AddKeyword("reality");
        AddKeyword("xtls");
        AddKeyword("vision");
        AddKeyword("direct");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return ProcessRealityTraffic(packet.data);
    }
    
private:
    // REALITY константы из Xray-core
    static constexpr uint8_t REALITY_VERSION = 0x01;
    static constexpr uint8_t REALITY_COMMAND_TCP = 0x01;
    static constexpr uint8_t REALITY_COMMAND_UDP = 0x02;
    
    // Российские домены для REALITY маскировки
    std::array<std::string, 10> russia_domains_ = {
        "mail.ru",
        "yandex.ru", 
        "vk.com",
        "ok.ru",
        "rambler.ru",
        "rutracker.org",
        "1c.ru",
        "gismeteo.ru",
        "kinopoisk.ru",
        "avito.ru"
    };
    
    bool ProcessRealityTraffic(ByteArray& data) {
        RealityType reality_type = DetectRealityType(data);
        
        switch (reality_type) {
            case RealityType::REALITY_TLS:
                return MaskRealityTls(data);
            case RealityType::REALITY_VISION:
                return MaskRealityVision(data);
            case RealityType::REALITY_DIRECT:
                return MaskRealityDirect(data);
            case RealityType::REALITY_PROXY:
                return MaskRealityProxy(data);
            default:
                return MaskGenericReality(data);
        }
    }
    
    enum class RealityType {
        REALITY_TLS,
        REALITY_VISION,
        REALITY_DIRECT,
        REALITY_PROXY,
        UNKNOWN
    };
    
    RealityType DetectRealityType(const ByteArray& data) {
        if (data.size() < 5) return RealityType::UNKNOWN;
        
        // Проверяем REALITY TLS паттерн
        if (data[0] == 0x17 && data[1] == 0x03 && data[2] == 0x03) {
            return RealityType::REALITY_TLS;
        }
        
        // Проверяем на Vision паттерны
        std::string content(data.begin(), data.end());
        if (content.find("xtls-rprx-vision") != std::string::npos) {
            return RealityType::REALITY_VISION;
        }
        
        if (content.find("xtls-rprx-direct") != std::string::npos) {
            return RealityType::REALITY_DIRECT;
        }
        
        if (content.find("reality") != std::string::npos) {
            return RealityType::REALITY_PROXY;
        }
        
        return RealityType::UNKNOWN;
    }
    
    bool MaskRealityTls(ByteArray& data) {
        // Маскируем REALITY TLS как обычный TLS handshake
        if (data.size() < 5) return false;
        
        // Сохраняем TLS заголовок, но маскируем payload
        uint16_t tls_length = (data[3] << 8) | data[4];
        
        if (data.size() > 5) {
            MaskTlsPayload(data, 5, tls_length);
        }
        
        return true;
    }
    
    bool MaskRealityVision(ByteArray& data) {
        // Маскируем Vision как стандартный TLS поток
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем Vision паттерны на стандартные TLS
        std::vector<std::pair<std::string, std::string>> vision_replacements = {
            {"xtls-rprx-vision", "tls1.2"},
            {"xtls-rprx-direct", "tls-direct"},
            {"reality", "tls"},
            {"REALITY", "TLS"}
        };
        
        for (const auto& [vision_pattern, tls_replacement] : vision_replacements) {
            size_t pos = content.find(vision_pattern);
            if (pos != std::string::npos) {
                content.replace(pos, vision_pattern.length(), tls_replacement);
                modified = true;
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskRealityDirect(ByteArray& data) {
        // Маскируем Direct как обычный HTTPS соединение
        if (data.size() < 10) return false;
        
        // Создаем поддельный HTTPS заголовок
        ByteArray fake_https = {
            0x16, 0x03, 0x03, 0x00, 0x4a,  // TLS Handshake
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
        
        // Заменяем начало данных на поддельный HTTPS
        size_t replace_size = std::min(fake_https.size(), data.size());
        for (size_t i = 0; i < replace_size; ++i) {
            data[i] = fake_https[i];
        }
        
        return true;
    }
    
    bool MaskRealityProxy(ByteArray& data) {
        // Маскируем REALITY прокси как российские сервисы
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем REALITY прокси на российские домены
        std::vector<std::pair<std::string, std::string>> proxy_replacements = {
            {"reality://", "https://"},
            {"REALITY://", "HTTPS://"},
            {"xtls-rprx-vision", "tls1.2"},
            {"xtls-rprx-direct", "tls-direct"},
            {"@reality", "@mail.ru"},
            {"@REALITY", "@yandex.ru"}
        };
        
        for (const auto& [reality_pattern, russia_replacement] : proxy_replacements) {
            size_t pos = content.find(reality_pattern);
            if (pos != std::string::npos) {
                content.replace(pos, reality_pattern.length(), russia_replacement);
                modified = true;
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskGenericReality(ByteArray& data) {
        // Общая маскировка REALITY трафика
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        // Маскируем случайными байтами, сохраняя первые 5 байт
        for (size_t i = 5; i < data.size(); ++i) {
            if (i % 4 == 0) {  // Маскируем каждый четвертый байт
                data[i] = dis(gen);
            }
        }
        
        return true;
    }
    
    void MaskTlsPayload(ByteArray& data, size_t offset, size_t length) {
        if (offset + length > data.size()) {
            length = data.size() - offset;
        }
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        // Маскируем TLS payload случайными байтами
        for (size_t i = offset; i < offset + length && i < data.size(); ++i) {
            data[i] = dis(gen);
        }
    }
};

// XTLS маскировщик для совместимости с Xray-core
class XtlsMasker : public BaseSignatureProcessor {
public:
    XtlsMasker() : BaseSignatureProcessor("xtls_masker") {
        AddPattern("xtls");
        AddPattern("XTLS");
        AddPattern("xtls-rprx");
        AddPattern("xtls-rprx-vision");
        AddPattern("xtls-rprx-direct");
        AddKeyword("xtls");
        AddKeyword("rprx");
        AddKeyword("vision");
        AddKeyword("direct");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskXtlsTraffic(packet.data);
    }
    
private:
    bool MaskXtlsTraffic(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем XTLS паттерны на стандартные TLS
        std::vector<std::pair<std::string, std::string>> xtls_replacements = {
            {"xtls-rprx-vision", "tls1.2"},
            {"xtls-rprx-direct", "tls-direct"},
            {"xtls", "tls"},
            {"XTLS", "TLS"},
            {"rprx", "tls"},
            {"RPRX", "TLS"}
        };
        
        for (const auto& [xtls_pattern, tls_replacement] : xtls_replacements) {
            size_t pos = content.find(xtls_pattern);
            if (pos != std::string::npos) {
                content.replace(pos, xtls_pattern.length(), tls_replacement);
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

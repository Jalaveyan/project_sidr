#pragma once

#include "trafficmask.h"
#include <regex>
#include <vector>
#include <random>
#include <unordered_map>

namespace TrafficMask {

// Улучшенный процессор для VK Tunnel с поддержкой CDN
class EnhancedVkTunnelMasker : public BaseSignatureProcessor {
public:
    EnhancedVkTunnelMasker() : BaseSignatureProcessor("enhanced_vk_tunnel_masker") {
        // Расширенные паттерны для VK Tunnel
        AddPattern("tunnel\\.vk-apps\\.com");
        AddPattern("vk-apps\\.com");
        AddPattern("vkontakte\\.ru");
        AddPattern("vk-cdn\\.net");
        AddPattern("vk-cdn\\.com");
        AddPattern("vk-video\\.com");
        AddPattern("vk-audio\\.com");
        AddPattern("vk-images\\.com");
        AddKeyword("vk-tunnel");
        AddKeyword("vk_apps");
        AddKeyword("vkontakte");
        AddKeyword("vk-cdn");
        AddKeyword("websocket");
        AddKeyword("ws://");
        AddKeyword("wss://");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        // Определяем тип VK трафика и применяем соответствующую маскировку
        TrafficType traffic_type = DetectTrafficType(packet.data);
        return ApplyTrafficMasking(packet.data, traffic_type);
    }
    
private:
    enum class TrafficType {
        HTTP_REQUEST,
        WEBSOCKET_UPGRADE,
        WEBSOCKET_DATA,
        CDN_REQUEST,
        API_REQUEST,
        STATIC_ASSETS,
        UNKNOWN
    };
    
    std::unordered_map<std::string, std::string> cdn_replacements_ = {
        {"vk-cdn.net", "yandex.ru"},
        {"vk-cdn.com", "cloud.yandex.ru"},
        {"vk-video.com", "video.yandex.ru"},
        {"vk-audio.com", "music.yandex.io"},
        {"vk-images.com", "images.yandex.net"}
    };
    
    TrafficType DetectTrafficType(const ByteArray& data) {
        std::string content(data.begin(), data.end());
        
        if (content.find("GET /") != std::string::npos) {
            if (content.find("Upgrade: websocket") != std::string::npos) {
                return TrafficType::WEBSOCKET_UPGRADE;
            } else if (content.find(".js") != std::string::npos || 
                      content.find(".css") != std::string::npos ||
                      content.find(".png") != std::string::npos ||
                      content.find(".jpg") != std::string::npos) {
                return TrafficType::STATIC_ASSETS;
            } else if (content.find("/api/") != std::string::npos) {
                return TrafficType::API_REQUEST;
            } else {
                return TrafficType::HTTP_REQUEST;
            }
        }
        
        if (content.find("\x81") != std::string::npos || // WebSocket frame
            content.find("\x82") != std::string::npos) {
            return TrafficType::WEBSOCKET_DATA;
        }
        
        // Проверяем CDN запросы
        for (const auto& [cdn_domain, _] : cdn_replacements_) {
            if (content.find(cdn_domain) != std::string::npos) {
                return TrafficType::CDN_REQUEST;
            }
        }
        
        return TrafficType::UNKNOWN;
    }
    
    bool ApplyTrafficMasking(ByteArray& data, TrafficType type) {
        switch (type) {
            case TrafficType::HTTP_REQUEST:
                return MaskHttpRequest(data);
            case TrafficType::WEBSOCKET_UPGRADE:
                return MaskWebSocketUpgrade(data);
            case TrafficType::WEBSOCKET_DATA:
                return MaskWebSocketData(data);
            case TrafficType::CDN_REQUEST:
                return MaskCdnRequest(data);
            case TrafficType::API_REQUEST:
                return MaskApiRequest(data);
            case TrafficType::STATIC_ASSETS:
                return MaskStaticAssets(data);
            default:
                return MaskGenericVkTraffic(data);
        }
    }
    
    bool MaskHttpRequest(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем VK домены на популярные российские домены
        std::vector<std::regex> vk_patterns = {
            std::regex(R"([a-zA-Z0-9-]+\.tunnel\.vk-apps\.com)", std::regex_constants::icase),
            std::regex(R"(vk-apps\.com)", std::regex_constants::icase),
            std::regex(R"(vkontakte\.ru)", std::regex_constants::icase)
        };
        
        std::vector<std::string> replacement_domains = {
            "mail.ru",
            "ok.ru", 
            "rambler.ru",
            "rutracker.org"
        };
        
        for (const auto& pattern : vk_patterns) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, replacement_domains.size() - 1);
            std::string replacement = replacement_domains[dis(gen)];
            
            std::string new_content = std::regex_replace(content, pattern, replacement);
            if (new_content != content) {
                content = new_content;
                modified = true;
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskWebSocketUpgrade(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем WebSocket пути
        std::string ws_pattern = R"(/ws|/websocket|/tunnel|/stream)";
        std::vector<std::string> ws_replacements = {"/im", "/chat", "/api", "/service"};
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, ws_replacements.size() - 1);
        
        try {
            std::regex ws_regex(ws_pattern, std::regex_constants::icase);
            std::string replacement = ws_replacements[dis(gen)];
            
            std::string new_content = std::regex_replace(content, ws_regex, replacement);
            if (new_content != content) {
                content = new_content;
                modified = true;
            }
        } catch (const std::regex_error& e) {
            // Игнорируем ошибки regex
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskWebSocketData(ByteArray& data) {
        // Маскируем WebSocket данные случайными байтами
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        // Сохраняем WebSocket заголовок (первые 6 байт)
        for (size_t i = 6; i < data.size(); ++i) {
            data[i] = dis(gen);
        }
        
        return true;
    }
    
    bool MaskCdnRequest(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем VK CDN домены на Яндекс CDN домены
        for (const auto& [vk_cdn, yandex_cdn] : cdn_replacements_) {
            size_t pos = content.find(vk_cdn);
            if (posit != std::string::npos) {
                content.replace(pos, vk_cdn.length(), yandex_cdn);
                modified = true;
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskApiRequest(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем VK API пути на Яндекс API пути
        std::vector<std::pair<std::string, std::string>> api_replacements = {
            {"/api/vk/", "/api/yandex/"},
            {"/method/", "/method/v1/"},
            {"/oauth/", "/auth/"},
            {"/photos/", "/images/"},
            {"/audio/", "/music/"}
        };
        
        for (const auto& [vk_api, yandex_api] : api_replacements) {
            size_t pos = content.find(vk_api);
            if (pos != std::string::npos) {
                content.replace(pos, vk_api.length(), yandex_api);
                modified = true;
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskStaticAssets(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем пути к статическим ресурсам
        std::vector<std::pair<std::string, std::string>> asset_replacements = {
            {"/static/", "/assets/"},
            {"/images/", "/img/"},
            {"/styles/", "/css/"},
            {"/scripts/", "/js/"},
            {"/fonts/", "/f/"}
        };
        
        for (const auto& [vk_path, replacement_path] : asset_replacements) {
            size_t pos = content.find(vk_path);
            if (pos != std::string::npos) {
                content.replace(pos, vk_path.length(), replacement_path);
                modified = true;
            }
        }
        
        if (modified) {
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
    
    bool MaskGenericVkTraffic(ByteArray& data) {
        // Общая маскировка для неизвестного VK трафика
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        // Маскируем часть данных, сохраняя начало
        size_t mask_start = std::min(data.size() / 4, size_t(10));
        for (size_t i = mask_start; i < data.size(); ++i) {
            if (rand() % 3 == 0) {  // Маскируем каждый третий байт
                data[i] = dis(gen);
            }
        }
        
        return true;
    }
};

} // namespace TrafficMask

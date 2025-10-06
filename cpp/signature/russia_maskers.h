#pragma once

#include "trafficmask.h"
#include <regex>
#include <vector>
#include <random>

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
        // Заменяем VK Tunnel домены на популярные российские домены
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
            // Обновляем данные пакета
            data.assign(content.begin(), content.end());
            return true;
        }
        
        return false;
    }
};

// Процессор для маскировки российских CDN и хостинг-провайдеров
class RussiaCdnMasker : public BaseSignatureProcessor {
public:
    RussiaCdnMasker() : BaseSignatureProcessor("russia_cdn_masker") {
        // Российские CDN и хостинг-провайдеры
        AddPattern("cdn\\.yandex\\.ru");
        AddPattern("yastatic\\.net");
        AddPattern("rcntr\\.com");
        AddPattern("mail\\.ru");
        AddPattern("cdn\\.mail\\.ru");
        AddPattern("vk-cdn\\.com");
        AddPattern("rambler\\.ru");
        AddPattern("cdn\\.rambler\\.ru");
        AddPattern("1cbitrix\\.ru");
        AddPattern("cdn\\.1cbitrix\\.ru");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskRussiaCdn(packet.data);
    }
    
private:
    bool MaskRussiaCdn(ByteArray& data) {
        std::vector<std::string> cdn_replacements = {
            "vk.com",
            "mail.ru",
            "yandex.ru",
            "ok.ru"
        };
        
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Заменяем CDN домены на основные домены компаний
        std::string replacements[][2] = {
            {"cdn.yandex.ru", "yandex.ru"},
            {"yastatic.net", "yandex.ru"},
            {"rcntr.com", "mail.ru"},
            {"cdn.mail.ru", "mail.ru"},
            {"vk-cdn.com", "vk.com"},
            {"cdn.rambler.ru", "rambler.ru"},
            {"1cbitrix.ru", "1c.ru"},
            {"cdn.1cbitrix.ru", "1c.ru"}
        };
        
        for (const auto& replacement : replacements) {
            size_t pos = content.find(replacement[0]);
            if (pos != std::string::npos) {
                content.replace(pos, replacement[0].length(), replacement[1]);
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

// Процессор для маскировки российских API endpoints
class RussiaApiMasker : public BaseSignatureProcessor {
public:
    RussiaApiMasker() : BaseSignatureProcessor("russia_api_masker") {
        // Российские API endpoints
        AddPattern("/api/vk/");
        AddPattern("/api/mail/");
        AddPattern("/api/yandex/");
        AddPattern("/api/ok/");
        AddPattern("/api/rambler/");
        AddPattern("/api/1c/");
        AddKeyword("apimail");
        AddKeyword("apivk");
        AddKeyword("apiyandex");
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskRussiaApi(packet.data);
    }
    
private:
    bool MaskRussiaApi(ByteArray& data) {
        std::string content(data.begin(), data.end());
        bool modified = false;
        
        // Маскируем API пути, чтобы они выглядели как обычные веб-запросы
        std::string api_replacements[][2] = {
            {"/api/vk/", "/vk/"},
            {"/api/mail/", "/mail/"},
            {"/api/yandex/", "/yandex/"},
            {"/api/ok/", "/ok/"},
            {"/api/rambler/", "/rambler/"},
            {"/api/1c/", "/1c/"},
            {"apimail", "mail"},
            {"apivk", "vk"},
            {"apiyandex", "yandex"}
        };
        
        for (const auto& replacement : api_replacements) {
            size_t pos = content.find(replacement[0]);
            if (pos != std::string::npos) {
                content.replace(pos, replacement[0].length(), replacement[1]);
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

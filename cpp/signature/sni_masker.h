#pragma once

#include "trafficmask.h"
#include <unordered_map>
#include <vector>

namespace TrafficMask {

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
        // Заменяем SNI на популярный домен
        std::vector<std::string> mask_domains = {
            "www.google.com",
            "www.cloudflare.com", 
            "www.microsoft.com",
            "www.amazon.com"
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

} // namespace TrafficMask

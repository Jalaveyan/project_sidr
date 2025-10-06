#pragma once

#include "trafficmask.h"
#include <unordered_map>
#include <random>

namespace TrafficMask {

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
        // Пул популярных IP адресов для маскировки
        static std::vector<uint32_t> mask_ips = {
            0x08080808, // 8.8.8.8 (Google DNS)
            0x08080404, // 8.8.4.4 (Google DNS)
            0x01010101, // 1.1.1.1 (Cloudflare DNS)
            0x01000001, // 1.0.0.1 (Cloudflare DNS)
            0x4A7D7D7D, // 74.125.125.125 (Google)
            0x4A7D7D7E, // 74.125.125.126 (Google)
            0x4A7D7D7F, // 74.125.125.127 (Google)
            0x4A7D7D80, // 74.125.125.128 (Google)
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

} // namespace TrafficMask

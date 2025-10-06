#pragma once

#include "trafficmask.h"
#include <unordered_map>
#include <vector>
#include <random>
#include <algorithm>

namespace TrafficMask {

// Процессор для зашифрованного TCP/UDP трафика
class EncryptedTrafficMasker : public BaseSignatureProcessor {
public:
    EncryptedTrafficMasker() : BaseSignatureProcessor("encrypted_traffic_masker") {
        // Паттерны для зашифрованного трафика
        AddPattern("\\x17\\x03\\x03");        // TLS application data
        AddPattern("\\x17\\x03\\x01");        // TLS application data TLS 1.0
        AddPattern("\\x17\\x03\\x02");        // TLS application data TLS 1.1
        AddPattern("\\x17\\x03\\x03");        // TLS application data TLS 1.2
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
        
        // Определяем тип зашифрованного трафика
        uint8_t content_type = data[0];
        
        if (content_type == 0x17) {  // Application Data
            return MaskTlsApplicationData(data);
        } else if (content_type >= 0x16 && content_type <= 0x18) {
            return MaskGenericEncryptedData(data);
        }
        
        return false;
    }
    
    bool MaskTlsApplicationData(ByteArray& data) {
        if (data.size() < 5) return false;
        
        // TLS заголовок: [content_type][version][length]
        uint8_t content_type = data[0];  // 0x17 для Application Data
        uint16_t version = (data[1] << 8) | data[2];  // TLS version
        uint16_t length = (data[3] << 8) | data[4];   // Length
        
        // Проверяем корректность TLS версии
        if (version < 0x0301 || version > 0x0304) return false;
        
        // Маскируем зашифрованные данные (но сохраняем заголовки)
        if (data.size() > 5) {
            MaskEncryptedPayload(data, 5, length);
            return true;
        }
        
        return false;
    }
    
    bool MaskGenericEncryptedData(ByteArray& data) {
        // Для других типов зашифрованного трафика
        if (data.size() < 4) return false;
        
        // Простая маскировка случайными байтами
        MaskRandomBytes(data);
        return true;
    }
    
    void MaskEncryptedPayload(ByteArray& data, size_t offset, size_t length) {
        if (offset + length > data.size()) {
            length = data.size() - offset;
        }
        
        // Случайная маскировка зашифрованных данных
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (size_t i = offset; i < offset + length && i < data.size(); ++i) {
            data[i] = dis(gen);
        }
    }
    
    void MaskRandomBytes(ByteArray& data) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        // Маскируем случайными байтами, но сохраняем первые 4 байта
        for (size_t i = 4; i < data.size(); ++i) {
            data[i] = dis(gen);
        }
    }
};

// Процессор для TCP потоков
class TcpStreamMasker : public BaseSignatureProcessor {
public:
    TcpStreamMasker() : BaseSignatureProcessor("tcp_stream_masker") {
        AddKeyword("TCP");
        AddKeyword("stream");
        AddPattern("\\x50\\x00\\x00\\x00");  // TCP header patterns
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskTcpStream(packet.data, packet.connection_id);
    }
    
private:
    std::unordered_map<ConnectionId, std::vector<uint8_t>> tcp_streams_;
    
    bool MaskTcpStream(ByteArray& data, const ConnectionId& conn_id) {
        // Анализируем TCP заголовок
        if (data.size() < 20) return false;
        
        // Извлекаем TCP заголовок
        uint16_t src_port = (data[0] << 8) | data[1];
        uint16_t dst_port = (data[2] << 8) | data[3];
        uint32_t seq_num = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
        uint32_t ack_num = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];
        uint16_t flags = data[13] & 0x3F;
        
        // Вычисляем длину заголовка
        uint8_t header_length = (flags >> 4) * 4;
        if (header_length < 20) header_length = 20;
        
        // Маскируем TCP payload
        if (data.size() > header_length) {
            MaskTcpPayload(data, header_length);
            return true;
        }
        
        return false;
    }
    
    void MaskTcpPayload(ByteArray& data, size_t header_length) {
        if (header_length >= data.size()) return;
        
        // Маскируем payload случайными байтами
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (size_t i = header_length; i < data.size(); ++i) {
            data[i] = dis(gen);
        }
    }
};

// Процесс etc для UDP пакетов
class UdpPacketMasker : public BaseSignatureProcessor {
public:
    UdpPacketMasker() : BaseSignatureProcessor("udp_packet_masker") {
        AddKeyword("UDP");
        AddKeyword("packet");
        AddPattern("\\x45\\x00");  // IPv4 UDP patterns
    }
    
    bool ProcessPacket(Packet& packet) override {
        if (!IsActive() || !CheckSignature(packet.data)) {
            return false;
        }
        
        return MaskUdpPacket(packet.data);
    }
    
private:
    bool MaskUdpPacket(ByteArray& data) {
        if (data.size() < 28) return false; // IP(20) + UDP(8) минимум
        
        // Определяем позицию UDP заголовка (после IP заголовка)
        uint8_t ip_header_length = (data[0] & 0x0F) * 4;
        if (ip_header_length < 20 || data.size() < ip_header_length + 8) return false;
        
        // Извлекаем UDP заголовок
        uint16_t src_port = (data[ip_header_length] << 8) | data[ip_header_length + 1];
        uint16_t dst_port = (data[ip_header_length + 2] << 8) | data[ip_header_length + 3];
        uint16_t length = (data[ip_header_length + 4] << 8) | data[ip_header_length + 5];
        
        // Маскируем UDP payload
        size_t udp_header_start = ip_header_length + 8;
        if (udp_header_start < data.size()) {
            MaskUdpPayload(data, udp_header_start);
            return true;
        }
        
        return false;
    }
    
    void MaskUdpPayload(ByteArray& data, size_t offset) {
        if (offset >= data.size()) return;
        
        // Маскируем UDP payload случайными байтами
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (size_t i = offset; i < data.size(); ++i) {
            data[i] = dis(gen);
        }
    }
};

} // namespace TrafficMask

#include "trafficmask.h"
#include "signature_engine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

using namespace TrafficMask;

// Генератор тестовых пакетов
class TestPacketGenerator {
public:
    static Packet GenerateHttpPacket(const std::string& connection_id) {
        std::string http_data = 
            "GET /test HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "User-Agent: CustomBrowser/1.0\r\n"
            "Accept: text/html,application/xhtml+xml\r\n"
            "Accept-Language: en-US,en;q=0.9\r\n"
            "Accept-Encoding: gzip, deflate\r\n"
            "Connection: keep-alive\r\n"
            "Upgrade-Insecure-Requests: 1\r\n\r\n";
        
        ByteArray data(http_data.begin(), http_data.end());
        return Packet(data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateTlsPacket(const std::string& connection_id) {
        ByteArray tls_data = {
            0x16, 0x03, 0x01, 0x00, 0x4a, 0x01, 0x00, 0x00, 0x46, 0x03, 0x03,
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33,
            0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee,
            0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
            0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
            0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
            0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
            0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35
        };
        
        return Packet(tls_data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateDnsPacket(const std::string& connection_id) {
        ByteArray dns_data = {
            0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x07, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d,
            0x00, 0x00, 0x01, 0x00, 0x01
        };
        
        return Packet(dns_data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateSniPacket(const std::string& connection_id) {
        // TLS ClientHello с SNI extension
        ByteArray sni_data = {
            0x16, 0x03, 0x01, 0x00, 0x4a, 0x01, 0x00, 0x00, 0x46, 0x03, 0x03,
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33,
            0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee,
            0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
            0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
            0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
            0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
            0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
            // SNI extension
            0x00, 0x00, 0x00, 0x0f, 0x00, 0x0d, 0x00, 0x00, 0x0a, 0x65, 0x78,
            0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d
        };
        
        return Packet(sni_data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateIpSidrPacket(const std::string& connection_id) {
        // IP пакет с кастомным source IP
        ByteArray ip_data = {
            0x45, 0x00, 0x00, 0x3c, 0x12, 0x34, 0x40, 0x00, 0x40, 0x06, 0x00, 0x00,
            0xc0, 0xa8, 0x01, 0x01,  // Source IP: 192.168.1.1
            0x08, 0x08, 0x08, 0x08,  // Dest IP: 8.8.8.8
            0x12, 0x34, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x50, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        
        return Packet(ip_data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateVkTunnelPacket(const std::string& connection_id) {
        // VK Tunnel пакет с характерными паттернами
        std::string vk_tunnel_data = 
            "GET /ws HTTP/1.1\r\n"
            "Host: random-tunnel-id.tunnel.vk-apps.com\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Origin: https://vkontakte.ru\r\n"
            "Referer: https://vk-apps.com\r\n\r\n";
        
        return Packet(data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateEncryptedTlsPacket(const std::string& connection_id) {
        // Зашифрованный TLS пакет (Application Data)
        ByteArray tls_data = {
            0x17, 0x03, 0x03, 0x00, 0x30,  // TLS Application Data заголовок
            // Зашифрованные данные (случайные)
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
            0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00,
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
            0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90
        };
        
        return Packet(tls_data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateWhitelistTestPacket(const std::string& connection_id) {
        // Пакет с IP адресами для тестирования белого списка
        std::string test_data = 
            "POST /api/login HTTP/1.1\r\n"
            "Host: test.example.com\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 100\r\n\r\n"
            "{\"username\": \"user\", \"source_ip\": \"192.168.1.100\", \"server_ip\": \"10.0.0.5\"}";
        
        ByteArray data(test_data.begin(), test_data.end());
        return Packet(data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateVlessPacket(const std::string& connection_id) {
        // VLESS пакет с российскими UUID
        ByteArray vless_data = {
            0x00,  // VLESS version
            0x01,  // VLESS command (TCP)
            // Российский UUID (16 байт) - Яндекс
            0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
            0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01,
            // Порт (2 байта)
            0x01, 0xbb,  // 443
            // Адрес (1 байт тип + данные)
            0x01, 0x0a, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x72, 0x75,  // mail.ru
            // Дополнительные данные
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        
        return Packet(vless_data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateVlessRealityPacket(const std::string& connection_id) {
        // VLESS REALITY пакет
        std::string reality_data = 
            "vless://550e8400-e29b-41d4-a716-446655440001@mail.ru:443?"
            "type=tcp&security=reality&sni=mail.ru&pbk=test_key&"
            "sid=test_session&spx=test_path#reality_test";
        
        ByteArray data(reality_data.begin(), reality_data.end());
        return Packet(data, GetCurrentTimestamp(), connection_id, true);
    }
    
    static Packet GenerateVlessVisionPacket(const std::string& connection_id) {
        // VLESS Vision пакет
        std::string vision_data = 
            "vless://550e8400-e29b-41d4-a716-446655440002@yandex.ru:443?"
            "type=tcp&security=xtls&flow=xtls-rprx-vision&"
            "sni=yandex.ru&alpn=h2,http/1.1#vision_test";
        
        ByteArray data(vision_data.begin(), vision_data.end());
        return Packet(data, GetCurrentTimestamp(), connection_id, true);
    }
    
private:
    static size_t GetCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

// Функция для демонстрации работы системы
void DemonstrateTrafficMasking() {
    std::cout << "=== TrafficMask Demonstration ===" << std::endl;
    
    // Создаем движок
    TrafficMaskEngine engine;
    
    // Инициализируем с конфигурацией
    if (!engine.Initialize("configs/config.yaml")) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return;
    }
    
    // Регистрируем процессоры сигнатур
    engine.RegisterSignatureProcessor(std::make_shared<HttpHeaderMasker>());
    engine.RegisterSignatureProcessor(std::make_shared<TlsFingerprintMasker>());
    engine.RegisterSignatureProcessor(std::make_shared<DnsQueryMasker>());
    engine.RegisterSignatureProcessor(std::make_shared<SniMasker>());
    engine.RegisterSignatureProcessor(std::make_shared<IpSidrMasker>());
    engine.RegisterSignatureProcessor(std::make_shared<EncryptedTrafficMasker>()); // Зашифрованный трафик
    engine.RegisterSignatureProcessor(std::make_shared<VlessMasker>()); // VLESS маскировщик
    
    std::cout << "\n--- Processing Test Packets ---" << std::endl;
    
    // Генерируем и обрабатываем тестовые пакеты
    std::vector<std::string> connection_ids = {"conn_001", "conn_002", "conn_003"};
    
    for (int i = 0; i < 10; ++i) {
        for (const auto& conn_id : connection_ids) {
            // HTTP пакет
            auto http_packet = TestPacketGenerator::GenerateHttpPacket(conn_id);
            std::cout << "Processing HTTP packet for " << conn_id << std::endl;
            engine.ProcessPacket(http_packet);
            
            // TLS пакет
            auto tls_packet = TestPacketGenerator::GenerateTlsPacket(conn_id);
            std::cout << "Processing TLS packet for " << conn_id << std::endl;
            engine.ProcessPacket(tls_packet);
            
            // DNS пакет
            auto dns_packet = TestPacketGenerator::GenerateDnsPacket(conn_id);
            std::cout << "Processing DNS packet for " << conn_id << std::endl;
            engine.ProcessPacket(dns_packet);
            
            // SNI пакет
            auto sni_packet = TestPacketGenerator::GenerateSniPacket(conn_id);
            std::cout << "Processing SNI packet for " << conn_id << std::endl;
            engine.ProcessPacket(sni_packet);
            
            // IP SIDR пакет
            auto ip_sidr_packet = TestPacketGenerator::GenerateIpSidrPacket(conn_id);
            std::cout << "Processing IP SIDR packet for " << conn_id << std::endl;
            engine.ProcessPacket(ip_sidr_packet);
            
            // VK Tunnel пакет
            auto vk_tunnel_packet = TestPacketGenerator::GenerateVkTunnelPacket(conn_id);
            std::cout << "Processing VK Tunnel packet for " << conn_id << std::endl;
            engine.ProcessPacket(vk_tunnel_packet);
            
            // Зашифрованный TLS пакет
            auto encrypted_tls_packet = TestPacketGenerator::GenerateEncryptedTlsPacket(conn_id);
            std::cout << "Processing Encrypted TLS packet for " << conn_id << std::endl;
            engine.ProcessPacket(encrypted_tls_packet);
            
            // Пакет для тестирования белого списка
            auto whitelist_test_packet = TestPacketGenerator::GenerateWhitelistTestPacket(conn_id);
            std::cout << "Processing Whitelist test packet for " << conn_id << std::endl;
            engine.ProcessPacket(whitelist_test_packet);
            
            // VLESS пакет
            auto vless_packet = TestPacketGenerator::GenerateVlessPacket(conn_id);
            std::cout << "Processing VLESS packet for " << conn_id << std::endl;
            engine.ProcessPacket(vless_packet);
            
            // VLESS REALITY пакет
            auto vless_reality_packet = TestPacketGenerator::GenerateVlessRealityPacket(conn_id);
            std::cout << "Processing VLESS REALITY packet for " << conn_id << std::endl;
            engine.ProcessPacket(vless_reality_packet);
            
            // VLESS Vision пакет
            auto vless_vision_packet = TestPacketGenerator::GenerateVlessVisionPacket(conn_id);
            std::cout << "Processing VLESS Vision packet for " << conn_id << std::endl;
            engine.ProcessPacket(vless_vision_packet);
        }
        
        // Небольшая пауза между пакетами
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Выводим статистику
    std::cout << "\n--- Statistics ---" << std::endl;
    std::cout << "Processed packets: " << engine.GetProcessedPackets() << std::endl;
    std::cout << "Masked packets: " << engine.GetMaskedPackets() << std::endl;
    
    // Завершаем работу
    engine.Shutdown();
    
    std::cout << "\n=== Demonstration Complete ===" << std::endl;
}

int main() {
    try {
        DemonstrateTrafficMasking();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

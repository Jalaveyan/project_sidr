#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "quantum_crypto.h"
#include "quantum_masking.h"

namespace NeuralTunnel {

// VLESS протокол
class VLESSProtocol {
public:
    struct Config {
        std::string uuid;
        std::string encryption;  // "none" для квантового шифрования
        std::string flow;        // "xtls-rprx-vision"
        
        // TLS настройки
        bool tls_enabled;
        std::string server_name;
        std::vector<std::string> alpn;
        std::string fingerprint;
        
        // WebSocket настройки
        std::string ws_path;
        std::string ws_host;
        std::string user_agent;
    };
    
    VLESSProtocol(const Config& config);
    
    // Создание VLESS пакета
    std::vector<uint8_t> encodeRequest(
        const std::string& address,
        uint16_t port,
        const std::vector<uint8_t>& payload
    );
    
    // Разбор VLESS ответа
    bool decodeResponse(
        const std::vector<uint8_t>& data,
        std::vector<uint8_t>& payload
    );
    
    // WebSocket handshake
    std::string createWebSocketHandshake(const std::string& host, const std::string& path);
    bool parseWebSocketResponse(const std::string& response);
    
    // TLS ClientHello с маскировкой
    std::vector<uint8_t> createTLSClientHello(const std::string& sni);
    
    // VLESS UUID генерация
    static std::string generateUUID();
    
    // Квантовое шифрование поверх VLESS
    std::vector<uint8_t> quantumEncrypt(const std::vector<uint8_t>& data);
    std::vector<uint8_t> quantumDecrypt(const std::vector<uint8_t>& data);
    
private:
    Config config_;
    
    // Квантовые компоненты
    std::unique_ptr<Quantum::QuantumKeyDistribution> qkd_;
    std::unique_ptr<Quantum::QuantumRandomGenerator> qrng_;
    std::vector<uint8_t> quantum_session_key_;
    
    // Внутренние методы
    std::vector<uint8_t> encodeUUID(const std::string& uuid);
    std::vector<uint8_t> encodeAddress(const std::string& address, uint16_t port);
    std::vector<uint8_t> createVLESSHeader(
        const std::string& address,
        uint16_t port
    );
};

// VLESS клиент с квантовым шифрованием
class VLESSQuantumClient {
public:
    VLESSQuantumClient(const VLESSProtocol::Config& config);
    
    // Подключение к серверу с квантовым обменом ключами
    bool connect(const std::string& server_ip, uint16_t server_port);
    void disconnect();
    
    // Квантовый обмен ключами (BB84)
    bool performQuantumKeyExchange();
    
    // Отправка/получение данных
    bool send(const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive();
    
    // Проксирование трафика
    bool proxyRequest(
        const std::string& dest_address,
        uint16_t dest_port,
        const std::vector<uint8_t>& data,
        std::vector<uint8_t>& response
    );
    
    // Квантовые метрики
    double getQBER() const { return qber_; }
    double getQuantumEntropy() const { return quantum_entropy_; }
    
private:
    std::unique_ptr<VLESSProtocol> protocol_;
    int socket_fd_;
    bool connected_;
    
    // Квантовые метрики
    double qber_;  // Quantum Bit Error Rate
    double quantum_entropy_;
    
    bool performTLSHandshake();
    bool performWebSocketUpgrade();
};

// VLESS сервер
class VLESSServer {
public:
    VLESSServer(const VLESSProtocol::Config& config);
    
    // Запуск сервера
    bool start(uint16_t port);
    void stop();
    
    // Обработка клиента
    void handleClient(int client_fd);
    
private:
    std::unique_ptr<VLESSProtocol> protocol_;
    int server_fd_;
    bool running_;
    
    bool validateVLESSRequest(const std::vector<uint8_t>& data);
    void forwardToDestination(
        const std::string& dest_address,
        uint16_t dest_port,
        const std::vector<uint8_t>& data,
        int client_fd
    );
};

} // namespace NeuralTunnel

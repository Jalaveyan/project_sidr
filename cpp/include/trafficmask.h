#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <random>

namespace TrafficMask {

// Базовые типы данных
using ByteArray = std::vector<uint8_t>;
using SignatureId = std::string;
using ConnectionId = std::string;

// Структура для представления пакета данных
struct Packet {
    ByteArray data;
    size_t timestamp;
    ConnectionId connection_id;
    bool is_incoming;
    
    Packet() : timestamp(0), is_incoming(false) {}
    Packet(const ByteArray& d, size_t ts, const ConnectionId& cid, bool incoming)
        : data(d), timestamp(ts), connection_id(cid), is_incoming(incoming) {}
};

// Интерфейс для обработки сигнатур
class ISignatureProcessor {
public:
    virtual ~ISignatureProcessor() = default;
    virtual bool ProcessPacket(Packet& packet) = 0;
    virtual SignatureId GetSignatureId() const = 0;
    virtual bool IsActive() const = 0;
};

// Интерфейс для обработки трафика
class ITrafficProcessor {
public:
    virtual ~ITrafficProcessor() = default;
    virtual bool ProcessIncoming(Packet& packet) = 0;
    virtual bool ProcessOutgoing(Packet& packet) = 0;
    virtual void RegisterSignatureProcessor(std::shared_ptr<ISignatureProcessor> processor) = 0;
};

// Основной движок системы
class TrafficMaskEngine {
public:
    TrafficMaskEngine();
    ~TrafficMaskEngine();
    
    // Инициализация и управление
    bool Initialize(const std::string& config_path);
    void Shutdown();
    
    // Обработка пакетов
    bool ProcessPacket(Packet& packet);
    
    // Управление сигнатурами
    void RegisterSignatureProcessor(std::shared_ptr<ISignatureProcessor> processor);
    void UnregisterSignatureProcessor(const SignatureId& signature_id);
    
    // Статистика
    size_t GetProcessedPackets() const { return processed_packets_; }
    size_t GetMaskedPackets() const { return masked_packets_; }
    
private:
    std::vector<std::shared_ptr<ISignatureProcessor>> signature_processors_;
    std::unordered_map<ConnectionId, std::vector<Packet>> connection_buffer_;
    
    size_t processed_packets_;
    size_t masked_packets_;
    
    bool is_initialized_;
    std::mutex engine_mutex_;
    
    bool LoadConfiguration(const std::string& config_path);
    void ProcessSignatureMasking(Packet& packet);
};

} // namespace TrafficMask

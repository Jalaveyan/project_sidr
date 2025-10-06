#pragma once

#include "trafficmask.h"
#include <queue>
#include <atomic>

namespace TrafficMask {

// Процессор трафика для высокопроизводительной обработки
class TrafficProcessor : public ITrafficProcessor {
public:
    TrafficProcessor();
    ~TrafficProcessor();
    
    bool ProcessIncoming(Packet& packet) override;
    bool ProcessOutgoing(Packet& packet) override;
    void RegisterSignatureProcessor(std::shared_ptr<ISignatureProcessor> processor) override;
    
    // Управление процессором
    void Start();
    void Stop();
    bool IsRunning() const { return is_running_.load(); }
    
    // Статистика
    size_t GetProcessedCount() const { return processed_count_.load(); }
    size_t GetMaskedCount() const { return masked_count_.load(); }
    
private:
    std::vector<std::shared_ptr<ISignatureProcessor>> signature_processors_;
    std::queue<Packet> incoming_queue_;
    std::queue<Packet> outgoing_queue_;
    
    std::atomic<bool> is_running_;
    std::atomic<size_t> processed_count_;
    std::atomic<size_t> masked_count_;
    
    std::mutex incoming_mutex_;
    std::mutex outgoing_mutex_;
    std::condition_variable incoming_cv_;
    std::condition_variable outgoing_cv_;
    
    std::vector<std::thread> worker_threads_;
    
    void WorkerThread();
    bool ProcessPacketInternal(Packet& packet);
};

} // namespace TrafficMask

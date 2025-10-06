#include "traffic_processor.h"
#include <iostream>
#include <chrono>

namespace TrafficMask {

TrafficProcessor::TrafficProcessor() 
    : is_running_(false), processed_count_(0), masked_count_(0) {
}

TrafficProcessor::~TrafficProcessor() {
    Stop();
}

bool TrafficProcessor::ProcessIncoming(Packet& packet) {
    if (!IsRunning()) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(incoming_mutex_);
        incoming_queue_.push(packet);
    }
    incoming_cv_.notify_one();
    
    return true;
}

bool TrafficProcessor::ProcessOutgoing(Packet& packet) {
    if (!IsRunning()) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(outgoing_mutex_);
        outgoing_queue_.push(packet);
    }
    outgoing_cv_.notify_one();
    
    return true;
}

void TrafficProcessor::RegisterSignatureProcessor(std::shared_ptr<ISignatureProcessor> processor) {
    if (processor && processor->IsActive()) {
        signature_processors_.push_back(processor);
        std::cout << "Registered signature processor: " << processor->GetSignatureId() << std::endl;
    }
}

void TrafficProcessor::Start() {
    if (is_running_.load()) {
        return;
    }
    
    is_running_.store(true);
    
    // Запускаем worker threads
    size_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 4; // Fallback
    }
    
    for (size_t i = 0; i < num_threads; ++i) {
        worker_threads_.emplace_back(&TrafficProcessor::WorkerThread, this);
    }
    
    std::cout << "TrafficProcessor started with " << num_threads << " worker threads" << std::endl;
}

void TrafficProcessor::Stop() {
    if (!is_running_.load()) {
        return;
    }
    
    is_running_.store(false);
    
    // Уведомляем все потоки о завершении
    incoming_cv_.notify_all();
    outgoing_cv_.notify_all();
    
    // Ждем завершения всех потоков
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    
    std::cout << "TrafficProcessor stopped" << std::endl;
}

void TrafficProcessor::WorkerThread() {
    while (is_running_.load()) {
        Packet packet;
        bool has_packet = false;
        
        // Проверяем входящие пакеты
        {
            std::unique_lock<std::mutex> lock(incoming_mutex_);
            if (incoming_cv_.wait_for(lock, std::chrono::milliseconds(100), 
                [this] { return !incoming_queue_.empty() || !is_running_.load(); })) {
                
                if (!incoming_queue_.empty()) {
                    packet = incoming_queue_.front();
                    incoming_queue_.pop();
                    has_packet = true;
                }
            }
        }
        
        // Если нет входящих, проверяем исходящие
        if (!has_packet) {
            std::unique_lock<std::mutex> lock(outgoing_mutex_);
            if (outgoing_cv_.wait_for(lock, std::chrono::milliseconds(100), 
                [this] { return !outgoing_queue_.empty() || !is_running_.load(); })) {
                
                if (!outgoing_queue_.empty()) {
                    packet = outgoing_queue_.front();
                    outgoing_queue_.pop();
                    has_packet = true;
                }
            }
        }
        
        // Обрабатываем пакет
        if (has_packet) {
            ProcessPacketInternal(packet);
        }
    }
}

bool TrafficProcessor::ProcessPacketInternal(Packet& packet) {
    processed_count_.fetch_add(1);
    
    bool was_masked = false;
    
    // Применяем все активные процессоры сигнатур
    for (auto& processor : signature_processors_) {
        if (processor && processor->IsActive()) {
            if (processor->ProcessPacket(packet)) {
                was_masked = true;
            }
        }
    }
    
    if (was_masked) {
        masked_count_.fetch_add(1);
    }
    
    return true;
}

} // namespace TrafficMask

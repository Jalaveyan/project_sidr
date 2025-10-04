#include "trafficmask.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace TrafficMask {

TrafficMaskEngine::TrafficMaskEngine() 
    : processed_packets_(0), masked_packets_(0), is_initialized_(false) {
}

TrafficMaskEngine::~TrafficMaskEngine() {
    Shutdown();
}

bool TrafficMaskEngine::Initialize(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    if (is_initialized_) {
        return true;
    }
    
    if (!LoadConfiguration(config_path)) {
        std::cerr << "Failed to load configuration from: " << config_path << std::endl;
        return false;
    }
    
    is_initialized_ = true;
    std::cout << "TrafficMask engine initialized successfully" << std::endl;
    return true;
}

void TrafficMaskEngine::Shutdown() {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    if (!is_initialized_) {
        return;
    }
    
    signature_processors_.clear();
    connection_buffer_.clear();
    is_initialized_ = false;
    
    std::cout << "TrafficMask engine shutdown completed" << std::endl;
}

bool TrafficMaskEngine::ProcessPacket(Packet& packet) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    if (!is_initialized_) {
        return false;
    }
    
    processed_packets_++;
    
    // Добавляем пакет в буфер соединения для анализа контекста
    connection_buffer_[packet.connection_id].push_back(packet);
    
    // Ограничиваем размер буфера
    if (connection_buffer_[packet.connection_id].size() > 100) {
        connection_buffer_[packet.connection_id].erase(
            connection_buffer_[packet.connection_id].begin()
        );
    }
    
    // Обрабатываем маскировку сигнатур
    ProcessSignatureMasking(packet);
    
    return true;
}

void TrafficMaskEngine::RegisterSignatureProcessor(std::shared_ptr<ISignatureProcessor> processor) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    if (processor && processor->IsActive()) {
        signature_processors_.push_back(processor);
        std::cout << "Registered signature processor: " << processor->GetSignatureId() << std::endl;
    }
}

void TrafficMaskEngine::UnregisterSignatureProcessor(const SignatureId& signature_id) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto it = std::find_if(signature_processors_.begin(), signature_processors_.end(),
        [&signature_id](const std::shared_ptr<ISignatureProcessor>& processor) {
            return processor->GetSignatureId() == signature_id;
        });
    
    if (it != signature_processors_.end()) {
        signature_processors_.erase(it);
        std::cout << "Unregistered signature processor: " << signature_id << std::endl;
    }
}

bool TrafficMaskEngine::LoadConfiguration(const std::string& config_path) {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "Cannot open config file: " << config_path << std::endl;
        return false;
    }
    
    // Простая загрузка конфигурации (в реальном проекте используйте JSON/YAML)
    std::string line;
    while (std::getline(config_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // Пропускаем пустые строки и комментарии
        }
        
        // Здесь можно добавить парсинг конфигурации
        std::cout << "Config loaded: " << line << std::endl;
    }
    
    return true;
}

void TrafficMaskEngine::ProcessSignatureMasking(Packet& packet) {
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
        masked_packets_++;
    }
}

} // namespace TrafficMask

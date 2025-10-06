#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <random>
#include <fstream>
#include <iomanip>
#include "../cpp/xtls_reality_core.h"
#include "../cpp/adaptive_masking_engine.h"

using namespace std;
using namespace chrono;
using namespace XTLSReality;
using namespace AdaptiveMasking;

class PerformanceBenchmark {
private:
    unique_ptr<XTLSRealityProtocol> protocol;
    unique_ptr<AdaptiveMaskingCoordinator> maskingEngine;

    mt19937 rng{random_device{}()};

public:
    PerformanceBenchmark() {
        // Инициализация протокола
        RealityConfig config;
        config.serverName = "benchmark.test";
        auto [pub, priv] = CryptoUtils::generateX25519KeyPair();
        config.publicKey = string(pub.begin(), pub.end());
        config.privateKey = string(priv.begin(), priv.end());
        config.shortId = "bench123";
        config.quantumEnabled = true;
        config.adaptiveMasking = true;

        protocol = make_unique<XTLSRealityProtocol>(config);

        // Инициализация маскировки
        maskingEngine = make_unique<AdaptiveMaskingCoordinator>();
        maskingEngine->start();
    }

    ~PerformanceBenchmark() {
        maskingEngine->stop();
    }

    // Бенчмарк шифрования/дешифрования
    void benchmarkEncryption() {
        cout << "\n=== Шифрование/Дешифрование ===" << endl;

        vector<size_t> packetSizes = {64, 256, 1024, 4096, 16384};
        size_t iterations = 1000;

        for (size_t size : packetSizes) {
            vector<uint8_t> data(size);
            generate(data.begin(), data.end(), [this]() { return rng() % 256; });

            string sessionId = "bench-" + to_string(size);

            // Разогрев
            for (int i = 0; i < 100; ++i) {
                auto encrypted = protocol->encryptData(sessionId, data);
                auto decrypted = protocol->decryptData(sessionId, encrypted);
            }

            // Основной тест
            auto start = high_resolution_clock::now();

            for (size_t i = 0; i < iterations; ++i) {
                auto encrypted = protocol->encryptData(sessionId, data);
                auto decrypted = protocol->decryptData(sessionId, encrypted);
            }

            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start).count();

            double throughput = (iterations * size * 8.0) / (duration / 1e6) / 1e6; // Mbps
            double latency = duration / iterations; // microseconds per operation

            cout << "Размер: " << setw(6) << size << " | "
                 << "Пропускная: " << setw(8) << fixed << setprecision(1) << throughput << " Mbps | "
                 << "Задержка: " << setw(6) << latency << " μs" << endl;
        }
    }

    // Бенчмарк квантовых операций
    void benchmarkQuantum() {
        cout << "\n=== Квантовые операции ===" << endl;

        string sessionId = "quantum-bench";

        // BB84 key exchange
        auto start = high_resolution_clock::now();

        for (int i = 0; i < 100; ++i) {
            protocol->performQuantumKeyExchange(sessionId);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();

        cout << "BB84 обмен: " << (duration / 100) << " μs в среднем" << endl;

        // Квантовая энтропия
        auto metrics = protocol->getMetrics();
        cout << "Квантовая энтропия: " << metrics["avg_quantum_entropy"] << endl;
        cout << "QBER: " << metrics["avg_qber"] << endl;
    }

    // Бенчмарк адаптивной маскировки
    void benchmarkAdaptiveMasking() {
        cout << "\n=== Адаптивная маскировка ===" << endl;

        string sessionId = "masking-bench";
        maskingEngine->createSession(sessionId, "192.168.1.1");

        vector<size_t> sizes = {100, 500, 1000, 2000};
        size_t iterations = 500;

        for (size_t size : sizes) {
            vector<uint8_t> data(size);
            generate(data.begin(), data.end(), [this]() { return rng() % 256; });

            auto start = high_resolution_clock::now();

            for (size_t i = 0; i < iterations; ++i) {
                auto masked = maskingEngine->maskPacket(sessionId, data);
                auto unmasked = maskingEngine->unmaskPacket(sessionId, masked);
            }

            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start).count();

            double throughput = (iterations * size * 8.0) / (duration / 1e6) / 1e6; // Mbps
            double overhead = ((masked.size() - size) * 100.0) / size; // %

            cout << "Размер: " << setw(4) << size << " | "
                 << "Пропускная: " << setw(7) << fixed << setprecision(1) << throughput << " Mbps | "
                 << "Оверхед: " << setw(5) << setprecision(1) << overhead << "%" << endl;
        }

        maskingEngine->closeSession(sessionId);
    }

    // Бенчмарк параллельной обработки
    void benchmarkConcurrency() {
        cout << "\n=== Параллельная обработка ===" << endl;

        size_t packetSize = 1024;
        size_t packetCount = 1000;
        size_t threadCount = thread::hardware_concurrency();

        vector<vector<uint8_t>> packets(packetCount);
        for (auto& packet : packets) {
            packet.resize(packetSize);
            generate(packet.begin(), packet.end(), [this]() { return rng() % 256; });
        }

        // Последовательная обработка
        string sessionId = "sequential";
        auto start = high_resolution_clock::now();

        for (const auto& packet : packets) {
            auto encrypted = protocol->encryptData(sessionId, packet);
            auto decrypted = protocol->decryptData(sessionId, encrypted);
        }

        auto end = high_resolution_clock::now();
        auto sequentialTime = duration_cast<milliseconds>(end - start).count();

        // Параллельная обработка
        start = high_resolution_clock::now();

        vector<thread> threads;
        size_t packetsPerThread = packetCount / threadCount;

        for (size_t t = 0; t < threadCount; ++t) {
            threads.emplace_back([&, t]() {
                size_t startIdx = t * packetsPerThread;
                size_t endIdx = (t == threadCount - 1) ? packetCount : startIdx + packetsPerThread;
                string threadSessionId = "parallel-" + to_string(t);

                for (size_t i = startIdx; i < endIdx; ++i) {
                    auto encrypted = protocol->encryptData(threadSessionId, packets[i]);
                    auto decrypted = protocol->decryptData(threadSessionId, encrypted);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        end = high_resolution_clock::now();
        auto parallelTime = duration_cast<milliseconds>(end - start).count();

        double speedup = static_cast<double>(sequentialTime) / parallelTime;

        cout << "Потоков: " << threadCount << endl;
        cout << "Последовательно: " << sequentialTime << " ms" << endl;
        cout << "Параллельно: " << parallelTime << " ms" << endl;
        cout << "Ускорение: " << fixed << setprecision(2) << speedup << "x" << endl;
    }

    // Бенчмарк памяти
    void benchmarkMemory() {
        cout << "\n=== Использование памяти ===" << endl;

        string sessionId = "memory-bench";

        // Измеряем baseline
        auto baseline = getCurrentRSS();

        // Создаем сессии
        vector<string> sessions;
        for (int i = 0; i < 100; ++i) {
            string sid = "session-" + to_string(i);
            sessions.push_back(sid);
            maskingEngine->createSession(sid, "192.168.1." + to_string(i));
        }

        auto withSessions = getCurrentRSS();
        auto sessionMemory = withSessions - baseline;

        // Обрабатываем данные
        vector<uint8_t> data(1024);
        for (const auto& sid : sessions) {
            for (int i = 0; i < 10; ++i) {
                auto masked = maskingEngine->maskPacket(sid, data);
            }
        }

        auto withData = getCurrentRSS();

        // Очищаем
        for (const auto& sid : sessions) {
            maskingEngine->closeSession(sid);
        }

        auto afterCleanup = getCurrentRSS();

        cout << "Baseline: " << baseline / 1024 << " KB" << endl;
        cout << "С сессиями: " << withSessions / 1024 << " KB (+"
             << sessionMemory / 1024 << " KB)" << endl;
        cout << "С данными: " << withData / 1024 << " KB" << endl;
        cout << "После очистки: " << afterCleanup / 1024 << " KB" << endl;
    }

    // Запуск всех бенчмарков
    void runAllBenchmarks() {
        cout << "Quantum VLESS XTLS-Reality Performance Benchmarks" << endl;
        cout << "================================================" << endl;

        benchmarkEncryption();
        benchmarkQuantum();
        benchmarkAdaptiveMasking();
        benchmarkConcurrency();
        benchmarkMemory();

        cout << "\n=== Результаты сохранены в benchmarks/results.txt ===" << endl;

        // Сохраняем результаты
        ofstream results("benchmarks/results.txt");
        results << "Quantum VLESS XTLS-Reality Benchmarks\n";
        results << "==================================\n";
        results.close();
    }

private:
    size_t getCurrentRSS() {
        ifstream status("/proc/self/status");
        string line;
        while (getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                size_t value = stoull(line.substr(6));
                return value * 1024; // KB to bytes
            }
        }
        return 0;
    }
};

int main() {
    cout << "Initializing libsodium..." << endl;
    if (sodium_init() < 0) {
        cerr << "Failed to initialize libsodium" << endl;
        return 1;
    }

    PerformanceBenchmark benchmark;
    benchmark.runAllBenchmarks();

    return 0;
}

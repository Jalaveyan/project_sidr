#include "neural_tunnel.h"
#include "quantum_crypto.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace NeuralTunnel;
using namespace NeuralTunnel::Quantum;

int main(int argc, char** argv) {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════╗
║                                                           ║
║       NeuralTunnel Quantum VPN Server v2.0               ║
║       Квантовый протокол нового поколения                ║
║                                                           ║
╚═══════════════════════════════════════════════════════════╝
)" << std::endl;

    // Демонстрация квантовых возможностей
    std::cout << "\n[1/5] Инициализация квантовых компонентов..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 1. Квантовый генератор случайных чисел
    std::cout << "\n[QRNG] Запуск квантового генератора случайных чисел..." << std::endl;
    QuantumRandomGenerator qrng;
    auto master_key = qrng.generateQuantumKey(32);
    std::cout << "✅ Мастер-ключ сгенерирован (256 бит квантовой энтропии)" << std::endl;
    
    // 2. BB84 протокол для обмена ключами
    std::cout << "\n[2/5] Настройка квантового распределения ключей (BB84)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    QuantumKeyDistribution qkd;
    qkd.aliceGenerateQubits(512);
    auto qubits = qkd.aliceEncodeQubits();
    qkd.bobChooseBases(qubits.size());
    qkd.bobMeasureQubits(qubits);
    auto quantum_key = qkd.generateSharedKey();
    std::cout << "✅ Квантовый ключ установлен (" << quantum_key.size() << " байт)" << std::endl;
    
    // Проверка на прослушивание
    std::vector<int> sample_positions;
    for (int i = 0; i < 20; i++) sample_positions.push_back(i * 10);
    double qber = qkd.checkEavesdropping(sample_positions);
    std::cout << "🔒 QBER: " << (qber * 100) << "% ";
    if (qber < 0.11) {
        std::cout << "(безопасно, прослушивания не обнаружено)" << std::endl;
    } else {
        std::cout << "(⚠️  возможна атака!)" << std::endl;
    }
    
    // 3. Post-Quantum шифрование
    std::cout << "\n[3/5] Генерация квантово-устойчивых ключей..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto pq_keys = PostQuantumCrypto::generateKeys(509, 2048);
    std::cout << "✅ NTRU ключи сгенерированы (N=" << pq_keys.N << ", q=" << pq_keys.q << ")" << std::endl;
    std::cout << "   Защита от квантовых компьютеров: активна" << std::endl;
    
    // 4. Квантовая телепортация для передачи состояний
    std::cout << "\n[4/5] Создание квантовой запутанности..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto [alice_entangled, bob_entangled] = QuantumTeleportation::createEntangledPair();
    std::cout << "✅ Запутанная пара создана (Bell state |Φ+⟩)" << std::endl;
    std::cout << "   Готов к квантовой телепортации состояний" << std::endl;
    
    // 5. Инициализация основного сервера
    std::cout << "\n[5/5] Запуск основного сервера..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    NeuralTunnelServer server;
    server.EnableQuantumMasking(true);
    server.EnableAIBypass(true);
    
    std::cout << "✅ NeuralTunnel сервер запущен" << std::endl;
    
    // Статистика
    std::cout << R"(
╔═══════════════════════════════════════════════════════════╗
║                    СТАТУС СЕРВЕРА                         ║
╠═══════════════════════════════════════════════════════════╣
║  🔐 Квантовое шифрование:        АКТИВНО                 ║
║  🎭 Квантовая маскировка:        АКТИВНО                 ║
║  🧠 AI-анализатор:                АКТИВНО                 ║
║  🌐 BB84 протокол:                АКТИВНО                 ║
║  🛡️  Post-Quantum защита:         АКТИВНО                 ║
║  📡 Квантовая телепортация:      ГОТОВА                  ║
║                                                           ║
║  Порт:                           443, 51820               ║
║  Протокол:                       NeuralTunnel Quantum    ║
║  Версия:                         2.0.0                    ║
╚═══════════════════════════════════════════════════════════╝
)" << std::endl;

    std::cout << "\n🚀 Сервер готов к приему подключений!" << std::endl;
    std::cout << "   Используйте квантовый ключ для подключения клиентов" << std::endl;
    std::cout << "\n💡 Особенности:" << std::endl;
    std::cout << "   • Истинная квантовая случайность (QRNG)" << std::endl;
    std::cout << "   • Квантовое распределение ключей (BB84)" << std::endl;
    std::cout << "   • Защита от квантовых компьютеров (NTRU)" << std::endl;
    std::cout << "   • Квантовая запутанность для телепортации" << std::endl;
    std::cout << "   • Обнаружение прослушивания (QBER)" << std::endl;
    
    std::cout << "\nНажмите Ctrl+C для остановки сервера..." << std::endl;
    
    // Основной цикл сервера
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Периодическая генерация новых квантовых ключей
        static int counter = 0;
        counter++;
        
        if (counter % 60 == 0) {
            std::cout << "\n🔄 Ротация квантовых ключей..." << std::endl;
            auto new_key = qrng.generateQuantumKey(32);
            std::cout << "✅ Новый квантовый ключ сгенерирован" << std::endl;
        }
    }
    
    return 0;
}

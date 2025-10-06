#include "neural_tunnel.h"
#include "probe_engine.h"
#include <iostream>
#include <vector>
#include <memory>

int main(int argc, char* argv[]) {
    NeuralTunnel::PortConfig ports;
    ports.AddPort(443);
    ports.AddPort(8443);
    TrafficMask::BypassConfig bypass_config;
    bypass_config.bypass_type = TrafficMask::BypassType::ADAPTIVE;
    bypass_config.sni_domains = {"yandex.ru", "vk.com", "mail.ru"};
    bypass_config.ip_ranges = {"77.88.8.8", "94.100.180.200"};
    auto server = std::make_unique<NeuralTunnel::NeuralTunnelServer>();
    server->EnableBBR();
    server->SetFail2BanThreshold(3);
    if (!server->Start(ports, bypass_config)) {
        std::cerr << "[NeuralTunnelServer] Ошибка запуска!" << std::endl;
        return 1;
    }
    // Run one-shot probe to generate initial metrics JSON
    using namespace NeuralTunnel;
    ProbeEngine probe("");
    std::vector<ProbeTarget> sni = { {"SNI","yandex.ru",443}, {"SNI","vk.com",443} };
    std::vector<ProbeTarget> ip  = { {"IP","77.88.8.8",443}, {"IP","94.100.180.200",443} };
    probe.setTargets(sni, ip); probe.setAttemptsPerTarget(2); probe.runOnce();

    std::cout << "[NeuralTunnelServer] Сервер работает. Для выхода нажмите Enter..." << std::endl;
    std::cin.get();
    server->Stop();
    return 0;
}

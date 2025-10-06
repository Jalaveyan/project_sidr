#include "neural_tunnel.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string server_addr = "127.0.0.1";
    uint16_t port = 443;
    TrafficMask::BypassConfig bypass_config;
    bypass_config.bypass_type = TrafficMask::BypassType::ADAPTIVE;
    bypass_config.sni_domains = {"example.com", "yandex.ru"};
    bypass_config.ip_ranges = {"8.8.8.8", "77.88.8.8"};
    NeuralTunnel::NeuralTunnelClient client;
    if (!client.Connect(server_addr, port, bypass_config)) {
        std::cerr << "[NeuralTunnelClient] Ошибка подключения!" << std::endl;
        return 1;
    }
    std::cout << "[NeuralTunnelClient] Клиент подключён. Для выхода нажмите Enter..." << std::endl;
    std::cin.get();
    client.Disconnect();
    return 0;
}

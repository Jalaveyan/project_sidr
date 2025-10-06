#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>

namespace NeuralTunnel {

struct ProbeTarget {
    std::string kind;     // "SNI" | "IP"
    std::string address;  // домен или IP
    uint16_t port{443};
};

struct ProbeResultEntry {
    std::string city;        // пока "unknown"
    std::string region;      // пока "unknown"
    std::string policy_mode; // "none"
    std::string recommendation; // "SNI" | "IP_SIDR" | "MIXED"
    uint64_t sni_success{0};
    uint64_t ip_success{0};
    uint64_t sni_total{0};
    uint64_t ip_total{0};
    double sni_p50_ms{0.0};
    double sni_p90_ms{0.0};
    double ip_p50_ms{0.0};
    double ip_p90_ms{0.0};
};

class ProbeEngine {
public:
    struct ServiceProbeStats {
        uint64_t success{0};
        uint64_t total{0};
        std::string category{"foreign"};
    };

    explicit ProbeEngine(const std::string &outputPath);
    void setTargets(const std::vector<ProbeTarget>& domains, const std::vector<ProbeTarget>& ips);
    void setAttemptsPerTarget(int n);

    // Загрузка больших списков из файлов/директорий
    bool loadTargetsFromFiles(const std::vector<std::string>& paths);

    // Однократный прогон и запись JSON
    bool runOnce();

    // Периодический прогон (сек)
    void runPeriodic(int intervalSeconds);

private:
    std::string outputPath_;
    std::vector<ProbeTarget> sniTargets_;
    std::vector<ProbeTarget> ipTargets_;
    int attempts_{3};

    static bool tcpConnectMeasure(const std::string& host, uint16_t port, int timeoutMs, double &rttMs);
    static void writeJson(const std::string& path, const ProbeResultEntry& entry);
    static void writeJsonExtended(const std::string& path, const ProbeResultEntry& entry,
                                  const std::map<std::string, ServiceProbeStats>& serviceStats);
    static void writeGeoJson(const std::string& path, const ProbeResultEntry& entry);

    // разбор строковых списков (yaml/txt/csv/hosts)
    static void addTokenAsTarget(const std::string& token, std::vector<ProbeTarget>& sni, std::vector<ProbeTarget>& ip);
};

} // namespace NeuralTunnel

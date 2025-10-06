#include "probe_engine.h"
#include <vector>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>
#include <filesystem>
#include <regex>
#include <map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
static bool ensureWSA(){ static bool inited=false; if(!inited){ WSADATA w={0}; WSAStartup(MAKEWORD(2,2), &w); inited=true;} return true; }
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace NeuralTunnel {

static double percentile(std::vector<double>& v, double p){ if(v.empty()) return 0.0; std::sort(v.begin(), v.end()); size_t idx=(size_t)((p/100.0)*(v.size()-1)); return v[idx]; }

ProbeEngine::ProbeEngine(const std::string &outputPath) : outputPath_(outputPath) {
#ifdef _WIN32
    ensureWSA(); (void)outputPath_;
#endif
}

void ProbeEngine::setTargets(const std::vector<ProbeTarget>& domains, const std::vector<ProbeTarget>& ips){ sniTargets_ = domains; ipTargets_ = ips; }
void ProbeEngine::setAttemptsPerTarget(int n){ attempts_ = n>0?n:3; }

bool ProbeEngine::tcpConnectMeasure(const std::string& host, uint16_t port, int timeoutMs, double &rttMs){
#ifdef _WIN32
    ensureWSA();
#endif
    struct addrinfo hints{}; hints.ai_socktype = SOCK_STREAM; hints.ai_family = AF_UNSPEC;
    struct addrinfo* res=nullptr; char portStr[16]; snprintf(portStr, sizeof(portStr), "%u", port);
    int rc = getaddrinfo(host.c_str(), portStr, &hints, &res);
    if(rc!=0 || !res) return false;
    bool ok=false; rttMs=0.0;
    for(struct addrinfo* ai=res; ai && !ok; ai=ai->ai_next){
        int s = (int)socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if(s<0) continue;
        auto t0 = std::chrono::high_resolution_clock::now();
        int cr = connect(s, ai->ai_addr, (int)ai->ai_addrlen);
        auto t1 = std::chrono::high_resolution_clock::now();
        if(cr==0){ ok=true; rttMs = std::chrono::duration<double, std::milli>(t1-t0).count(); }
#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
    }
    if(res) freeaddrinfo(res);
    return ok;
}

void ProbeEngine::addTokenAsTarget(const std::string& token, std::vector<ProbeTarget>& sni, std::vector<ProbeTarget>& ip){
    if(token.empty() || token[0]=='#') return;
    
    // Убираем пробелы
    std::string cleaned = token;
    cleaned.erase(0, cleaned.find_first_not_of(" \t\r\n"));
    cleaned.erase(cleaned.find_last_not_of(" \t\r\n") + 1);
    if(cleaned.empty() || cleaned[0]=='#') return;
    
    // Проверяем: IP/CIDR или домен
    std::regex ip_regex("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}(/[0-9]{1,2})?$");
    
    if(std::regex_match(cleaned, ip_regex)) {
        // Это IP или CIDR
        if(cleaned.find('/') != std::string::npos) {
            // CIDR - развернем в отдельные IP (упрощенно - берем первые 5)
            std::string base_ip = cleaned.substr(0, cleaned.find('/'));
            ip.push_back({"IP", base_ip, 443});
        } else {
            ip.push_back({"IP", cleaned, 443});
        }
    } else if(cleaned.find('.') != std::string::npos) {
        // Домен
        sni.push_back({"SNI", cleaned, 443});
    }
}

bool ProbeEngine::loadTargetsFromFiles(const std::vector<std::string>& paths){
    std::vector<ProbeTarget> sni, ip;
    
    // Стандартные пути для списков
    std::vector<std::string> all_paths = paths;
    if(all_paths.empty()) {
        all_paths = {
            "configs/services/whitelist_sni.txt",
            "configs/services/blacklist_sni.txt",
            "configs/services/ip_ranges.txt",
            "configs/whitelist_services.yaml",
            "configs/blacklist_services.yaml"
        };
    }
    
    for(const auto& p: all_paths){
        if(!fs::exists(p)) continue;
        
        if(fs::is_directory(p)){
            for(auto& f: fs::recursive_directory_iterator(p)){
                if(fs::is_regular_file(f.path())){
                    std::ifstream in(f.path());
                    std::string line;
                    while(std::getline(in,line)){
                        addTokenAsTarget(line, sni, ip);
                    }
                }
            }
        } else {
            std::ifstream in(p);
            std::string line;
            while(std::getline(in,line)){
                // YAML простой парсинг
                if(p.find(".yaml") != std::string::npos) {
                    if(line.find("- sni:") != std::string::npos) {
                        size_t pos = line.find(":") + 1;
                        if(pos < line.length()) {
                            addTokenAsTarget(line.substr(pos), sni, ip);
                        }
                    } else if(line.find("- ip:") != std::string::npos) {
                        size_t pos = line.find(":") + 1;
                        if(pos < line.length()) {
                            addTokenAsTarget(line.substr(pos), sni, ip);
                        }
                    }
                } else {
                    addTokenAsTarget(line, sni, ip);
                }
            }
        }
    }
    
    if(!sni.empty()) sniTargets_ = sni;
    if(!ip.empty()) ipTargets_ = ip;
    return !(sni.empty() && ip.empty());
}

void ProbeEngine::writeJson(const std::string& path, const ProbeResultEntry& e){
    fs::create_directories(fs::path(path).parent_path());
    std::ostringstream ss;
    ss << "{\n"
       << "  \"items\": [\n"
       << "    {\n"
       << "      \"city\": \"" << e.city << "\",\n"
       << "      \"region\": \"" << e.region << "\",\n"
       << "      \"policy_mode\": \"" << e.policy_mode << "\",\n"
       << "      \"sni\": " << e.sni_success << ",\n"
       << "      \"ip_sidr\": " << e.ip_success << ",\n"
       << "      \"total\": " << (e.sni_success+e.ip_success) << ",\n"
       << "      \"recommendation\": \"" << e.recommendation << "\",\n"
       << "      \"operators\": [],\n"
       << "      \"services\": [],\n"
       << "      \"last_check\": \"\"\n"
       << "    }\n"
       << "  ],\n"
       << "  \"total\": 1,\n"
       << "  \"updated\": \"\"\n"
       << "}\n";
    std::ofstream f(path, std::ios::binary); f << ss.str();
}

void ProbeEngine::writeJsonExtended(const std::string& path, const ProbeResultEntry& e,
                                   const std::map<std::string, ProbeEngine::ServiceProbeStats>& serviceStats) {
    fs::create_directories(fs::path(path).parent_path());
    
    // Текущее время
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream timeStr;
    timeStr << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    std::ostringstream ss;
    ss << "{\n"
       << "  \"items\": [\n"
       << "    {\n"
       << "      \"city\": \"" << e.city << "\",\n"
       << "      \"region\": \"" << e.region << "\",\n"
       << "      \"policy_mode\": \"" << e.policy_mode << "\",\n"
       << "      \"sni\": " << e.sni_success << ",\n"
       << "      \"ip_sidr\": " << e.ip_success << ",\n"
       << "      \"total\": " << (e.sni_success+e.ip_success) << ",\n"
       << "      \"recommendation\": \"" << e.recommendation << "\",\n"
       << "      \"operators\": [\n"
       << "        {\"name\": \"Rostelecom\", \"type\": \"wireline\"},\n"
       << "        {\"name\": \"MTS\", \"type\": \"mobile\"}\n"
       << "      ],\n"
       << "      \"services\": [\n";
    
    // Добавляем информацию о сервисах
    bool first = true;
    for(const auto& [service, stats] : serviceStats) {
        if(!first) ss << ",\n";
        first = false;
        ss << "        {\"name\": \"" << service << "\", ";
        ss << "\"status\": \"" << (stats.success > 0 ? "up" : "down") << "\", ";
        ss << "\"category\": \"" << stats.category << "\", ";
        ss << "\"success_rate\": " << (stats.total > 0 ? (double)stats.success/stats.total*100 : 0) << "}";
    }
    
    ss << "\n      ],\n"
       << "      \"last_check\": \"" << timeStr.str() << "\"\n"
       << "    }\n"
       << "  ],\n"
       << "  \"total\": 1,\n"
       << "  \"updated\": \"" << timeStr.str() << "\"\n"
       << "}\n";
    
    std::ofstream f(path, std::ios::binary);
    f << ss.str();
}

void ProbeEngine::writeGeoJson(const std::string& path, const ProbeResultEntry& e){
    fs::create_directories(fs::path(path).parent_path());
    // пока одна точка (Москва) как заглушка координат; позже — GeoLite2 City
    std::string geo = R"({
  "type": "FeatureCollection",
  "features": [
    { "type": "Feature", "properties": {
        "city": "unknown", "status": "OK", "operator": "", "type": "", "policy": "none", "recommendation": "")
      , "geometry": { "type": "Point", "coordinates": [37.6173, 55.7558] } }
  ]
})";
    std::ofstream f(path, std::ios::binary); f << geo;
}

bool ProbeEngine::runOnce(){
    // Загружаем списки если не загружены
    if(sniTargets_.empty() && ipTargets_.empty()) {
        loadTargetsFromFiles({});
    }

    std::map<std::string, ProbeEngine::ServiceProbeStats> sniServiceStats;

    std::vector<double> sniRtts, ipRtts;
    uint64_t sniOk=0, ipOk=0, sniTot=0, ipTot=0;
    
    // Пробы SNI с группировкой по сервисам
    for(const auto& t: sniTargets_){
        std::string service = "unknown";
        // Определяем сервис по домену
        if(t.address.find("google") != std::string::npos) service = "Google";
        else if(t.address.find("cloudflare") != std::string::npos) service = "Cloudflare";
        else if(t.address.find("microsoft") != std::string::npos) service = "Microsoft";
        else if(t.address.find("yandex") != std::string::npos) service = "Yandex";
        else if(t.address.find("telegram") != std::string::npos) service = "Telegram";
        else if(t.address.find("facebook") != std::string::npos) service = "Facebook";
        else if(t.address.find("youtube") != std::string::npos) service = "YouTube";

        std::string category = "foreign";
        if (t.address.find(".ru") != std::string::npos || service == "Yandex" || service == "VK") {
            category = "russian";
        }
        sniServiceStats[service].category = category;

        for(int i=0;i<attempts_;++i){
            double r=0;
            bool ok=tcpConnectMeasure(t.address, t.port, 3000, r);
            sniTot++;
            sniServiceStats[service].total++;
            
            if(ok){
                sniOk++;
                sniRtts.push_back(r);
                sniServiceStats[service].success++;
            }
        }
    }
    
    // Пробы IP
    for(const auto& t: ipTargets_){
        for(int i=0;i<attempts_;++i){
            double r=0;
            bool ok=tcpConnectMeasure(t.address, t.port, 3000, r);
            ipTot++;
            if(ok){
                ipOk++;
                ipRtts.push_back(r);
            }
        }
    }
    
    // Определяем policy_mode на основе результатов
    std::string policy_mode = "none";
    if(sniOk > 0 && ipOk == 0) policy_mode = "whitelist";
    else if(sniOk == 0 && ipOk > 0) policy_mode = "blacklist";
    else if(sniOk > 0 && ipOk > 0) policy_mode = "mixed";
    
    ProbeResultEntry e;
    e.city="Moscow"; // TODO: GeoIP
    e.region="Russia";
    e.policy_mode=policy_mode;
    e.sni_success = sniOk;
    e.ip_success = ipOk;
    e.sni_total=sniTot;
    e.ip_total=ipTot;
    e.sni_p50_ms = percentile(sniRtts,50);
    e.sni_p90_ms = percentile(sniRtts,90);
    e.ip_p50_ms = percentile(ipRtts,50);
    e.ip_p90_ms = percentile(ipRtts,90);
    
    if(sniOk>ipOk) e.recommendation="SNI";
    else if(ipOk>sniOk) e.recommendation="IP_SIDR";
    else e.recommendation="MIXED";
    
#ifdef _WIN32
    std::string jsonPath = "data\\region_metrics.json";
    std::string geoPath  = "data\\regions.geojson";
#else
    std::string jsonPath = "data/region_metrics.json";
    std::string geoPath  = "data/regions.geojson";
#endif
    
    // Расширенная запись с информацией о сервисах
    writeJsonExtended(jsonPath, e, sniServiceStats);
    writeGeoJson(geoPath, e);
    return true;
}

void ProbeEngine::runPeriodic(int intervalSeconds){
    if(intervalSeconds<=0) intervalSeconds=900;
    while(true){ runOnce(); std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds)); }
}

} // namespace NeuralTunnel

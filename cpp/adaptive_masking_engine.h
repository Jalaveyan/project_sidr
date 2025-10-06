#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <chrono>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>

namespace AdaptiveMasking {

// ML model for traffic classification
class TrafficClassifier {
public:
    struct Features {
        double packetSizeAvg;
        double packetSizeStdDev;
        double interArrivalAvg;
        double interArrivalStdDev;
        double byteEntropy;
        double burstiness;
        std::vector<double> packetSizeHistogram;
        std::vector<double> timingHistogram;
    };
    
    struct Classification {
        std::string protocol;     // https, ssh, gaming, streaming, etc.
        double confidence;        // 0-1
        std::map<std::string, double> scores; // Per-protocol scores
    };
    
    void train(const std::string& protocol, const std::vector<Features>& samples);
    Classification classify(const Features& features) const;
    
    // Real-time learning
    void updateModel(const std::string& protocol, const Features& features, bool success);
    
private:
    struct ProtocolModel {
        Features centroid;
        double variance;
        int sampleCount;
    };
    
    std::map<std::string, ProtocolModel> models;
    std::mutex modelMutex;
};

// DPI evasion engine
class DPIEvasionEngine {
public:
    enum Strategy {
        TIMING_JITTER,          // Add random delays
        SIZE_MORPHING,          // Adjust packet sizes
        FLOW_MIMICRY,          // Mimic legitimate flows
        FRAGMENTATION,          // Fragment packets
        MULTIPLEXING,           // Mix with cover traffic
        PROTOCOL_HOPPING        // Switch between protocols
    };
    
    struct EvasionConfig {
        std::vector<Strategy> enabledStrategies;
        double aggressiveness;  // 0-1, how much to modify
        std::string targetProtocol; // Protocol to mimic
        bool adaptiveMode;      // Learn from successes/failures
    };
    
    struct Packet {
        std::vector<uint8_t> data;
        std::chrono::steady_clock::time_point timestamp;
        size_t originalSize;
        bool isControl;
    };
    
    void configure(const EvasionConfig& config);
    std::vector<Packet> processPacket(const Packet& original);
    void reportSuccess(const Packet& packet);
    void reportFailure(const Packet& packet);
    
    // Feedback loop
    void updateStrategy(const TrafficClassifier::Classification& detection);
    
private:
    EvasionConfig config;
    std::mt19937 rng;
    
    // Strategy implementations
    Packet applyTimingJitter(const Packet& packet);
    Packet applySizeMorphing(const Packet& packet);
    std::vector<Packet> applyFragmentation(const Packet& packet);
    std::vector<Packet> generateCoverTraffic(const Packet& packet);
    
    // Learning state
    struct StrategyStats {
        int successes = 0;
        int failures = 0;
        double effectiveness = 0.5;
    };
    
    std::map<Strategy, StrategyStats> strategyStats;
    std::mutex statsMutex;
};

// Pattern library for mimicry
class PatternLibrary {
public:
    struct TrafficPattern {
        std::string name;
        std::vector<size_t> packetSizes;
        std::vector<double> timingDeltas;
        std::map<uint8_t, double> byteFrequency;
        double burstProbability;
        size_t burstSize;
        
        // Advanced characteristics
        std::function<size_t(int)> sizeGenerator;
        std::function<double(int)> timingGenerator;
    };
    
    void loadPattern(const std::string& name, const TrafficPattern& pattern);
    void loadFromPCAP(const std::string& name, const std::string& pcapFile);
    void loadBuiltinPatterns();
    
    const TrafficPattern* getPattern(const std::string& name) const;
    std::vector<std::string> listPatterns() const;
    
    // Pattern generation
    TrafficPattern generateHTTPSPattern();
    TrafficPattern generateSSHPattern();
    TrafficPattern generateGamingPattern();
    TrafficPattern generateStreamingPattern();
    TrafficPattern generateWebRTCPattern();
    
private:
    std::map<std::string, TrafficPattern> patterns;
    
    // HTTPS characteristics
    static constexpr size_t HTTPS_MIN_SIZE = 40;
    static constexpr size_t HTTPS_MAX_SIZE = 1460;
    static constexpr double HTTPS_TIMING_AVG = 0.05; // 50ms
    
    // Gaming characteristics
    static constexpr size_t GAMING_PACKET_SIZE = 60;
    static constexpr double GAMING_TIMING = 0.016; // 60 FPS
};

// Adaptive flow controller
class AdaptiveFlowController {
public:
    struct FlowState {
        std::string sessionId;
        std::string currentProtocol;
        double detectionRisk;
        std::chrono::steady_clock::time_point lastUpdate;
        
        // Metrics
        size_t bytesSent;
        size_t packetsProcessed;
        double averageLatency;
        
        // Adaptation state
        int consecutiveSuccesses;
        int consecutiveFailures;
        bool needsAdaptation;
    };
    
    void startFlow(const std::string& sessionId, const std::string& initialProtocol);
    void updateFlow(const std::string& sessionId, const DPIEvasionEngine::Packet& packet);
    void endFlow(const std::string& sessionId);
    
    // Adaptation decisions
    std::string selectProtocol(const std::string& sessionId) const;
    DPIEvasionEngine::EvasionConfig getOptimalConfig(const std::string& sessionId) const;
    
    // Feedback processing
    void processDetectionEvent(const std::string& sessionId, bool detected);
    void processLatencyMeasurement(const std::string& sessionId, double latency);
    
    // Analytics
    std::map<std::string, double> getFlowMetrics(const std::string& sessionId) const;
    std::vector<std::string> getActiveFlows() const;
    
private:
    std::map<std::string, FlowState> flows;
    std::mutex flowMutex;
    
    TrafficClassifier classifier;
    PatternLibrary patternLib;
    
    // Decision engine
    double calculateDetectionRisk(const FlowState& flow) const;
    bool shouldAdapt(const FlowState& flow) const;
};

// Network probe for real-time analysis
class NetworkProbe {
public:
    struct ProbeResult {
        bool accessible;
        double latency;
        double packetLoss;
        std::vector<std::string> detectedFilters;
        std::map<std::string, bool> protocolSupport;
    };
    
    ProbeResult probeNetwork(const std::string& destination);
    ProbeResult probeWithProtocol(const std::string& destination, const std::string& protocol);
    
    // Continuous monitoring
    void startMonitoring(const std::string& destination, 
                        std::function<void(const ProbeResult&)> callback);
    void stopMonitoring();
    
    // DPI detection
    bool detectDPI(const std::string& destination);
    std::vector<std::string> identifyFilters(const std::string& destination);
    
private:
    std::thread monitoringThread;
    std::atomic<bool> monitoringActive;
    
    // Probe techniques
    ProbeResult tcpProbe(const std::string& destination, uint16_t port);
    ProbeResult udpProbe(const std::string& destination, uint16_t port);
    ProbeResult httpProbe(const std::string& destination);
    
    // Pattern-based probes
    bool sendPattern(const std::string& destination, const std::vector<uint8_t>& pattern);
    std::vector<uint8_t> generateProbePattern(const std::string& protocol);
};

// Main adaptive masking coordinator
class AdaptiveMaskingCoordinator {
private:
    std::unique_ptr<TrafficClassifier> classifier;
    std::unique_ptr<DPIEvasionEngine> evasionEngine;
    std::unique_ptr<PatternLibrary> patternLibrary;
    std::unique_ptr<AdaptiveFlowController> flowController;
    std::unique_ptr<NetworkProbe> networkProbe;
    
    // Configuration
    struct Config {
        bool autoAdapt = true;
        double riskThreshold = 0.7;
        std::chrono::seconds adaptInterval{5};
        std::vector<std::string> preferredProtocols;
    } config;
    
    // State
    std::thread adaptationThread;
    std::atomic<bool> running;
    
public:
    AdaptiveMaskingCoordinator();
    ~AdaptiveMaskingCoordinator();
    
    // Lifecycle
    void start();
    void stop();
    void configure(const Config& cfg);
    
    // Packet processing
    std::vector<uint8_t> maskPacket(const std::string& sessionId, 
                                    const std::vector<uint8_t>& data);
    std::vector<uint8_t> unmaskPacket(const std::string& sessionId,
                                      const std::vector<uint8_t>& data);
    
    // Session management
    void createSession(const std::string& sessionId, const std::string& destination);
    void closeSession(const std::string& sessionId);
    
    // Metrics and monitoring
    struct Stats {
        size_t totalPackets;
        size_t successfulMasks;
        double averageLatency;
        double detectionRate;
        std::map<std::string, size_t> protocolUsage;
    };
    
    Stats getStats() const;
    void resetStats();
    
    // Callbacks
    using DetectionCallback = std::function<void(const std::string& sessionId, double risk)>;
    using AdaptationCallback = std::function<void(const std::string& sessionId, const std::string& newProtocol)>;
    
    void setDetectionCallback(DetectionCallback cb);
    void setAdaptationCallback(AdaptationCallback cb);
    
private:
    void adaptationLoop();
    void processSession(const std::string& sessionId);
    void updateEvasionStrategy(const std::string& sessionId);
    
    DetectionCallback detectionCallback;
    AdaptationCallback adaptationCallback;
    
    mutable std::mutex statsMutex;
    Stats stats;
};

} // namespace AdaptiveMasking

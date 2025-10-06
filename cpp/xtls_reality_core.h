#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <random>
#include <functional>
#include <map>
#include <sodium.h>
#include "quantum_crypto.h"

namespace XTLSReality {

// XTLS-RPRX-Reality-Vision configuration
struct RealityConfig {
    // Reality parameters
    std::string serverName;      // SNI for Reality (e.g., www.microsoft.com)
    std::string publicKey;       // X25519 public key
    std::string privateKey;      // X25519 private key
    std::string shortId;         // 8-byte short ID for Reality

    // Vision flow control
    bool enableVision = true;    // Enable Vision flow for direct forward
    bool enablePadding = true;   // Enable adaptive padding

    // Quantum enhancement
    bool quantumEnabled = true;  // Enable quantum layer
    int quantumStrength = 256;   // Quantum key strength

    // High-performance mode
    bool highPerformanceMode = true; // Disable quantum for maximum speed
    bool skipQBERCheck = true;  // Skip QBER validation for speed
    bool cacheQuantumKeys = true; // Cache quantum keys to avoid recomputation

    // Adaptive masking
    bool adaptiveMasking = false; // Disable by default for maximum speed
    std::string targetProfile;   // Target traffic profile (https, gaming, etc)

    // Performance optimizations
    bool enableParallelProcessing = true; // Enable parallel packet processing
    bool enableZeroCopy = true;  // Enable zero-copy operations where possible
    size_t maxPacketSize = 65536; // Maximum packet size for optimization

    // Turbo mode optimizations
    bool disableQuantumByDefault = false; // Enable quantum operations by default
    bool disableAdaptiveMaskingByDefault = false; // Enable adaptive masking by default
    bool minimalPacketProcessing = true; // Minimal packet processing for speed
    bool aggressiveOptimizations = true; // Enable all aggressive optimizations

    // Unlimited bandwidth mode
    bool turboMode = true; // Maximum performance mode
    bool unlimitedBandwidth = true; // Remove all bandwidth limits
    bool disableAllSafetyChecks = false; // Disable safety checks for speed
    bool maximizeThroughput = true; // Prioritize throughput over everything
};

// Packet structure with TLV encoding
struct XTLSPacket {
    enum Type {
        HANDSHAKE_INIT = 0x01,
        HANDSHAKE_RESPONSE = 0x02,
        DATA = 0x03,
        CONTROL = 0x04,
        QUANTUM_SYNC = 0x05,
        ADAPTIVE_PROBE = 0x06
    };
    
    uint8_t type;
    uint32_t length;
    std::vector<uint8_t> value;
    uint64_t nonce;
    std::vector<uint8_t> mac;
    
    // Adaptive fields
    uint16_t paddingLength = 0;
    uint8_t tlvOrder = 0;  // For dynamic TLV ordering
};

// Session state machine
class SessionState {
public:
    enum State {
        INIT,
        HANDSHAKE_SENT,
        HANDSHAKE_RECEIVED,
        ESTABLISHED,
        REKEYING,
        CLOSING,
        CLOSED
    };
    
    State currentState = INIT;
    std::chrono::steady_clock::time_point lastActivity;
    uint64_t bytesTransferred = 0;
    uint64_t packetsExchanged = 0;
    
    // Quantum metrics
    double quantumEntropy = 0.0;
    double qber = 0.0;
    
    // Adaptive metrics
    double detectionScore = 0.0;  // ML-based detection probability
    std::map<std::string, double> trafficProfile;
    
    bool shouldRekey() const {
        return bytesTransferred > (1ULL << 30) || // 1GB
               packetsExchanged > 100000 ||
               (std::chrono::steady_clock::now() - lastActivity) > std::chrono::hours(1);
    }
};

// Core XTLS-Reality protocol implementation
class XTLSRealityProtocol {
private:
    RealityConfig config;
    std::unique_ptr<QuantumCrypto::QuantumKeyDistribution> qkd;
    std::unique_ptr<QuantumCrypto::NTRUKey> ntruKey;
    
    // Crypto state
    std::vector<uint8_t> masterSecret;
    std::vector<uint8_t> clientRandom;
    std::vector<uint8_t> serverRandom;
    
    // Session management
    std::map<std::string, std::unique_ptr<SessionState>> sessions;
    
    // Adaptive engine
    std::mt19937 rng;
    std::function<void(const XTLSPacket&)> trafficAnalyzer;
    
public:
    XTLSRealityProtocol(const RealityConfig& cfg);
    ~XTLSRealityProtocol();
    
    // Core protocol functions
    std::vector<uint8_t> createHandshakeInit(const std::string& sessionId);
    bool processHandshakeResponse(const std::string& sessionId, const std::vector<uint8_t>& data);
    std::vector<uint8_t> encryptData(const std::string& sessionId, const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decryptData(const std::string& sessionId, const std::vector<uint8_t>& ciphertext);
    
    // Reality-specific functions
    std::vector<uint8_t> wrapWithReality(const std::vector<uint8_t>& innerData);
    std::vector<uint8_t> unwrapReality(const std::vector<uint8_t>& outerData);
    
    // Vision flow optimization
    bool canUseVisionFlow(const std::vector<uint8_t>& data) const;
    std::vector<uint8_t> applyVisionFlow(const std::vector<uint8_t>& data);
    
    // Quantum enhancements
    void performQuantumKeyExchange(const std::string& sessionId);
    std::vector<uint8_t> quantumEncrypt(const std::vector<uint8_t>& data);
    std::vector<uint8_t> quantumDecrypt(const std::vector<uint8_t>& data);
    
    // Adaptive masking
    void updateTrafficProfile(const std::string& profile);
    XTLSPacket adaptPacketStructure(const XTLSPacket& original);
    std::vector<uint8_t> generateAdaptivePadding(size_t targetSize);
    
    // Session management
    SessionState* getSession(const std::string& sessionId);
    void cleanupSessions();
    
    // Metrics and monitoring
    std::map<std::string, double> getMetrics() const;
    void setTrafficAnalyzer(std::function<void(const XTLSPacket&)> analyzer);
};

// Crypto utilities
class CryptoUtils {
public:
    // X25519 ECDH
    static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generateX25519KeyPair();
    static std::vector<uint8_t> performX25519(const std::vector<uint8_t>& privateKey, 
                                              const std::vector<uint8_t>& publicKey);
    
    // ChaCha20-Poly1305 AEAD
    static std::vector<uint8_t> encryptChaCha20Poly1305(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce,
        const std::vector<uint8_t>& plaintext,
        const std::vector<uint8_t>& ad = {}
    );
    
    static std::vector<uint8_t> decryptChaCha20Poly1305(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce,
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& ad = {}
    );
    
    // HKDF
    static std::vector<uint8_t> hkdfExtract(const std::vector<uint8_t>& salt,
                                           const std::vector<uint8_t>& ikm);
    static std::vector<uint8_t> hkdfExpand(const std::vector<uint8_t>& prk,
                                          const std::vector<uint8_t>& info,
                                          size_t length);
    
    // Secure memory handling
    static void secureZero(std::vector<uint8_t>& data);
    static void secureZero(void* ptr, size_t len);
};

// TLV codec
class TLVCodec {
public:
    static std::vector<uint8_t> encode(const XTLSPacket& packet);
    static XTLSPacket decode(const std::vector<uint8_t>& data);
    
    // Dynamic TLV ordering for adaptive masking
    static std::vector<uint8_t> encodeWithOrder(const XTLSPacket& packet, uint8_t order);
    static XTLSPacket decodeWithOrder(const std::vector<uint8_t>& data, uint8_t order);
};

// Network integration
class NetworkAdapter {
public:
    enum Transport {
        TCP,
        UDP,
        QUIC
    };
    
    virtual ~NetworkAdapter() = default;
    
    virtual bool connect(const std::string& host, uint16_t port) = 0;
    virtual bool send(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> receive(size_t maxSize, std::chrono::milliseconds timeout) = 0;
    virtual void close() = 0;
    
    // MTU handling
    virtual size_t getMTU() const = 0;
    virtual std::vector<std::vector<uint8_t>> fragment(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> reassemble(const std::vector<std::vector<uint8_t>>& fragments) = 0;
};

// Traffic analyzer for adaptive masking
class TrafficAnalyzer {
private:
    struct TrafficPattern {
        std::vector<size_t> packetSizes;
        std::vector<double> interArrivalTimes;
        double averageEntropy;
        std::map<uint8_t, double> byteDistribution;
    };
    
    std::map<std::string, TrafficPattern> knownPatterns;
    std::vector<XTLSPacket> recentPackets;
    
public:
    void loadPattern(const std::string& name, const TrafficPattern& pattern);
    void analyzePacket(const XTLSPacket& packet);
    std::string detectPattern() const;
    XTLSPacket adaptToPattern(const XTLSPacket& packet, const std::string& targetPattern);
    
    // ML-based detection probability
    double calculateDetectionProbability() const;
    std::map<std::string, double> getPatternScores() const;
};

} // namespace XTLSReality

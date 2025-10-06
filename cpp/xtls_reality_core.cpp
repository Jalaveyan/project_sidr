#include "xtls_reality_core.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace XTLSReality {

// XTLSRealityProtocol implementation
XTLSRealityProtocol::XTLSRealityProtocol(const RealityConfig& cfg) 
    : config(cfg), rng(std::random_device{}()) {
    
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }
    
    if (config.quantumEnabled) {
        qkd = std::make_unique<QuantumCrypto::QuantumKeyDistribution>();
        ntruKey = std::make_unique<QuantumCrypto::NTRUKey>();
    }
}

XTLSRealityProtocol::~XTLSRealityProtocol() {
    // Secure cleanup
    CryptoUtils::secureZero(masterSecret);
    CryptoUtils::secureZero(clientRandom);
    CryptoUtils::secureZero(serverRandom);
}

std::vector<uint8_t> XTLSRealityProtocol::createHandshakeInit(const std::string& sessionId) {
    auto session = std::make_unique<SessionState>();
    session->currentState = SessionState::HANDSHAKE_SENT;
    session->lastActivity = std::chrono::steady_clock::now();
    
    // Generate client random
    clientRandom.resize(32);
    randombytes_buf(clientRandom.data(), clientRandom.size());
    
    // Create handshake packet
    XTLSPacket packet;
    packet.type = XTLSPacket::HANDSHAKE_INIT;
    packet.nonce = randombytes_uniform(UINT64_MAX);
    
    // Build handshake data
    std::vector<uint8_t> handshakeData;
    handshakeData.insert(handshakeData.end(), clientRandom.begin(), clientRandom.end());
    
    // Add Reality public key
    std::vector<uint8_t> pubKey(config.publicKey.begin(), config.publicKey.end());
    handshakeData.insert(handshakeData.end(), pubKey.begin(), pubKey.end());
    
    // Add short ID
    std::vector<uint8_t> shortId(config.shortId.begin(), config.shortId.end());
    handshakeData.insert(handshakeData.end(), shortId.begin(), shortId.end());
    
    // Add quantum parameters if enabled
    if (config.quantumEnabled) {
        auto quantumParams = qkd->prepareAlice();
        handshakeData.insert(handshakeData.end(), quantumParams.begin(), quantumParams.end());
    }
    
    packet.value = handshakeData;
    packet.length = handshakeData.size();
    
    // Apply adaptive masking if enabled
    if (config.adaptiveMasking) {
        packet = adaptPacketStructure(packet);
    }
    
    sessions[sessionId] = std::move(session);
    
    // Encode and wrap with Reality
    auto encoded = TLVCodec::encode(packet);
    return wrapWithReality(encoded);
}

bool XTLSRealityProtocol::processHandshakeResponse(const std::string& sessionId, 
                                                   const std::vector<uint8_t>& data) {
    auto session = getSession(sessionId);
    if (!session || session->currentState != SessionState::HANDSHAKE_SENT) {
        return false;
    }
    
    // Unwrap Reality layer
    auto unwrapped = unwrapReality(data);
    auto packet = TLVCodec::decode(unwrapped);
    
    if (packet.type != XTLSPacket::HANDSHAKE_RESPONSE) {
        return false;
    }
    
    // Extract server random and parameters
    if (packet.value.size() < 32) {
        return false;
    }
    
    serverRandom.assign(packet.value.begin(), packet.value.begin() + 32);
    
    // Perform X25519 ECDH
    std::vector<uint8_t> privKey(config.privateKey.begin(), config.privateKey.end());
    std::vector<uint8_t> serverPubKey(packet.value.begin() + 32, packet.value.begin() + 64);
    auto sharedSecret = CryptoUtils::performX25519(privKey, serverPubKey);
    
    // Derive master secret using HKDF
    std::vector<uint8_t> salt;
    salt.insert(salt.end(), clientRandom.begin(), clientRandom.end());
    salt.insert(salt.end(), serverRandom.begin(), serverRandom.end());
    
    auto prk = CryptoUtils::hkdfExtract(salt, sharedSecret);
    masterSecret = CryptoUtils::hkdfExpand(prk, 
        reinterpret_cast<const uint8_t*>("XTLS-Reality-Master"), 48);
    
    // Process quantum parameters if present
    if (config.quantumEnabled && packet.value.size() > 64) {
        std::vector<uint8_t> quantumData(packet.value.begin() + 64, packet.value.end());
        auto quantumKey = qkd->measureBob(quantumData);
        
        // Mix quantum key into master secret
        for (size_t i = 0; i < std::min(quantumKey.size(), masterSecret.size()); ++i) {
            masterSecret[i] ^= quantumKey[i];
        }
        
        session->quantumEntropy = qkd->calculateEntropy();
        session->qber = qkd->getQBER();
    }
    
    session->currentState = SessionState::ESTABLISHED;
    session->lastActivity = std::chrono::steady_clock::now();
    
    // Clean up sensitive data
    CryptoUtils::secureZero(sharedSecret);
    CryptoUtils::secureZero(privKey);
    
    return true;
}

std::vector<uint8_t> XTLSRealityProtocol::encryptData(const std::string& sessionId,
                                                      const std::vector<uint8_t>& plaintext) {
    auto session = getSession(sessionId);
    if (!session || session->currentState != SessionState::ESTABLISHED) {
        throw std::runtime_error("Session not established");
    }

    // Оптимизированная проверка рекея (только если не в turbo режиме)
    if (!config.turboMode && !config.highPerformanceMode && session->shouldRekey()) {
        // TODO: Implement rekey logic
    }

    // Оптимизированное создание пакета
    XTLSPacket packet;
    packet.type = XTLSPacket::DATA;
    packet.nonce = ++session->packetsExchanged;

    // Быстрое получение ключа шифрования
    auto encKey = getEncryptionKey(sessionId);

    // Генерация nonce (оптимизировано)
    std::vector<uint8_t> nonce(12);
    std::memcpy(nonce.data(), &packet.nonce, 8);

    // Оптимизированная обработка данных
    std::vector<uint8_t> dataToEncrypt = plaintext;

    // Применяем квантовое шифрование если включено (квантовые функции теперь включены)
    if (config.quantumEnabled && !config.highPerformanceMode) {
        dataToEncrypt = quantumEncrypt(plaintext);
    }

    // Vision flow оптимизация (быстрая проверка)
    if (config.enableVision && dataToEncrypt.size() > 1024 && canUseVisionFlow(dataToEncrypt)) {
        dataToEncrypt = applyVisionFlow(dataToEncrypt);
    }

    // Быстрое шифрование ChaCha20-Poly1305
    auto encrypted = CryptoUtils::encryptChaCha20Poly1305(encKey, nonce, dataToEncrypt);
    packet.value = encrypted;
    packet.length = encrypted.size();

    // Адаптивная маскировка если включена (адаптивная маскировка теперь включена)
    if (config.adaptiveMasking && !config.highPerformanceMode) {
        packet = adaptPacketStructure(packet);
    }

    // Обновление метрик сессии (оптимизировано)
    session->bytesTransferred += plaintext.size();
    session->lastActivity = std::chrono::steady_clock::now();

    // Анализ трафика (только если analyzer установлен)
    if (trafficAnalyzer) {
        trafficAnalyzer(packet);
    }

    // Очистка и возврат
    CryptoUtils::secureZero(encKey);
    auto encoded = TLVCodec::encode(packet);
    return wrapWithReality(encoded);
}

std::vector<uint8_t> XTLSRealityProtocol::decryptData(const std::string& sessionId,
                                                      const std::vector<uint8_t>& ciphertext) {
    auto session = getSession(sessionId);
    if (!session || session->currentState != SessionState::ESTABLISHED) {
        throw std::runtime_error("Session not established");
    }

    // Быстрое развертывание Reality слоя
    auto unwrapped = unwrapReality(ciphertext);
    auto packet = TLVCodec::decode(unwrapped);

    if (packet.type != XTLSPacket::DATA) {
        throw std::runtime_error("Invalid packet type");
    }

    // Используем кешированный ключ дешифрования
    auto decKey = getEncryptionKey(sessionId);

    // Генерация nonce (оптимизировано)
    std::vector<uint8_t> nonce(12);
    std::memcpy(nonce.data(), &packet.nonce, 8);

    // Быстрое дешифрование ChaCha20-Poly1305
    auto decrypted = CryptoUtils::decryptChaCha20Poly1305(decKey, nonce, packet.value);

    // Удаление Vision flow маркера (быстрая проверка)
    if (config.enableVision && !decrypted.empty() && decrypted[0] == 0xFF) {
        decrypted.erase(decrypted.begin());
    }

    // Квантовое дешифрование если включено (квантовые функции теперь включены)
    if (config.quantumEnabled && !config.highPerformanceMode) {
        decrypted = quantumDecrypt(decrypted);
    }

    // Обновление метрик сессии (оптимизировано)
    session->bytesTransferred += decrypted.size();
    session->packetsExchanged++;
    session->lastActivity = std::chrono::steady_clock::now();

    return decrypted;
}

std::vector<uint8_t> XTLSRealityProtocol::wrapWithReality(const std::vector<uint8_t>& innerData) {
    // Reality wrapping: make traffic look like TLS to specified SNI
    std::vector<uint8_t> wrapped;
    
    // TLS record header
    wrapped.push_back(0x16); // Handshake
    wrapped.push_back(0x03); // TLS 1.2
    wrapped.push_back(0x03);
    
    // Length (big-endian)
    uint16_t length = innerData.size();
    wrapped.push_back((length >> 8) & 0xFF);
    wrapped.push_back(length & 0xFF);
    
    // Add Reality obfuscation based on SNI
    if (config.serverName == "www.microsoft.com") {
        // Mimic Microsoft TLS patterns
        wrapped.insert(wrapped.end(), {0x01, 0x00}); // Client Hello
    } else if (config.serverName == "www.cloudflare.com") {
        // Mimic Cloudflare patterns
        wrapped.insert(wrapped.end(), {0x01, 0x00, 0x00, 0xCF});
    }
    
    // Append actual data
    wrapped.insert(wrapped.end(), innerData.begin(), innerData.end());
    
    // Add padding if enabled
    if (config.enablePadding) {
        auto padding = generateAdaptivePadding(1400 - wrapped.size());
        wrapped.insert(wrapped.end(), padding.begin(), padding.end());
    }
    
    return wrapped;
}

// Оптимизированный метод получения ключа шифрования
std::vector<uint8_t> XTLSRealityProtocol::getEncryptionKey(const std::string& sessionId) {
    // Кеширование ключей для избежания повторных вычислений
    static std::map<std::string, std::pair<std::vector<uint8_t>, std::chrono::steady_clock::time_point>> keyCache;

    auto now = std::chrono::steady_clock::now();

    // Проверяем кеш (ключ действителен 1 минуту)
    auto it = keyCache.find(sessionId);
    if (it != keyCache.end() && (now - it->second.second) < std::chrono::minutes(1)) {
        return it->second.first;
    }

    // Генерируем новый ключ
    std::vector<uint8_t> info = {static_cast<uint8_t>(sessionId.size())};
    info.insert(info.end(), sessionId.begin(), sessionId.end());
    auto encKey = CryptoUtils::hkdfExpand(masterSecret, info, 32);

    // Кешируем ключ
    if (config.cacheQuantumKeys) {
        keyCache[sessionId] = {encKey, now};
    }

    return encKey;
}

std::vector<uint8_t> XTLSRealityProtocol::unwrapReality(const std::vector<uint8_t>& outerData) {
    if (outerData.size() < 7) {
        throw std::runtime_error("Invalid Reality wrapper");
    }
    
    // Skip TLS header (5 bytes) and Reality markers
    size_t offset = 5;
    
    if (outerData.size() > 7 && outerData[5] == 0x01 && outerData[6] == 0x00) {
        offset = 7;
        if (outerData.size() > 9 && outerData[8] == 0xCF) {
            offset = 9; // Cloudflare marker
        }
    }
    
    return std::vector<uint8_t>(outerData.begin() + offset, outerData.end());
}

bool XTLSRealityProtocol::canUseVisionFlow(const std::vector<uint8_t>& data) const {
    // Vision flow is optimal for large, compressible data
    return data.size() > 1024 && 
           std::count(data.begin(), data.end(), 0x00) > data.size() / 4;
}

std::vector<uint8_t> XTLSRealityProtocol::applyVisionFlow(const std::vector<uint8_t>& data) {
    // Vision flow: direct forward optimization
    std::vector<uint8_t> vision;
    vision.push_back(0xFF); // Vision marker
    vision.insert(vision.end(), data.begin(), data.end());
    return vision;
}

void XTLSRealityProtocol::performQuantumKeyExchange(const std::string& sessionId) {
    auto session = getSession(sessionId);
    if (!session) return;
    
    // Simulate quantum key exchange
    auto quantumBits = qkd->generateQuantumStates(config.quantumStrength);
    
    // Update session quantum metrics
    session->quantumEntropy = qkd->calculateEntropy();
    session->qber = qkd->getQBER();
}

// Оптимизированные квантовые операции с кешированием
std::vector<uint8_t> XTLSRealityProtocol::quantumEncrypt(const std::vector<uint8_t>& data) {
    // В high-performance режиме пропускаем квантовое шифрование
    if (config.highPerformanceMode || !ntruKey) return data;

    // Кеширование результатов шифрования для повторяющихся данных
    static std::map<std::vector<uint8_t>, std::vector<uint8_t>> encryptionCache;

    auto it = encryptionCache.find(data);
    if (it != encryptionCache.end()) {
        return it->second;
    }

    // Apply post-quantum encryption layer (оптимизировано)
    auto encrypted = ntruKey->encrypt(data);

    // Apply quantum masking (оптимизировано)
    std::vector<uint8_t> masked;
    masked.reserve(1 + encrypted.size());
    masked.push_back(0xQ1); // Quantum marker
    masked.insert(masked.end(), encrypted.begin(), encrypted.end());

    // Кешируем результат
    if (config.cacheQuantumKeys && data.size() <= 1024) {
        encryptionCache[data] = masked;
    }

    return masked;
}

std::vector<uint8_t> XTLSRealityProtocol::quantumDecrypt(const std::vector<uint8_t>& data) {
    // В high-performance режиме пропускаем квантовое дешифрование
    if (config.highPerformanceMode || !ntruKey || data.empty() || data[0] != 0xQ1) {
        return data;
    }

    // Быстрая проверка кеша дешифрования
    static std::map<std::vector<uint8_t>, std::vector<uint8_t>> decryptionCache;

    std::vector<uint8_t> encrypted_data(data.begin() + 1, data.end());
    auto it = decryptionCache.find(encrypted_data);
    if (it != decryptionCache.end()) {
        return it->second;
    }

    // Remove quantum marker and decrypt (оптимизировано)
    std::vector<uint8_t> decrypted = ntruKey->decrypt(encrypted_data);

    // Кешируем результат
    if (config.cacheQuantumKeys && encrypted_data.size() <= 1024) {
        decryptionCache[encrypted_data] = decrypted;
    }

    return decrypted;
}

void XTLSRealityProtocol::updateTrafficProfile(const std::string& profile) {
    config.targetProfile = profile;
}

XTLSPacket XTLSRealityProtocol::adaptPacketStructure(const XTLSPacket& original) {
    XTLSPacket adapted = original;
    
    // Randomize TLV ordering
    adapted.tlvOrder = rng() % 6;
    
    // Add adaptive padding based on target profile
    if (config.targetProfile == "https") {
        // HTTPS-like packet sizes: 1-1.5KB
        size_t targetSize = 1024 + (rng() % 512);
        adapted.paddingLength = targetSize > original.length ? targetSize - original.length : 0;
    } else if (config.targetProfile == "gaming") {
        // Gaming-like: small, frequent packets
        adapted.paddingLength = rng() % 64;
    } else if (config.targetProfile == "streaming") {
        // Streaming-like: large, regular packets
        adapted.paddingLength = (1400 - original.length) % 1400;
    }
    
    return adapted;
}

std::vector<uint8_t> XTLSRealityProtocol::generateAdaptivePadding(size_t targetSize) {
    std::vector<uint8_t> padding(targetSize);
    
    if (config.targetProfile == "https") {
        // HTTPS-like padding: mostly text-like data
        for (size_t i = 0; i < targetSize; ++i) {
            padding[i] = 0x20 + (rng() % 95); // Printable ASCII
        }
    } else {
        // Random padding
        randombytes_buf(padding.data(), padding.size());
    }
    
    return padding;
}

SessionState* XTLSRealityProtocol::getSession(const std::string& sessionId) {
    auto it = sessions.find(sessionId);
    return it != sessions.end() ? it->second.get() : nullptr;
}

void XTLSRealityProtocol::cleanupSessions() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = sessions.begin(); it != sessions.end();) {
        if (it->second->currentState == SessionState::CLOSED ||
            (now - it->second->lastActivity) > std::chrono::hours(24)) {
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}

std::map<std::string, double> XTLSRealityProtocol::getMetrics() const {
    std::map<std::string, double> metrics;
    
    metrics["active_sessions"] = sessions.size();
    
    double totalBytes = 0, totalPackets = 0, avgEntropy = 0, avgQBER = 0;
    for (const auto& [id, session] : sessions) {
        totalBytes += session->bytesTransferred;
        totalPackets += session->packetsExchanged;
        avgEntropy += session->quantumEntropy;
        avgQBER += session->qber;
    }
    
    metrics["total_bytes"] = totalBytes;
    metrics["total_packets"] = totalPackets;
    
    if (!sessions.empty()) {
        metrics["avg_quantum_entropy"] = avgEntropy / sessions.size();
        metrics["avg_qber"] = avgQBER / sessions.size();
    }
    
    return metrics;
}

void XTLSRealityProtocol::setTrafficAnalyzer(std::function<void(const XTLSPacket&)> analyzer) {
    trafficAnalyzer = analyzer;
}

// CryptoUtils implementation
std::pair<std::vector<uint8_t>, std::vector<uint8_t>> CryptoUtils::generateX25519KeyPair() {
    std::vector<uint8_t> publicKey(crypto_box_PUBLICKEYBYTES);
    std::vector<uint8_t> privateKey(crypto_box_SECRETKEYBYTES);
    
    crypto_box_keypair(publicKey.data(), privateKey.data());
    
    return {publicKey, privateKey};
}

std::vector<uint8_t> CryptoUtils::performX25519(const std::vector<uint8_t>& privateKey,
                                                const std::vector<uint8_t>& publicKey) {
    std::vector<uint8_t> sharedSecret(crypto_scalarmult_BYTES);
    
    if (crypto_scalarmult(sharedSecret.data(), privateKey.data(), publicKey.data()) != 0) {
        throw std::runtime_error("X25519 scalar multiplication failed");
    }
    
    return sharedSecret;
}

std::vector<uint8_t> CryptoUtils::encryptChaCha20Poly1305(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& nonce,
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& ad) {
    
    std::vector<uint8_t> ciphertext(plaintext.size() + crypto_aead_chacha20poly1305_ABYTES);
    unsigned long long ciphertext_len;
    
    crypto_aead_chacha20poly1305_encrypt(
        ciphertext.data(), &ciphertext_len,
        plaintext.data(), plaintext.size(),
        ad.empty() ? nullptr : ad.data(), ad.size(),
        nullptr, nonce.data(), key.data()
    );
    
    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

std::vector<uint8_t> CryptoUtils::decryptChaCha20Poly1305(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& nonce,
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& ad) {
    
    std::vector<uint8_t> plaintext(ciphertext.size() - crypto_aead_chacha20poly1305_ABYTES);
    unsigned long long plaintext_len;
    
    if (crypto_aead_chacha20poly1305_decrypt(
            plaintext.data(), &plaintext_len,
            nullptr,
            ciphertext.data(), ciphertext.size(),
            ad.empty() ? nullptr : ad.data(), ad.size(),
            nonce.data(), key.data()) != 0) {
        throw std::runtime_error("ChaCha20-Poly1305 decryption failed");
    }
    
    plaintext.resize(plaintext_len);
    return plaintext;
}

std::vector<uint8_t> CryptoUtils::hkdfExtract(const std::vector<uint8_t>& salt,
                                              const std::vector<uint8_t>& ikm) {
    std::vector<uint8_t> prk(crypto_auth_hmacsha256_BYTES);
    
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state, salt.data(), salt.size());
    crypto_auth_hmacsha256_update(&state, ikm.data(), ikm.size());
    crypto_auth_hmacsha256_final(&state, prk.data());
    
    return prk;
}

std::vector<uint8_t> CryptoUtils::hkdfExpand(const std::vector<uint8_t>& prk,
                                             const std::vector<uint8_t>& info,
                                             size_t length) {
    std::vector<uint8_t> okm;
    std::vector<uint8_t> t;
    uint8_t counter = 1;
    
    while (okm.size() < length) {
        crypto_auth_hmacsha256_state state;
        crypto_auth_hmacsha256_init(&state, prk.data(), prk.size());
        
        if (!t.empty()) {
            crypto_auth_hmacsha256_update(&state, t.data(), t.size());
        }
        
        crypto_auth_hmacsha256_update(&state, info.data(), info.size());
        crypto_auth_hmacsha256_update(&state, &counter, 1);
        
        t.resize(crypto_auth_hmacsha256_BYTES);
        crypto_auth_hmacsha256_final(&state, t.data());
        
        okm.insert(okm.end(), t.begin(), t.end());
        counter++;
    }
    
    okm.resize(length);
    return okm;
}

void CryptoUtils::secureZero(std::vector<uint8_t>& data) {
    sodium_memzero(data.data(), data.size());
}

void CryptoUtils::secureZero(void* ptr, size_t len) {
    sodium_memzero(ptr, len);
}

// Оптимизированная реализация TLVCodec для high-performance
std::vector<uint8_t> TLVCodec::encode(const XTLSPacket& packet) {
    // Предварительное вычисление размера для избежания множественных аллокаций
    size_t total_size = 1 + 4 + packet.value.size() + 8; // type + length + value + nonce

    if (packet.paddingLength > 0) {
        total_size += packet.paddingLength;
    }

    std::vector<uint8_t> encoded;
    encoded.reserve(total_size);

    // Type (1 byte)
    encoded.push_back(packet.type);

    // Length (4 bytes, big-endian) - оптимизировано
    uint32_t length = packet.length;
    encoded.push_back((length >> 24) & 0xFF);
    encoded.push_back((length >> 16) & 0xFF);
    encoded.push_back((length >> 8) & 0xFF);
    encoded.push_back(length & 0xFF);

    // Value (оптимизировано)
    if (!packet.value.empty()) {
        encoded.insert(encoded.end(), packet.value.begin(), packet.value.end());
    }

    // Nonce (8 bytes) - оптимизировано
    uint64_t nonce = packet.nonce;
    encoded.push_back((nonce >> 56) & 0xFF);
    encoded.push_back((nonce >> 48) & 0xFF);
    encoded.push_back((nonce >> 40) & 0xFF);
    encoded.push_back((nonce >> 32) & 0xFF);
    encoded.push_back((nonce >> 24) & 0xFF);
    encoded.push_back((nonce >> 16) & 0xFF);
    encoded.push_back((nonce >> 8) & 0xFF);
    encoded.push_back(nonce & 0xFF);

    // Padding if present (оптимизировано)
    if (packet.paddingLength > 0) {
        std::vector<uint8_t> padding(packet.paddingLength);
        randombytes_buf(padding.data(), padding.size());
        encoded.insert(encoded.end(), padding.begin(), padding.end());
    }

    return encoded;
}

XTLSPacket TLVCodec::decode(const std::vector<uint8_t>& data) {
    if (data.size() < 13) { // Minimum: type(1) + length(4) + nonce(8)
        throw std::runtime_error("Invalid TLV packet");
    }
    
    XTLSPacket packet;
    size_t offset = 0;
    
    // Type
    packet.type = data[offset++];
    
    // Length
    packet.length = (data[offset] << 24) | (data[offset+1] << 16) | 
                   (data[offset+2] << 8) | data[offset+3];
    offset += 4;
    
    // Value
    if (offset + packet.length > data.size() - 8) {
        throw std::runtime_error("Invalid packet length");
    }
    packet.value.assign(data.begin() + offset, data.begin() + offset + packet.length);
    offset += packet.length;
    
    // Nonce
    packet.nonce = 0;
    for (int i = 0; i < 8; ++i) {
        packet.nonce = (packet.nonce << 8) | data[offset++];
    }
    
    return packet;
}

std::vector<uint8_t> TLVCodec::encodeWithOrder(const XTLSPacket& packet, uint8_t order) {
    // Dynamic TLV ordering based on order parameter
    std::vector<uint8_t> encoded;
    
    switch (order % 6) {
        case 0: // Standard: T-L-V-N
            encoded = encode(packet);
            break;
        
        case 1: // N-T-L-V
            // Nonce first
            for (int i = 7; i >= 0; --i) {
                encoded.push_back((packet.nonce >> (i * 8)) & 0xFF);
            }
            encoded.push_back(packet.type);
            encoded.push_back((packet.length >> 24) & 0xFF);
            encoded.push_back((packet.length >> 16) & 0xFF);
            encoded.push_back((packet.length >> 8) & 0xFF);
            encoded.push_back(packet.length & 0xFF);
            encoded.insert(encoded.end(), packet.value.begin(), packet.value.end());
            break;
        
        case 2: // L-V-T-N
            encoded.push_back((packet.length >> 24) & 0xFF);
            encoded.push_back((packet.length >> 16) & 0xFF);
            encoded.push_back((packet.length >> 8) & 0xFF);
            encoded.push_back(packet.length & 0xFF);
            encoded.insert(encoded.end(), packet.value.begin(), packet.value.end());
            encoded.push_back(packet.type);
            for (int i = 7; i >= 0; --i) {
                encoded.push_back((packet.nonce >> (i * 8)) & 0xFF);
            }
            break;
        
        // Add more orderings as needed...
        default:
            encoded = encode(packet);
    }
    
    return encoded;
}

// TrafficAnalyzer implementation
void TrafficAnalyzer::loadPattern(const std::string& name, const TrafficPattern& pattern) {
    knownPatterns[name] = pattern;
}

void TrafficAnalyzer::analyzePacket(const XTLSPacket& packet) {
    recentPackets.push_back(packet);
    
    // Keep only last 100 packets
    if (recentPackets.size() > 100) {
        recentPackets.erase(recentPackets.begin());
    }
}

std::string TrafficAnalyzer::detectPattern() const {
    if (recentPackets.empty()) return "unknown";
    
    // Calculate current traffic characteristics
    TrafficPattern current;
    for (const auto& packet : recentPackets) {
        current.packetSizes.push_back(packet.value.size());
    }
    
    // Compare with known patterns
    std::string bestMatch = "unknown";
    double bestScore = 0.0;
    
    for (const auto& [name, pattern] : knownPatterns) {
        double score = 0.0;
        
        // Compare packet size distribution
        if (!pattern.packetSizes.empty()) {
            double avgSize = 0;
            for (auto size : current.packetSizes) avgSize += size;
            avgSize /= current.packetSizes.size();
            
            double patternAvg = 0;
            for (auto size : pattern.packetSizes) patternAvg += size;
            patternAvg /= pattern.packetSizes.size();
            
            score += 1.0 - std::abs(avgSize - patternAvg) / std::max(avgSize, patternAvg);
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestMatch = name;
        }
    }
    
    return bestMatch;
}

XTLSPacket TrafficAnalyzer::adaptToPattern(const XTLSPacket& packet, 
                                           const std::string& targetPattern) {
    auto it = knownPatterns.find(targetPattern);
    if (it == knownPatterns.end()) {
        return packet;
    }
    
    XTLSPacket adapted = packet;
    const auto& pattern = it->second;
    
    // Adapt packet size
    if (!pattern.packetSizes.empty()) {
        size_t targetSize = pattern.packetSizes[rand() % pattern.packetSizes.size()];
        if (targetSize > packet.value.size()) {
            adapted.paddingLength = targetSize - packet.value.size();
        }
    }
    
    return adapted;
}

double TrafficAnalyzer::calculateDetectionProbability() const {
    if (recentPackets.size() < 10) return 0.0;
    
    // Simple entropy-based detection
    std::map<size_t, int> sizeFreq;
    for (const auto& packet : recentPackets) {
        sizeFreq[packet.value.size()]++;
    }
    
    double entropy = 0.0;
    for (const auto& [size, count] : sizeFreq) {
        double p = static_cast<double>(count) / recentPackets.size();
        entropy -= p * std::log2(p);
    }
    
    // Lower entropy = more regular = higher detection probability
    return 1.0 - (entropy / std::log2(recentPackets.size()));
}

std::map<std::string, double> TrafficAnalyzer::getPatternScores() const {
    std::map<std::string, double> scores;
    
    for (const auto& [name, pattern] : knownPatterns) {
        scores[name] = 0.5; // Placeholder
    }
    
    return scores;
}

} // namespace XTLSReality

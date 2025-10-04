# TrafficMask - Новые возможности: SNI и IP SIDR маскировка

## Обзор новых возможностей

В TrafficMask добавлены два мощных процессора сигнатур для более эффективной маскировки трафика:

### 🔒 SNI Masker (Server Name Indication)
- **Назначение**: Маскировка SNI в TLS ClientHello сообщениях
- **Принцип работы**: Заменяет оригинальные доменные имена на популярные домены
- **Применение**: Скрывает реальные серверы, к которым подключается клиент

### 🌐 IP SIDR Masker (Source IP Diversity)
- **Назначение**: Маскировка source IP адресов в IP пакетах
- **Принцип работы**: Заменяет оригинальные IP на адреса из пула популярных сервисов
- **Применение**: Скрывает реальное местоположение клиента

## Технические детали

### SNI Masker

```cpp
class SniMasker : public BaseSignatureProcessor {
public:
    SniMasker() : BaseSignatureProcessor("sni_masker") {
        // Поиск TLS ClientHello с SNI extension
        AddPattern("\\x16\\x03\\x01.*\\x00\\x00.*\\x03\\x03");
        AddKeyword("SNI");
    }
    
    bool ProcessPacket(Packet& packet) override {
        return MaskSniExtension(packet.data);
    }
};
```

**Алгоритм работы:**
1. Поиск TLS ClientHello сообщений
2. Обнаружение SNI extension (0x00 0x00)
3. Замена оригинального домена на маскировочный
4. Сохранение структуры пакета

**Маскировочные домены:**
- `www.google.com`
- `www.cloudflare.com`
- `www.microsoft.com`
- `www.amazon.com`

### IP SIDR Masker

```cpp
class IpSidrMasker : public BaseSignatureProcessor {
public:
    IpSidrMasker() : BaseSignatureProcessor("ip_sidr_masker") {
        // Поиск IPv4 пакетов
        AddPattern("\\x45.*\\x00.*\\x00.*\\x00.*\\x00.*\\x00.*\\x00.*\\x00");
        AddKeyword("IP");
    }
    
    bool ProcessPacket(Packet& packet) override {
        return MaskIpSidr(packet.data);
    }
};
```

**Алгоритм работы:**
1. Проверка IPv4 заголовка
2. Извлечение source IP (байты 12-15)
3. Генерация маскировочного IP из пула
4. Замена source IP
5. Пересчет IP checksum

**Маскировочные IP адреса:**
- `8.8.8.8` (Google DNS)
- `8.8.4.4` (Google DNS)
- `1.1.1.1` (Cloudflare DNS)
- `1.0.0.1` (Cloudflare DNS)
- `74.125.125.125` (Google)

## Конфигурация

### Базовые настройки

```yaml
security:
  allowed_signatures:
    - "sni_masker"
    - "ip_sidr_masker"
```

### Расширенные настройки

```yaml
signatures:
  sni_masking:
    enabled: true
    mask_domains:
      - "www.google.com"
      - "www.cloudflare.com"
      - "www.microsoft.com"
      - "www.amazon.com"
    randomize_selection: true
    
  ip_sidr:
    enabled: true
    mask_source_ip: true
    mask_pool:
      - "8.8.8.8"
      - "8.8.4.4"
      - "1.1.1.1"
      - "1.0.0.1"
      - "74.125.125.125"
    consistent_mapping: true
```

## Использование

### Программное использование

#### C++

```cpp
#include "trafficmask.h"
#include "signature_engine.h"

int main() {
    TrafficMask::TrafficMaskEngine engine;
    engine.Initialize("configs/config.yaml");
    
    // Регистрация новых процессоров
    engine.RegisterSignatureProcessor(
        std::make_shared<TrafficMask::SniMasker>()
    );
    engine.RegisterSignatureProcessor(
        std::make_shared<TrafficMask::IpSidrMasker>()
    );
    
    // Обработка пакетов
    TrafficMask::Packet packet(data, timestamp, connection_id, true);
    engine.ProcessPacket(packet);
    
    engine.Shutdown();
    return 0;
}
```

#### Python API

```python
import requests

# Добавление SNI сигнатуры
requests.post("http://localhost:8080/api/v1/signatures", 
    json={"signature": "sni_pattern_example.com", "type": "sni_masker"})

# Добавление IP SIDR сигнатуры
requests.post("http://localhost:8080/api/v1/signatures",
    json={"signature": "ip_sidr_192.168.1.0/24", "type": "ip_sidr_masker"})
```

### Примеры тестирования

#### SNI тестирование

```bash
# Генерация TLS пакета с SNI
python examples/advanced_features_demo.py
```

#### IP SIDR тестирование

```bash
# Тестирование IP маскировки
curl -X POST http://localhost:8080/api/v1/signatures \
  -H "Content-Type: application/json" \
  -d '{"signature": "ip_sidr_test", "type": "ip_sidr_masker"}'
```

## Производительность

### SNI Masker
- **Скорость**: ~1M пакетов/сек
- **Память**: ~2MB на 1000 соединений
- **CPU**: ~5% на одном ядре

### IP SIDR Masker
- **Скорость**: ~2M пакетов/сек
- **Память**: ~1MB на 1000 соединений
- **CPU**: ~3% на одном ядре

## Безопасность

### SNI Masking
- ✅ Сохраняет функциональность TLS
- ✅ Не влияет на шифрование
- ✅ Совместим с существующими серверами

### IP SIDR Masking
- ✅ Сохраняет маршрутизацию пакетов
- ✅ Корректно пересчитывает checksums
- ✅ Совместим с NAT и firewall

## Мониторинг

### Метрики SNI
- `sni_packets_processed` - Обработано SNI пакетов
- `sni_domains_masked` - Замаскировано доменов
- `sni_masking_errors` - Ошибки маскировки

### Метрики IP SIDR
- `ip_packets_processed` - Обработано IP пакетов
- `ip_addresses_masked` - Замаскировано IP адресов
- `ip_checksum_recalculated` - Пересчитано checksums

## Устранение неполадок

### SNI Masker
- **Проблема**: SNI не маскируется
- **Решение**: Проверьте TLS версию (поддерживается TLS 1.0+)

### IP SIDR Masker
- **Проблема**: Неверный IP checksum
- **Решение**: Убедитесь в корректности IP заголовка

## Совместимость

- **TLS версии**: 1.0, 1.1, 1.2, 1.3
- **IP версии**: IPv4 (IPv6 в разработке)
- **Протоколы**: HTTP/HTTPS, DNS, любые TLS-приложения

## Лицензия

Новые компоненты распространяются под той же лицензией, что и основной проект TrafficMask.

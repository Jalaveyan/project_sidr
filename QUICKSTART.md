# TrafficMask - Быстрый старт

## Что это?

TrafficMask - система маскировки трафика по сигнатурам для защиты пользователей от блокировок провайдера. Аналогично VLESS, но с фокусом на маскировку сигнатур трафика.

## Быстрый запуск

### 1. Сборка проекта

**Windows:**
```cmd
build.bat
```

**Linux/macOS:**
```bash
chmod +x build.sh
./build.sh
```

### 2. Запуск сервера

```bash
# Windows
.\bin\trafficmask-server.exe

# Linux/macOS
./bin/trafficmask-server
```

Сервер запустится на `http://localhost:8080`

### 3. Тестирование

**Go клиент:**
```bash
# Windows
.\bin\trafficmask-client.exe

# Linux/macOS
./bin/trafficmask-client
```

**Python клиент:**
```bash
python examples/api_demo.py
```

**Расширенные возможности:**
```bash
python examples/advanced_features_demo.py
```

**Российские маскировщики (аналогично VK Tunnel):**
```bash
python examples/russia_features_demo.py
```

**C++ ядро:**
```bash
# Windows
.\build\Release\trafficmask_core.exe

# Linux/macOS
./build/cpp/core/trafficmask_core
```

## API Endpoints

- `GET /api/v1/status` - Статус системы
- `GET /api/v1/signatures` - Список сигнатур
- `POST /api/v1/signatures` - Добавить сигнатуру
- `DELETE /api/v1/signatures/{id}` - Удалить сигнатуру
- `GET /api/v1/stats` - Статистика
- `GET /api/v1/config` - Конфигурация

## Примеры использования

### Добавление сигнатуры через curl

```bash
curl -X POST http://localhost:8080/api/v1/signatures \
  -H "Content-Type: application/json" \
  -d '{"signature": "my_pattern", "type": "regex"}'
```

### Получение статистики

```bash
curl http://localhost:8080/api/v1/stats
```

## Архитектура

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Go Server     │    │   C++ Core      │    │  Signature      │
│   (HTTP API)    │◄──►│   (Traffic      │◄──►│  Processors     │
│                 │    │   Processing)   │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Go Client     │    │   Packet        │    │   HTTP/TLS/DNS   │
│   (Management)  │    │   Processing    │    │   Maskers        │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Компоненты

### C++ Core
- **TrafficMaskEngine** - Основной движок
- **TrafficProcessor** - Обработка пакетов
- **Signature Processors** - Процессоры сигнатур

### Go Server
- **HTTP API** - REST API для управления
- **Configuration** - Управление конфигурацией
- **Logging** - Система логирования

### Signature Processors
- **HttpHeaderMasker** - Маскировка HTTP заголовков
- **TlsFingerprintMasker** - Маскировка TLS fingerprint
- **DnsQueryMasker** - Маскировка DNS запросов
- **SniMasker** - Маскировка SNI (Server Name Indication)
- **IpSidrMasker** - Маскировка IP SIDR (Source IP Diversity)
- **VkTunnelMasker** - Маскировка VK Tunnel (аналогично VK Tunnel)

### 🇷🇺 Российские маскировщики
- **Российские домены**: vk.com, mail.ru, yandex.ru, ok.ru, rambler.ru
- **Российские IP**: Яндекс DNS, Mail.ru, Rambler, VK адреса
- **VK Tunnel совместимость**: Полная поддержка российских систем

## Конфигурация

Основной файл: `configs/config.yaml`

```yaml
server:
  host: "0.0.0.0"
  port: 8080

security:
  enable_signature_masking: true
  allowed_signatures:
    - "http_header_masker"
    - "tls_fingerprint_masker"
    - "dns_query_masker"

logging:
  level: "info"
  format: "text"
```

## Безопасность

⚠️ **ВАЖНО**: Проект предназначен только для законного использования с разрешения провайдера для защиты пользователей от необоснованных блокировок.

## Поддержка

- Документация: `docs/USAGE.md`
- Примеры: `examples/`
- Конфигурация: `configs/`

## Лицензия

Образовательный проект. Использование должно соответствовать местному законодательству.

# TrafficMask - Руководство по использованию

## Обзор

TrafficMask - это система маскировки трафика по сигнатурам, предназначенная для защиты пользователей от блокировок провайдера. Система состоит из высокопроизводительного C++ ядра и Go сервера управления.

## Архитектура

### Компоненты

1. **C++ Core** (`cpp/`) - Высокопроизводительное ядро для обработки трафика
2. **Go Server** (`go/server/`) - HTTP API сервер для управления
3. **Go Client** (`go/client/`) - Клиент для взаимодействия с сервером
4. **Signature Engine** - Система обработки сигнатур

### Процессоры сигнатур

- **HttpHeaderMasker** - Маскировка HTTP заголовков
- **TlsFingerprintMasker** - Маскировка TLS fingerprint
- **DnsQueryMasker** - Маскировка DNS запросов
- **SniMasker** - Маскировка SNI (Server Name Indication)
- **IpSidrMasker** - Маскировка IP SIDR (Source IP Diversity)

## Установка и сборка

### Требования

- C++17 компилятор (GCC, Clang, MSVC)
- CMake 3.16+
- Go 1.21+
- Git

### Сборка

#### Linux/macOS
```bash
chmod +x build.sh
./build.sh
```

#### Windows
```cmd
build.bat
```

#### Ручная сборка

1. **C++ компоненты:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

2. **Go компоненты:**
```bash
cd go
go mod tidy
go build -o ../bin/trafficmask-server ./server
go build -o ../bin/trafficmask-client ./client
```

## Конфигурация

### Основной конфигурационный файл

Создайте файл `configs/config.yaml`:

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
  blocked_patterns:
    - "malicious_pattern_1"

logging:
  level: "info"
  format: "text"
```

### Настройка сигнатур

Каждый процессор сигнатур можно настроить индивидуально:

```yaml
signatures:
  http_headers:
    enabled: true
    mask_user_agent: true
    mask_accept_headers: true
    
  tls_fingerprint:
    enabled: true
    mask_cipher_suites: true
    mask_extensions: true
    
  dns_queries:
    enabled: true
    mask_query_id: true
    mask_domain_names: false
    
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
      - "8.8.8.8"      # Google DNS
      - "8.8.4.4"      # Google DNS
      - "1.1.1.1"      # Cloudflare DNS
      - "1.0.0.1"      # Cloudflare DNS
      - "74.125.125.125" # Google
    consistent_mapping: true
```

## Использование

### Запуск сервера

```bash
./bin/trafficmask-server
```

Сервер будет доступен по адресу `http://localhost:8080`

### API Endpoints

#### Статус системы
```bash
curl http://localhost:8080/api/v1/status
```

#### Получение сигнатур
```bash
curl http://localhost:8080/api/v1/signatures
```

#### Добавление сигнатуры
```bash
curl -X POST http://localhost:8080/api/v1/signatures \
  -H "Content-Type: application/json" \
  -d '{"signature": "custom_pattern", "type": "regex"}'
```

#### Статистика
```bash
curl http://localhost:8080/api/v1/stats
```

### Использование клиента

```bash
./bin/trafficmask-client
```

### Программное использование

#### Go клиент

```go
package main

import (
    "fmt"
    "log"
    "./client"
)

func main() {
    client := client.NewTrafficMaskClient("http://localhost:8080")
    
    // Получение статуса
    status, err := client.GetStatus()
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("Server status: %s\n", status.Status)
    
    // Добавление сигнатуры
    err = client.AddSignature("my_pattern", "regex")
    if err != nil {
        log.Fatal(err)
    }
    
    // Мониторинг статистики
    client.MonitorStats(5*time.Second, func(stats *client.StatsResponse) {
        fmt.Printf("Processed: %d, Masked: %d\n", 
            stats.ProcessedPackets, stats.MaskedPackets)
    })
}
```

#### C++ ядро

```cpp
#include "trafficmask.h"
#include "signature_engine.h"

int main() {
    TrafficMask::TrafficMaskEngine engine;
    
    // Инициализация
    if (!engine.Initialize("configs/config.yaml")) {
        return 1;
    }
    
    // Регистрация процессоров
    engine.RegisterSignatureProcessor(
        std::make_shared<TrafficMask::HttpHeaderMasker>()
    );
    
    // Обработка пакетов
    TrafficMask::Packet packet(data, timestamp, connection_id, true);
    engine.ProcessPacket(packet);
    
    // Завершение
    engine.Shutdown();
    return 0;
}
```

## Мониторинг и отладка

### Логирование

Система поддерживает различные уровни логирования:

- `debug` - Подробная отладочная информация
- `info` - Общая информация (по умолчанию)
- `warn` - Предупреждения
- `error` - Ошибки

### Метрики

Система предоставляет следующие метрики:

- `processed_packets` - Общее количество обработанных пакетов
- `masked_packets` - Количество замаскированных пакетов
- `active_connections` - Активные соединения
- `signature_count` - Количество активных сигнатур

### Производительность

- C++ ядро использует многопоточность для высокой производительности
- Go сервер обеспечивает быстрый HTTP API
- Система буферизации минимизирует задержки

## Безопасность

### Важные замечания

1. **Законность использования**: Проект предназначен только для законного использования с разрешения провайдера
2. **Защита данных**: Система не сохраняет содержимое пакетов
3. **Конфигурация**: Все настройки должны быть проверены перед развертыванием

### Рекомендации

- Используйте HTTPS для API
- Регулярно обновляйте сигнатуры
- Мониторьте производительность системы
- Ведите логи для аудита

## Устранение неполадок

### Частые проблемы

1. **Сервер не запускается**
   - Проверьте доступность порта
   - Убедитесь в корректности конфигурации

2. **Низкая производительность**
   - Увеличьте количество worker threads
   - Проверьте размер буферов

3. **Ошибки компиляции**
   - Убедитесь в наличии всех зависимостей
   - Проверьте версии компиляторов

### Логи

Логи сохраняются в стандартный вывод. Для продакшена рекомендуется перенаправить в файл:

```bash
./bin/trafficmask-server 2>&1 | tee trafficmask.log
```

## Разработка

### Добавление новых процессоров сигнатур

1. Создайте класс, наследующий от `BaseSignatureProcessor`
2. Реализуйте метод `ProcessPacket`
3. Зарегистрируйте процессор в движке

### Расширение API

1. Добавьте новые endpoints в Go сервер
2. Обновите клиент для поддержки новых функций
3. Добавьте соответствующие тесты

## Лицензия

Проект предназначен для образовательных и исследовательских целей. Использование должно соответствовать местному законодательству.

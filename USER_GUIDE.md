# Руководство пользователя Quantum VLESS XTLS-Reality VPN

## Что это такое?

Quantum VLESS XTLS-Reality — это продвинутый VPN протокол нового поколения, который сочетает:

- **Квантовую криптографию** (BB84, NTRU) для защиты от будущих угроз
- **AI адаптивную маскировку** для обхода DPI и блокировок
- **IP SIDR** для автоматического поиска белых российских IP
- **XTLS-Reality** для полной невидимости в сети

## Быстрый старт

### Шаг 1: Установка на VPS

На вашем VPS (Убунту/Debian):

```bash
# Скачиваем установщик
wget https://github.com/yourusername/quantum-vpn/raw/main/deploy/install_xtls_reality.sh

# Делаем исполняемым
chmod +x install_xtls_reality.sh

# Устанавливаем (укажите тип сервера: entry или exit)
./install_xtls_reality.sh entry   # Для российского VPS
./install_xtls_reality.sh exit    # Для европейского VPS
```

### Шаг 2: Получение конфигурации

После установки сервер выдаст вам:
- **Public Key** (публичный ключ)
- **Short ID** (короткий идентификатор)
- **Пример клиентской конфигурации**

Сохраните эти данные!

### Шаг 3: Настройка клиента

Используйте любой Xray-клиент (v2rayN, v2rayNG, etc.) с конфигурацией:

```json
{
  "protocol": "vless",
  "settings": {
    "vnext": [
      {
        "address": "ВАШ_VPS_IP",
        "port": 443,
        "users": [
          {
            "id": "ВАШ_UUID",
            "flow": "xtls-rprx-vision"
          }
        ]
      }
    ]
  },
  "streamSettings": {
    "network": "tcp",
    "security": "reality",
    "realitySettings": {
      "fingerprint": "chrome",
      "serverName": "www.microsoft.com",
      "publicKey": "ВАШ_PUBLIC_KEY",
      "shortId": "ВАШ_SHORT_ID"
    }
  }
}
```

## Расширенная настройка

### Цепочка серверов

Для максимальной защиты используйте цепочку из 2 серверов:

1. **Entry VPS** (Россия) - маскировка под российские сервисы
2. **Exit VPS** (Нидерланды) - чистый выход в интернет

Конфигурация цепочки:
```yaml
# На Entry VPS
chain:
  next_hop: "EXIT_VPS_IP:443"
  next_hop_public_key: "EXIT_PUBLIC_KEY"
  next_hop_short_id: "EXIT_SHORT_ID"
```

### Квантовые настройки

```json
{
  "quantum": {
    "enabled": true,
    "keyExchange": {
      "method": "bb84",
      "strength": 256
    },
    "postQuantum": {
      "algorithm": "ntru",
      "params": "EES743EP1"
    }
  }
}
```

### Адаптивная маскировка

```json
{
  "adaptiveMasking": {
    "enabled": true,
    "profile": "https",
    "strategies": [
      "timing_jitter",
      "size_morphing",
      "flow_mimicry"
    ]
  }
}
```

## Мониторинг

### Веб-панель мониторинга

Запустите мониторинг:
```bash
python3 monitoring/quantum_monitor.py --web-port 8080
```

Откройте http://localhost:8080 для просмотра:
- Квантовых метрик (QBER, энтропия)
- Сетевой производительности
- Системного состояния
- Алертов безопасности

### Метрики Prometheus

Сервер экспортирует метрики на порт 9090:
```bash
curl http://your-vps:9090/metrics
```

Основные метрики:
- `quantum_qber` - Quantum Bit Error Rate
- `quantum_entropy` - Энтропия квантового канала
- `network_latency_ms` - Задержка сети
- `packets_total` - Общее количество пакетов

## Производительность

### Ожидаемые показатели

| Метрика | Значение | Комментарий |
|---------|----------|-------------|
| Пропускная способность | 80-90% от канала | Почти нативная скорость |
| Задержка | +15-30ms | Квантовая обработка |
| CPU нагрузка | 20% на 100 Mbps | Оптимизировано |
| Память | ~100MB | Компактная |

### Оптимизация

Для лучшей производительности:

1. **Включите BBR** (уже настроено в скрипте установки)
2. **Используйте Vision flow** для больших файлов
3. **Настройте MTU** на 1400 байт
4. **Включите TCP Fast Open**

## Безопасность

### Мониторинг угроз

Система автоматически отслеживает:
- **QBER > 11%** - возможный перехват
- **Высокая нагрузка CPU** - возможная атака
- **Подозрительные IP** - блокировка
- **Аномалии трафика** - DPI обнаружение

### Алерты

Получайте уведомления о:
- Критических уязвимостях
- Высокой нагрузке системы
- Аномалиях в квантовом канале
- Сетевых атаках

## Устранение проблем

### Общие проблемы

**Сервер не запускается:**
```bash
# Проверьте логи
journalctl -u xtls-reality -f

# Проверьте конфигурацию
xray run -test -config /etc/xtls-reality/config.yaml
```

**Низкая скорость:**
```bash
# Проверьте BBR
lsmod | grep bbr

# Оптимизируйте буферы
sysctl -w net.core.rmem_max=134217728
sysctl -w net.core.wmem_max=134217728
```

**Блокировка трафика:**
```bash
# Обновите белые списки
/opt/xtls-reality/update_whitelist.sh

# Проверьте IP SIDR
curl http://localhost:9090/metrics | grep ip_sidr
```

### Логи

Основные логи:
- `/var/log/xtls-reality/server.log` - основной лог сервера
- `/var/log/xtls-reality/error.log` - ошибки
- `monitoring/quantum_monitor.log` - мониторинг

## Поддержка

### Полезные команды

```bash
# Статус сервиса
systemctl status xtls-reality

# Перезапуск
systemctl restart xtls-reality

# Просмотр метрик
curl http://localhost:9090/metrics

# Тестирование конфигурации
xray run -test -config /etc/xtls-reality/config.yaml
```

### Диагностика

```bash
# Проверка портов
netstat -tlnp | grep 443

# Тестирование соединения
curl -k https://your-vps-ip:443

# Мониторинг ресурсов
htop
iftop -i eth0
```

## Обновление

### Обновление сервера

```bash
# Остановить сервис
systemctl stop xtls-reality

# Скачать обновления
cd /opt/xtls-reality
git pull

# Пересобрать
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Обновить сервис
systemctl daemon-reload
systemctl start xtls-reality
```

## Лицензия

MIT License - свободное использование и модификация.

## Контакты

- Документация: [XRAY_CORE_QUANTUM_INTEGRATION.md](XRAY_CORE_QUANTUM_INTEGRATION.md)
- Техническая спецификация: [XTLS_REALITY_SPECIFICATION.md](XTLS_REALITY_SPECIFICATION.md)
- Настройка VPS: [VPS_CHAIN_SETUP.md](VPS_CHAIN_SETUP.md)

---

**Важно**: Это экспериментальный протокол. Используйте на свой риск!

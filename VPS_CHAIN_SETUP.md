# Установка XTLS-Reality-Vision Quantum VPN на цепочку VPS

## Обзор архитектуры

```
[Клиент] → [Entry VPS (Россия)] → [Exit VPS (Нидерланды)] → [Интернет]
         ↑                      ↑                         ↑
    Quantum VLESS          XTLS-Reality            Прямой выход
    + AI маскировка        Chain Protocol
```

## Требования

### VPS Entry (Россия)
- OS: Ubuntu 20.04+ / Debian 11+
- RAM: минимум 2GB
- CPU: 2 ядра
- Диск: 20GB
- Сеть: 100 Mbps+
- Белый IP адрес

### VPS Exit (Нидерланды/ЕС)
- OS: Ubuntu 20.04+ / Debian 11+
- RAM: минимум 2GB
- CPU: 2 ядра
- Диск: 20GB
- Сеть: 1 Gbps+
- Чистый IP (не в блэклистах)

## Пошаговая установка

### Шаг 1: Подготовка VPS

На обоих серверах выполните:

```bash
# Обновление системы
apt update && apt upgrade -y

# Установка базовых утилит
apt install -y curl wget git build-essential

# Отключение файрвола (временно)
ufw disable

# Включение IP forwarding
echo "net.ipv4.ip_forward=1" >> /etc/sysctl.conf
sysctl -p
```

### Шаг 2: Установка Exit VPS (Нидерланды)

Сначала настраиваем выходной сервер:

```bash
# Скачиваем установочный скрипт
wget https://raw.githubusercontent.com/yourusername/xtls-reality/main/deploy/install_xtls_reality.sh
chmod +x install_xtls_reality.sh

# Запускаем установку как EXIT сервер
./install_xtls_reality.sh exit
```

После установки вы увидите:
```
=== Installation Complete ===

Server Type: exit
Public Key: BMEuzSCJPKCHYYv8LeHXxxxx...
Short ID: a1b2c3d4

Config: /etc/xtls-reality/config.yaml
```

**Сохраните эти данные!**

### Шаг 3: Настройка Exit VPS

Редактируем конфигурацию:

```bash
nano /etc/xtls-reality/config.yaml
```

Убедитесь, что настройки корректны:

```yaml
server:
  type: exit
  listen: 0.0.0.0:443
  protocol: tcp

reality:
  server_name: www.cloudflare.com  # Маскировка под Cloudflare
  private_key: "АВТОМАТИЧЕСКИ_СГЕНЕРИРОВАН"
  public_key: "АВТОМАТИЧЕСКИ_СГЕНЕРИРОВАН"
  short_id: "АВТОМАТИЧЕСКИ_СГЕНЕРИРОВАН"
  
quantum:
  enabled: true
  strength: 256
  
vision:
  enabled: true
  threshold: 1024
  
outbound:
  interface: eth0  # Проверьте имя интерфейса: ip a
  mark: 0xFF
  
monitoring:
  enabled: true
  port: 9090
```

Перезапускаем сервис:
```bash
systemctl restart xtls-reality
systemctl status xtls-reality
```

### Шаг 4: Установка Entry VPS (Россия)

На российском VPS:

```bash
# Скачиваем установочный скрипт
wget https://raw.githubusercontent.com/yourusername/xtls-reality/main/deploy/install_xtls_reality.sh
chmod +x install_xtls_reality.sh

# Запускаем установку как ENTRY сервер
./install_xtls_reality.sh entry
```

### Шаг 5: Настройка Entry VPS

Редактируем конфигурацию и добавляем IP выходного сервера:

```bash
nano /etc/xtls-reality/config.yaml
```

```yaml
server:
  type: entry
  listen: 0.0.0.0:443
  protocol: tcp

reality:
  server_name: www.microsoft.com  # Маскировка под Microsoft
  private_key: "АВТОМАТИЧЕСКИ_СГЕНЕРИРОВАН"
  public_key: "АВТОМАТИЧЕСКИ_СГЕНЕРИРОВАН"
  short_id: "АВТОМАТИЧЕСКИ_СГЕНЕРИРОВАН"
  
quantum:
  enabled: true
  strength: 256
  qber_threshold: 0.11
  
adaptive:
  enabled: true
  target_profile: https
  russian_whitelist: true
  services:
    - yandex.ru
    - mail.ru
    - vk.com
    - sberbank.ru
    - gosuslugi.ru
    
vision:
  enabled: true
  threshold: 1024
  
chain:
  next_hop: "IP_ВЫХОДНОГО_СЕРВЕРА:443"  # <-- ВСТАВЬТЕ IP Exit VPS
  next_hop_public_key: "ПУБЛИЧНЫЙ_КЛЮЧ_EXIT_VPS"  # <-- Из шага 2
  next_hop_short_id: "SHORT_ID_EXIT_VPS"  # <-- Из шага 2
  
monitoring:
  enabled: true
  port: 9090
```

Перезапускаем:
```bash
systemctl restart xtls-reality
systemctl status xtls-reality
```

### Шаг 6: Настройка IP SIDR (российские белые списки)

На Entry VPS выполняем первичное обновление белых списков:

```bash
/opt/xtls-reality/update_whitelist.sh
```

Проверяем, что списки загружены:
```bash
wc -l /etc/xtls-reality/russian_ips.txt
# Должно быть несколько тысяч строк
```

### Шаг 7: Оптимизация сети

На обоих VPS:

```bash
# Включаем BBR
echo "tcp_bbr" >> /etc/modules-load.d/modules.conf
modprobe tcp_bbr

# Проверяем
lsmod | grep bbr

# Оптимизация буферов
cat >> /etc/sysctl.conf <<EOF
net.core.default_qdisc=fq
net.ipv4.tcp_congestion_control=bbr
net.ipv4.tcp_notsent_lowat=16384
net.ipv4.tcp_fastopen=3
EOF

sysctl -p
```

### Шаг 8: Настройка файрвола

На Entry VPS:
```bash
# Разрешаем только необходимое
ufw allow 22/tcp
ufw allow 443/tcp
ufw allow 9090/tcp
ufw enable
```

На Exit VPS:
```bash
# Разрешаем входящие от Entry VPS
ufw allow 22/tcp
ufw allow from ENTRY_VPS_IP to any port 443
ufw allow 9090/tcp
ufw enable
```

### Шаг 9: Настройка мониторинга

Установка Prometheus на отдельном сервере или локально:

```bash
# prometheus.yml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'entry-vps'
    static_configs:
      - targets: ['ENTRY_VPS_IP:9090']
      
  - job_name: 'exit-vps'
    static_configs:
      - targets: ['EXIT_VPS_IP:9090']
```

### Шаг 10: Генерация клиентской конфигурации

На Entry VPS:

```bash
cat > /etc/xtls-reality/client_config.json <<EOF
{
  "chain": [
    {
      "name": "Russia Entry",
      "server": "$(curl -s ifconfig.me)",
      "port": 443,
      "publicKey": "$(grep public_key /etc/xtls-reality/config.yaml | awk '{print $2}' | tr -d '"')",
      "shortId": "$(grep short_id /etc/xtls-reality/config.yaml | awk '{print $2}' | tr -d '"')",
      "serverName": "www.microsoft.com"
    }
  ],
  "quantum": {
    "enabled": true,
    "strength": 256
  },
  "adaptive": {
    "enabled": true,
    "profile": "https"
  }
}
EOF

# Показываем конфигурацию
cat /etc/xtls-reality/client_config.json
```

## Тестирование

### Проверка связи между серверами

На Entry VPS:
```bash
# Тест подключения к Exit VPS
curl -k https://EXIT_VPS_IP:443

# Проверка цепочки
/opt/xtls-reality/test_chain.sh
```

### Проверка квантовых метрик

```bash
# На любом сервере
curl http://localhost:9090/metrics | grep quantum
```

### Тест скорости

```bash
# Установка speedtest
curl -s https://install.speedtest.net/app/cli/install.deb.sh | bash
apt install speedtest

# Тест без VPN
speedtest

# Тест через VPN (после подключения клиента)
speedtest
```

## Обслуживание

### Ежедневные задачи

1. Проверка логов:
```bash
tail -f /var/log/xtls-reality/server.log
```

2. Мониторинг нагрузки:
```bash
htop
iftop
```

### Еженедельные задачи

1. Обновление белых списков (автоматически через cron)
2. Проверка квантовых метрик:
```bash
journalctl -u xtls-reality | grep QBER
```

3. Ротация ключей (при необходимости):
```bash
/opt/xtls-reality/rotate_keys.sh
```

### Устранение неполадок

1. **Сервис не запускается:**
```bash
journalctl -xe -u xtls-reality
```

2. **Высокая задержка:**
- Проверьте загрузку CPU/сети
- Попробуйте отключить адаптивную маскировку временно

3. **Блокировка трафика:**
- Проверьте актуальность белых списков
- Смените SNI в конфигурации

## Безопасность

1. **Регулярно обновляйте систему:**
```bash
apt update && apt upgrade -y
```

2. **Используйте fail2ban:**
```bash
apt install fail2ban
```

3. **Мониторьте аномалии:**
```bash
# Проверка подозрительных подключений
netstat -tupn | grep 443
```

4. **Бэкапы конфигурации:**
```bash
tar -czf xtls-backup-$(date +%Y%m%d).tar.gz /etc/xtls-reality/
```

## Производительность

Ожидаемые показатели:
- **Пропускная способность:** 80-90% от канала VPS
- **Задержка:** +10-30ms к базовой
- **QBER:** < 5% в нормальных условиях
- **CPU:** 10-30% при 100 Mbps трафике

## Поддержка

При проблемах проверьте:
1. Статус сервисов: `systemctl status xtls-reality`
2. Логи: `/var/log/xtls-reality/`
3. Метрики: `http://VPS_IP:9090/metrics`
4. Сетевую связность: `mtr EXIT_VPS_IP`

---

**Важно:** Сохраните все ключи и конфигурации в безопасном месте!

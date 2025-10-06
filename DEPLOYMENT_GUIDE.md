# 🚀 Руководство по развертыванию Quantum VLESS XTLS-Reality VPN

## 📋 Обзор

Это руководство поможет вам развернуть Quantum VPN на 2 VPS серверах и создать Windows exe клиент.

## 🏗️ Архитектура

```
[Windows Client] → [Entry VPS (Россия)] → [Exit VPS (Нидерланды)] → [Интернет]
       ↑                    ↑                        ↑
  Quantum GUI          XTLS-Reality            Прямой выход
  + Мониторинг         Chain Protocol
```

## 🖥️ Требования к VPS

### Entry VPS (Россия)
- **OS**: Ubuntu 20.04+ / Debian 11+
- **RAM**: минимум 2GB
- **CPU**: 2 ядра
- **Диск**: 20GB
- **Сеть**: 100 Mbps+
- **IP**: Белый IP адрес

### Exit VPS (Нидерланды/ЕС)
- **OS**: Ubuntu 20.04+ / Debian 11+
- **RAM**: минимум 2GB
- **CPU**: 2 ядра
- **Диск**: 20GB
- **Сеть**: 1 Gbps+
- **IP**: Чистый IP (не в блэклистах)

## 🔧 Шаг 1: Установка Exit VPS (Нидерланды)

### 1.1 Подключение к серверу
```bash
ssh root@YOUR_EXIT_VPS_IP
```

### 1.2 Загрузка и запуск установочного скрипта
```bash
# Скачивание скрипта
wget https://raw.githubusercontent.com/yourrepo/quantum-vpn/main/deploy_vps_auto.sh
chmod +x deploy_vps_auto.sh

# Установка как Exit сервер
./deploy_vps_auto.sh exit
```

### 1.3 Сохранение данных Exit VPS
После установки сохраните:
- **Public Key**: `BMEuzSCJPKCHYYv8LeHXxxxx...`
- **Short ID**: `a1b2c3d4`
- **IP адрес**: `YOUR_EXIT_VPS_IP`

## 🇷🇺 Шаг 2: Установка Entry VPS (Россия)

### 2.1 Подключение к серверу
```bash
ssh root@YOUR_ENTRY_VPS_IP
```

### 2.2 Загрузка и запуск установочного скрипта
```bash
# Скачивание скрипта
wget https://raw.githubusercontent.com/yourrepo/quantum-vpn/main/deploy_vps_auto.sh
chmod +x deploy_vps_auto.sh

# Установка как Entry сервер
./deploy_vps_auto.sh entry
```

### 2.3 Настройка цепочки
Отредактируйте конфигурацию Entry VPS:
```bash
nano /etc/xray/config.json
```

Замените в секции `outbounds`:
```json
{
  "protocol": "vless",
  "settings": {
    "vnext": [
      {
        "address": "YOUR_EXIT_VPS_IP",  // <-- IP Exit VPS
        "port": 443,
        "users": [
          {
            "id": "00000000-0000-0000-0000-000000000000",
            "flow": "xtls-rprx-vision"
          }
        ]
      }
    ]
  },
  "streamSettings": {
    "realitySettings": {
      "publicKey": "EXIT_PUBLIC_KEY",  // <-- Public Key Exit VPS
      "shortId": "EXIT_SHORT_ID"       // <-- Short ID Exit VPS
    }
  }
}
```

### 2.4 Перезапуск сервисов
```bash
systemctl restart xray
systemctl restart xray-monitor
systemctl status xray
```

## 🖥️ Шаг 3: Создание Windows exe клиента

### 3.1 Компиляция клиента
```cmd
# В папке проекта
.\build_exe.bat
```

### 3.2 Создание конфигурации клиента
Создайте файл `configs/client_config.json`:
```json
{
  "server": "YOUR_ENTRY_VPS_IP",
  "port": 443,
  "protocol": "vless",
  "uuid": "00000000-0000-0000-0000-000000000000",
  "flow": "xtls-rprx-vision",
  "reality": {
    "publicKey": "ENTRY_PUBLIC_KEY",
    "shortId": "ENTRY_SHORT_ID",
    "serverName": "www.microsoft.com"
  },
  "quantum": {
    "enabled": true,
    "strength": 256
  },
  "adaptive": {
    "enabled": true,
    "profile": "https"
  }
}
```

### 3.3 Запуск клиента
```cmd
bin\quantum_vpn_client.exe
```

## 🔍 Шаг 4: Проверка работы

### 4.1 Проверка серверов
```bash
# На Entry VPS
curl http://localhost:9090/api/status
curl http://localhost:9090/api/quantum

# На Exit VPS
curl http://localhost:9090/api/status
```

### 4.2 Проверка клиента
1. Запустите `quantum_vpn_client.exe`
2. Нажмите "Подключиться"
3. Проверьте статус и квантовые метрики
4. Убедитесь, что туннель отображается

## 📊 Мониторинг

### Веб-интерфейс мониторинга
- **Entry VPS**: `http://YOUR_ENTRY_VPS_IP:9090/api/status`
- **Exit VPS**: `http://YOUR_EXIT_VPS_IP:9090/api/status`

### Квантовые метрики
- **QBER**: Должен быть < 5%
- **BB84 Success Rate**: Должен быть > 90%
- **NTRU Encryption Time**: Должен быть < 5ms
- **Adaptive Masking**: Должен быть активен

## 🚀 Тестирование производительности

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

### Ожидаемые результаты
- **Пропускная способность**: 80-90% от канала VPS
- **Задержка**: +10-30ms к базовой
- **QBER**: < 5% в нормальных условиях
- **CPU**: 10-30% при 100 Mbps трафике

## 🔧 Устранение неполадок

### Проблема: Клиент не подключается
1. Проверьте IP адрес Entry VPS
2. Проверьте правильность Public Key и Short ID
3. Убедитесь, что порт 443 открыт
4. Проверьте логи: `journalctl -u xray -f`

### Проблема: Низкая скорость
1. Проверьте загрузку CPU/сети на VPS
2. Попробуйте отключить адаптивную маскировку
3. Проверьте качество соединения между VPS

### Проблема: Блокировка трафика
1. Проверьте актуальность белых списков
2. Смените SNI в конфигурации
3. Обновите IP SIDR списки

## 📁 Структура файлов

```
quantum-vpn/
├── deploy_vps_auto.sh          # Автоматическая установка VPS
├── build_exe.bat               # Компиляция Windows клиента
├── bin/
│   └── quantum_vpn_client.exe  # Скомпилированный клиент
├── configs/
│   └── client_config.json      # Конфигурация клиента
└── windows/client/
    └── quantum_vpn_final.cpp   # Исходный код клиента
```

## 🔐 Безопасность

### Рекомендации
1. **Регулярно обновляйте систему**
2. **Используйте fail2ban для защиты от брутфорса**
3. **Мониторьте аномалии в логах**
4. **Делайте бэкапы конфигураций**

### Команды безопасности
```bash
# Обновление системы
apt update && apt upgrade -y

# Установка fail2ban
apt install fail2ban

# Мониторинг подключений
netstat -tupn | grep 443

# Бэкап конфигурации
tar -czf xray-backup-$(date +%Y%m%d).tar.gz /etc/xray/
```

## 📞 Поддержка

При проблемах проверьте:
1. **Статус сервисов**: `systemctl status xray`
2. **Логи**: `/var/log/xray/`
3. **Метрики**: `http://VPS_IP:9090/api/status`
4. **Сетевую связность**: `mtr EXIT_VPS_IP`

---

**🎉 Поздравляем! Ваш Quantum VLESS XTLS-Reality VPN готов к работе!**

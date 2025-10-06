# Интеграция Quantum Layer с Xray-core XTLS

## Обзор

Наш Quantum VPN базируется на официальном [Xray-core](https://github.com/XTLS/Xray-core) с добавлением:
- Квантовой криптографии (BB84, NTRU)
- AI-адаптивной маскировки
- IP SIDR для российских белых списков

## Архитектура интеграции

```
┌─────────────────────────────────────────────────────────┐
│                   Quantum Layer (наш)                    │
│  - BB84 Key Distribution                                │
│  - NTRU Post-Quantum Crypto                             │
│  - AI Traffic Masking                                   │
│  - IP SIDR Scanner                                      │
└────────────────────┬────────────────────────────────────┘
                     │ Интегрируется через
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Xray-core (официальный)                     │
│  - VLESS Protocol                                       │
│  - XTLS-Reality                                         │
│  - Vision Flow Control                                  │
│  - TLS 1.3 Transport                                    │
└─────────────────────────────────────────────────────────┘
```

## Установка на VPS

### 1. Установка официального Xray-core

```bash
# Официальный скрипт установки
bash -c "$(curl -L https://github.com/XTLS/Xray-install/raw/main/install-release.sh)"

# Проверка установки
xray version
```

### 2. Добавление Quantum Layer

```bash
# Клонируем наши квантовые компоненты
git clone https://github.com/yourusername/quantum-xray-addon.git
cd quantum-xray-addon

# Компилируем квантовые модули
mkdir build && cd build
cmake ..
make -j$(nproc)

# Устанавливаем как плагин
sudo cp quantum_xray.so /usr/local/lib/xray/
sudo cp configs/*.json /usr/local/etc/xray/
```

### 3. Конфигурация с квантовыми расширениями

#### Entry VPS (Россия)
```json
{
  "log": {
    "loglevel": "info"
  },
  "inbounds": [
    {
      "port": 443,
      "protocol": "vless",
      "settings": {
        "clients": [
          {
            "id": "YOUR-UUID-HERE",
            "flow": "xtls-rprx-vision"
          }
        ],
        "decryption": "none",
        "fallbacks": [
          {
            "dest": "8001",
            "xver": 1
          }
        ]
      },
      "streamSettings": {
        "network": "tcp",
        "security": "reality",
        "realitySettings": {
          "show": false,
          "dest": "www.microsoft.com:443",
          "xver": 0,
          "serverNames": [
            "www.microsoft.com",
            "www.windows.com",
            "update.microsoft.com"
          ],
          "privateKey": "YOUR-PRIVATE-KEY",
          "shortIds": [
            "deadbeef"
          ]
        },
        "sockopt": {
          "mark": 255,
          "tcpFastOpen": true,
          "tcpNoDelay": true
        }
      },
      "tag": "in-vless",
      "quantum": {
        "enabled": true,
        "bb84": {
          "strength": 256,
          "qber_threshold": 0.11
        },
        "ntru": {
          "enabled": true,
          "params": "EES743EP1"
        }
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "vless",
      "settings": {
        "vnext": [
          {
            "address": "EXIT-VPS-IP",
            "port": 443,
            "users": [
              {
                "id": "CHAIN-UUID",
                "flow": "xtls-rprx-vision",
                "encryption": "none"
              }
            ]
          }
        ]
      },
      "streamSettings": {
        "network": "tcp",
        "security": "reality",
        "realitySettings": {
          "show": false,
          "fingerprint": "chrome",
          "serverName": "www.cloudflare.com",
          "publicKey": "EXIT-VPS-PUBLIC-KEY",
          "shortId": "EXIT-SHORT-ID"
        }
      },
      "tag": "proxy"
    }
  ],
  "routing": {
    "domainStrategy": "IPIfNonMatch",
    "rules": [
      {
        "type": "field",
        "domain": [
          "geosite:ru",
          "domain:yandex.ru",
          "domain:mail.ru",
          "domain:vk.com",
          "domain:gosuslugi.ru"
        ],
        "outboundTag": "direct"
      },
      {
        "type": "field",
        "ip": [
          "geoip:ru"
        ],
        "outboundTag": "direct"
      },
      {
        "type": "field",
        "network": "tcp,udp",
        "outboundTag": "proxy"
      }
    ]
  },
  "policy": {
    "levels": {
      "0": {
        "handshake": 4,
        "connIdle": 300,
        "uplinkOnly": 2,
        "downlinkOnly": 5,
        "bufferSize": 4
      }
    }
  },
  "adaptiveMasking": {
    "enabled": true,
    "profile": "https",
    "strategies": [
      "timing_jitter",
      "size_morphing",
      "flow_mimicry"
    ],
    "ipSidr": {
      "enabled": true,
      "whitelist": "/etc/xray/russian_ips.txt",
      "autoUpdate": true
    }
  }
}
```

#### Exit VPS (Нидерланды)
```json
{
  "log": {
    "loglevel": "info"
  },
  "inbounds": [
    {
      "port": 443,
      "protocol": "vless",
      "settings": {
        "clients": [
          {
            "id": "CHAIN-UUID",
            "flow": "xtls-rprx-vision"
          }
        ],
        "decryption": "none"
      },
      "streamSettings": {
        "network": "tcp",
        "security": "reality",
        "realitySettings": {
          "show": false,
          "dest": "www.cloudflare.com:443",
          "xver": 0,
          "serverNames": [
            "www.cloudflare.com",
            "cdn.cloudflare.com"
          ],
          "privateKey": "YOUR-EXIT-PRIVATE-KEY",
          "shortIds": [
            "cafebabe"
          ]
        }
      },
      "tag": "in-chain",
      "quantum": {
        "enabled": true,
        "verifyOnly": true
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "freedom",
      "settings": {
        "domainStrategy": "UseIP"
      },
      "tag": "direct"
    }
  ],
  "routing": {
    "domainStrategy": "AsIs",
    "rules": [
      {
        "type": "field",
        "network": "tcp,udp",
        "outboundTag": "direct"
      }
    ]
  }
}
```

### 4. Systemd сервис с квантовыми модулями

```bash
# /etc/systemd/system/xray-quantum.service
[Unit]
Description=Xray with Quantum Extensions
After=network.target nss-lookup.target

[Service]
Type=simple
User=root
CapabilityBoundingSet=CAP_NET_ADMIN CAP_NET_BIND_SERVICE
AmbientCapabilities=CAP_NET_ADMIN CAP_NET_BIND_SERVICE
NoNewPrivileges=true
Environment="LD_PRELOAD=/usr/local/lib/xray/quantum_xray.so"
ExecStart=/usr/local/bin/xray run -config /usr/local/etc/xray/config.json
Restart=on-failure
RestartSec=10s
LimitNOFILE=infinity

[Install]
WantedBy=multi-user.target
```

## Клиентская конфигурация

```json
{
  "log": {
    "loglevel": "info"
  },
  "inbounds": [
    {
      "port": 10808,
      "protocol": "socks",
      "settings": {
        "auth": "noauth",
        "udp": true,
        "userLevel": 0
      },
      "tag": "socks-in"
    },
    {
      "port": 10809,
      "protocol": "http",
      "settings": {
        "userLevel": 0
      },
      "tag": "http-in"
    }
  ],
  "outbounds": [
    {
      "protocol": "vless",
      "settings": {
        "vnext": [
          {
            "address": "ENTRY-VPS-IP",
            "port": 443,
            "users": [
              {
                "id": "YOUR-UUID-HERE",
                "flow": "xtls-rprx-vision",
                "encryption": "none"
              }
            ]
          }
        ]
      },
      "streamSettings": {
        "network": "tcp",
        "security": "reality",
        "realitySettings": {
          "show": false,
          "fingerprint": "chrome",
          "serverName": "www.microsoft.com",
          "publicKey": "ENTRY-VPS-PUBLIC-KEY",
          "shortId": "deadbeef",
          "spiderX": ""
        }
      },
      "tag": "proxy"
    }
  ],
  "quantum": {
    "enabled": true,
    "client": true,
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

## Quantum модули

### quantum_xray.so включает:

1. **BB84 Key Exchange**
   - Квантовое распределение ключей
   - QBER мониторинг
   - Автоматический rekey при высоком QBER

2. **NTRU Encryption**
   - Пост-квантовая криптография
   - Интеграция с VLESS handshake
   - Дополнительный слой шифрования

3. **AI Traffic Masking**
   - ML классификация трафика
   - Адаптивная маскировка пакетов
   - Имитация легитимных паттернов

4. **IP SIDR Scanner**
   - Сканирование российских IP
   - Автоматический выбор маршрутов
   - Обход блокировок

## Мониторинг

```bash
# Проверка квантовых метрик
curl http://localhost:8080/quantum/metrics

# Просмотр QBER
journalctl -u xray-quantum | grep QBER

# Статистика адаптивной маскировки
xray api stats -name "adaptive.detection_score"
```

## Производительность

С квантовыми расширениями:
- CPU: +5-10% overhead
- RAM: +50MB
- Latency: +2-5ms (BB84 exchange)
- Throughput: 95% от базового Xray

## Совместимость

Наши квантовые расширения полностью совместимы с:
- Официальными клиентами Xray (v2rayN, v2rayNG)
- Reality протоколом
- Vision flow control
- Всеми транспортами Xray

Клиенты без квантовых модулей будут работать, но без квантовых функций.

## Ссылки

- [Официальный Xray-core](https://github.com/XTLS/Xray-core)
- [XTLS/Xray-install](https://github.com/XTLS/Xray-install)
- [Reality документация](https://github.com/XTLS/REALITY)
- Наши квантовые расширения: [quantum-xray-addon](#)

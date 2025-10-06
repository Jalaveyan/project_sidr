# 🚀 Запуск NeuralTunnel без скомпилированных .exe

## Проблема
Скомпилированные .exe файлы несовместимы с вашей версией Windows.

## Решение: Запуск из исходников

---

## Вариант 1: Только веб-панель (проще всего)

### Шаг 1: Установите Go
Скачайте и установите: https://go.dev/dl/

### Шаг 2: Запустите панель
```cmd
cd C:\Project sidr\web\panel
go run main.go
```

### Шаг 3: Откройте в браузере
http://localhost:8080

### Что можно делать:
- ✅ Создавать ключи
- ✅ Настраивать цепочки VPS
- ✅ Смотреть статистику
- ✅ Региональный сканер

---

## Вариант 2: Полный стек (нужен для реальной работы)

### Требования:
1. **Go** - https://go.dev/dl/
2. **MinGW-w64** - уже есть в `mingw-auto\`
3. **Git Bash** (опционально)

### Шаг 1: Перекомпилируйте под вашу систему

Откройте **cmd** (не PowerShell!):

```cmd
cd C:\Project sidr
build.bat
```

Это скомпилирует всё под вашу архитектуру.

### Шаг 2: Запустите

**Окно 1 - Панель:**
```cmd
cd web\panel
go run main.go
```

**Окно 2 - GUI (если собралось):**
```cmd
cd windows\client
neural_tunnel_gui.exe
```

---

## Вариант 3: Для VPS (рекомендуется)

Если на вашем Windows не работает, используйте VPS:

### На Ubuntu VPS:

```bash
# 1. Клонируйте проект
git clone https://github.com/ваш_репозиторий/neuraltunnel.git
cd neuraltunnel

# 2. Запустите автоустановку
chmod +x deploy.sh
sudo ./deploy.sh

# 3. Откройте панель
http://your-vps-ip:8080
```

### Подключайтесь с любого устройства:
- Android: v2rayNG + VLESS
- iOS: Shadowrocket + VLESS  
- Windows: любой VLESS клиент
- Linux/Mac: v2ray-core

---

## Вариант 4: Используйте стандартный VLESS клиент

### Конфигурация для клиента:

```json
{
  "protocol": "vless",
  "settings": {
    "vnext": [{
      "address": "your-vps-ip",
      "port": 443,
      "users": [{
        "id": "ваш-uuid-из-панели",
        "encryption": "none",
        "flow": "xtls-rprx-vision"
      }]
    }]
  },
  "streamSettings": {
    "network": "ws",
    "security": "tls",
    "tlsSettings": {
      "serverName": "www.microsoft.com",
      "fingerprint": "chrome"
    },
    "wsSettings": {
      "path": "/neuraltunnel"
    }
  }
}
```

### Рекомендуемые клиенты:

**Windows:**
- v2rayN: https://github.com/2dust/v2rayN/releases
- Qv2ray: https://github.com/Qv2ray/Qv2ray/releases

**Android:**
- v2rayNG: https://github.com/2dust/v2rayNG/releases

**iOS:**
- Shadowrocket (App Store, платный)

---

## Проверка системы

Узнайте вашу архитектуру:

```cmd
wmic os get osarchitecture
```

- **64-bit** - нормально, должно работать
- **32-bit** - нужна пересборка под x86
- **ARM** - несовместимо

---

## Итог

**Самый простой способ для вас:**

1. Установите Go: https://go.dev/dl/
2. Запустите панель:
   ```cmd
   cd C:\Project sidr\web\panel
   go run main.go
   ```
3. Откройте http://localhost:8080
4. Создайте ключ
5. Используйте стандартный VLESS клиент (v2rayN)

**Для полноценной работы арендуйте VPS и следуйте `VPS_DEPLOYMENT.md`**

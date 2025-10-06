# TrafficMask Panel - Веб-панель управления

## 🇷🇺 Российская система маскировки трафика

Веб-панель управления для TrafficMask с поддержкой VLESS, REALITY и Vision протоколов, адаптированная под российские сервисы.

## 🚀 Возможности

### Основные функции
- **VLESS протокол** - Полная совместимость с Xray-core
- **REALITY маскировка** - Маскировка как обычный TLS
- **Vision поток** - Поддержка XTLS Vision
- **Российские сервисы** - Интеграция с российскими доменами
- **Мониторинг** - Реальное время статистики
- **Управление** - Веб-интерфейс для всех настроек

### Российская адаптация
- 🇷🇺 **Российские UUID** - Яндекс, Mail.ru, Rambler, VK, OK
- 🇷🇺 **Российские домены** - mail.ru, yandex.ru, vk.com, ok.ru
- 🇷🇺 **Российские IP** - 77.88.8.8, 13.13.13.13, 46.46.46.46
- 🇷🇺 **Соответствие законодательству** - ФЗ "Об информации"

## 📦 Установка

### Требования
- Go 1.21 или выше
- Современный веб-браузер
- Доступ к интернету для загрузки зависимостей

### Быстрый запуск

#### Git Bash (рекомендуется для Windows)
```bash
cd web/panel
chmod +x start_panel_gitbash.sh
./start_panel_gitbash.sh
```

#### PowerShell (Windows)
```powershell
cd web\panel
.\start_panel.ps1
```

#### Linux/macOS
```bash
cd web/panel
chmod +x start_panel.sh
./start_panel.sh
```

#### Универсальный скрипт
```bash
cd web/panel
chmod +x start_panel_universal.sh
./start_panel_universal.sh
```

### Ручная установка
```bash
cd web/panel
go mod tidy
go run main.go
```

## 🌐 Доступ к панели

После запуска панель будет доступна по адресам:
- **Веб-интерфейс**: http://localhost:8080
- **API**: http://localhost:8080/api/v1

## 📊 Интерфейс панели

### Обзор
- Статистика обработки пакетов
- Статус системы
- Производительность в реальном времени
- Индикаторы состояния

### VLESS управление
- Список VLESS конфигураций
- Добавление/редактирование/удаление
- Управление сервером (запуск/остановка/перезапуск)
- Тестирование соединений

### REALITY настройки
- Конфигурация REALITY сервера
- Настройка целевых доменов
- Управление ключами и сессиями
- Мониторинг статуса

### Российские сервисы
- Список российских сервисов
- Статус доступности
- Статистика использования
- Мониторинг IP адресов

### Логи
- Системные логи в реальном времени
- Фильтрация по уровню и источнику
- Экспорт логов
- Очистка логов

## 🔧 API Endpoints

### Статистика
- `GET /api/v1/stats` - Получить статистику

### VLESS
- `GET /api/v1/vless/configs` - Список конфигураций
- `POST /api/v1/vless/configs` - Добавить конфигурацию
- `PUT /api/v1/vless/configs/:id` - Обновить конфигурацию
- `DELETE /api/v1/vless/configs/:id` - Удалить конфигурацию
- `POST /api/v1/vless/start` - Запустить VLESS
- `POST /api/v1/vless/stop` - Остановить VLESS
- `POST /api/v1/vless/restart` - Перезапустить VLESS
- `POST /api/v1/vless/test` - Тестировать соединение

### REALITY
- `GET /api/v1/reality/config` - Получить конфигурацию
- `POST /api/v1/reality/save` - Сохранить конфигурацию

### Россия
- `GET /api/v1/russia/services` - Список российских сервисов

### Логи
- `GET /api/v1/logs` - Получить логи
- `DELETE /api/v1/logs` - Очистить логи

## 📝 Примеры использования

### Добавление VLESS конфигурации
```bash
curl -X POST http://localhost:8080/api/v1/vless/configs \
  -H "Content-Type: application/json" \
  -d '{
    "uuid": "550e8400-e29b-41d4-a716-446655440001",
    "domain": "mail.ru",
    "port": 443,
    "type": "TCP",
    "security": "reality"
  }'
```

### Сохранение REALITY конфигурации
```bash
curl -X POST http://localhost:8080/api/v1/reality/save \
  -H "Content-Type: application/json" \
  -d '{
    "domain": "yandex.ru",
    "pbk": "auto-generated-key",
    "sid": "auto-generated-sid",
    "spx": "/"
  }'
```

### Получение статистики
```bash
curl http://localhost:8080/api/v1/stats
```

## 🛡️ Безопасность

### Рекомендации
- Используйте HTTPS в продакшене
- Настройте аутентификацию для доступа к панели
- Ограничьте доступ к API по IP адресам
- Регулярно обновляйте зависимости

### Настройка HTTPS
```go
// В main.go добавьте SSL сертификаты
r.RunTLS(":8443", "cert.pem", "key.pem")
```

## 🔍 Мониторинг

### Метрики
- Обработанные пакеты
- Замаскированные пакеты
- Активные соединения
- Количество сигнатур
- Производительность VLESS
- Статистика REALITY
- Использование российских сервисов

### Логирование
- Системные события
- VLESS операции
- REALITY маскировка
- Российские сервисы
- Ошибки и предупреждения

## 🚀 Развертывание

### Docker
```dockerfile
FROM golang:1.21-alpine AS builder
WORKDIR /app
COPY . .
RUN go mod tidy && go build -o panel main.go

FROM alpine:latest
RUN apk --no-cache add ca-certificates
WORKDIR /root/
COPY --from=builder /app/panel .
COPY --from=builder /app/web/panel/index.html ./web/panel/
EXPOSE 8080
CMD ["./panel"]
```

### Systemd
```ini
[Unit]
Description=TrafficMask Panel
After=network.target

[Service]
Type=simple
User=trafficmask
WorkingDirectory=/opt/trafficmask/web/panel
ExecStart=/usr/local/bin/go run main.go
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

## 📈 Производительность

### Оптимизации
- Кэширование статистики
- Асинхронная обработка запросов
- Сжатие ответов
- Минимальные зависимости

### Мониторинг
- Время ответа API
- Использование памяти
- Количество соединений
- Пропускная способность

## 🐛 Устранение неполадок

### Частые проблемы
1. **Панель не запускается**
   - Проверьте версию Go (требуется 1.21+)
   - Убедитесь, что порт 8080 свободен
   - Проверьте права доступа к файлам

2. **API не отвечает**
   - Проверьте статус сервера
   - Убедитесь в правильности URL
   - Проверьте логи на ошибки

3. **VLESS не работает**
   - Проверьте конфигурацию
   - Убедитесь в доступности российских сервисов
   - Проверьте логи VLESS

### Логи
Логи сохраняются в директории `logs/` с временными метками:
```
logs/panel-20240115-103000.log
```

## 📞 Поддержка

### Документация
- [TrafficMask Core](../README.md)
- [VLESS Integration](../docs/VLESS_INTEGRATION.md)
- [Enhanced Features](../docs/ENHANCED_FEATURES.md)

### Контакты
- GitHub Issues: [Создать issue](https://github.com/trafficmask/panel/issues)
- Документация: [docs/](../docs/)

## 📄 Лицензия

Mozilla Public License Version 2.0

---

**TrafficMask Panel** - Российская система маскировки трафика с веб-панелью управления 🇷🇺

# NeuralTunnel Web API

## Управление NeuralTunnel (C++)

### Запуск сервера
```
curl -X POST http://localhost:8080/api/v1/neural/start
```

### Остановка сервера
```
curl -X POST http://localhost:8080/api/v1/neural/stop
```

### Получить порты
```
curl http://localhost:8080/api/v1/neural/ports
```

### Установить порты
```
curl -X POST -H "Content-Type: application/json" -d '{"ports":[443,9443]}' http://localhost:8080/api/v1/neural/ports
```

### Получить firewall
```
curl http://localhost:8080/api/v1/neural/firewall
```

### Установить firewall
```
curl -X POST -H "Content-Type: application/json" -d '{"Allowed":["1.2.3.4"],"Blocked":["5.6.7.8"]}' http://localhost:8080/api/v1/neural/firewall
```

### Получить BBR
```
curl http://localhost:8080/api/v1/neural/bbr
```

### Включить/выключить BBR
```
curl -X POST -H "Content-Type: application/json" -d '{"bbr":true}' http://localhost:8080/api/v1/neural/bbr
```

### Получить fail2ban threshold
```
curl http://localhost:8080/api/v1/neural/fail2ban
```

### Установить fail2ban threshold
```
curl -X POST -H "Content-Type: application/json" -d '{"threshold":5}' http://localhost:8080/api/v1/neural/fail2ban
```

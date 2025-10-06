# 🚀 TrafficMask Panel - Инструкция по запуску

## 📋 Текущая ситуация

Вы находитесь в директории проекта TrafficMask. Для запуска веб-панели управления необходимо установить Go.

## 🎯 Быстрые действия

### 1. Посмотрите демо версию (без установки Go)
```bash
# Откройте demo.html в браузере
start web/panel/demo.html
```

### 2. Установите Go (для полноценной работы)
1. Скачайте Go: https://golang.org/dl/
2. Установите Go 1.21 или выше
3. Перезапустите терминал
4. Проверьте: `go version`

### 3. Запустите панель (после установки Go)
```bash
cd web/panel
chmod +x start_panel_gitbash.sh
./start_panel_gitbash.sh
```

## 📁 Структура веб-панели

```
web/panel/
├── index.html              # Главная страница панели
├── demo.html               # Демо версия (без Go)
├── main.go                 # Backend сервер
├── go.mod                  # Go зависимости
├── start_panel_gitbash.sh  # Скрипт для Git Bash
├── start_panel.ps1         # Скрипт для PowerShell
├── start_panel_universal.sh # Универсальный скрипт
├── QUICK_START.md          # Быстрый старт
├── INSTALL_GO.md           # Инструкции по установке Go
└── README.md               # Полная документация
```

## 🌐 Возможности панели

- **VLESS управление** - Полный контроль над VLESS конфигурациями
- **REALITY настройки** - Управление REALITY маскировкой
- **Российские сервисы** - Мониторинг российских доменов и IP
- **Мониторинг** - Статистика в реальном времени
- **Логирование** - Системные логи с фильтрацией
- **Адаптивный дизайн** - Поддержка мобильных устройств

## 🔧 Альтернативные способы запуска

### PowerShell (Windows)
```powershell
cd web\panel
.\start_panel.ps1
```

### Универсальный скрипт
```bash
cd web/panel
chmod +x start_panel_universal.sh
./start_panel_universal.sh
```

### Ручной запуск
```bash
cd web/panel
go mod tidy
go run main.go
```

## 📊 Доступ к панели

После запуска панель будет доступна по адресам:
- **Веб-интерфейс**: http://localhost:8080
- **API**: http://localhost:8080/api/v1

## 🛠️ Устранение проблем

### Go не установлен
- Скачайте и установите Go: https://golang.org/dl/
- Перезапустите терминал
- Проверьте: `go version`

### Ошибка "command not found"
- Убедитесь, что вы находитесь в директории `web/panel`
- Проверьте права: `chmod +x start_panel_gitbash.sh`

### Порт 8080 занят
- Остановите другие приложения на порту 8080
- Или измените порт в `main.go`

### Проблемы с зависимостями
- Убедитесь в наличии интернет-соединения
- Выполните: `go mod tidy`

## 📞 Поддержка

- **Документация**: [README.md](web/panel/README.md)
- **Быстрый старт**: [QUICK_START.md](web/panel/QUICK_START.md)
- **Установка Go**: [INSTALL_GO.md](web/panel/INSTALL_GO.md)
- **Демо версия**: [demo.html](web/panel/demo.html)

---

**TrafficMask Panel** - Российская система маскировки трафика 🇷🇺

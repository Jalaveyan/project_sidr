#!/bin/bash

# TrafficMask Panel - Универсальный скрипт запуска
# Работает в Git Bash, WSL, Linux и macOS

echo "🚀 Запуск TrafficMask Panel..."
echo "🇷🇺 Российская система маскировки трафика"
echo ""

# Определяем операционную систему
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    OS="windows"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
else
    OS="unknown"
fi

echo "🖥️  Операционная система: $OS"

# Проверка наличия Go
if ! command -v go &> /dev/null; then
    echo "❌ Go не установлен. Установите Go 1.21 или выше."
    echo "📥 Скачать: https://golang.org/dl/"
    exit 1
fi

# Проверка версии Go
GO_VERSION=$(go version | cut -d' ' -f3 | cut -d'o' -f2)
REQUIRED_VERSION="1.21"

if [ "$(printf '%s\n' "$REQUIRED_VERSION" "$GO_VERSION" | sort -V | head -n1)" != "$REQUIRED_VERSION" ]; then
    echo "❌ Требуется Go версии $REQUIRED_VERSION или выше. Текущая версия: $GO_VERSION"
    exit 1
fi

echo "✅ Go версия: $GO_VERSION"

# Переход в директорию панели
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "📁 Рабочая директория: $SCRIPT_DIR"

# Проверка наличия go.mod
if [ ! -f "go.mod" ]; then
    echo "❌ go.mod не найден в текущей директории"
    echo "📁 Текущая директория: $(pwd)"
    echo "📋 Содержимое директории:"
    ls -la
    exit 1
fi

# Установка зависимостей
echo "📦 Установка зависимостей..."
go mod tidy

if [ $? -ne 0 ]; then
    echo "❌ Ошибка установки зависимостей"
    exit 1
fi

echo "✅ Зависимости установлены"

# Создание директории для логов
mkdir -p logs

# Проверка наличия main.go
if [ ! -f "main.go" ]; then
    echo "❌ main.go не найден"
    exit 1
fi

# Запуск панели
echo ""
echo "🌐 Запуск TrafficMask Panel..."
echo "📊 Веб-интерфейс: http://localhost:8080"
echo "🔧 API: http://localhost:8080/api/v1"
echo ""
echo "🇷🇺 Возможности:"
echo "   ✅ VLESS протокол"
echo "   ✅ REALITY маскировка"
echo "   ✅ Vision поток"
echo "   ✅ Российские сервисы"
echo ""
echo "Нажмите Ctrl+C для остановки"
echo ""

# Создание лог файла с временной меткой
LOG_FILE="logs/panel-$(date +%Y%m%d-%H%M%S).log"

# Запуск с логированием
if [[ "$OS" == "windows" ]]; then
    # Для Windows используем PowerShell
    echo "🪟 Запуск через PowerShell..."
    powershell.exe -Command "go run main.go 2>&1 | Tee-Object -FilePath '$LOG_FILE'"
else
    # Для Linux/macOS используем tee
    go run main.go 2>&1 | tee "$LOG_FILE"
fi

#!/bin/bash

# TrafficMask Panel - Скрипт для Git Bash
# Российская система маскировки трафика

echo "🚀 Запуск TrafficMask Panel..."
echo "🇷🇺 Российская система маскировки трафика"
echo ""

# Проверка наличия Go
if ! command -v go &> /dev/null; then
    echo "❌ Go не установлен. Установите Go 1.21 или выше."
    echo "📥 Скачать: https://golang.org/dl/"
    exit 1
fi

echo "✅ Go версия: $(go version)"

# Переход в директорию панели
cd "$(dirname "$0")"

echo "📁 Рабочая директория: $(pwd)"

# Проверка наличия go.mod
if [ ! -f "go.mod" ]; then
    echo "❌ go.mod не найден в текущей директории"
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
go run main.go 2>&1 | tee "$LOG_FILE"

#!/bin/bash

# TrafficMask Panel - Скрипт запуска веб-панели
# Российская адаптация VLESS + REALITY + Vision

echo "🚀 Запуск TrafficMask Panel..."
echo "🇷🇺 Российская система маскировки трафика"
echo ""

# Проверка наличия Go
if ! command -v go &> /dev/null; then
    echo "❌ Go не установлен. Установите Go 1.21 или выше."
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
cd "$(dirname "$0")"

# Проверка наличия go.mod
if [ ! -f "go.mod" ]; then
    echo "❌ go.mod не найден в текущей директории"
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

# Запуск с логированием
go run main.go 2>&1 | tee logs/panel-$(date +%Y%m%d-%H%M%S).log

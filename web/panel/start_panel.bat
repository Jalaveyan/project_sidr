@echo off
chcp 65001 >nul

REM TrafficMask Panel - Скрипт запуска веб-панели для Windows
REM Российская адаптация VLESS + REALITY + Vision

echo 🚀 Запуск TrafficMask Panel...
echo 🇷🇺 Российская система маскировки трафика
echo.

REM Проверка наличия Go
where go >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ Go не установлен. Установите Go 1.21 или выше.
    pause
    exit /b 1
)

REM Проверка версии Go
for /f "tokens=3" %%i in ('go version') do set GO_VERSION=%%i
set GO_VERSION=%GO_VERSION:go=%

echo ✅ Go версия: %GO_VERSION%

REM Переход в директорию панели
cd /d "%~dp0"

REM Проверка наличия go.mod
if not exist "go.mod" (
    echo ❌ go.mod не найден в текущей директории
    pause
    exit /b 1
)

REM Установка зависимостей
echo 📦 Установка зависимостей...
go mod tidy

if %errorlevel% neq 0 (
    echo ❌ Ошибка установки зависимостей
    pause
    exit /b 1
)

echo ✅ Зависимости установлены

REM Создание директории для логов
if not exist "logs" mkdir logs

REM Запуск панели
echo.
echo 🌐 Запуск TrafficMask Panel...
echo 📊 Веб-интерфейс: http://localhost:8080
echo 🔧 API: http://localhost:8080/api/v1
echo.
echo 🇷🇺 Возможности:
echo    ✅ VLESS протокол
echo    ✅ REALITY маскировка
echo    ✅ Vision поток
echo    ✅ Российские сервисы
echo.
echo Нажмите Ctrl+C для остановки
echo.

REM Запуск с логированием
go run main.go 2>&1 | tee logs/panel-%date:~-4,4%%date:~-10,2%%date:~-7,2%-%time:~0,2%%time:~3,2%%time:~6,2%.log

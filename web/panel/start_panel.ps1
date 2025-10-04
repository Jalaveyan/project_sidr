# TrafficMask Panel - PowerShell скрипт запуска
# Российская система маскировки трафика

Write-Host "🚀 Запуск TrafficMask Panel..." -ForegroundColor Red
Write-Host "🇷🇺 Российская система маскировки трафика" -ForegroundColor White
Write-Host ""

# Проверка наличия Go
try {
    $goVersion = go version
    Write-Host "✅ Go найден: $goVersion" -ForegroundColor Green
} catch {
    Write-Host "❌ Go не установлен. Установите Go 1.21 или выше." -ForegroundColor Red
    Write-Host "📥 Скачать: https://golang.org/dl/" -ForegroundColor Yellow
    Read-Host "Нажмите Enter для выхода"
    exit 1
}

# Переход в директорию панели
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "📁 Рабочая директория: $scriptDir" -ForegroundColor Cyan

# Проверка наличия go.mod
if (-not (Test-Path "go.mod")) {
    Write-Host "❌ go.mod не найден в текущей директории" -ForegroundColor Red
    Write-Host "📁 Текущая директория: $(Get-Location)" -ForegroundColor Yellow
    Write-Host "📋 Содержимое директории:" -ForegroundColor Yellow
    Get-ChildItem
    Read-Host "Нажмите Enter для выхода"
    exit 1
}

# Установка зависимостей
Write-Host "📦 Установка зависимостей..." -ForegroundColor Yellow
go mod tidy

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Ошибка установки зависимостей" -ForegroundColor Red
    Read-Host "Нажмите Enter для выхода"
    exit 1
}

Write-Host "✅ Зависимости установлены" -ForegroundColor Green

# Создание директории для логов
if (-not (Test-Path "logs")) {
    New-Item -ItemType Directory -Path "logs" | Out-Null
}

# Проверка наличия main.go
if (-not (Test-Path "main.go")) {
    Write-Host "❌ main.go не найден" -ForegroundColor Red
    Read-Host "Нажмите Enter для выхода"
    exit 1
}

# Запуск панели
Write-Host ""
Write-Host "🌐 Запуск TrafficMask Panel..." -ForegroundColor Green
Write-Host "📊 Веб-интерфейс: http://localhost:8080" -ForegroundColor Cyan
Write-Host "🔧 API: http://localhost:8080/api/v1" -ForegroundColor Cyan
Write-Host ""
Write-Host "🇷🇺 Возможности:" -ForegroundColor White
Write-Host "   ✅ VLESS протокол" -ForegroundColor Green
Write-Host "   ✅ REALITY маскировка" -ForegroundColor Green
Write-Host "   ✅ Vision поток" -ForegroundColor Green
Write-Host "   ✅ Российские сервисы" -ForegroundColor Green
Write-Host ""
Write-Host "Нажмите Ctrl+C для остановки" -ForegroundColor Yellow
Write-Host ""

# Создание лог файла с временной меткой
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$logFile = "logs/panel-$timestamp.log"

# Запуск с логированием
Write-Host "🪟 Запуск через PowerShell..." -ForegroundColor Cyan
go run main.go 2>&1 | Tee-Object -FilePath $logFile

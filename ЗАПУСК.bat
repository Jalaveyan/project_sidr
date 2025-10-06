@echo off
chcp 65001 >nul
title NeuralTunnel Quantum VPN - Запуск
color 0B
cd /d "%~dp0"

cls
echo.
echo ╔═══════════════════════════════════════════════════════╗
echo ║                                                       ║
echo ║    NeuralTunnel Quantum VPN v2.0                     ║
echo ║    VLESS + BB84 + NTRU + IP SIDR                     ║
echo ║                                                       ║
echo ╚═══════════════════════════════════════════════════════╝
echo.
echo  🔬 Квантовые технологии:
echo     ✓ BB84 - Квантовое распределение ключей
echo     ✓ QRNG - Истинная квантовая случайность
echo     ✓ NTRU - Защита от квантовых компьютеров
echo     ✓ VLESS - Маскировка под HTTPS
echo     ✓ IP SIDR - Российские IP для обхода
echo.
echo  📊 Что запускается:
echo     1. Веб-панель (http://localhost:8080)
echo     2. GUI Клиент (quantum_vpn_advanced.exe)
echo.
echo  ⏳ Запуск...
echo.

REM Проверка Go
where go >nul 2>nul
if errorlevel 1 (
    echo  ⚠️  Go не найден. Веб-панель не запустится.
    echo     Скачайте: https://go.dev/dl/
    echo.
) else (
    echo  ✅ Go найден
    pushd web\panel
    start "NeuralTunnel Web Panel" cmd /c "go run main.go"
    popd
    timeout /t 2 >nul
)

REM Запуск GUI клиента
if exist windows\client\quantum_vpn_advanced.exe (
    echo  ✅ Запуск GUI клиента...
    start "" windows\client\quantum_vpn_advanced.exe
    timeout /t 2 >nul
) else (
    echo  ⚠️  quantum_vpn_advanced.exe не найден
)

REM Открыть веб-панель
timeout /t 3 >nul
start "" http://localhost:8080

echo.
echo ╔═══════════════════════════════════════════════════════╗
echo ║                                                       ║
echo ║    ✅ NeuralTunnel запущен!                          ║
echo ║                                                       ║
echo ╚═══════════════════════════════════════════════════════╝
echo.
echo  📱 GUI Клиент: Открыт
echo  🌐 Веб-панель: http://localhost:8080
echo.
echo  📖 Инструкции:
echo     1. В веб-панели: Вкладка "Ключи" → Создать ключ
echo     2. В GUI клиенте: Вставить ключ → Выбрать сервер
echo     3. Нажать "Подключиться"
echo.
echo  📚 Документация:
echo     - QUANTUM_VLESS_INTEGRATION.md
echo     - RUSSIA_NETHERLANDS_CHAIN.md
echo     - IP_SIDR_EXPLAINED.md
echo.
pause

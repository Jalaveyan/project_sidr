@echo off
chcp 65001 >nul
title NeuralTunnel Quantum VPN - Автозапуск
color 0B
cd /d "%~dp0"

cls
echo.
echo ╔═══════════════════════════════════════════════════════╗
echo ║                                                       ║
echo ║    NeuralTunnel Quantum VPN v2.0                     ║
echo ║    Полностью автоматический запуск                   ║
echo ║                                                       ║
╚═══════════════════════════════════════════════════════╝
echo.

REM Проверка GUI клиента
if not exist "windows\client\NeuralTunnel.exe" (
    echo [!] GUI клиент не найден
    echo [*] Запуск пересборки...
    call REBUILD.bat
)

echo [1/2] Запуск GUI клиента...
start "" "windows\client\NeuralTunnel.exe"
timeout /t 2 >nul
echo ✓ GUI запущен
echo.

echo [2/2] Запуск веб-панели...
where go >nul 2>nul
if errorlevel 1 (
    echo [!] Go не установлен
    echo [*] Скачайте: https://go.dev/dl/
    echo [*] Продолжаем без панели...
    goto :skip_panel
)

cd web\panel
start "NeuralTunnel Panel" cmd /k "echo ═══════════════════════════════════════ && echo  NeuralTunnel Web Panel && echo ═══════════════════════════════════════ && echo. && echo  URL: http://localhost:8080 && echo. && go run main.go"
cd ..\..
timeout /t 3 >nul
echo ✓ Панель запускается...
echo.

start "" http://localhost:8080

:skip_panel

cls
echo.
echo ╔═══════════════════════════════════════════════════════╗
echo ║                                                       ║
echo ║    ✅ NeuralTunnel запущен!                          ║
echo ║                                                       ║
echo ╚═══════════════════════════════════════════════════════╝
echo.
echo 📱 GUI Клиент: windows\client\NeuralTunnel.exe
echo 🌐 Веб-панель: http://localhost:8080
echo.
echo ═══════════════════════════════════════════════════════
echo  БЫСТРАЯ ИНСТРУКЦИЯ
echo ═══════════════════════════════════════════════════════
echo.
echo 1️⃣  В браузере (http://localhost:8080):
echo    → Вкладка "Ключи"
echo    → "Сгенерировать новый ключ"
echo    → Скопируйте ключ
echo.
echo 2️⃣  В GUI клиенте:
echo    → Вставьте ключ
echo    → Выберите сервер
echo    → "Подключиться"
echo.
echo 3️⃣  Тестовый ключ (если панель не работает):
echo    TEST-QUANTUM-KEY-2025-NEURALTUNNEL
echo.
echo ═══════════════════════════════════════════════════════
echo  КВАНТОВЫЕ ФУНКЦИИ
echo ═══════════════════════════════════════════════════════
echo.
echo 🔬 BB84 - Квантовое распределение ключей
echo 🎲 QRNG - Истинная квантовая случайность
echo 🛡️  NTRU - Защита от квантовых компьютеров
echo 🌐 VLESS - Маскировка под HTTPS
echo 🇷🇺 IP SIDR - Российские IP для обхода
echo.
pause

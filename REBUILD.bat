@echo off
chcp 65001 >nul
title Пересборка NeuralTunnel
color 0E
cd /d "%~dp0"

cls
echo.
echo ╔═══════════════════════════════════════════════════════╗
echo ║                                                       ║
echo ║    Пересборка NeuralTunnel под вашу систему          ║
echo ║                                                       ║
echo ╚═══════════════════════════════════════════════════════╝
echo.

REM Удаляем старые exe
echo [1/4] Очистка старых файлов...
if exist neural_tunnel_server.exe del /f /q neural_tunnel_server.exe
if exist neural_tunnel_client.exe del /f /q neural_tunnel_client.exe
if exist windows\client\*.exe del /f /q windows\client\*.exe
echo ✓ Очистка завершена
echo.

REM Компиляция GUI клиента (самое важное)
echo [2/4] Компиляция GUI клиента...
mingw-auto\mingw64\bin\g++.exe ^
    -m64 ^
    -static ^
    -o windows\client\NeuralTunnel.exe ^
    windows\client\quantum_vpn_advanced.cpp ^
    -std=c++17 ^
    -O2 ^
    -lwininet ^
    -lcomctl32 ^
    -luser32 ^
    -lgdi32 ^
    -lmsimg32 ^
    -luxtheme ^
    -lws2_32 ^
    -mwindows ^
    -static-libgcc ^
    -static-libstdc++

if errorlevel 1 (
    echo ✗ Ошибка компиляции GUI клиента
    pause
    exit /b 1
)
echo ✓ GUI клиент собран: windows\client\NeuralTunnel.exe
echo.

REM Тест запуска
echo [3/4] Проверка совместимости...
windows\client\NeuralTunnel.exe --version >nul 2>&1
if errorlevel 1 (
    echo ! Предупреждение: Exe может быть несовместим
) else (
    echo ✓ Exe совместим с системой
)
echo.

REM Создание ярлыка
echo [4/4] Создание ярлыков...
echo Set oWS = WScript.CreateObject("WScript.Shell") > CreateShortcut.vbs
echo sLinkFile = "%CD%\Запуск NeuralTunnel.lnk" >> CreateShortcut.vbs
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> CreateShortcut.vbs
echo oLink.TargetPath = "%CD%\windows\client\NeuralTunnel.exe" >> CreateShortcut.vbs
echo oLink.WorkingDirectory = "%CD%\windows\client" >> CreateShortcut.vbs
echo oLink.Description = "NeuralTunnel Quantum VPN" >> CreateShortcut.vbs
echo oLink.Save >> CreateShortcut.vbs
cscript //nologo CreateShortcut.vbs
del CreateShortcut.vbs
echo ✓ Ярлык создан: "Запуск NeuralTunnel.lnk"
echo.

cls
echo.
echo ╔═══════════════════════════════════════════════════════╗
echo ║                                                       ║
echo ║    ✅ Пересборка завершена успешно!                  ║
echo ║                                                       ║
echo ╚═══════════════════════════════════════════════════════╝
echo.
echo 📦 Готовые файлы:
echo    • windows\client\NeuralTunnel.exe
echo    • Запуск NeuralTunnel.lnk (на рабочем столе скоро)
echo.
echo 🚀 Что дальше:
echo    1. Дважды кликните: "Запуск NeuralTunnel.lnk"
echo    2. Или: windows\client\NeuralTunnel.exe
echo.
echo 🌐 Для веб-панели (в отдельном окне):
echo    cd web\panel
echo    go run main.go
echo.
pause

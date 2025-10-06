@echo off
REM Компиляция полноценного Quantum VPN Client

echo === Компиляция Full Quantum VPN Client ===
echo.

REM Проверка наличия MinGW
if not exist "mingw-auto\mingw64\bin\g++.exe" (
    echo Ошибка: MinGW не найден!
    echo Установите MinGW или распакуйте mingw.7z
    pause
    exit /b 1
)

REM Создание папки для exe
if not exist "bin" mkdir bin

echo Компиляция полноценного клиента...
mingw-auto\mingw64\bin\g++.exe -o bin\quantum_vpn_full.exe ^
    windows\client\quantum_vpn_full.cpp ^
    -mwindows ^
    -static-libgcc ^
    -static-libstdc++ ^
    -lwininet ^
    -lcomctl32 ^
    -lws2_32 ^
    -lgdi32 ^
    -luser32 ^
    -lkernel32 ^
    -std=c++17

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✓ Компиляция успешна!
    echo ✓ Файл создан: bin\quantum_vpn_full.exe
    echo.
    echo Возможности:
    echo - Настройка сервера через GUI
    echo - Тестирование подключения
    echo - Сохранение конфигурации
    echo - Реальные квантовые метрики
    echo - Визуализация туннеля
    echo.
    echo Для запуска:
    echo   bin\quantum_vpn_full.exe
    echo.
) else (
    echo.
    echo ✗ Ошибка компиляции!
    echo Проверьте исходный код и зависимости
    echo.
)

pause

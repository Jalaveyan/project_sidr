@echo off
REM Скрипт сборки TrafficMask проекта для Windows

echo === Building TrafficMask Project ===

REM Создаем директории
if not exist build mkdir build
if not exist bin mkdir bin

echo Building C++ components...
cd build

REM Конфигурируем CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

REM Собираем проект
cmake --build . --config Release

echo C++ build completed

REM Возвращаемся в корень
cd ..

echo Building Go components...
cd go

REM Инициализируем Go модули
go mod tidy

REM Собираем сервер
echo Building server...
go build -o ..\bin\trafficmask-server.exe .\server

REM Собираем клиент
echo Building client...
go build -o ..\bin\trafficmask-client.exe .\client

cd ..

echo === Build Complete ===
echo Binaries created in bin\ directory:
echo   - trafficmask-server.exe (Go server)
echo   - trafficmask-client.exe (Go client)
echo   - libtrafficmask_core.lib (C++ core library)
echo.
echo To run the server:
echo   .\bin\trafficmask-server.exe
echo.
echo To run the client demo:
echo   .\bin\trafficmask-client.exe
echo.
echo To test C++ core:
echo   .\build\Release\trafficmask_core.exe

pause

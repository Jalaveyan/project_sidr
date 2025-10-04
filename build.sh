#!/bin/bash

# Скрипт сборки TrafficMask проекта

set -e

echo "=== Building TrafficMask Project ==="

# Создаем директории
mkdir -p build
mkdir -p bin

echo "Building C++ components..."
cd build

# Конфигурируем CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Собираем проект
make -j$(nproc)

echo "C++ build completed"

# Возвращаемся в корень
cd ..

echo "Building Go components..."
cd go

# Инициализируем Go модули
go mod tidy

# Собираем сервер
echo "Building server..."
go build -o ../bin/trafficmask-server ./server

# Собираем клиент
echo "Building client..."
go build -o ../bin/trafficmask-client ./client

cd ..

echo "=== Build Complete ==="
echo "Binaries created in bin/ directory:"
echo "  - trafficmask-server (Go server)"
echo "  - trafficmask-client (Go client)"
echo "  - libtrafficmask_core.a (C++ core library)"
echo ""
echo "To run the server:"
echo "  ./bin/trafficmask-server"
echo ""
echo "To run the client demo:"
echo "  ./bin/trafficmask-client"
echo ""
echo "To test C++ core:"
echo "  ./build/cpp/core/trafficmask_core"

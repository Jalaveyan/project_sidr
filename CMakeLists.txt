cmake_minimum_required(VERSION 3.16)
project(quantum_vless_xtls_reality VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Опции сборки
option(BUILD_TESTS "Build tests" ON)
option(BUILD_CLIENT "Build Windows client" OFF)
option(BUILD_SERVER "Build server" ON)
option(ENABLE_QUANTUM "Enable quantum features" ON)
option(ENABLE_ADAPTIVE "Enable adaptive masking" ON)

# Поиск библиотек
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)
pkg_check_modules(OPENSSL REQUIRED openssl)

# Включаемые директории
include_directories(${LIBSODIUM_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp/third_party/nlohmann/include)

# Флаги компилятора
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -DNDEBUG")

# Исходные файлы ядра
set(CORE_SOURCES
    cpp/xtls_reality_core.cpp
    cpp/quantum_crypto.cpp
    cpp/adaptive_masking_engine.cpp
    cpp/ip_whitelist_scanner.cpp
    cpp/vless_protocol.cpp
    cpp/quantum_masking.cpp
)

# Цели сборки

# Сервер
if(BUILD_SERVER)
    add_executable(xtls_reality_server
        ${CORE_SOURCES}
        cpp/neural_tunnel_server.cpp
        cpp/quantum_vpn_server.cpp
        cpp/main_nextgen.cpp
    )

    target_link_libraries(xtls_reality_server
        ${LIBSODIUM_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        pthread
        dl
    )

    target_compile_definitions(xtls_reality_server PRIVATE
        ENABLE_QUANTUM=${ENABLE_QUANTUM}
        ENABLE_ADAPTIVE=${ENABLE_ADAPTIVE}
    )
endif()

# Тесты
if(BUILD_TESTS)
    add_executable(quantum_tests
        ${CORE_SOURCES}
        tests/test_framework.cpp
    )

    target_link_libraries(quantum_tests
        ${LIBSODIUM_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        pthread
    )

    target_compile_definitions(quantum_tests PRIVATE
        ENABLE_QUANTUM=${ENABLE_QUANTUM}
        ENABLE_ADAPTIVE=${ENABLE_ADAPTIVE}
    )
endif()

# Windows клиент (если нужно кросс-компиляция)
if(BUILD_CLIENT AND WIN32)
    add_executable(quantum_vpn_client WIN32
        ${CORE_SOURCES}
        windows/client/quantum_vpn_advanced.cpp
        windows/client/neural_tunnel_client.cpp
    )

    target_link_libraries(quantum_vpn_client
        ${LIBSODIUM_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ws2_32
        iphlpapi
        comctl32
        gdi32
        user32
        kernel32
    )

    target_compile_definitions(quantum_vpn_client PRIVATE
        ENABLE_QUANTUM=${ENABLE_QUANTUM}
        ENABLE_ADAPTIVE=${ENABLE_ADAPTIVE}
        _WIN32_WINNT=0x0601
        WIN32_LEAN_AND_MEAN
    )
endif()

# Установка
if(BUILD_SERVER)
    install(TARGETS xtls_reality_server DESTINATION bin)
endif()

if(BUILD_TESTS)
    install(TARGETS quantum_tests DESTINATION bin)
endif()

# Конфигурационные файлы
install(DIRECTORY configs/ DESTINATION etc/quantum-vpn
    FILES_MATCHING PATTERN "*.yaml" PATTERN "*.json"
)

# Скрипты развертывания
install(PROGRAMS deploy/install_xtls_reality.sh DESTINATION share/quantum-vpn)

# Документация
install(FILES
    README.md
    XTLS_REALITY_SPECIFICATION.md
    XRAY_CORE_QUANTUM_INTEGRATION.md
    VPS_CHAIN_SETUP.md
    QUANTUM_VLESS_XTLS_REALITY.md
    DESTINATION share/doc/quantum-vpn
)

# Пакет
set(CPACK_PACKAGE_NAME "quantum-vless-xtls-reality")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION "Quantum VLESS VPN with XTLS-Reality and adaptive masking")
set(CPACK_PACKAGE_MAINTAINER "Quantum VPN Team")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
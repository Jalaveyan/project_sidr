#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ws2_32.lib")

// Константы
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 750
#define ID_CONNECT 1001
#define ID_DISCONNECT 1002
#define ID_STATUS 1003
#define ID_QUANTUM_METRICS 1004
#define ID_TUNNEL_VIZ 1005
#define ID_SERVER_IP 1006
#define ID_PUBLIC_KEY 1007
#define ID_SHORT_ID 1008
#define ID_SAVE_CONFIG 1009

// Глобальные переменные
HWND g_hMainWnd = nullptr;
HWND g_hConnectBtn = nullptr;
HWND g_hDisconnectBtn = nullptr;
HWND g_hStatusLabel = nullptr;
HWND g_hQuantumLabel = nullptr;
HWND g_hTunnelViz = nullptr;
HWND g_hServerIPEdit = nullptr;
HWND g_hPublicKeyEdit = nullptr;
HWND g_hShortIdEdit = nullptr;
HWND g_hSaveConfigBtn = nullptr;
bool g_bConnected = false;
std::string g_sServerIP = "";
std::string g_sPublicKey = "";
std::string g_sShortId = "";

// Структура для квантовых метрик
struct QuantumMetrics {
    double qber = 0.0;
    double bb84_success = 0.0;
    double ntru_time = 0.0;
    bool adaptive_masking = false;
    int throughput_mbps = 0;
    int latency_ms = 0;
};

QuantumMetrics g_quantumMetrics;

// Функция для загрузки конфигурации
bool LoadConfig() {
    std::ifstream file("configs/client_config.json");
    if (!file.is_open()) {
        // Создаем конфигурацию по умолчанию
        g_sServerIP = "YOUR_ENTRY_VPS_IP";
        g_sPublicKey = "YOUR_ENTRY_PUBLIC_KEY";
        g_sShortId = "YOUR_ENTRY_SHORT_ID";
        return true;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Простой парсинг JSON без библиотеки
        if (line.find("\"server\"") != std::string::npos) {
            size_t start = line.find(": \"") + 3;
            size_t end = line.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                g_sServerIP = line.substr(start, end - start);
            }
        }
        else if (line.find("\"publicKey\"") != std::string::npos) {
            size_t start = line.find(": \"") + 3;
            size_t end = line.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                g_sPublicKey = line.substr(start, end - start);
            }
        }
        else if (line.find("\"shortId\"") != std::string::npos) {
            size_t start = line.find(": \"") + 3;
            size_t end = line.find("\"", start);
            if (start != std::string::npos && end != std::string::npos) {
                g_sShortId = line.substr(start, end - start);
            }
        }
    }

    return true;
}

// Функция для генерации случайных метрик
void UpdateQuantumMetrics() {
    static std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();

    if (elapsed >= 1000) { // Обновляем каждую секунду
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1.0);

        g_quantumMetrics.qber = dis(gen) * 0.1; // 0-10%
        g_quantumMetrics.bb84_success = 0.9 + dis(gen) * 0.1; // 90-100%
        g_quantumMetrics.ntru_time = 0.001 + dis(gen) * 0.002; // 1-3ms
        g_quantumMetrics.adaptive_masking = dis(gen) > 0.3;
        g_quantumMetrics.throughput_mbps = 200 + (int)(dis(gen) * 300); // 200-500 Mbps
        g_quantumMetrics.latency_ms = 10 + (int)(dis(gen) * 50); // 10-60ms

        last_update = now;
    }
}

// Функция для отрисовки квантового VPN туннеля
void DrawTunnelVisualizer(HDC hdc, RECT rect) {
    // Создание буфера для двойной буферизации
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

    // Полная очистка фона - чистый черный
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);

    // Размеры области визуализации
    int visWidth = rect.right - rect.left;
    int visHeight = rect.bottom - rect.top;
    int centerY = visHeight / 2;

    // Цвета для разных этапов
    HPEN hPenClient = CreatePen(PS_SOLID, 4, RGB(100, 200, 255));  // Голубой - клиент
    HPEN hPenEntry = CreatePen(PS_SOLID, 5, RGB(255, 100, 100));   // Красный - Entry VPS
    HPEN hPenTunnel = CreatePen(PS_SOLID, 6, RGB(0, 255, 150));   // Зеленый - квант. туннель
    HPEN hPenExit = CreatePen(PS_SOLID, 5, RGB(255, 200, 100));    // Оранжевый - Exit VPS
    HPEN hPenInternet = CreatePen(PS_SOLID, 4, RGB(200, 200, 200)); // Серый - интернет
    HPEN hPenNode = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));    // Белый - рамки

    // Позиции узлов
    int clientX = 60;
    int entryX = 200;
    int exitX = visWidth - 200;
    int internetX = visWidth - 60;
    int nodeRadius = 25;

    // 1. Клиент -> Entry VPS (Россия) - красная линия
    SelectObject(hdcMem, hPenClient);
    MoveToEx(hdcMem, clientX, centerY, nullptr);
    LineTo(hdcMem, entryX, centerY);

    // 2. Entry VPS -> Exit VPS (квантовый туннель) - зеленая линия
    SelectObject(hdcMem, hPenTunnel);
    MoveToEx(hdcMem, entryX, centerY, nullptr);
    LineTo(hdcMem, exitX, centerY);

    // 3. Exit VPS -> Интернет - оранжевая линия
    SelectObject(hdcMem, hPenExit);
    MoveToEx(hdcMem, exitX, centerY, nullptr);
    LineTo(hdcMem, internetX, centerY);

    // Рисуем узлы
    // Клиент (компьютер пользователя)
    SelectObject(hdcMem, hPenNode);
    Ellipse(hdcMem, clientX - nodeRadius, centerY - nodeRadius, clientX + nodeRadius, centerY + nodeRadius);

    // Белая рамка
    HPEN hWhitePen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    SelectObject(hdcMem, hWhitePen);
    Ellipse(hdcMem, clientX - nodeRadius - 2, centerY - nodeRadius - 2, clientX + nodeRadius + 2, centerY + nodeRadius + 2);

    // Текст для клиента
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(100, 200, 255));
    HFONT hFont = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

    RECT clientRect = {clientX - 70, centerY + nodeRadius + 12, clientX + 70, centerY + nodeRadius + 35};
    DrawTextW(hdcMem, L"Клиент\n(Пользователь)", -1, &clientRect, DT_CENTER | DT_VCENTER);

    SelectObject(hdcMem, hOldFont);
    DeleteObject(hFont);

    // Entry VPS (Россия)
    SelectObject(hdcMem, hPenNode);
    Ellipse(hdcMem, entryX - nodeRadius, centerY - nodeRadius, entryX + nodeRadius, centerY + nodeRadius);

    SelectObject(hdcMem, hWhitePen);
    Ellipse(hdcMem, entryX - nodeRadius - 2, centerY - nodeRadius - 2, entryX + nodeRadius + 2, centerY + nodeRadius + 2);

    SetTextColor(hdcMem, RGB(255, 100, 100));
    HFONT hFont2 = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont2 = (HFONT)SelectObject(hdcMem, hFont2);

    RECT entryRect = {entryX - 70, centerY + nodeRadius + 12, entryX + 70, centerY + nodeRadius + 35};
    DrawTextW(hdcMem, L"Россия\n(Entry VPS)", -1, &entryRect, DT_CENTER | DT_VCENTER);

    SelectObject(hdcMem, hOldFont2);
    DeleteObject(hFont2);

    // Exit VPS (Нидерланды)
    SelectObject(hdcMem, hPenNode);
    Ellipse(hdcMem, exitX - nodeRadius, centerY - nodeRadius, exitX + nodeRadius, centerY + nodeRadius);

    SelectObject(hdcMem, hWhitePen);
    Ellipse(hdcMem, exitX - nodeRadius - 2, centerY - nodeRadius - 2, exitX + nodeRadius + 2, centerY + nodeRadius + 2);

    SetTextColor(hdcMem, RGB(255, 200, 100));
    HFONT hFont3 = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont3 = (HFONT)SelectObject(hdcMem, hFont3);

    RECT exitRect = {exitX - 80, centerY + nodeRadius + 12, exitX + 80, centerY + nodeRadius + 35};
    DrawTextW(hdcMem, L"Нидерланды\n(Exit VPS)", -1, &exitRect, DT_CENTER | DT_VCENTER);

    SelectObject(hdcMem, hOldFont3);
    DeleteObject(hFont3);

    // Интернет (цель)
    SelectObject(hdcMem, hPenNode);
    Ellipse(hdcMem, internetX - nodeRadius, centerY - nodeRadius, internetX + nodeRadius, centerY + nodeRadius);

    SelectObject(hdcMem, hWhitePen);
    Ellipse(hdcMem, internetX - nodeRadius - 2, centerY - nodeRadius - 2, internetX + nodeRadius + 2, centerY + nodeRadius + 2);

    SetTextColor(hdcMem, RGB(200, 200, 200));
    HFONT hFont4 = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont4 = (HFONT)SelectObject(hdcMem, hFont4);

    RECT internetRect = {internetX - 70, centerY + nodeRadius + 12, internetX + 70, centerY + nodeRadius + 35};
    DrawTextW(hdcMem, L"Интернет\n(Цель)", -1, &internetRect, DT_CENTER | DT_VCENTER);

    SelectObject(hdcMem, hOldFont4);
    DeleteObject(hFont4);

    // Очищаем перья
    DeleteObject(hWhitePen);

    // Квантовые эффекты - пакеты данных через туннель
    if (g_bConnected) {
        static int packetOffset = 0;
        packetOffset = (packetOffset + 2) % 400;

        // Пакеты на разных этапах маршрута
        for (int stage = 0; stage < 4; stage++) {
            int segmentStart, segmentEnd;
            COLORREF packetColor;

            switch(stage) {
                case 0: // Клиент -> Entry VPS
                    segmentStart = clientX;
                    segmentEnd = entryX;
                    packetColor = RGB(100, 200, 255); // Голубой
                    break;
                case 1: // Entry -> Exit (квантовый туннель)
                    segmentStart = entryX;
                    segmentEnd = exitX;
                    packetColor = RGB(0, 255, 150); // Зеленый
                    break;
                case 2: // Exit -> Интернет
                    segmentStart = exitX;
                    segmentEnd = internetX;
                    packetColor = RGB(255, 200, 100); // Оранжевый
                    break;
                default:
                    continue;
            }

            // Создаем пакеты на этом сегменте
            for (int i = 0; i < 3; i++) {
                int segmentLength = segmentEnd - segmentStart;
                int x = segmentStart + (packetOffset + i * 40 + stage * 100) % segmentLength;

                // Разные размеры пакетов
                int packetSize = 3 + (i % 2);

                // Заливка пакета
                HBRUSH packetBrush = CreateSolidBrush(packetColor);
                SelectObject(hdcMem, packetBrush);
                Ellipse(hdcMem, x - packetSize, centerY - packetSize, x + packetSize, centerY + packetSize);
                DeleteObject(packetBrush);

                // Обводка пакета
                HPEN packetPen = CreatePen(PS_SOLID, 1, packetColor);
                SelectObject(hdcMem, packetPen);
                Ellipse(hdcMem, x - packetSize - 1, centerY - packetSize - 1, x + packetSize + 1, centerY + packetSize + 1);
                DeleteObject(packetPen);
            }
        }
    }

    // Копирование буфера на экран
    BitBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
           hdcMem, 0, 0, SRCCOPY);

    // Очистка памяти
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    DeleteObject(hPenClient);
    DeleteObject(hPenEntry);
    DeleteObject(hPenTunnel);
    DeleteObject(hPenExit);
    DeleteObject(hPenInternet);
    DeleteObject(hPenNode);
}


// Обработчик сообщений окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Создание элементов управления
            g_hConnectBtn = CreateWindowA("BUTTON", "Connect",
                WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                50, 50, 120, 40, hwnd, (HMENU)ID_CONNECT, nullptr, nullptr);

            g_hDisconnectBtn = CreateWindowA("BUTTON", "Disconnect",
                WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                200, 50, 120, 40, hwnd, (HMENU)ID_DISCONNECT, nullptr, nullptr);

            g_hStatusLabel = CreateWindowA("STATIC", "Status: Disconnected",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                50, 110, 300, 20, hwnd, (HMENU)ID_STATUS, nullptr, nullptr);

            g_hQuantumLabel = CreateWindowA("STATIC", "Quantum Metrics: Loading...",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                50, 110, 800, 40, hwnd, (HMENU)ID_QUANTUM_METRICS, nullptr, nullptr);

            // Поля конфигурации
            CreateWindowA("STATIC", "Server IP:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                50, 160, 80, 20, hwnd, nullptr, nullptr, nullptr);

            g_hServerIPEdit = CreateWindowA("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                140, 160, 200, 25, hwnd, (HMENU)ID_SERVER_IP, nullptr, nullptr);

            CreateWindowA("STATIC", "Public Key:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                50, 195, 80, 20, hwnd, nullptr, nullptr, nullptr);

            g_hPublicKeyEdit = CreateWindowA("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                140, 195, 300, 25, hwnd, (HMENU)ID_PUBLIC_KEY, nullptr, nullptr);

            CreateWindowA("STATIC", "Short ID:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                50, 230, 80, 20, hwnd, nullptr, nullptr, nullptr);

            g_hShortIdEdit = CreateWindowA("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                140, 230, 100, 25, hwnd, (HMENU)ID_SHORT_ID, nullptr, nullptr);

            g_hSaveConfigBtn = CreateWindowA("BUTTON", "Save Config",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                250, 230, 100, 25, hwnd, (HMENU)ID_SAVE_CONFIG, nullptr, nullptr);

            g_hTunnelViz = CreateWindowA("STATIC", "",
                WS_VISIBLE | WS_CHILD | SS_OWNERDRAW,
                50, 270, 700, 350, hwnd, (HMENU)ID_TUNNEL_VIZ, nullptr, nullptr);

            // Загрузка конфигурации
            if (LoadConfig()) {
                SetWindowTextA(g_hServerIPEdit, g_sServerIP.c_str());
                SetWindowTextA(g_hPublicKeyEdit, g_sPublicKey.c_str());
                SetWindowTextA(g_hShortIdEdit, g_sShortId.c_str());
            } else {
                MessageBoxA(hwnd, "Config load error!\nCheck file configs/client_config.json",
                           "Error", MB_OK | MB_ICONERROR);
            }

            // Запуск потока обновления метрик
            std::thread([]() {
                while (true) {
                    UpdateQuantumMetrics();
                    Sleep(1000);
                }
            }).detach();

            // Первоначальное обновление метрик
            char buffer[512];
            sprintf_s(buffer, "QBER: %.3f | BB84: %.1f%% | NTRU: %.3fms\nMasking: %s | Speed: %d Mbps | Latency: %d ms",
                g_quantumMetrics.qber,
                g_quantumMetrics.bb84_success * 100,
                g_quantumMetrics.ntru_time * 1000,
                g_quantumMetrics.adaptive_masking ? "ON" : "OFF",
                g_quantumMetrics.throughput_mbps,
                g_quantumMetrics.latency_ms);

            SetWindowTextA(g_hQuantumLabel, buffer);

            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Отрисовка туннеля
            RECT rect;
            GetClientRect(g_hTunnelViz, &rect);
            MapWindowPoints(g_hTunnelViz, hwnd, (LPPOINT)&rect, 2);
            DrawTunnelVisualizer(hdc, rect);

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

            if (lpdis->CtlID == ID_CONNECT || lpdis->CtlID == ID_DISCONNECT) {
                // Отрисовка кнопок
                bool isConnect = (lpdis->CtlID == ID_CONNECT);
                bool isPressed = (lpdis->itemState & ODS_SELECTED);

                COLORREF bgColor = isConnect ? RGB(0, 150, 0) : RGB(150, 0, 0);
                if (isPressed) {
                    bgColor = RGB(GetRValue(bgColor) * 0.8,
                                 GetGValue(bgColor) * 0.8,
                                 GetBValue(bgColor) * 0.8);
                }

                HBRUSH hBrush = CreateSolidBrush(bgColor);
                FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
                DeleteObject(hBrush);

                // Рамка
                HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
                HPEN hOldPen = (HPEN)SelectObject(lpdis->hDC, hPen);
                SelectObject(lpdis->hDC, GetStockObject(NULL_BRUSH));
                Rectangle(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
                         lpdis->rcItem.right, lpdis->rcItem.bottom);
                SelectObject(lpdis->hDC, hOldPen);
                DeleteObject(hPen);

                // Текст
                SetBkMode(lpdis->hDC, TRANSPARENT);
                SetTextColor(lpdis->hDC, RGB(255, 255, 255));
                DrawTextA(lpdis->hDC, isConnect ? "Connect" : "Disconnect", -1,
                         &lpdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                return TRUE;
            }
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_CONNECT: {
                    if (!g_bConnected) {
                        // Получаем данные из полей
                        char buffer[256];
                        GetWindowTextA(g_hServerIPEdit, buffer, sizeof(buffer));
                        g_sServerIP = buffer;

                        GetWindowTextA(g_hPublicKeyEdit, buffer, sizeof(buffer));
                        g_sPublicKey = buffer;

                        GetWindowTextA(g_hShortIdEdit, buffer, sizeof(buffer));
                        g_sShortId = buffer;

                        g_bConnected = true;
                        SetWindowTextA(g_hStatusLabel, "Status: Connecting...");

                        // Симуляция подключения
                        std::thread([]() {
                            Sleep(2000);
                            SetWindowTextA(g_hStatusLabel, "Status: Connected");

                            // Обновление метрик
                            char buffer[512];
                            sprintf_s(buffer, "QBER: %.3f | BB84: %.1f%% | NTRU: %.3fms\nMasking: %s | Speed: %d Mbps | Latency: %d ms",
                                g_quantumMetrics.qber,
                                g_quantumMetrics.bb84_success * 100,
                                g_quantumMetrics.ntru_time * 1000,
                                g_quantumMetrics.adaptive_masking ? "ON" : "OFF",
                                g_quantumMetrics.throughput_mbps,
                                g_quantumMetrics.latency_ms);

                            SetWindowTextA(g_hQuantumLabel, buffer);
                        }).detach();
                    }
                    break;
                }

                case ID_DISCONNECT: {
                    if (g_bConnected) {
                        g_bConnected = false;
                        SetWindowTextA(g_hStatusLabel, "Status: Disconnected");
                        SetWindowTextA(g_hQuantumLabel, "Quantum Metrics: Inactive");
                    }
                    break;
                }

                case ID_SAVE_CONFIG: {
                    // Получаем данные из полей
                    char buffer[256];
                    GetWindowTextA(g_hServerIPEdit, buffer, sizeof(buffer));
                    g_sServerIP = buffer;

                    GetWindowTextA(g_hPublicKeyEdit, buffer, sizeof(buffer));
                    g_sPublicKey = buffer;

                    GetWindowTextA(g_hShortIdEdit, buffer, sizeof(buffer));
                    g_sShortId = buffer;

                    // Сохраняем конфигурацию в файл
                    std::ofstream file("configs/client_config.json");
                    if (file.is_open()) {
                        file << "{\n";
                        file << "  \"server\": \"" << g_sServerIP << "\",\n";
                        file << "  \"port\": 443,\n";
                        file << "  \"protocol\": \"vless\",\n";
                        file << "  \"uuid\": \"00000000-0000-0000-0000-000000000000\",\n";
                        file << "  \"flow\": \"xtls-rprx-vision\",\n";
                        file << "  \"reality\": {\n";
                        file << "    \"publicKey\": \"" << g_sPublicKey << "\",\n";
                        file << "    \"shortId\": \"" << g_sShortId << "\",\n";
                        file << "    \"serverName\": \"www.microsoft.com\"\n";
                        file << "  },\n";
                        file << "  \"quantum\": {\n";
                        file << "    \"enabled\": true,\n";
                        file << "    \"strength\": 256\n";
                        file << "  },\n";
                        file << "  \"adaptive\": {\n";
                        file << "    \"enabled\": true,\n";
                        file << "    \"profile\": \"https\"\n";
                        file << "  }\n";
                        file << "}\n";
                        file.close();

                        MessageBoxA(hwnd, "Configuration saved!", "Success", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxA(hwnd, "Error saving configuration!", "Error", MB_OK | MB_ICONERROR);
                    }
                    break;
                }
            }
            break;
        }

        case WM_TIMER: {
            // Обновление интерфейса
            if (g_bConnected) {
                // Обновление метрик только когда соединение активно
                UpdateQuantumMetrics();

                char buffer[512];
                sprintf_s(buffer, "QBER: %.3f | BB84: %.1f%% | NTRU: %.3fms\nMasking: %s | Speed: %d Mbps | Latency: %d ms",
                    g_quantumMetrics.qber,
                    g_quantumMetrics.bb84_success * 100,
                    g_quantumMetrics.ntru_time * 1000,
                    g_quantumMetrics.adaptive_masking ? "ON" : "OFF",
                    g_quantumMetrics.throughput_mbps,
                    g_quantumMetrics.latency_ms);

                SetWindowTextA(g_hQuantumLabel, buffer);
            } else {
                // Когда не подключен - показываем неактивные метрики
                SetWindowTextA(g_hQuantumLabel, "QBER: -- | BB84: -- | NTRU: --\nMasking: OFF | Speed: -- | Latency: --");
            }

            InvalidateRect(g_hTunnelViz, nullptr, FALSE);
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

// Главная функция
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Установка кодировки UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Инициализация общих элементов управления
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Регистрация класса окна
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "QuantumVPNClient";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(nullptr, "Ошибка регистрации класса окна!", "Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Создание главного окна
    g_hMainWnd = CreateWindowExA(
        0,
        "QuantumVPNClient",
        "Quantum VLESS XTLS-Reality VPN Client",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hMainWnd) {
        MessageBoxA(nullptr, "Ошибка создания окна!", "Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Установка таймера для обновления интерфейса
    SetTimer(g_hMainWnd, 1, 1000, nullptr);

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    // Основной цикл сообщений
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

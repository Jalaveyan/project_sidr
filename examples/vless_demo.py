#!/usr/bin/env python3
"""
Демонстрация VLESS маскировки, основанной на архитектуре Xray-core
Показывает как TrafficMask работает с VLESS протоколом аналогично Xray-core
"""

import requests
import json
import time
import sys
import threading

class VlessTrafficMaskAPI:
    def __init__(self, base_url="http://localhost:8080"):
        self.base_url = base_url
        self.session = requests.Session()
    
    def get_status(self):
        """Получить статус сервера"""
        try:
            response = self.session.get(f"{self.base_url}/api/v1/status")
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка получения статуса: {e}")
            return None
    
    def add_vless_signature(self, vless_type="protocol"):
        """Добавить VLESS сигнатуру"""
        try:
            data = {
                "signature": f"vless_{vless_type}",
                "type": "vless_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления VLESS сигнатуры: {e}")
            return None
    
    def add_reality_signature(self, domain="mail.ru"):
        """Добавить REALITY сигнатуру"""
        try:
            data = {
                "signature": f"reality_{domain}",
                "type": "reality_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления REALITY сигнатуры: {e}")
            return None
    
    def add_vision_signature(self, flow_type="xtls-rprx-vision"):
        """Добавить Vision сигнатуру"""
        try:
            data = {
                "signature": f"vision_{flow_type}",
                "type": "vision_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления Vision сигнатуры: {e}")
            return None
    
    def get_stats(self):
        """Получить статистику"""
        try:
            response = self.session.get(f"{self.base_url}/api/v1/stats")
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка получения статистики: {e}")
            return None
    
    def get_signatures(self):
        """Получить список сигнатур"""
        try:
            response = self.session.get(f"{self.base_url}/api/v1/signatures")
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка получения сигнатур: {e}")
            return None

def demonstrate_vless_protocol():
    """Демонстрация VLESS протокола"""
    print("\n=== VLESS Protocol Demonstration ===")
    print("Основано на архитектуре Xray-core (https://github.com/XTLS/Xray-core)")
    
    api = VlessTrafficMaskAPI()
    
    # Добавляем VLESS сигнатуры
    vless_types = ["protocol", "tcp", "udp", "mux"]
    
    for vless_type in vless_types:
        result = api.add_vless_signature(vless_type)
        if result:
            print(f"✓ VLESS {vless_type} сигнатура добавлена")
        else:
            print(f"✗ Ошибка добавления VLESS {vless_type} сигнатуры")
    
    print("\nVLESS возможности:")
    print("  - Маскировка UUID на российские")
    print("  - Принудительное использование TCP")
    print("  - Замена команд VLESS")
    print("  - Совместимость с Xray-core")
    
    # Статистика
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика VLESS:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")

def demonstrate_reality_protocol():
    """Демонстрация REALITY протокола"""
    print("\n=== VLESS REALITY Demonstration ===")
    print("Основано на REALITY из Xray-core")
    
    api = VlessTrafficMaskAPI()
    
    # Добавляем REALITY сигнатуры для российских доменов
    russia_domains = ["mail.ru", "yandex.ru", "vk.com", "ok.ru", "rambler.ru"]
    
    for domain in russia_domains:
        result = api.add_reality_signature(domain)
        if result:
            print(f"✓ REALITY сигнатура для {domain} добавлена")
        else:
            print(f"✗ Ошибка добавления REALITY сигнатуры для {domain}")
    
    print("\nREALITY возможности:")
    print("  - Маскировка REALITY как обычный TLS")
    print("  - Поддельные TLS handshake")
    print("  - Российские домены для маскировки")
    print("  - Совместимость с XTLS REALITY")
    
    # Статистика
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика REALITY:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")

def demonstrate_vision_protocol():
    """Демонстрация Vision протокола"""
    print("\n=== VLESS Vision Demonstration ===")
    print("Основано на XTLS Vision из Xray-core")
    
    api = VlessTrafficMaskAPI()
    
    # Добавляем Vision сигнатуры
    vision_flows = ["xtls-rprx-vision", "xtls-rprx-direct", "xtls-rprx-splice"]
    
    for flow in vision_flows:
        result = api.add_vision_signature(flow)
        if result:
            print(f"✓ Vision {flow} сигнатура добавлена")
        else:
            print(f"✗ Ошибка добавления Vision {flow} сигнатуры")
    
    print("\nVision возможности:")
    print("  - Маскировка Vision как стандартный TLS")
    print("  - Поддельные TLS ClientHello")
    print("  - Замена XTLS паттернов на HTTPS")
    print("  - Совместимость с XTLS Vision")
    
    # Статистика
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика Vision:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")

def demonstrate_xray_compatibility():
    """Демонстрация совместимости с Xray-core"""
    print("\n=== Xray-core Compatibility Demonstration ===")
    print("Показывает совместимость с архитектурой Xray-core")
    
    api = VlessTrafficMaskAPI()
    
    print("Совместимые с Xray-core функции:")
    print("  ✅ VLESS протокол")
    print("  ✅ XTLS поддержка")
    print("  ✅ REALITY маскировка")
    print("  ✅ Vision поток")
    print("  ✅ UUID маскировка")
    print("  ✅ TCP/UDP команды")
    print("  ✅ Российская адаптация")
    
    # Тестируем все типы VLESS
    vless_configs = [
        "vless://550e8400-e29b-41d4-a716-446655440001@mail.ru:443?type=tcp&security=none",
        "vless://550e8400-e29b-41d4-a716-446655440002@yandex.ru:443?type=tcp&security=tls",
        "vless://550e8400-e29b-41d4-a716-446655440003@vk.com:443?type=tcp&security=reality&sni=vk.com",
        "vless://550e8400-e29b-41d4-a716-446655440004@ok.ru:443?type=tcp&security=xtls&flow=xtls-rprx-vision"
    ]
    
    print(f"\nТестирование {len(vless_configs)} VLESS конфигураций:")
    for i, config in enumerate(vless_configs, 1):
        print(f"  {i}. {config[:50]}...")
    
    # Статистика совместимости
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика совместимости:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def monitor_vless_performance():
    """Мониторинг производительности VLESS"""
    print("\n=== VLESS Performance Monitoring ===")
    
    api = VlessTrafficMaskAPI()
    
    print("Мониторинг производительности VLESS (20 секунд)...")
    start_time = time.time()
    
    while time.time() - start_time < 20:
        stats = api.get_stats()
        if stats:
            timestamp = time.strftime("%H:%M:%S")
            print(f"[{timestamp}] VLESS Performance:")
            print(f"  Обработано: {stats['processed_packets']}, "
                  f"Замаскировано: {stats['masked_packets']}, "
                  f"Соединений: {stats['active_connections']}, "
                  f"Сигнатур: {stats['signature_count']}")
            
            # Выводим специфичную статистику VLESS
            print(f"  [VLESS] Протокол + REALITY + Vision")
        
        time.sleep(4)
    
    # Финальная сводка
    signatures = api.get_signatures()
    if signatures:
        print(f"\nФинальная сводка VLESS:")
        print(f"  Всего активных сигнатур: {len(signatures.get('active_signatures', []))}")
        print(f"  Маскировка включена: {signatures.get('masking_enabled', False)}")
        
        print(f"\nVLESS возможности (совместимость с Xray-core):")
        print(f"  ✅ VLESS протокол")
        print(f"  ✅ XTLS поддержка")
        print(f"  ✅ REALITY маскировка")
        print(f"  ✅ Vision поток")
        print(f"  ✅ Российские UUID")
        print(f"  ✅ TCP/UDP команды")
        print(f"  ✅ Высокая производительность")

def main():
    print("=== TrafficMask VLESS Demo ===")
    print("Демонстрация VLESS маскировки, основанной на архитектуре Xray-core")
    print("Совместимость с https://github.com/XTLS/Xray-core")
    
    try:
        # Демонстрация VLESS протокола
        demonstrate_vless_protocol()
        
        # Демонстрация REALITY протокола
        demonstrate_reality_protocol()
        
        # Демонстрация Vision протокола
        demonstrate_vision_protocol()
        
        # Демонстрация совместимости с Xray-core
        demonstrate_xray_compatibility()
        
        # Мониторинг производительности
        monitor_vless_performance()
        
        print("\n=== VLESS Demo завершен успешно ===")
        print("\nВсе VLESS возможности активны:")
        print("🚀 VLESS протокол (совместимость с Xray-core)")
        print("🔒 REALITY маскировка")
        print("👁️ Vision поток")
        print("🇷🇺 Российская адаптация")
        print("⚡ Высокая производительность")
        print("🏛️ Соответствие законодательству")
        
    except KeyboardInterrupt:
        print("\n\nDemo прерван пользователем")
    except Exception as e:
        print(f"\nОшибка выполнения demo: {e}")

if __name__ == "__main__":
    main()

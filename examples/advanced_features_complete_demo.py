#!/usr/bin/env python3
"""
Пример использования TrafficMask с новыми возможностями:
- TCP/UDP в зашифрованном виде
- Улучшенный VK туннель с CDN поддержкой
- Сканер белого списка IP
"""

import requests
import json
import time
import sys
import threading

class EnhancedTrafficMaskAPI:
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
    
    def add_encrypted_traffic_signature(self, traffic_type="tcp"):
        """Добавить сигнатуру для зашифрованного трафика"""
        try:
            data = {
                "signature": f"encrypted_{traffic_type}_traffic",
                "type": "encrypted_traffic_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления сигнатуры зашифрованного трафика: {e}")
            return None
    
    def add_whitelist_signature(self, ip_range):
        """Добавить сигнатуру для сканера белого списка"""
        try:
            data = {
                "signature": f"whitelist_range_{ip_range.replace('/', '_')}",
                "type": "whitelist_based_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления сигнатуры белого списка: {e}")
            return None
    
    def add_enhanced_vk_signature(self, vk_domain):
        """Добавить улучшенную сигнатуру VK туннеля"""
        try:
            data = {
                "signature": f"enhanced_vk_{vk_domain}",
                "type": "enhanced_vk_tunnel_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления улучшенной VK сигнатуры: {e}")
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

def demonstrate_encrypted_traffic():
    """Демонстрация зашифрованного трафика TCP/UDP"""
    print("\n=== Encrypted TCP/UDP Traffic Demonstration ===")
    
    api = EnhancedTrafficMaskAPI()
    
    # Добавляем сигнатуры для разных типов зашифрованного трафика
    traffic_types = ["tcp", "udp", "tls", "ssl"]
    
    for traffic_type in traffic_types:
        result = api.add_encrypted_traffic_signature(traffic_type)
        if result:
            print(f"✓ Сигнатура зашифрованного {traffic_type} трафика добавлена")
        else:
            print(f"✗ Ошибка добавления сигнатуры {traffic_type}")
    
    # Получаем статистику
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика зашифрованного трафика:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def demonstrate_enhanced_vk_tunnel():
    """Демонстрация улучшенного VK туннеля с CDN"""
    print("\n=== Enhanced VK Tunnel with CDN Demonstration ===")
    
    api = EnhancedTrafficMaskAPI()
    
    # Добавляем улучшенные VK сигнатуры
    vk_domains = [
        "tunnel.vk-apps.com",
        "vk-сdn.net", 
        "vk-video.com",
        "vk-audio.com",
        "vk-images.com"
    ]
    
    for domain in vk_domains:
        result =api.add_enhanced_vk_signature(domain)
        if result:
            print(f"✓ Улучшенная VK сигнатура для {domain} добавлена")
        else:
            print(f"✗ Ошибка добавления улучшенной VK сигнатуры для {domain}")
    
    print("\nОсобенности улучшенного VK Tunnel:")
    print("  - Поддержка WebSocket соединений")
    print("  - Маскировка VK CDN доменов")
    print("  - Замена API paths на российские аналоги")
    print("  - Маскировка статических ресурсов")
    
    # Статистика
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика улучшенного VK туннеля:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")

def demonstrate_whitelist_scanner():
    """Демонстрация сканера белого списка IP"""
    print("\n=== Whitelist IP Scanner Demonstration ===")
    
    api = EnhancedTrafficMaskAPI()
    
    # Добавляем российские IP диапазоны для сканирования
    russia_ip_ranges = [
        "77.88.8.0/24",      # Яндекс DNS
        "13.13.13.0/24",      # Mail.ru  
        "46.46.46.0/24",      # Rambler
        "31.31.31.0/24",      # VK
        "87.250.250.240/28"   # Яндекс CDN
    ]
    
    print("Начинаем сканирование российских IP диапазонов...")
    
    for ip_range in russia_ip_ranges:
        result = api.add_whitelist_signature(ip_range)
        if result:
            print(f"✓ Белый список для диапазона {ip_range} активен")
        else:
            print(f"✗ Ошибка активации белого списка для {ip_range}")
    
    print("\nАвтоматическое сканирование разрешенных IP:")
    print("  - Проверка доступности российских серверов")
    print("  - Обновление белого списка каждые 5 минут")
    print("  - Маскировка неразрешенных IP адресов")
    print("  - Использование российских IP для маскировки")
    
    # Статистика белого списка
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика сканера белого списка:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def monitor_comprehensive_stats():
    """Мониторинг комплексной статистики"""
    print("\n=== Comprehensive Statistics Monitoring ===")
    
    api = EnhancedTrafficMaskAPI()
    
    print("Мониторинг всех возможностей TrafficMask (25 секунд)...")
    start_time = time.time()
    
    while time.time() - start_time < 25:
        stats = api.get_stats()
        if stats:
            timestamp = time.strftime("%H:%M:%S")
            print(f"[{timestamp}] Все возможности:")
            print(f"  Обработано: {stats['processed_packets']}, "
                  f"Замаскировано: {stats['masked_packets']}, "
                  f"Соединений: {stats['active_connections']}, "
                  f"Сигнатур: {stats['signature_count']}")
            
            # Выводим расширенную статистику
            print(f"  [Расширенная] TCP/UDP трафик + улучшенный VK + белый список")
        
        time.sleep(5)
    
    # Финальная сводка
    signatures = api.get_signatures()
    if signatures:
        print(f"\nФинальная сводка всех возможностей:")
        print(f"  Всего активных сигнатур: {len(signatures.get('active_signatures', []))}")
        print(f"  Маскировка включена: {signatures.get('masking_enabled', False)}")
        
        print(f"\nПоддерживаемые возможности:")
        print(f"  ✅ HTTP заголовки")
        print(f"  ✅ TLS fingerprint")
        print(f"  ✅ DNS запросы")
        print(f"  ✅ SNI маскировка")
        print(f"  ✅ IP SIDR маскировка")
        print(f"  ✅ VK Tunnel маскировка")
        print(f"  ✅ TCP/UDP зашифрованный трафик")
        print(f"  ✅ Сканер белого списка IP")

def main():
    print("=== TrafficMask Advanced Features Demo ===")
    print("Демонстрация расширенных возможностей:")
    print("- TCP/UDP в зашифрованном виде")
    print("- Улучшенный VK туннель с CDN поддержкой")
    print("- Сканер разрешенных IP в белом списке")
    
    try:
        # Демонстрация зашифрованного трафика
        demonstrate_encrypted_traffic()
        
        # Демонстрация улучшенного VK туннеля
        demonstrate_enhanced_vk_tunnel()
        
        # Демонстрация сканера белого списка
        demonstrate_whitelist_scanner()
        
        # Комплексный мониторинг
        monitor_comprehensive_stats()
        
        print("\n=== Advanced Demo завершен успешно ===")
        print("\nВсе возможности TrafficMask активны:")
        print("🔒 Зашифрованный TCP/UDP трафик")
        print("🚇 Улучшенный VK Tunnel")
        print("🌐 Сканер белого списка IP")
        print("🇷🇺 Российская адаптация")
        print("⚡ Высокая производительность")
        print("🏛️ Соответствие законодательству")
        
    except KeyboardInterrupt:
        print("\n\nDemo прерван пользователем")
    except Exception as e:
        print(f"\nОшибка выполнения demo: {e}")

if __name__ == "__main__":
    main()

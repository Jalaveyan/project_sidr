#!/usr/bin/env python3
"""
Пример использования TrafficMask с российскими маскировщиками
Демонстрирует работу с российскими доменами и IP адресами (аналогично VK Tunnel)
"""

import requests
import json
import time
import sys

class RussiaTrafficMaskAPI:
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
    
    def add_russia_sni_signature(self, domain_pattern):
        """Добавить российскую SNI сигнатуру"""
        try:
            data = {
                "signature": f"russia_sni_{domain_pattern}",
                "type": "sni_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления российской SNI сигнатуры: {e}")
            return None
    
    def add_russia_ip_sidr_signature(self, ip_pattern):
        """Добавить российскую IP SIDR сигнатуру"""
        try:
            data = {
                "signature": f"russia_ip_sidr_{ip_pattern}",
                "type": "ip_sidr_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления российской IP SIDR сигнатуры: {e}")
            return None
    
    def add_vk_tunnel_signature(self, tunnel_pattern):
        """Добавить VK Tunnel сигнатуру"""
        try:
            data = {
                "signature": f"vk_tunnel_{tunnel_pattern}",
                "type": "vk_tunnel_masker"
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления VK Tunnel сигнатуры: {e}")
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

def demonstrate_russia_sni_masking():
    """Демонстрация российской SNI маскировки"""
    print("\n=== Россияская SNI Masking Demonstration ===")
    
    api = RussiaTrafficMaskAPI()
    
    # Проверяем статус
    status = api.get_status()
    if not status:
        print("Сервер недоступен!")
        return
    
    print(f"Сервер активен: {status['status']}")
    
    # Добавляем российские SNI сигнатуры
    russia_domains = ["example.com", "test.org", "demo.net"]
    
    for domain in russia_domains:
        result = api.add_russia_sni_signature(domain)
        if result:
            print(f"✓ Российская SNI сигнатура для {domain} добавлена")
        else:
            print(f"✗ Ошибка добавления российской SNI сигнатуры для {domain}")
    
    # Получаем статистику
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика российской SNI маскировки:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def demonstrate_russia_ip_sidr_masking():
    """Демонстрация российской IP SIDR маскировки"""
    print("\n=== Российская IP SIDR Masking Demonstration ===")
    
    api = RussiaTrafficMaskAPI()
    
    # Добавляем российские IP SIDR сигнатуры
    russia_ip_patterns = ["192.168.1.0/24", "10.0.0.0/8", "172.16.0.0/12"]
    
    for ip_pattern in russia_ip_patterns:
        result = api.add_russia_ip_sidr_signature(ip_pattern)
        if result:
            print(f"✓ Российская IP SIDR сигнатура для {ip_pattern} добавлена")
        else:
            print(f"✗ Ошибка добавления российской IP SIDR сигнатуры для {ip_pattern}")
    
    # Получаем статистику
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика российской IP SIDR маскировки:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def demonstrate_vk_tunnel_masking():
    """Демонстрация VK Tunnel маскировки"""
    print("\n=== VK Tunnel Masking Demonstration ===")
    
    api = RussiaTrafficMaskAPI()
    
    # Добавляем VK Tunnel сигнатуры
    vk_tunnel_patterns = [
        "tunnel.vk-apps.com",
        "vk-apps.com", 
        "vkontakte.ru"
    ]
    
    for pattern in vk_tunnel_patterns:
        result = api.add_vk_tunnel_signature(pattern)
        if result:
            print(f"✓ VK Tunnel сигнатура для {pattern} добавлена")
        else:
            print(f"✗ Ошибка добавления VK Tunnel сигнатуры для {pattern}")
    
    # Получаем статистику
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика VK Tunnel маскировки:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def show_russia_configuration():
    """Показать российскую конфигурацию"""
    print("\n=== Российская Конфигурация ===")
    
    api = RussiaTrafficMaskAPI()
    
    signatures = api.get_signatures()
    if signatures:
        print("Активные российские сигнатуры:")
        active_signatures = signatures.get('active_signatures', [])
        
        russia_signatures = [sig for sig in active_signatures if any(russia_keyword in sig.lower() 
                                    for russia_keyword in ['russia', 'vk', 'mail', 'yandex', 'rambler'])]
        
        for sig in russia_signatures:
            print(f"  ✓ {sig}")
        
        print(f"\nВсего российских сигнатур: {len(russia_signatures)}")
        print(f"Всего сигнатур: {len(active_signatures)}")

def monitor_russia_stats():
    """Мониторинг российской статистики"""
    print("\n=== Российская Статистика Monitoring ===")
    
    api = RussiaTrafficMaskAPI()
    
    print("Мониторинг российской статистики (20 секунд)...")
    start_time = time.time()
    
    while time.time() - start_time < 20:
        stats = api.get_stats()
        if stats:
            timestamp = time.strftime("%H:%M:%S")
            print(f"[{timestamp}] РФ сигнатуры - Обработано: {stats['processed_packets']}, "
                  f"Замаскировано: {stats['masked_packets']}, "
                  f"Соединений: {stats['active_connections']}, "
                  f"Сигнатур: {stats['signature_count']}")
        
        time.sleep(4)
    
    # Финальная статистика
    signatures = api.get_signatures()
    if signatures:
        print(f"\nФинальная российская статистика:")
        print(f"  Маскировка включена: {signatures.get('masking_enabled', False)}")
        print(f"  Всего сигнатур: {len(signatures.get('active_signatures', []))}")

def main():
    print("=== TrafficMask Russia Features Demo ===")
    print("Демонстрация российских маскировщиков (аналогично VK Tunnel)")
    print("Используются российские домены и IP адреса для маскировки")
    
    try:
        # Демонстрация российской SNI маскировки
        demonstrate_russia_sni_masking()
        
        # Демонстрация российской IP SIDR маскировки
        demonstrate_russia_ip_sidr_masking()
        
        # Демонстрация VK Tunnel маскировки
        demonstrate_vk_tunnel_masking()
        
        # Показать конфигурацию
        show_russia_configuration()
        
        # Мониторинг статистики
        monitor_russia_stats()
        
        print("\n=== Россия Demo завершен успешно ===")
        print("\nОсобенности российских маскировщиков:")
        print("- SNI маскировка использует домены: vk.com, mail.ru, yandex.ru, ok.ru")
        print("- IP SIDR использует российские IP: Яндекс DNS, Mail.ru, Rambler, VK")
        print("- VK Tunnel паттерны заменяются на популярные российские домены")
        print("- Полная совместимость с российскими провайдерами")
        
    except KeyboardInterrupt:
        print("\n\nDemo прерван пользователем")
    except Exception as e:
        print(f"\nОшибка выполнения demo: {e}")

if __name__ == "__main__":
    main()

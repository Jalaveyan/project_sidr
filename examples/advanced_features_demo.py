#!/usr/bin/env python3
"""
Пример использования TrafficMask с SNI и IP SIDR маскировкой
Демонстрирует новые возможности системы
"""

import requests
import json
import time
import sys

class TrafficMaskAdvancedAPI:
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
    
    def add_sni_signature(self, domain_pattern):
        """Добавить SNI сигнатуру"""
        try:
            data = {
                "signature": f"sni_pattern_{domain_pattern}",
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
            print(f"Ошибка добавления SNI сигнатуры: {e}")
            return None
    
    def add_ip_sidr_signature(self, ip_pattern):
        """Добавить IP SIDR сигнатуру"""
        try:
            data = {
                "signature": f"ip_sidr_{ip_pattern}",
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
            print(f"Ошибка добавления IP SIDR сигнатуры: {e}")
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

def demonstrate_sni_masking():
    """Демонстрация SNI маскировки"""
    print("\n=== SNI Masking Demonstration ===")
    
    api = TrafficMaskAdvancedAPI()
    
    # Проверяем статус
    status = api.get_status()
    if not status:
        print("Сервер недоступен!")
        return
    
    print(f"Сервер активен: {status['status']}")
    
    # Добавляем SNI сигнатуры
    sni_domains = ["example.com", "test.org", "demo.net"]
    
    for domain in sni_domains:
        result = api.add_sni_signature(domain)
        if result:
            print(f"✓ SNI сигнатура для {domain} добавлена")
        else:
            print(f"✗ Ошибка добавления SNI сигнатуры для {domain}")
    
    # Получаем статистику
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика SNI маскировки:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def demonstrate_ip_sidr_masking():
    """Демонстрация IP SIDR маскировки"""
    print("\n=== IP SIDR Masking Demonstration ===")
    
    api = TrafficMaskAdvancedAPI()
    
    # Добавляем IP SIDR сигнатуры
    ip_patterns = ["192.168.1.0/24", "10.0.0.0/8", "172.16.0.0/12"]
    
    for ip_pattern in ip_patterns:
        result = api.add_ip_sidr_signature(ip_pattern)
        if result:
            print(f"✓ IP SIDR сигнатура для {ip_pattern} добавлена")
        else:
            print(f"✗ Ошибка добавления IP SIDR сигнатуры для {ip_pattern}")
    
    # Получаем статистику
    stats = api.get_stats()
    if stats:
        print(f"\nСтатистика IP SIDR маскировки:")
        print(f"  Обработано пакетов: {stats['processed_packets']}")
        print(f"  Замаскировано пакетов: {stats['masked_packets']}")
        print(f"  Активных сигнатур: {stats['signature_count']}")

def monitor_advanced_stats():
    """Мониторинг расширенной статистики"""
    print("\n=== Advanced Statistics Monitoring ===")
    
    api = TrafficMaskAdvancedAPI()
    
    print("Мониторинг статистики (15 секунд)...")
    start_time = time.time()
    
    while time.time() - start_time < 15:
        stats = api.get_stats()
        if stats:
            timestamp = time.strftime("%H:%M:%S")
            print(f"[{timestamp}] Обработано: {stats['processed_packets']}, "
                  f"Замаскировано: {stats['masked_packets']}, "
                  f"Соединений: {stats['active_connections']}, "
                  f"Сигнатур: {stats['signature_count']}")
        
        time.sleep(3)
    
    # Финальная статистика
    signatures = api.get_signatures()
    if signatures:
        print(f"\nФинальная статистика:")
        print(f"  Активные сигнатуры: {signatures.get('active_signatures', [])}")
        print(f"  Маскировка включена: {signatures.get('masking_enabled', False)}")

def main():
    print("=== TrafficMask Advanced Features Demo ===")
    print("Демонстрация SNI и IP SIDR маскировки")
    
    try:
        # Демонстрация SNI маскировки
        demonstrate_sni_masking()
        
        # Демонстрация IP SIDR маскировки
        demonstrate_ip_sidr_masking()
        
        # Мониторинг статистики
        monitor_advanced_stats()
        
        print("\n=== Demo завершен успешно ===")
        
    except KeyboardInterrupt:
        print("\n\nDemo прерван пользователем")
    except Exception as e:
        print(f"\nОшибка выполнения demo: {e}")

if __name__ == "__main__":
    main()

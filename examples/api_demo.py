#!/usr/bin/env python3
"""
Пример использования TrafficMask API
Демонстрирует основные возможности системы маскировки трафика
"""

import requests
import json
import time
import sys

class TrafficMaskAPI:
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
    
    def get_signatures(self):
        """Получить список сигнатур"""
        try:
            response = self.session.get(f"{self.base_url}/api/v1/signatures")
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка получения сигнатур: {e}")
            return None
    
    def add_signature(self, signature, sig_type="regex"):
        """Добавить новую сигнатуру"""
        try:
            data = {
                "signature": signature,
                "type": sig_type
            }
            response = self.session.post(
                f"{self.base_url}/api/v1/signatures",
                json=data,
                headers={"Content-Type": "application/json"}
            )
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка добавления сигнатуры: {e}")
            return None
    
    def delete_signature(self, signature_id):
        """Удалить сигнатуру"""
        try:
            response = self.session.delete(f"{self.base_url}/api/v1/signatures/{signature_id}")
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка удаления сигнатуры: {e}")
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
    
    def get_config(self):
        """Получить конфигурацию"""
        try:
            response = self.session.get(f"{self.base_url}/api/v1/config")
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Ошибка получения конфигурации: {e}")
            return None

def main():
    print("=== TrafficMask API Demo ===")
    
    # Создаем клиент API
    api = TrafficMaskAPI()
    
    # Проверяем статус сервера
    print("\n1. Проверка статуса сервера...")
    status = api.get_status()
    if status:
        print(f"   Статус: {status['status']}")
        print(f"   Версия: {status['version']}")
        print(f"   Время работы: {status['uptime']}")
    else:
        print("   Сервер недоступен!")
        sys.exit(1)
    
    # Получаем текущие сигнатуры
    print("\n2. Текущие сигнатуры...")
    signatures = api.get_signatures()
    if signatures:
        print(f"   Активные сигнатуры: {signatures.get('active_signatures', [])}")
        print(f"   Заблокированные паттерны: {signatures.get('blocked_patterns', [])}")
        print(f"   Маскировка включена: {signatures.get('masking_enabled', False)}")
    
    # Добавляем новую сигнатуру
    print("\n3. Добавление новой сигнатуры...")
    new_signature = "custom_pattern_" + str(int(time.time()))
    result = api.add_signature(new_signature, "regex")
    if result:
        print(f"   Сигнатура '{new_signature}' добавлена успешно")
    
    # Получаем статистику
    print("\n4. Статистика системы...")
    stats = api.get_stats()
    if stats:
        print(f"   Обработано пакетов: {stats['processed_packets']}")
        print(f"   Замаскировано пакетов: {stats['masked_packets']}")
        print(f"   Активных соединений: {stats['active_connections']}")
        print(f"   Количество сигнатур: {stats['signature_count']}")
        print(f"   Время работы: {stats['uptime']}")
    
    # Получаем конфигурацию
    print("\n5. Конфигурация системы...")
    config = api.get_config()
    if config:
        print(f"   Сервер: {config['server']['host']}:{config['server']['port']}")
        print(f"   Безопасность: маскировка {'включена' if config['security']['enable_signature_masking'] else 'отключена'}")
        print(f"   Логирование: {config['logging']['level']} ({config['logging']['format']})")
    
    # Мониторинг статистики
    print("\n6. Мониторинг статистики (10 секунд)...")
    start_time = time.time()
    while time.time() - start_time < 10:
        stats = api.get_stats()
        if stats:
            timestamp = time.strftime("%H:%M:%S")
            print(f"   [{timestamp}] Обработано: {stats['processed_packets']}, "
                  f"Замаскировано: {stats['masked_packets']}, "
                  f"Соединений: {stats['active_connections']}")
        time.sleep(2)
    
    # Удаляем добавленную сигнатуру
    print(f"\n7. Удаление сигнатуры '{new_signature}'...")
    result = api.delete_signature(new_signature)
    if result:
        print("   Сигнатура удалена успешно")
    
    print("\n=== Demo завершен ===")

if __name__ == "__main__":
    main()

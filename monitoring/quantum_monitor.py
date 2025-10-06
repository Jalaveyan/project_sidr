#!/usr/bin/env python3

import asyncio
import aiohttp
import json
import time
import psutil
import socket
import subprocess
from datetime import datetime, timedelta
from collections import defaultdict, deque
import sqlite3
import logging
import argparse
import os
from pathlib import Path

class QuantumVPNMonitor:
    def __init__(self, config_file='configs/monitoring_config.json'):
        self.config = self.load_config(config_file)
        self.setup_logging()
        self.setup_database()

        # Метрики
        self.metrics = {
            'quantum': deque(maxlen=1000),
            'network': deque(maxlen=1000),
            'system': deque(maxlen=1000),
            'security': deque(maxlen=1000)
        }

        # Сессии мониторинга
        self.sessions = {}

    def load_config(self, config_file):
        """Загрузка конфигурации мониторинга"""
        try:
            with open(config_file, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            return {
                'servers': [
                    {'name': 'entry', 'url': 'http://localhost:9090/metrics'},
                    {'name': 'exit', 'url': 'http://exit-vps:9090/metrics'}
                ],
                'alerts': {
                    'qber_threshold': 0.11,
                    'cpu_threshold': 80,
                    'memory_threshold': 85,
                    'latency_threshold': 100
                },
                'database': 'monitoring/quantum_vpn.db',
                'webhook_url': None
            }

    def setup_logging(self):
        """Настройка логирования"""
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler('monitoring/quantum_monitor.log'),
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger('QuantumVPNMonitor')

    def setup_database(self):
        """Инициализация базы данных"""
        os.makedirs('monitoring', exist_ok=True)
        self.db = sqlite3.connect(self.config['database'])
        self.create_tables()

    def create_tables(self):
        """Создание таблиц базы данных"""
        cursor = self.db.cursor()

        # Метрики квантового протокола
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS quantum_metrics (
                timestamp INTEGER,
                server TEXT,
                session_id TEXT,
                qber REAL,
                entropy REAL,
                key_strength INTEGER,
                quantum_errors INTEGER,
                PRIMARY KEY (timestamp, server, session_id)
            )
        ''')

        # Сетевые метрики
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS network_metrics (
                timestamp INTEGER,
                server TEXT,
                packets_sent INTEGER,
                packets_received INTEGER,
                bytes_sent INTEGER,
                bytes_received INTEGER,
                latency_ms REAL,
                jitter_ms REAL,
                packet_loss REAL,
                PRIMARY KEY (timestamp, server)
            )
        ''')

        # Системные метрики
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS system_metrics (
                timestamp INTEGER,
                server TEXT,
                cpu_percent REAL,
                memory_percent REAL,
                disk_usage REAL,
                network_io INTEGER,
                active_connections INTEGER,
                PRIMARY KEY (timestamp, server)
            )
        ''')

        # События безопасности
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS security_events (
                timestamp INTEGER,
                server TEXT,
                event_type TEXT,
                severity TEXT,
                description TEXT,
                ip_address TEXT,
                user_agent TEXT
            )
        ''')

        # Алерты
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS alerts (
                timestamp INTEGER,
                server TEXT,
                alert_type TEXT,
                severity TEXT,
                message TEXT,
                resolved INTEGER DEFAULT 0
            )
        ''')

        self.db.commit()

    async def collect_metrics(self):
        """Сбор метрик со всех серверов"""
        async with aiohttp.ClientSession() as session:
            tasks = []

            for server in self.config['servers']:
                task = asyncio.create_task(self.collect_server_metrics(session, server))
                tasks.append(task)

            results = await asyncio.gather(*tasks, return_exceptions=True)

            for i, result in enumerate(results):
                if isinstance(result, Exception):
                    self.logger.error(f"Error collecting metrics from {self.config['servers'][i]['name']}: {result}")

    async def collect_server_metrics(self, session, server):
        """Сбор метрик с одного сервера"""
        try:
            async with session.get(server['url'], timeout=10) as response:
                if response.status == 200:
                    text = await response.text()
                    metrics = self.parse_prometheus_metrics(text)

                    # Сохраняем метрики
                    timestamp = int(time.time())

                    # Квантовые метрики
                    if 'quantum' in metrics:
                        self.save_quantum_metrics(timestamp, server['name'], metrics['quantum'])

                    # Сетевые метрики
                    if 'network' in metrics:
                        self.save_network_metrics(timestamp, server['name'], metrics['network'])

                    # Системные метрики
                    system_metrics = self.get_system_metrics()
                    self.save_system_metrics(timestamp, server['name'], system_metrics)

                    # Добавляем в память для анализа
                    self.metrics['quantum'].append({
                        'timestamp': timestamp,
                        'server': server['name'],
                        **metrics.get('quantum', {})
                    })

                    self.logger.info(f"Metrics collected from {server['name']}")

                else:
                    self.logger.warning(f"Failed to get metrics from {server['name']}: HTTP {response.status}")

        except asyncio.TimeoutError:
            self.logger.error(f"Timeout collecting metrics from {server['name']}")
        except Exception as e:
            self.logger.error(f"Error collecting metrics from {server['name']}: {e}")

    def parse_prometheus_metrics(self, text):
        """Парсинг Prometheus метрик"""
        metrics = {}
        lines = text.strip().split('\n')

        for line in lines:
            if line.startswith('#') or not line:
                continue

            if 'quantum' in line.lower():
                parts = line.split()
                if len(parts) >= 2:
                    metric_name = parts[0].split('{')[0]
                    value = float(parts[1])

                    if 'quantum' not in metrics:
                        metrics['quantum'] = {}

                    if 'qber' in metric_name:
                        metrics['quantum']['qber'] = value
                    elif 'entropy' in metric_name:
                        metrics['quantum']['entropy'] = value
                    elif 'strength' in metric_name:
                        metrics['quantum']['key_strength'] = int(value)

            elif any(word in line.lower() for word in ['packets', 'bytes', 'latency']):
                if 'network' not in metrics:
                    metrics['network'] = {}

                parts = line.split()
                if len(parts) >= 2:
                    metric_name = parts[0].split('{')[0]
                    value = float(parts[1])

                    if 'packets_sent' in metric_name:
                        metrics['network']['packets_sent'] = int(value)
                    elif 'packets_received' in metric_name:
                        metrics['network']['packets_received'] = int(value)
                    elif 'latency' in metric_name:
                        metrics['network']['latency_ms'] = value

        return metrics

    def save_quantum_metrics(self, timestamp, server, metrics):
        """Сохранение квантовых метрик"""
        cursor = self.db.cursor()
        cursor.execute('''
            INSERT OR REPLACE INTO quantum_metrics
            (timestamp, server, session_id, qber, entropy, key_strength, quantum_errors)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (
            timestamp, server, metrics.get('session_id', 'unknown'),
            metrics.get('qber', 0), metrics.get('entropy', 0),
            metrics.get('key_strength', 0), metrics.get('quantum_errors', 0)
        ))
        self.db.commit()

    def save_network_metrics(self, timestamp, server, metrics):
        """Сохранение сетевых метрик"""
        cursor = self.db.cursor()
        cursor.execute('''
            INSERT OR REPLACE INTO network_metrics
            (timestamp, server, packets_sent, packets_received, bytes_sent, bytes_received,
             latency_ms, jitter_ms, packet_loss)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            timestamp, server,
            metrics.get('packets_sent', 0), metrics.get('packets_received', 0),
            metrics.get('bytes_sent', 0), metrics.get('bytes_received', 0),
            metrics.get('latency_ms', 0), 0, 0  # jitter and loss need calculation
        ))
        self.db.commit()

    def save_system_metrics(self, timestamp, server, metrics):
        """Сохранение системных метрик"""
        cursor = self.db.cursor()
        cursor.execute('''
            INSERT OR REPLACE INTO system_metrics
            (timestamp, server, cpu_percent, memory_percent, disk_usage, network_io, active_connections)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (
            timestamp, server,
            metrics.get('cpu_percent', 0), metrics.get('memory_percent', 0),
            metrics.get('disk_usage', 0), metrics.get('network_io', 0),
            metrics.get('active_connections', 0)
        ))
        self.db.commit()

    def get_system_metrics(self):
        """Получение системных метрик локального сервера"""
        metrics = {}

        # CPU
        metrics['cpu_percent'] = psutil.cpu_percent(interval=1)

        # Memory
        memory = psutil.virtual_memory()
        metrics['memory_percent'] = memory.percent

        # Disk
        disk = psutil.disk_usage('/')
        metrics['disk_usage'] = disk.percent

        # Network I/O
        network = psutil.net_io_counters()
        metrics['network_io'] = network.bytes_sent + network.bytes_recv

        # Active connections
        connections = psutil.net_connections()
        metrics['active_connections'] = len(connections)

        return metrics

    def check_alerts(self):
        """Проверка условий для алертов"""
        alerts = []

        # Проверяем последние метрики
        if self.metrics['quantum']:
            latest = self.metrics['quantum'][-1]

            # QBER алерт
            if latest.get('qber', 0) > self.config['alerts']['qber_threshold']:
                alerts.append({
                    'type': 'quantum',
                    'severity': 'high',
                    'message': f"QBER превысил порог: {latest['qber']:.3f} > {self.config['alerts']['qber_threshold']}",
                    'server': latest.get('server', 'unknown')
                })

        # Системные алерты
        if self.metrics['system']:
            latest = self.metrics['system'][-1]

            if latest.get('cpu_percent', 0) > self.config['alerts']['cpu_threshold']:
                alerts.append({
                    'type': 'system',
                    'severity': 'medium',
                    'message': f"Высокая нагрузка CPU: {latest['cpu_percent']:.1f}%",
                    'server': latest.get('server', 'unknown')
                })

            if latest.get('memory_percent', 0) > self.config['alerts']['memory_threshold']:
                alerts.append({
                    'type': 'system',
                    'severity': 'high',
                    'message': f"Высокое использование памяти: {latest['memory_percent']:.1f}%",
                    'server': latest.get('server', 'unknown')
                })

        # Сохраняем алерты
        if alerts:
            self.save_alerts(alerts)

        return alerts

    def save_alerts(self, alerts):
        """Сохранение алертов в базу данных"""
        cursor = self.db.cursor()
        timestamp = int(time.time())

        for alert in alerts:
            cursor.execute('''
                INSERT INTO alerts (timestamp, server, alert_type, severity, message)
                VALUES (?, ?, ?, ?, ?)
            ''', (timestamp, alert['server'], alert['type'], alert['severity'], alert['message']))

            # Отправляем webhook если настроен
            if self.config.get('webhook_url'):
                asyncio.create_task(self.send_webhook_alert(alert))

        self.db.commit()

    async def send_webhook_alert(self, alert):
        """Отправка алерта через webhook"""
        try:
            payload = {
                'timestamp': datetime.now().isoformat(),
                'alert': alert,
                'source': 'QuantumVPNMonitor'
            }

            async with aiohttp.ClientSession() as session:
                async with session.post(self.config['webhook_url'], json=payload) as response:
                    if response.status == 200:
                        self.logger.info(f"Alert sent to webhook: {alert['message']}")
                    else:
                        self.logger.error(f"Failed to send webhook alert: HTTP {response.status}")

        except Exception as e:
            self.logger.error(f"Error sending webhook alert: {e}")

    def generate_report(self):
        """Генерация отчета о состоянии системы"""
        report = {
            'timestamp': datetime.now().isoformat(),
            'quantum_status': self.analyze_quantum_status(),
            'network_status': self.analyze_network_status(),
            'system_status': self.analyze_system_status(),
            'recent_alerts': self.get_recent_alerts(),
            'recommendations': self.generate_recommendations()
        }

        # Сохраняем отчет
        with open('monitoring/daily_report.json', 'w') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)

        return report

    def analyze_quantum_status(self):
        """Анализ состояния квантовых компонентов"""
        if not self.metrics['quantum']:
            return {'status': 'no_data'}

        recent = list(self.metrics['quantum'])[-10:]  # Последние 10 записей

        avg_qber = sum(m.get('qber', 0) for m in recent) / len(recent)
        avg_entropy = sum(m.get('entropy', 0) for m in recent) / len(recent)

        status = 'healthy'
        if avg_qber > 0.08:
            status = 'warning'
        if avg_qber > 0.11:
            status = 'critical'

        return {
            'status': status,
            'avg_qber': round(avg_qber, 4),
            'avg_entropy': round(avg_entropy, 2),
            'sessions_count': len(set(m.get('session_id', 'unknown') for m in recent))
        }

    def analyze_network_status(self):
        """Анализ сетевых метрик"""
        if not self.metrics['network']:
            return {'status': 'no_data'}

        recent = list(self.metrics['network'])[-10:]

        avg_latency = sum(m.get('latency_ms', 0) for m in recent) / len(recent)
        total_packets = sum(m.get('packets_sent', 0) + m.get('packets_received', 0) for m in recent)

        status = 'healthy'
        if avg_latency > 50:
            status = 'warning'
        if avg_latency > 100:
            status = 'critical'

        return {
            'status': status,
            'avg_latency_ms': round(avg_latency, 1),
            'total_packets': total_packets,
            'packet_rate': round(total_packets / 10, 1)  # packets per measurement interval
        }

    def analyze_system_status(self):
        """Анализ системных метрик"""
        if not self.metrics['system']:
            return {'status': 'no_data'}

        recent = list(self.metrics['system'])[-5:]

        avg_cpu = sum(m.get('cpu_percent', 0) for m in recent) / len(recent)
        avg_memory = sum(m.get('memory_percent', 0) for m in recent) / len(recent)

        status = 'healthy'
        if avg_cpu > 70 or avg_memory > 80:
            status = 'warning'
        if avg_cpu > 90 or avg_memory > 95:
            status = 'critical'

        return {
            'status': status,
            'avg_cpu_percent': round(avg_cpu, 1),
            'avg_memory_percent': round(avg_memory, 1),
            'active_connections': recent[-1].get('active_connections', 0)
        }

    def get_recent_alerts(self, limit=10):
        """Получение последних алертов"""
        cursor = self.db.cursor()
        cursor.execute('''
            SELECT timestamp, server, alert_type, severity, message
            FROM alerts
            ORDER BY timestamp DESC
            LIMIT ?
        ''', (limit,))

        return [
            {
                'timestamp': datetime.fromtimestamp(row[0]).isoformat(),
                'server': row[1],
                'type': row[2],
                'severity': row[3],
                'message': row[4]
            }
            for row in cursor.fetchall()
        ]

    def generate_recommendations(self):
        """Генерация рекомендаций"""
        recommendations = []

        quantum_status = self.analyze_quantum_status()
        if quantum_status['status'] == 'critical':
            recommendations.append({
                'type': 'security',
                'priority': 'high',
                'message': 'Критический уровень QBER. Рекомендуется проверить целостность квантового канала.'
            })

        network_status = self.analyze_network_status()
        if network_status['status'] == 'critical':
            recommendations.append({
                'type': 'performance',
                'priority': 'high',
                'message': 'Высокая задержка сети. Рекомендуется проверить качество соединения.'
            })

        system_status = self.analyze_system_status()
        if system_status['status'] == 'critical':
            recommendations.append({
                'type': 'system',
                'priority': 'high',
                'message': 'Критическая нагрузка системы. Рекомендуется оптимизировать ресурсы.'
            })

        if not recommendations:
            recommendations.append({
                'type': 'info',
                'priority': 'low',
                'message': 'Все системы работают в штатном режиме.'
            })

        return recommendations

    async def run_monitoring(self, interval=30):
        """Основной цикл мониторинга"""
        self.logger.info("Starting Quantum VPN monitoring...")

        try:
            while True:
                start_time = time.time()

                # Сбор метрик
                await self.collect_metrics()

                # Проверка алертов
                alerts = self.check_alerts()
                if alerts:
                    self.logger.warning(f"Generated {len(alerts)} alerts")

                # Генерация ежедневного отчета (каждый час)
                if datetime.now().minute == 0:
                    report = self.generate_report()
                    self.logger.info("Daily report generated")

                # Ожидание следующего цикла
                elapsed = time.time() - start_time
                sleep_time = max(0, interval - elapsed)
                await asyncio.sleep(sleep_time)

        except KeyboardInterrupt:
            self.logger.info("Monitoring stopped by user")
        except Exception as e:
            self.logger.error(f"Monitoring error: {e}")
        finally:
            self.db.close()

    def start_web_dashboard(self, port=8080):
        """Запуск веб-панели мониторинга"""
        from http.server import HTTPServer, SimpleHTTPRequestHandler
        import threading

        class DashboardHandler(SimpleHTTPRequestHandler):
            def __init__(self, *args, monitor=None, **kwargs):
                self.monitor = monitor
                super().__init__(*args, **kwargs)

        def run_server():
            handler = DashboardHandler
            handler.monitor = self
            httpd = HTTPServer(('', port), handler)
            httpd.serve_forever()

        # Создаем HTML dashboard
        self.create_dashboard_html()

        # Запускаем сервер
        server_thread = threading.Thread(target=run_server, daemon=True)
        server_thread.start()
        self.logger.info(f"Web dashboard started at http://localhost:{port}")

    def create_dashboard_html(self):
        """Создание HTML панели мониторинга"""
        html_content = '''
        <!DOCTYPE html>
        <html>
        <head>
            <title>Quantum VPN Monitor</title>
            <meta charset="utf-8">
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <style>
                body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
                .container { max-width: 1200px; margin: 0 auto; }
                .card { background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin: 20px 0; padding: 20px; }
                .metric { display: inline-block; margin: 10px 20px; text-align: center; }
                .metric-value { font-size: 2em; font-weight: bold; color: #007bff; }
                .metric-label { color: #666; margin-top: 5px; }
                .status-healthy { color: #28a745; }
                .status-warning { color: #ffc107; }
                .status-critical { color: #dc3545; }
                .alert { background: #fff3cd; border: 1px solid #ffeaa7; border-radius: 4px; padding: 10px; margin: 10px 0; }
                .chart-container { width: 100%; height: 300px; margin: 20px 0; }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>Quantum VPN Monitor Dashboard</h1>

                <div class="card">
                    <h2>Статус системы</h2>
                    <div id="system-status">
                        <!-- Статус будет загружен динамически -->
                    </div>
                </div>

                <div class="card">
                    <h2>Квантовые метрики</h2>
                    <div id="quantum-metrics">
                        <!-- Метрики будут загружены динамически -->
                    </div>
                </div>

                <div class="card">
                    <h2>Сетевые метрики</h2>
                    <div id="network-metrics">
                        <!-- Метрики будут загружены динамически -->
                    </div>
                </div>

                <div class="card">
                    <h2>Алерты</h2>
                    <div id="alerts">
                        <!-- Алерты будут загружены динамически -->
                    </div>
                </div>
            </div>

            <script>
                // Загрузка данных с сервера мониторинга
                async function loadData() {
                    try {
                        const response = await fetch('/api/status');
                        const data = await response.json();

                        updateSystemStatus(data.system_status);
                        updateQuantumMetrics(data.quantum_status);
                        updateNetworkMetrics(data.network_status);
                        updateAlerts(data.alerts);

                    } catch (error) {
                        console.error('Error loading data:', error);
                    }
                }

                function updateSystemStatus(status) {
                    const container = document.getElementById('system-status');
                    container.innerHTML = `
                        <div class="metric">
                            <div class="metric-value status-${status.status}">${status.status}</div>
                            <div class="metric-label">Системный статус</div>
                        </div>
                        <div class="metric">
                            <div class="metric-value">${status.avg_cpu_percent}%</div>
                            <div class="metric-label">CPU</div>
                        </div>
                        <div class="metric">
                            <div class="metric-value">${status.avg_memory_percent}%</div>
                            <div class="metric-label">Память</div>
                        </div>
                    `;
                }

                function updateQuantumMetrics(metrics) {
                    const container = document.getElementById('quantum-metrics');
                    container.innerHTML = `
                        <div class="metric">
                            <div class="metric-value status-${metrics.status}">${metrics.status}</div>
                            <div class="metric-label">Квантовый статус</div>
                        </div>
                        <div class="metric">
                            <div class="metric-value">${metrics.avg_qber}</div>
                            <div class="metric-label">Средний QBER</div>
                        </div>
                        <div class="metric">
                            <div class="metric-value">${metrics.avg_entropy}</div>
                            <div class="metric-label">Энтропия</div>
                        </div>
                    `;
                }

                function updateNetworkMetrics(metrics) {
                    const container = document.getElementById('network-metrics');
                    container.innerHTML = `
                        <div class="metric">
                            <div class="metric-value status-${metrics.status}">${metrics.status}</div>
                            <div class="metric-label">Сетевой статус</div>
                        </div>
                        <div class="metric">
                            <div class="metric-value">${metrics.avg_latency_ms}ms</div>
                            <div class="metric-label">Задержка</div>
                        </div>
                        <div class="metric">
                            <div class="metric-value">${metrics.packet_rate}</div>
                            <div class="metric-label">Пакетов/с</div>
                        </div>
                    `;
                }

                function updateAlerts(alerts) {
                    const container = document.getElementById('alerts');
                    container.innerHTML = alerts.map(alert => `
                        <div class="alert">
                            <strong>${alert.severity.toUpperCase()}</strong>: ${alert.message}
                            <br><small>${alert.timestamp} - ${alert.server}</small>
                        </div>
                    `).join('');
                }

                // Обновление каждые 30 секунд
                setInterval(loadData, 30000);
                loadData(); // Загрузка при старте
            </script>
        </body>
        </html>
        '''

        os.makedirs('monitoring', exist_ok=True)
        with open('monitoring/dashboard.html', 'w') as f:
            f.write(html_content)

def main():
    parser = argparse.ArgumentParser(description='Quantum VPN Monitor')
    parser.add_argument('--config', default='configs/monitoring_config.json',
                       help='Configuration file')
    parser.add_argument('--interval', type=int, default=30,
                       help='Monitoring interval in seconds')
    parser.add_argument('--web-port', type=int, default=8080,
                       help='Web dashboard port')

    args = parser.parse_args()

    monitor = QuantumVPNMonitor(args.config)
    monitor.start_web_dashboard(args.web_port)

    asyncio.run(monitor.run_monitoring(args.interval))

if __name__ == '__main__':
    main()

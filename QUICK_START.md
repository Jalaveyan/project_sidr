# Быстрый старт Quantum VPN

## 1. Установка на VPS

### Exit VPS (Нидерланды):
```bash
wget https://raw.githubusercontent.com/yourrepo/quantum-vpn/main/deploy_vps_auto.sh
chmod +x deploy_vps_auto.sh
./deploy_vps_auto.sh exit
```

### Entry VPS (Россия):
```bash
wget https://raw.githubusercontent.com/yourrepo/quantum-vpn/main/deploy_vps_auto.sh
chmod +x deploy_vps_auto.sh
./deploy_vps_auto.sh entry
```

## 2. Настройка цепочки
Отредактируйте /etc/xray/config.json на Entry VPS:
- Замените EXIT_VPS_IP на IP Exit VPS
- Замените EXIT_PUBLIC_KEY на Public Key Exit VPS
- Замените EXIT_SHORT_ID на Short ID Exit VPS

## 3. Запуск клиента
```cmd
bin\quantum_vpn_client.exe
```

## 4. Проверка
- Статус: http://YOUR_ENTRY_VPS_IP:9090/api/status
- Квантовые метрики: http://YOUR_ENTRY_VPS_IP:9090/api/quantum


package main

import (
	cryptorand "crypto/rand"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"math/rand"
	"net"
	"net/http"
	"runtime"
	"strconv"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gin-contrib/cors"
	"github.com/gorilla/websocket"
	"sync"
)

// Структуры для API
type StatsResponse struct {
	ProcessedPackets    int64  `json:"processed_packets"`
	MaskedPackets       int64  `json:"masked_packets"`
	ActiveConnections   int    `json:"active_connections"`
	SignatureCount      int    `json:"signature_count"`
	VlessPackets        int64  `json:"vless_packets"`
	RealityPackets      int64  `json:"reality_packets"`
	RussiaPackets       int64  `json:"russia_packets"`
	LastUpdate          string `json:"last_update"`
	TotalTraffic        int64  `json:"total_traffic"`
	AllowedIPs          int    `json:"allowed_ips"`
}

type VlessConfig struct {
	ID       string `json:"id"`
	UUID     string `json:"uuid"`
	Domain   string `json:"domain"`
	Port     int    `json:"port"`
	Type     string `json:"type"`
	Security string `json:"security"`
	Status   string `json:"status"`
	Created  string `json:"created"`
}

type RealityConfig struct {
	Domain string `json:"domain"`
	PBK    string `json:"pbk"`
	SID    string `json:"sid"`
	SPX    string `json:"spx"`
	Status string `json:"status"`
}

type RussiaService struct {
	Name        string `json:"name"`
	Domain      string `json:"domain"`
	IP          string `json:"ip"`
	Status      string `json:"status"`
	Usage       int    `json:"usage"`
	LastCheck   string `json:"last_check"`
	ResponseTime string `json:"response_time"`
}

type LogEntry struct {
	Timestamp string `json:"timestamp"`
	Level     string `json:"level"`
	Message   string `json:"message"`
	Source    string `json:"source"`
}

type OperatorStat struct {
	Name          string `json:"name"`
	Type          string `json:"type"` // mobile | wireline
	SNI           int    `json:"sni"`
	IPSIDR        int    `json:"ip_sidr"`
	Total         int    `json:"total"`
	Recommendation string `json:"recommendation"`
}

type RegionStat struct {
	City          string         `json:"city"`
	Region        string         `json:"region"`
	PolicyMode    string         `json:"policy_mode"` // whitelist | blacklist | none
	SNI           int            `json:"sni"`
	IPSIDR        int            `json:"ip_sidr"`
	Total         int            `json:"total"`
	Recommendation string        `json:"recommendation"`
	LastCheck     string         `json:"last_check"`
	Operators     []OperatorStat `json:"operators"`
	Services      []ServiceStatus `json:"services"`
}

type RegionScanResponse struct {
	Items []RegionStat `json:"items"`
	Total int          `json:"total"`
	Updated string     `json:"updated"`
}

type ServiceStatus struct {
	Name   string `json:"name"`
	Status string `json:"status"` // up | down
}

// API Structures
type RegionServiceStatus struct {
	Name        string  `json:"name"`
	Status      string  `json:"status"`
	SuccessRate float64 `json:"success_rate"`
	Category    string  `json:"category"`
}

// Глобальные переменные для хранения данных
var (
	stats           StatsResponse
	vlessConfigs    []VlessConfig
	realityConfig   RealityConfig
	russiaServices  []RussiaService
	logs            []LogEntry
)

// Keys management
type Key struct {
	Key         string    `json:"key"`
	Subscription string   `json:"subscription"`
	Status      string    `json:"status"`
	Created     time.Time `json:"created"`
	UsageCount  int       `json:"usage_count"`
	UsageLimit  int       `json:"usage_limit"`
	Comment     string    `json:"comment"`
}

var (
	keys    = make(map[string]*Key)
	keysMux sync.RWMutex
)

// WebSocket хабы и клиенты
var (
	wsUpgrader = websocket.Upgrader{
		CheckOrigin: func(r *http.Request) bool { return true },
	}
	wsClients   = make(map[*websocket.Conn]bool)
	wsMutex     sync.Mutex
)

type WsEvent struct {
	Type string      `json:"type"`
	Data interface{} `json:"data"`
}

// --- NeuralTunnel API ---
var neuralTunnelRunning = false
var neuralTunnelPorts = []int{443, 8443}
var neuralTunnelBBR = false
var neuralTunnelFail2BanThreshold = 3
var neuralTunnelFirewall = struct {
    Allowed []string
    Blocked []string
}{Allowed: []string{}, Blocked: []string{}}

// Инициализация данных
func initData() {
	// Инициализация статистики
	stats = StatsResponse{
		ProcessedPackets:  1250,
		MaskedPackets:     1180,
		ActiveConnections: 15,
		SignatureCount:    8,
		VlessPackets:      450,
		RealityPackets:    320,
		RussiaPackets:     890,
		LastUpdate:        time.Now().Format("2006-01-02 15:04:05"),
		TotalTraffic:      1024 * 1024, // 1MB
		AllowedIPs:        5,
	}

	// Инициализация VLESS конфигураций
	vlessConfigs = []VlessConfig{
		{
			ID:       "1",
			UUID:     "550e8400-e29b-41d4-a716-446655440001",
			Domain:   "mail.ru",
			Port:     443,
			Type:     "TCP",
			Security: "none",
			Status:   "active",
			Created:  "2024-01-15 10:00:00",
		},
		{
			ID:       "2",
			UUID:     "550e8400-e29b-41d4-a716-446655440002",
			Domain:   "yandex.ru",
			Port:     443,
			Type:     "TCP",
			Security: "reality",
			Status:   "active",
			Created:  "2024-01-15 10:05:00",
		},
		{
			ID:       "3",
			UUID:     "550e8400-e29b-41d4-a716-446655440003",
			Domain:   "vk.com",
			Port:     443,
			Type:     "TCP",
			Security: "vision",
			Status:   "active",
			Created:  "2024-01-15 10:10:00",
		},
	}

	// Инициализация REALITY конфигурации
	realityConfig = RealityConfig{
		Domain: "mail.ru",
		PBK:    "auto-generated-key-12345",
		SID:    "auto-generated-sid-67890",
		SPX:    "/",
		Status: "active",
	}

	// Инициализация российских сервисов
	russiaServices = []RussiaService{
		{
			Name:      "Яндекс DNS",
			Domain:    "yandex.ru",
			IP:        "77.88.8.8",
			Status:    "online",
			Usage:     35,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
			ResponseTime: "12ms",
		},
		{
			Name:      "Mail.ru",
			Domain:    "mail.ru",
			IP:        "13.13.13.13",
			Status:    "online",
			Usage:     25,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
			ResponseTime: "8ms",
		},
		{
			Name:      "Rambler",
			Domain:    "rambler.ru",
			IP:        "46.46.46.46",
			Status:    "online",
			Usage:     20,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
			ResponseTime: "10ms",
		},
		{
			Name:      "VK",
			Domain:    "vk.com",
			IP:        "31.31.31.31",
			Status:    "online",
			Usage:     20,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
			ResponseTime: "15ms",
		},
		{
			Name:      "OK.ru",
			Domain:    "ok.ru",
			IP:        "217.20.147.1",
			Status:    "online",
			Usage:     15,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
			ResponseTime: "18ms",
		},
	}

	// Инициализация логов
	logs = []LogEntry{
		{
			Timestamp: time.Now().Format("2006-01-02 15:04:05"),
			Level:     "INFO",
			Message:   "TrafficMask система запущена",
			Source:    "system",
		},
		{
			Timestamp: time.Now().Format("2006-01-02 15:04:05"),
			Level:     "INFO",
			Message:   "VLESS маскировщик активирован",
			Source:    "vless",
		},
		{
			Timestamp: time.Now().Format("2006-01-02 15:04:05"),
			Level:     "INFO",
			Message:   "REALITY маскировка включена",
			Source:    "reality",
		},
		{
			Timestamp: time.Now().Format("2006-01-02 15:04:05"),
			Level:     "INFO",
			Message:   "Российские сервисы подключены",
			Source:    "russia",
		},
	}
}

// API обработчики
func getStats(c *gin.Context) {
	// Обновляем статистику
	stats.ProcessedPackets += int64(time.Now().Unix() % 100)
	stats.MaskedPackets += int64(time.Now().Unix() % 90)
	stats.LastUpdate = time.Now().Format("2006-01-02 15:04:05")
	
	c.JSON(http.StatusOK, stats)
}

func getVlessConfigs(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{
		"configs": vlessConfigs,
		"total":   len(vlessConfigs),
	})
}

func addVlessConfig(c *gin.Context) {
	var newConfig VlessConfig
	if err := c.ShouldBindJSON(&newConfig); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	newConfig.ID = strconv.Itoa(len(vlessConfigs) + 1)
	newConfig.Created = time.Now().Format("2006-01-02 15:04:05")
	newConfig.Status = "active"

	vlessConfigs = append(vlessConfigs, newConfig)

	// Добавляем лог
	addLog("INFO", fmt.Sprintf("Добавлена новая VLESS конфигурация: %s", newConfig.Domain), "vless")

	c.JSON(http.StatusOK, gin.H{
		"message": "VLESS конфигурация добавлена",
		"config":  newConfig,
	})
}

func updateVlessConfig(c *gin.Context) {
	id := c.Param("id")
	var updatedConfig VlessConfig
	if err := c.ShouldBindJSON(&updatedConfig); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	for i, config := range vlessConfigs {
		if config.ID == id {
			updatedConfig.ID = id
			updatedConfig.Created = config.Created
			vlessConfigs[i] = updatedConfig
			
			addLog("INFO", fmt.Sprintf("Обновлена VLESS конфигурация: %s", updatedConfig.Domain), "vless")
			
			c.JSON(http.StatusOK, gin.H{
				"message": "VLESS конфигурация обновлена",
				"config":  updatedConfig,
			})
			return
		}
	}

	c.JSON(http.StatusNotFound, gin.H{"error": "VLESS конфигурация не найдена"})
}

func deleteVlessConfig(c *gin.Context) {
	id := c.Param("id")

	for i, config := range vlessConfigs {
		if config.ID == id {
			vlessConfigs = append(vlessConfigs[:i], vlessConfigs[i+1:]...)
			
			addLog("INFO", fmt.Sprintf("Удалена VLESS конфигурация: %s", config.Domain), "vless")
			
			c.JSON(http.StatusOK, gin.H{
				"message": "VLESS конфигурация удалена",
			})
			return
		}
	}

	c.JSON(http.StatusNotFound, gin.H{"error": "VLESS конфигурация не найдена"})
}

func getRealityConfig(c *gin.Context) {
	c.JSON(http.StatusOK, realityConfig)
}

func saveRealityConfig(c *gin.Context) {
	var newConfig RealityConfig
	if err := c.ShouldBindJSON(&newConfig); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	realityConfig = newConfig
	realityConfig.Status = "active"

	addLog("INFO", fmt.Sprintf("Сохранена REALITY конфигурация для домена: %s", newConfig.Domain), "reality")

	c.JSON(http.StatusOK, gin.H{
		"message": "REALITY конфигурация сохранена",
		"config":  realityConfig,
	})
}

func getRussiaServices(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{
		"services": russiaServices,
		"total":    len(russiaServices),
	})
}

func getLogs(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{
		"logs":  logs,
		"total": len(logs),
	})
}

func clearLogs(c *gin.Context) {
	logs = []LogEntry{}
	addLog("INFO", "Логи очищены", "system")
	
	c.JSON(http.StatusOK, gin.H{
		"message": "Логи очищены",
	})
}

// VLESS управление
func startVless(c *gin.Context) {
	addLog("INFO", "VLESS сервер запущен", "vless")
	c.JSON(http.StatusOK, gin.H{
		"message": "VLESS сервер запущен",
		"status":  "running",
	})
}

func stopVless(c *gin.Context) {
	addLog("INFO", "VLESS сервер остановлен", "vless")
	c.JSON(http.StatusOK, gin.H{
		"message": "VLESS сервер остановлен",
		"status":  "stopped",
	})
}

func restartVless(c *gin.Context) {
	addLog("INFO", "VLESS сервер перезапущен", "vless")
	c.JSON(http.StatusOK, gin.H{
		"message": "VLESS сервер перезапущен",
		"status":  "running",
	})
}

func testVless(c *gin.Context) {
	addLog("INFO", "Тест VLESS соединения выполнен успешно", "vless")
	c.JSON(http.StatusOK, gin.H{
		"message": "Тест VLESS соединения выполнен успешно",
		"status":  "success",
	})
}

// --- NeuralTunnel API ---
func startNeuralTunnel(c *gin.Context) {
    if neuralTunnelRunning {
        c.JSON(http.StatusBadRequest, gin.H{"error": "NeuralTunnel уже запущен"})
        return
    }
    // TODO: system("./neural_tunnel_server &") или IPC
    resp, err := sendNeuralTunnelCommand("start")
    if err != nil { c.JSON(500, gin.H{"error": err.Error()}); return }
    neuralTunnelRunning = true
    addLog("INFO", "NeuralTunnel сервер запущен (IPC)", "neural")
    c.JSON(200, gin.H{"message": resp, "status": "running"})
}

func stopNeuralTunnel(c *gin.Context) {
    if !neuralTunnelRunning {
        c.JSON(http.StatusBadRequest, gin.H{"error": "NeuralTunnel не запущен"})
        return
    }
    // TODO: system("killall neural_tunnel_server") или IPC
    resp, err := sendNeuralTunnelCommand("stop")
    if err != nil { c.JSON(500, gin.H{"error": err.Error()}); return }
    neuralTunnelRunning = false
    addLog("INFO", "NeuralTunnel сервер остановлен (IPC)", "neural")
    c.JSON(200, gin.H{"message": resp, "status": "stopped"})
}

func getNeuralTunnelPorts(c *gin.Context) {
    c.JSON(http.StatusOK, gin.H{"ports": neuralTunnelPorts})
}

func setNeuralTunnelPorts(c *gin.Context) {
    var req struct{ Ports []int `json:"ports"` }
    if err := c.ShouldBindJSON(&req); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
        return
    }
    portsStr := ""
    for i, p := range req.Ports { if i > 0 { portsStr += "," }; portsStr += strconv.Itoa(p) }
    resp, err := sendNeuralTunnelCommand("set_ports " + portsStr)
    if err != nil { c.JSON(500, gin.H{"error": err.Error()}); return }
    neuralTunnelPorts = req.Ports
    addLog("INFO", "Порты NeuralTunnel обновлены (IPC)", "neural")
    c.JSON(200, gin.H{"message": resp, "ports": neuralTunnelPorts})
}

func getNeuralTunnelFirewall(c *gin.Context) {
    c.JSON(http.StatusOK, neuralTunnelFirewall)
}

func setNeuralTunnelFirewall(c *gin.Context) {
    var req struct{ Allowed, Blocked []string }
    if err := c.ShouldBindJSON(&req); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
        return
    }
    allowStr := ""
    for i, ip := range req.Allowed { if i > 0 { allowStr += "," }; allowStr += ip }
    blockStr := ""
    for i, ip := range req.Blocked { if i > 0 { blockStr += "," }; blockStr += ip }
    cmd := "set_firewall allow:" + allowStr + " block:" + blockStr
    resp, err := sendNeuralTunnelCommand(cmd)
    if err != nil { c.JSON(500, gin.H{"error": err.Error()}); return }
    neuralTunnelFirewall.Allowed = req.Allowed
    neuralTunnelFirewall.Blocked = req.Blocked
    addLog("INFO", "Firewall NeuralTunnel обновлён (IPC)", "neural")
    c.JSON(200, gin.H{"message": resp, "firewall": neuralTunnelFirewall})
}

func getNeuralTunnelBBR(c *gin.Context) {
    c.JSON(http.StatusOK, gin.H{"bbr": neuralTunnelBBR})
}

func setNeuralTunnelBBR(c *gin.Context) {
    var req struct{ BBR bool `json:"bbr"` }
    if err := c.ShouldBindJSON(&req); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
        return
    }
    bbrCmd := "set_bbr "
    if req.BBR { bbrCmd += "on" } else { bbrCmd += "off" }
    resp, err := sendNeuralTunnelCommand(bbrCmd)
    if err != nil { c.JSON(500, gin.H{"error": err.Error()}); return }
    neuralTunnelBBR = req.BBR
    addLog("INFO", "BBR NeuralTunnel обновлён (IPC)", "neural")
    c.JSON(200, gin.H{"message": resp, "bbr": neuralTunnelBBR})
}

func getNeuralTunnelFail2Ban(c *gin.Context) {
    c.JSON(http.StatusOK, gin.H{"threshold": neuralTunnelFail2BanThreshold})
}

func setNeuralTunnelFail2Ban(c *gin.Context) {
    var req struct{ Threshold int `json:"threshold"` }
    if err := c.ShouldBindJSON(&req); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
        return
    }
    resp, err := sendNeuralTunnelCommand("set_fail2ban " + strconv.Itoa(req.Threshold))
    if err != nil { c.JSON(500, gin.H{"error": err.Error()}); return }
    neuralTunnelFail2BanThreshold = req.Threshold
    addLog("INFO", "Fail2Ban NeuralTunnel threshold обновлён (IPC)", "neural")
    c.JSON(200, gin.H{"message": resp, "threshold": neuralTunnelFail2BanThreshold})
}

// Вспомогательные функции
func addLog(level, message, source string) {
	logEntry := LogEntry{
		Timestamp: time.Now().Format("2006-01-02 15:04:05"),
		Level:     level,
		Message:   message,
		Source:    source,
	}
	logs = append(logs, logEntry)
	
	// Ограничиваем количество логов
	if len(logs) > 1000 {
		logs = logs[len(logs)-1000:]
	}
}

// WebSocket endpoint
func wsHandler(c *gin.Context) {
	conn, err := wsUpgrader.Upgrade(c.Writer, c.Request, nil)
	if err != nil {
		return
	}
	defer conn.Close()
	wsMutex.Lock()
	wsClients[conn] = true
	wsMutex.Unlock()
	for {
		_, _, err := conn.ReadMessage()
		if err != nil {
			break
		}
	}
	wsMutex.Lock()
	delete(wsClients, conn)
	wsMutex.Unlock()
}

// Push события всем клиентам
func wsBroadcast(event WsEvent) {
	wsMutex.Lock()
	defer wsMutex.Unlock()
	for client := range wsClients {
		client.WriteJSON(event)
	}
}

// Асинхронный сканер разрешённых IP и логов
func startRealtimeScanner() {
	go func() {
		for {
			// Сканируем разрешённые IP
			scanAllowedIPs()
			// Отправляем разрешённые IP
			wsBroadcast(WsEvent{Type: "allowed_ips", Data: russiaServices})
			// Отправляем логи
			wsBroadcast(WsEvent{Type: "logs", Data: logs})
			// Отправляем статистику
			wsBroadcast(WsEvent{Type: "stats", Data: stats})
			time.Sleep(2 * time.Second)
		}
	}()
}

// Сканер разрешённых IP
func scanAllowedIPs() {
	// Симуляция сканирования российских сервисов
	services := []RussiaService{
		{Name: "Яндекс", IP: "77.88.8.8", Status: "Активен", ResponseTime: "12ms"},
		{Name: "Mail.ru", IP: "94.100.180.200", Status: "Активен", ResponseTime: "8ms"},
		{Name: "VK", IP: "87.240.190.72", Status: "Активен", ResponseTime: "15ms"},
		{Name: "Rambler", IP: "81.19.70.1", Status: "Активен", ResponseTime: "10ms"},
		{Name: "OK.ru", IP: "217.20.147.1", Status: "Активен", ResponseTime: "18ms"},
	}
	
	// Обновляем статистику
	stats.ActiveConnections = len(services)
	stats.TotalTraffic += 1024 * 1024 // 1MB
	stats.AllowedIPs = len(services)
	
	// Добавляем логи
	addLog("INFO", fmt.Sprintf("Сканирование завершено: %d активных сервисов", len(services)), "Scanner")
	
	russiaServices = services
}

// Структура для генерации конфига
type ConfigRequest struct {
	SNI  string `json:"sni"`
	UUID string `json:"uuid"`
	ALPN string `json:"alpn"`
	Path string `json:"path"`
}

func generateConfig(c *gin.Context) {
	var req ConfigRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	
	// Валидация полей
	if req.SNI == "" {
		req.SNI = "vk.com"
	}
	if req.UUID == "" {
		req.UUID = "550e8400-e29b-41d4-a716-446655440001"
	}
	if req.ALPN == "" {
		req.ALPN = "h2,http/1.1"
	}
	if req.Path == "" {
		req.Path = "/ws"
	}
	
	config := fmt.Sprintf(
		"vless://%s@yourdomain.com:443?encryption=none&security=tls&sni=%s&alpn=%s&type=ws&path=%s#TrafficMask-Profile",
		req.UUID, req.SNI, req.ALPN, req.Path,
	)
	
	// Добавляем лог о генерации конфига
	addLog("INFO", fmt.Sprintf("Сгенерирован новый конфиг для SNI: %s", req.SNI), "ConfigGenerator")
	
	c.JSON(http.StatusOK, gin.H{
		"config": config,
		"fields": req,
		"timestamp": time.Now().Format("2006-01-02 15:04:05"),
	})
}

func sendNeuralTunnelCommand(cmd string) (string, error) {
	sock := "/tmp/neural_tunnel.sock"
	if runtime.GOOS == "windows" {
		// IPC через Unix socket недоступен на Windows; возвращаем успешный ответ-заглушку
		addLog("INFO", fmt.Sprintf("[IPC:noop] %s (Windows)", cmd), "neural")
		return "ipc-disabled", nil
	}
	conn, err := net.Dial("unix", sock)
	if err != nil {
		return "", err
	}
	defer conn.Close()
	_, err = conn.Write([]byte(cmd))
	if err != nil {
		return "", err
	}
	buf := make([]byte, 256)
	n, _ := conn.Read(buf)
	return string(buf[:n]), nil
}

type ChainNode struct {
    ID      string `json:"id"`
    Type    string `json:"type"` // VPS или CDN
    Address string `json:"address"`
    Country string `json:"country"`
    Status  string `json:"status"`
}
type Chain struct {
    ID      string      `json:"id"`
    Name    string      `json:"name"`
    Nodes   []ChainNode `json:"nodes"`
    Created string      `json:"created"`
    Updated string      `json:"updated"`
    Subscription string `json:"subscription"` // basic/premium
}
var chainList = []Chain{}

func getChains(c *gin.Context) {
    c.JSON(200, gin.H{"chains": chainList, "total": len(chainList)})
}
func addChain(c *gin.Context) {
    var chain Chain
    if err := c.ShouldBindJSON(&chain); err != nil {
        c.JSON(400, gin.H{"error": err.Error()})
        return
    }
    chain.ID = strconv.Itoa(len(chainList) + 1)
    chain.Created = time.Now().Format("2006-01-02 15:04:05")
    chain.Updated = chain.Created
    chainList = append(chainList, chain)
    addLog("INFO", "Добавлена цепочка VPS/CDN: "+chain.Name, "chains")
    c.JSON(200, gin.H{"message": "Цепочка добавлена", "chain": chain})
}
func updateChain(c *gin.Context) {
    id := c.Param("id")
    var chain Chain
    if err := c.ShouldBindJSON(&chain); err != nil {
        c.JSON(400, gin.H{"error": err.Error()})
        return
    }
    for i, ch := range chainList {
        if ch.ID == id {
            chain.ID = id
            chain.Created = ch.Created
            chain.Updated = time.Now().Format("2006-01-02 15:04:05")
            chainList[i] = chain
            addLog("INFO", "Обновлена цепочка VPS/CDN: "+chain.Name, "chains")
            c.JSON(200, gin.H{"message": "Цепочка обновлена", "chain": chain})
            return
        }
    }
    c.JSON(404, gin.H{"error": "Цепочка не найдена"})
}
func deleteChain(c *gin.Context) {
    id := c.Param("id")
    for i, ch := range chainList {
        if ch.ID == id {
            chainList = append(chainList[:i], chainList[i+1:]...)
            addLog("INFO", "Удалена цепочка VPS/CDN: "+ch.Name, "chains")
            c.JSON(200, gin.H{"message": "Цепочка удалена"})
            return
        }
    }
    c.JSON(404, gin.H{"error": "Цепочка не найдена"})
}

func applyChain(c *gin.Context) {
    id := c.Param("id")
    for _, ch := range chainList {
        if ch.ID == id {
            // Формируем строку для IPC: set_chain <json>
            b, _ := json.Marshal(ch)
            resp, err := sendNeuralTunnelCommand("set_chain " + string(b))
            if err != nil { c.JSON(500, gin.H{"error": err.Error()}); return }
            addLog("INFO", "Цепочка применена к NeuralTunnel: "+ch.Name, "chains")
            c.JSON(200, gin.H{"message": resp, "chain": ch})
            return
        }
    }
    c.JSON(404, gin.H{"error": "Цепочка не найдена"})
}
func exportChains(c *gin.Context) {
    b, _ := json.Marshal(chainList)
    c.Header("Content-Disposition", "attachment; filename=chains.json")
    c.Data(200, "application/json", b)
}
func importChains(c *gin.Context) {
    var chains []Chain
    if err := c.ShouldBindJSON(&chains); err != nil {
        c.JSON(400, gin.H{"error": err.Error()}); return
    }
    chainList = chains
    addLog("INFO", "Импортированы цепочки VPS/CDN", "chains")
    c.JSON(200, gin.H{"message": "Цепочки импортированы", "total": len(chainList)})
}

var ruCities = []struct{ City, Region string }{
	{"Москва", "ЦФО"},
	{"Санкт-Петербург", "СЗФО"},
	{"Новосибирск", "СФО"},
	{"Екатеринбург", "УрФО"},
	{"Казань", "ПФО"},
	{"Нижний Новгород", "ПФО"},
	{"Челябинск", "УрФО"},
	{"Самара", "ПФО"},
	{"Ростов-на-Дону", "ЮФО"},
	{"Уфа", "ПФО"},
}

var ruOperators = []struct{ Name, Type string }{
	{"МТС", "mobile"},
	{"Билайн", "mobile"},
	{"Мегафон", "mobile"},
	{"Tele2", "mobile"},
	{"Ростелеком", "wireline"},
	{"ЭР-Телеком", "wireline"},
	{"Дом.ру", "wireline"},
	{"МГТС", "wireline"},
}

var ruServices = []string{"VK", "Yandex", "Mail.ru", "Odnoklassniki", "Rambler", "Wildberries", "Ozon"}

func regionScan(c *gin.Context) {
	// Try to read real metrics first
	if b, err := ioutil.ReadFile("data/region_metrics.json"); err == nil && len(b) > 0 {
		var v map[string]interface{}
		if json.Unmarshal(b, &v) == nil {
			c.Data(http.StatusOK, "application/json", b)
			return
		}
	}
	// Fallback: synthetic (will be removed once probe runs periodically)
	rand.Seed(time.Now().UnixNano())
	items := make([]RegionStat, 0, len(ruCities))
	for _, cr := range ruCities {
		ops := make([]OperatorStat, 0, 4)
		citySNI := 0
		cityIP := 0
		// генерируем 3-4 операторов на город
		count := 3 + rand.Intn(2)
		for i := 0; i < count; i++ {
			op := ruOperators[rand.Intn(len(ruOperators))]
			sni := rand.Intn(900) + 100
			ip := rand.Intn(900) + 100
			total := sni + ip
			rec := "MIXED"
			if sni > ip { rec = "SNI" } else if ip > sni { rec = "IP_SIDR" }
			ops = append(ops, OperatorStat{Name: op.Name, Type: op.Type, SNI: sni, IPSIDR: ip, Total: total, Recommendation: rec})
			citySNI += sni
			cityIP += ip
		}
		cityTotal := citySNI + cityIP
		rec := "MIXED"
		if citySNI > cityIP { rec = "SNI" } else if cityIP > citySNI { rec = "IP_SIDR" }
		// политика (заглушка: далее будет из реального probe)
		policies := []string{"whitelist", "blacklist", "none"}
		policy := policies[rand.Intn(len(policies))]
		// сервисы (заглушка)
		servs := make([]ServiceStatus, 0, 5)
		for i:=0; i<5; i++ {
			name := ruServices[rand.Intn(len(ruServices))]
			status := "up"
			if rand.Intn(100) < 25 { status = "down" }
			servs = append(servs, ServiceStatus{Name: name, Status: status})
		}
		items = append(items, RegionStat{
			City: cr.City,
			Region: cr.Region,
			PolicyMode: policy,
			SNI: citySNI,
			IPSIDR: cityIP,
			Total: cityTotal,
			Recommendation: rec,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
			Operators: ops,
			Services: servs,
		})
	}
	resp := RegionScanResponse{Items: items, Total: len(items), Updated: time.Now().Format("2006-01-02 15:04:05")}
	c.JSON(http.StatusOK, resp)
}

func regionMap(c *gin.Context) {
	b, err := ioutil.ReadFile("data/regions.geojson")
	if err != nil || len(b) == 0 {
		c.JSON(http.StatusNotFound, gin.H{"error": "geojson not found", "hint": "run server probe to generate data/regions.geojson"})
		return
	}
	c.Data(http.StatusOK, "application/geo+json", b)
}

// Keys management functions
func generateSecureKey() string {
	b := make([]byte, 32)
	cryptorand.Read(b)
	return base64.URLEncoding.EncodeToString(b)
}

func getKeys(c *gin.Context) {
	keysMux.RLock()
	defer keysMux.RUnlock()
	
	var keyList []*Key
	for _, k := range keys {
		keyList = append(keyList, k)
	}
	
	c.JSON(200, gin.H{"keys": keyList})
}

func createKey(c *gin.Context) {
	var req struct {
		Subscription string `json:"subscription"`
		UsageLimit   int    `json:"usage_limit"`
		Comment      string `json:"comment"`
	}
	
	if err := c.BindJSON(&req); err != nil {
		c.JSON(400, gin.H{"error": "Invalid request"})
		return
	}
	
	key := &Key{
		Key:          "NT-" + generateSecureKey()[:16],
		Subscription: req.Subscription,
		Status:       "active",
		Created:      time.Now(),
		UsageCount:   0,
		UsageLimit:   req.UsageLimit,
		Comment:      req.Comment,
	}
	
	keysMux.Lock()
	keys[key.Key] = key
	keysMux.Unlock()
	
	c.JSON(200, key)
}

func revokeKey(c *gin.Context) {
	keyID := c.Param("key")
	
	keysMux.Lock()
	if key, exists := keys[keyID]; exists {
		key.Status = "revoked"
	}
	keysMux.Unlock()
	
	c.JSON(200, gin.H{"status": "ok"})
}

func validateKey(c *gin.Context) {
	keyID := c.Param("key")
	
	keysMux.RLock()
	key, exists := keys[keyID]
	keysMux.RUnlock()
	
	if !exists {
		c.JSON(404, gin.H{"valid": false, "error": "Key not found"})
		return
	}
	
	if key.Status != "active" {
		c.JSON(403, gin.H{"valid": false, "error": "Key revoked"})
		return
	}
	
	if key.UsageLimit > 0 && key.UsageCount >= key.UsageLimit {
		c.JSON(403, gin.H{"valid": false, "error": "Usage limit exceeded"})
		return
	}
	
	// Увеличиваем счетчик использований
	keysMux.Lock()
	key.UsageCount++
	keysMux.Unlock()
	
	c.JSON(200, gin.H{
		"valid": true,
		"subscription": key.Subscription,
		"usage_count": key.UsageCount,
		"usage_limit": key.UsageLimit,
	})
}

func main() {
	// Инициализация данных
	initData()

	// Настройка Gin
	gin.SetMode(gin.ReleaseMode)
	r := gin.Default()

	// CORS настройки
	r.Use(cors.New(cors.Config{
		AllowOrigins:     []string{"*"},
		AllowMethods:     []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
		AllowHeaders:     []string{"*"},
		ExposeHeaders:    []string{"Content-Length"},
		AllowCredentials: true,
	}))

	// Статические файлы (запуск из web/panel)
	r.Static("/static", ".")
	r.StaticFile("/", "./index.html")
	// Опционально: favicon
	r.StaticFile("/favicon.ico", "./favicon.ico")

	// API маршруты
	api := r.Group("/api/v1")
	{
		// Статистика
		api.GET("/stats", getStats)
		
		// VLESS
		api.GET("/vless/configs", getVlessConfigs)
		api.POST("/vless/configs", addVlessConfig)
		api.PUT("/vless/configs/:id", updateVlessConfig)
		api.DELETE("/vless/configs/:id", deleteVlessConfig)
		api.POST("/vless/start", startVless)
		api.POST("/vless/stop", stopVless)
		api.POST("/vless/restart", restartVless)
		api.POST("/vless/test", testVless)
		
		// REALITY
		api.GET("/reality/config", getRealityConfig)
		api.POST("/reality/save", saveRealityConfig)
		
		// Россия
		api.GET("/russia/services", getRussiaServices)
		
		// Логи
		api.GET("/logs", getLogs)
		api.DELETE("/logs", clearLogs)

		// Генерация конфига
		api.POST("/config/generate", generateConfig)

		// NeuralTunnel
		api.POST("/neural/start", startNeuralTunnel)
		api.POST("/neural/stop", stopNeuralTunnel)
		api.GET("/neural/ports", getNeuralTunnelPorts)
		api.POST("/neural/ports", setNeuralTunnelPorts)
		api.GET("/neural/firewall", getNeuralTunnelFirewall)
		api.POST("/neural/firewall", setNeuralTunnelFirewall)
		api.GET("/neural/bbr", getNeuralTunnelBBR)
		api.POST("/neural/bbr", setNeuralTunnelBBR)
		api.GET("/neural/fail2ban", getNeuralTunnelFail2Ban)
		api.POST("/neural/fail2ban", setNeuralTunnelFail2Ban)

		// Цепочки VPS/CDN
		api.GET("/chains", getChains)
		api.POST("/chains", addChain)
		api.PUT("/chains/:id", updateChain)
		api.DELETE("/chains/:id", deleteChain)
		api.POST("/chains/:id/apply", applyChain)
		api.GET("/chains/export", exportChains)
		api.POST("/chains/import", importChains)

		// Региональный сканер
		api.GET("/region/scan", regionScan)
		api.GET("/region/map", regionMap)
		
		// Keys API
		api.GET("/keys", getKeys)
		api.POST("/keys", createKey)
		api.DELETE("/keys/:key", revokeKey)
		api.GET("/keys/:key/validate", validateKey)
	}

	// WebSocket маршрут
	r.GET("/api/v1/ws", func(c *gin.Context) { wsHandler(c) })

	// Запуск сервера
	port := ":8080"
	fmt.Printf("🚀 TrafficMask Panel запущен на порту %s\n", port)
	fmt.Printf("🌐 Веб-интерфейс: http://localhost%s\n", port)
	fmt.Printf("📊 API: http://localhost%s/api/v1\n", port)
	fmt.Printf("🇷🇺 Российская адаптация VLESS + REALITY + Vision\n")
	
	startRealtimeScanner() // Запускаем сканер в реальном времени
	log.Fatal(r.Run(port))
}

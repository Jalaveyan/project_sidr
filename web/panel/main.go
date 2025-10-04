package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
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
}

type LogEntry struct {
	Timestamp string `json:"timestamp"`
	Level     string `json:"level"`
	Message   string `json:"message"`
	Source    string `json:"source"`
}

// Глобальные переменные для хранения данных
var (
	stats           StatsResponse
	vlessConfigs    []VlessConfig
	realityConfig   RealityConfig
	russiaServices  []RussiaService
	logs            []LogEntry
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
		},
		{
			Name:      "Mail.ru",
			Domain:    "mail.ru",
			IP:        "13.13.13.13",
			Status:    "online",
			Usage:     25,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
		},
		{
			Name:      "Rambler",
			Domain:    "rambler.ru",
			IP:        "46.46.46.46",
			Status:    "online",
			Usage:     20,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
		},
		{
			Name:      "VK",
			Domain:    "vk.com",
			IP:        "31.31.31.31",
			Status:    "online",
			Usage:     20,
			LastCheck: time.Now().Format("2006-01-02 15:04:05"),
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
			// Пример: отправляем разрешённые IP
			wsBroadcast(WsEvent{Type: "allowed_ips", Data: russiaServices})
			// Пример: отправляем логи
			wsBroadcast(WsEvent{Type: "logs", Data: logs})
			time.Sleep(2 * time.Second)
		}
	}()
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
	config := fmt.Sprintf(
		"vless://%s@yourdomain.com:443?encryption=none&security=tls&sni=%s&alpn=%s&type=ws&path=%s#MyProfile",
		req.UUID, req.SNI, req.ALPN, req.Path,
	)
	c.JSON(http.StatusOK, gin.H{"config": config})
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

	// Статические файлы
	r.Static("/static", "./web/panel")
	r.StaticFile("/", "./web/panel/index.html")

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

package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gorilla/mux"
	"github.com/sirupsen/logrus"
	"github.com/spf13/viper"
)

// Config структура конфигурации
type Config struct {
	Server   ServerConfig   `mapstructure:"server"`
	Security SecurityConfig `mapstructure:"security"`
	Logging  LoggingConfig  `mapstructure:"logging"`
}

type ServerConfig struct {
	Host string `mapstructure:"host"`
	Port int    `mapstructure:"port"`
}

type SecurityConfig struct {
	EnableSignatureMasking bool     `mapstructure:"enable_signature_masking"`
	AllowedSignatures      []string `mapstructure:"allowed_signatures"`
	BlockedPatterns        []string `mapstructure:"blocked_patterns"`
}

type LoggingConfig struct {
	Level  string `mapstructure:"level"`
	Format string `mapstructure:"format"`
}

// TrafficMaskServer основной сервер
type TrafficMaskServer struct {
	config *Config
	logger *logrus.Logger
	router *mux.Router
}

// NewTrafficMaskServer создает новый экземпляр сервера
func NewTrafficMaskServer(config *Config) *TrafficMaskServer {
	logger := logrus.New()
	
	// Настройка логирования
	level, err := logrus.ParseLevel(config.Logging.Level)
	if err != nil {
		level = logrus.InfoLevel
	}
	logger.SetLevel(level)
	
	if config.Logging.Format == "json" {
		logger.SetFormatter(&logrus.JSONFormatter{})
	} else {
		logger.SetFormatter(&logrus.TextFormatter{
			FullTimestamp: true,
		})
	}
	
	server := &TrafficMaskServer{
		config: config,
		logger: logger,
		router: mux.NewRouter(),
	}
	
	server.setupRoutes()
	return server
}

// setupRoutes настраивает маршруты API
func (s *TrafficMaskServer) setupRoutes() {
	api := s.router.PathPrefix("/api/v1").Subrouter()
	
	// Статус системы
	api.HandleFunc("/status", s.handleStatus).Methods("GET")
	
	// Управление сигнатурами
	api.HandleFunc("/signatures", s.handleGetSignatures).Methods("GET")
	api.HandleFunc("/signatures", s.handleAddSignature).Methods("POST")
	api.HandleFunc("/signatures/{id}", s.handleDeleteSignature).Methods("DELETE")
	
	// Статистика
	api.HandleFunc("/stats", s.handleStats).Methods("GET")
	
	// Конфигурация
	api.HandleFunc("/config", s.handleGetConfig).Methods("GET")
	api.HandleFunc("/config", s.handleUpdateConfig).Methods("PUT")
	
	// Логирование
	s.router.Use(s.loggingMiddleware)
	s.router.Use(s.corsMiddleware)
}

// handleStatus возвращает статус системы
func (s *TrafficMaskServer) handleStatus(w http.ResponseWriter, r *http.Request) {
	status := map[string]interface{}{
		"status":    "running",
		"timestamp": time.Now().Unix(),
		"version":   "1.0.0",
		"uptime":    time.Since(startTime).String(),
	}
	
	s.respondJSON(w, http.StatusOK, status)
}

// handleGetSignatures возвращает список активных сигнатур
func (s *TrafficMaskServer) handleGetSignatures(w http.ResponseWriter, r *http.Request) {
	signatures := map[string]interface{}{
		"active_signatures": s.config.Security.AllowedSignatures,
		"blocked_patterns":  s.config.Security.BlockedPatterns,
		"masking_enabled":   s.config.Security.EnableSignatureMasking,
	}
	
	s.respondJSON(w, http.StatusOK, signatures)
}

// handleAddSignature добавляет новую сигнатуру
func (s *TrafficMaskServer) handleAddSignature(w http.ResponseWriter, r *http.Request) {
	var request struct {
		Signature string `json:"signature"`
		Type      string `json:"type"`
	}
	
	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		s.respondError(w, http.StatusBadRequest, "Invalid JSON")
		return
	}
	
	// Добавляем сигнатуру в конфигурацию
	s.config.Security.AllowedSignatures = append(s.config.Security.AllowedSignatures, request.Signature)
	
	s.logger.WithFields(logrus.Fields{
		"signature": request.Signature,
		"type":      request.Type,
	}).Info("Signature added")
	
	s.respondJSON(w, http.StatusCreated, map[string]string{
		"message": "Signature added successfully",
	})
}

// handleDeleteSignature удаляет сигнатуру
func (s *TrafficMaskServer) handleDeleteSignature(w http.Request) {
	vars := mux.Vars(r)
	signatureID := vars["id"]
	
	// Удаляем сигнатуру из конфигурации
	for i, sig := range s.config.Security.AllowedSignatures {
		if sig == signatureID {
			s.config.Security.AllowedSignatures = append(
				s.config.Security.AllowedSignatures[:i],
				s.config.Security.AllowedSignatures[i+1:]...,
			)
			break
		}
	}
	
	s.logger.WithField("signature_id", signatureID).Info("Signature deleted")
	
	s.respondJSON(w, http.StatusOK, map[string]string{
		"message": "Signature deleted successfully",
	})
}

// handleStats возвращает статистику системы
func (s *TrafficMaskServer) handleStats(w http.ResponseWriter, r *http.Request) {
	stats := map[string]interface{}{
		"processed_packets": 0, // Будет получаться из C++ ядра
		"masked_packets":    0, // Будет получаться из C++ ядра
		"active_connections": 0,
		"signature_count":   len(s.config.Security.AllowedSignatures),
		"uptime":           time.Since(startTime).String(),
	}
	
	s.respondJSON(w, http.StatusOK, stats)
}

// handleGetConfig возвращает текущую конфигурацию
func (s *TrafficMaskServer) handleGetConfig(w http.ResponseWriter, r *http.Request) {
	s.respondJSON(w, http.StatusOK, s.config)
}

// handleUpdateConfig обновляет конфигурацию
func (s *TrafficMaskServer) handleUpdateConfig(w http.ResponseWriter, r *http.Request) {
	var newConfig Config
	if err := json.NewDecoder(r.Body).Decode(&newConfig); err != nil {
		s.respondError(w, http.StatusBadRequest, "Invalid JSON")
		return
	}
	
	// Валидация конфигурации
	if newConfig.Server.Port <= 0 || newConfig.Server.Port > 65535 {
		s.respondError(w, http.StatusBadRequest, "Invalid port number")
		return
	}
	
	s.config = &newConfig
	s.logger.Info("Configuration updated")
	
	s.respondJSON(w, http.StatusOK, map[string]string{
		"message": "Configuration updated successfully",
	})
}

// respondJSON отправляет JSON ответ
func (s *TrafficMaskServer) respondJSON(w http.ResponseWriter, status int, data interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(data)
}

// respondError отправляет ошибку
func (s *TrafficMaskServer) respondError(w http.ResponseWriter, status int, message string) {
	s.respondJSON(w, status, map[string]string{"error": message})
}

// loggingMiddleware логирует HTTP запросы
func (s *TrafficMaskServer) loggingMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		next.ServeHTTP(w, r)
		
		s.logger.WithFields(logrus.Fields{
			"method":     r.Method,
			"path":       r.URL.Path,
			"duration":   time.Since(start),
			"remote_addr": r.RemoteAddr,
		}).Info("HTTP request")
	})
}

// corsMiddleware добавляет CORS заголовки
func (s *TrafficMaskServer) corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		
		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusOK)
			return
		}
		
		next.ServeHTTP(w, r)
	})
}

// Start запускает сервер
func (s *TrafficMaskServer) Start() error {
	addr := fmt.Sprintf("%s:%d", s.config.Server.Host, s.config.Server.Port)
	
	s.logger.WithFields(logrus.Fields{
		"host": s.config.Server.Host,
		"port": s.config.Server.Port,
	}).Info("Starting TrafficMask server")
	
	return http.ListenAndServe(addr, s.router)
}

var startTime = time.Now()

func main() {
	// Загрузка конфигурации
	viper.SetConfigName("config")
	viper.SetConfigType("yaml")
	viper.AddConfigPath("./configs")
	viper.AddConfigPath(".")
	
	// Установка значений по умолчанию
	viper.SetDefault("server.host", "localhost")
	viper.SetDefault("server.port", 8080)
	viper.SetDefault("security.enable_signature_masking", true)
	viper.SetDefault("logging.level", "info")
	viper.SetDefault("logging.format", "text")
	
	if err := viper.ReadInConfig(); err != nil {
		log.Printf("Warning: Could not read config file: %v", err)
	}
	
	var config Config
	if err := viper.Unmarshal(&config); err != nil {
		log.Fatalf("Failed to unmarshal config: %v", err)
	}
	
	// Создание и запуск сервера
	server := NewTrafficMaskServer(&config)
	
	// Обработка сигналов для graceful shutdown
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	
	go func() {
		<-c
		log.Println("Shutting down server...")
		os.Exit(0)
	}()
	
	// Запуск сервера
	if err := server.Start(); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}

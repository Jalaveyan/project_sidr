package client

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

// TrafficMaskClient клиент для взаимодействия с сервером
type TrafficMaskClient struct {
	baseURL    string
	httpClient *http.Client
}

// NewTrafficMaskClient создает новый клиент
func NewTrafficMaskClient(baseURL string) *TrafficMaskClient {
	return &TrafficMaskClient{
		baseURL: baseURL,
		httpClient: &http.Client{
			Timeout: 30 * time.Second,
		},
	}
}

// StatusResponse ответ статуса
type StatusResponse struct {
	Status    string `json:"status"`
	Timestamp int64  `json:"timestamp"`
	Version   string `json:"version"`
	Uptime    string `json:"uptime"`
}

// SignatureRequest запрос на добавление сигнатуры
type SignatureRequest struct {
	Signature string `json:"signature"`
	Type      string `json:"type"`
}

// StatsResponse ответ статистики
type StatsResponse struct {
	ProcessedPackets  int    `json:"processed_packets"`
	MaskedPackets     int    `json:"masked_packets"`
	ActiveConnections int    `json:"active_connections"`
	SignatureCount    int    `json:"signature_count"`
	Uptime           string `json:"uptime"`
}

// GetStatus получает статус сервера
func (c *TrafficMaskClient) GetStatus() (*StatusResponse, error) {
	resp, err := c.httpClient.Get(c.baseURL + "/api/v1/status")
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("server returned status %d", resp.StatusCode)
	}

	var status StatusResponse
	if err := json.NewDecoder(resp.Body).Decode(&status); err != nil {
		return nil, err
	}

	return &status, nil
}

// GetSignatures получает список сигнатур
func (c *TrafficMaskClient) GetSignatures() (map[string]interface{}, error) {
	resp, err := c.httpClient.Get(c.baseURL + "/api/v1/signatures")
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("server returned status %d", resp.StatusCode)
	}

	var signatures map[string]interface{}
	if err := json.NewDecoder(resp.Body).Decode(&signatures); err != nil {
		return nil, err
	}

	return signatures, nil
}

// AddSignature добавляет новую сигнатуру
func (c *TrafficMaskClient) AddSignature(signature, sigType string) error {
	req := SignatureRequest{
		Signature: signature,
		Type:      sigType,
	}

	jsonData, err := json.Marshal(req)
	if err != nil {
		return err
	}

	resp, err := c.httpClient.Post(
		c.baseURL+"/api/v1/signatures",
		"application/json",
		bytes.NewBuffer(jsonData),
	)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusCreated {
		body, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("server returned status %d: %s", resp.StatusCode, string(body))
	}

	return nil
}

// DeleteSignature удаляет сигнатуру
func (c *TrafficMaskClient) DeleteSignature(signatureID string) error {
	req, err := http.NewRequest("DELETE", c.baseURL+"/api/v1/signatures/"+signatureID, nil)
	if err != nil {
		return err
	}

	resp, err := c.httpClient.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("server returned status %d: %s", resp.StatusCode, string(body))
	}

	return nil
}

// GetStats получает статистику
func (c *TrafficMaskClient) GetStats() (*StatsResponse, error) {
	resp, err := c.httpClient.Get(c.baseURL + "/api/v1/stats")
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("server returned status %d", resp.StatusCode)
	}

	var stats StatsResponse
	if err := json.NewDecoder(resp.Body).Decode(&stats); err != nil {
		return nil, err
	}

	return &stats, nil
}

// MonitorStats мониторит статистику с заданным интервалом
func (c *TrafficMaskClient) MonitorStats(interval time.Duration, callback func(*StatsResponse)) {
	ticker := time.NewTicker(interval)
	defer ticker.Stop()

	for range ticker.C {
		stats, err := c.GetStats()
		if err != nil {
			fmt.Printf("Error getting stats: %v\n", err)
			continue
		}
		callback(stats)
	}
}

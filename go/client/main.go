package main

import (
	"fmt"
	"log"
	"time"

	"./client"
)

func main() {
	// Создаем клиент
	client := client.NewTrafficMaskClient("http://localhost:8080")

	// Проверяем статус сервера
	fmt.Println("=== Checking Server Status ===")
	status, err := client.GetStatus()
	if err != nil {
		log.Fatalf("Failed to get status: %v", err)
	}
	fmt.Printf("Server Status: %s\n", status.Status)
	fmt.Printf("Version: %s\n", status.Version)
	fmt.Printf("Uptime: %s\n", status.Uptime)

	// Получаем текущие сигнатуры
	fmt.Println("\n=== Current Signatures ===")
	signatures, err := client.GetSignatures()
	if err != nil {
		log.Printf("Failed to get signatures: %v", err)
	} else {
		fmt.Printf("Signatures: %+v\n", signatures)
	}

	// Добавляем новую сигнатуру
	fmt.Println("\n=== Adding New Signature ===")
	err = client.AddSignature("custom_pattern_123", "regex")
	if err != nil {
		log.Printf("Failed to add signature: %v", err)
	} else {
		fmt.Println("Signature added successfully")
	}

	// Получаем статистику
	fmt.Println("\n=== Current Statistics ===")
	stats, err := client.GetStats()
	if err != nil {
		log.Printf("Failed to get stats: %v", err)
	} else {
		fmt.Printf("Processed Packets: %d\n", stats.ProcessedPackets)
		fmt.Printf("Masked Packets: %d\n", stats.MaskedPackets)
		fmt.Printf("Active Connections: %d\n", stats.ActiveConnections)
		fmt.Printf("Signature Count: %d\n", stats.SignatureCount)
	}

	// Мониторим статистику
	fmt.Println("\n=== Monitoring Statistics (10 seconds) ===")
	client.MonitorStats(2*time.Second, func(stats *client.StatsResponse) {
		fmt.Printf("[%s] Processed: %d, Masked: %d, Connections: %d\n",
			time.Now().Format("15:04:05"),
			stats.ProcessedPackets,
			stats.MaskedPackets,
			stats.ActiveConnections)
	})

	fmt.Println("\n=== Client Demo Complete ===")
}

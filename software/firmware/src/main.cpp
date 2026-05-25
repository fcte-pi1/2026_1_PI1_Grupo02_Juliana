#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Inclusão das classes de hardware
#include "Motor.h"
#include "PID.h"
#include "WatchdogMotor.h"

// --- Configurações de Rede (Conforme Seção 6.1 do Guia) ---
const char* ssid = "NOME_DA_REDE";
const char* password = "SENHA";
const char* mqtt_server = "IP_DO_SERVIDOR_DE_BANCADA";
const char* robot_id = "rato_01"; 
char run_id[40] = ""; 

WiFiClient espClient;
PubSubClient client(espClient);

// --- Instâncias de Hardware (Usando os pinos MOCK) ---
// alterar os pinos depois
Motor motorEsq(15, 14, 13);
Motor motorDir(12, 11, 10);
WatchdogMotor watchdog(&motorEsq, &motorDir, 1000);

// Variável global para controlar o tempo da telemetria
unsigned long ultimaTelemetria = 0;

void setup() {
    Serial.begin(115200);
    
    // Inicialização do Hardware
    motorEsq.inicializar();
    motorDir.inicializar();

    // Configuração Wi-Fi e MQTT
    WiFi.begin(ssid, password);
    client.setServer(mqtt_server, 1883);
}

void loop() {
    // 1. Atualiza a segurança
    watchdog.alimentar();
    watchdog.verificar();

    // 2. Mantém a rede viva
    if (!client.connected()) {
        // Lógica de reconexão MQTT entra aqui futuramente
    }
    client.loop();

    // 3. Execução física
    if (watchdog.isAtivo()) {
        motorEsq.setVelocidade(128);
        motorDir.setVelocidade(128);
    }

    // 4. Telemetria (HU12) - Dispara a cada 100ms
    if (millis() - ultimaTelemetria > 100) { 
        ultimaTelemetria = millis();
        
        // Simulação de dados
        float speed = 0.34; 
        int battery = 78;
        float voltage = 7.42;
        int pose_x = 0;
        int pose_y = 0;
        
        char payload[512];
        char topico[100];
        
        // Constrói o Tópico (Ex: micromouse/rato_01/telemetria)
        snprintf(topico, sizeof(topico), "micromouse/%s/telemetria", robot_id);

        // Constrói o Payload do Guia
        snprintf(payload, sizeof(payload), 
            "{\"ts\":\"2026-05-25T12:00:00Z\",\"run_id\":\"%s\",\"pose\":{\"x\":%d,\"y\":%d,\"heading\":\"N\"},\"maze_delta\":[],\"speed\":%.2f,\"battery\":%d,\"voltage\":%.2f}", 
            run_id, pose_x, pose_y, speed, battery, voltage);
            
        // Publica no Mosquitto
        if (client.connected()) {
            client.publish(topico, payload);
        }
        
        Serial.print("[MQTT] Publicado em ");
        Serial.print(topico);
        Serial.print(": ");
        Serial.println(payload);
    }

    delay(10); 
}
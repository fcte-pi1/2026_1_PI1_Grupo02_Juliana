#include <Arduino.h>
#include "MazeMapper.h"

void setup() {

    delay(2500); 

    Serial.println("--- Iniciando Teste da HU12 (MazeMapper) ---");

    Serial.println("\n[Simulacao] Entrando na celula (0,0)...");
    atualizar_mapa(0, 0, true, false, false, true);
    String json1 = gerar_payload_telemetria(0, 0, "run-teste-123");
    Serial.println("JSON Publicado no MQTT:");
    Serial.println(json1);

    Serial.println("\n[Simulacao] Entrando na celula (0,1)...");
    atualizar_mapa(0, 1, false, true, true, true);
    String json2 = gerar_payload_telemetria(0, 1, "run-teste-123");
    Serial.println("JSON Publicado no MQTT:");
    Serial.println(json2);
    
    Serial.println("\n[Simulacao] Voltando para a celula (0,0)...");
    atualizar_mapa(0, 0, false, false, false, false);
    String json3 = gerar_payload_telemetria(0, 0, "run-teste-123");
    Serial.println("JSON (Nao deve ter sobrescrito as paredes):");
    Serial.println(json3);
}

void loop() {
    // vazio no teste
}
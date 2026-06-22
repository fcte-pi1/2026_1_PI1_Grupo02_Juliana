#!/bin/bash

# Script de teste automatizado de motores para o firmware Pico W
# Ajuste MQTT_HOST e RUN_ID conforme sua configuração.

MQTT_HOST="192.168.1.100"
RUN_ID="micromouse_001"
TOPIC="micromouse/${RUN_ID}/cmd"
BROKER_OPTS="-h $MQTT_HOST -t $TOPIC"

echo "=== TESTE AUTOMATIZADO DE MOTORES ==="
echo "Broker MQTT: $MQTT_HOST"
echo "Tópico de comando: $TOPIC"

echo "[1] Ligando motor esquerdo (M1)"
mosquitto_pub $BROKER_OPTS -m "M1 ON"
sleep 2

echo "[2] Ligando motor direito (M2)"
mosquitto_pub $BROKER_OPTS -m "M2 ON"
sleep 2

echo "[3] Parando ambos os motores"
mosquitto_pub $BROKER_OPTS -m "STOP"
sleep 1

echo "[4] Rampa de velocidade M1"
for speed in 50 100 150 200 255; do
  echo "  SET M1 $speed"
  mosquitto_pub $BROKER_OPTS -m "SET M1 $speed"
  sleep 1
done

echo "[5] Parando ambos os motores novamente"
mosquitto_pub $BROKER_OPTS -m "STOP"
echo "=== FIM DO TESTE ==="
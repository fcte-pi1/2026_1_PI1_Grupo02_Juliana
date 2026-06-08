#!/bin/bash

# Monitor de telemetria MQTT para o firmware Pico W
# Ajuste MQTT_HOST e RUN_ID conforme sua configuração.

MQTT_HOST="192.168.1.100"
RUN_ID="micromouse_001"
TOPIC="micromouse/${RUN_ID}/#"

echo "Monitorando telemetria em $MQTT_HOST tópico $TOPIC"
mosquitto_sub -h "$MQTT_HOST" -t "$TOPIC" -v
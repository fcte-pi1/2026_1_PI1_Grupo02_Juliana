#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "pico/mutex.h"

#include "pico/cyw43_arch.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

// Inclusão das classes de hardware
#include "Motor.h"
#include "PID.h"
#include "WatchdogMotor.h"
#include "MazeMapper.h"

// --- Configurações de pinos para Pico W (hardware.md) ---
// Motor Esquerdo: PWM em GP15, Dir1 em GP14, Dir2 em GP13
// Motor Direito: PWM em GP12, Dir1 em GP11, Dir2 em GP10
Motor motorEsq(15, 14, 13);
Motor motorDir(12, 11, 10);
WatchdogMotor watchdog(&motorEsq, &motorDir, 1000);

// Desired speeds controladas remotamente (protegidas por mutex)
volatile int desiredLeft = 0;
volatile int desiredRight = 0;
mutex_t motor_mutex;

// ---------------------- MQTT helper functions ----------------------
static const char *mqtt_server = "192.168.1.100"; // ajustar o broker
static const int mqtt_port = 1883;
static int mqtt_sock = -1;

// Escreve string MQTT (2 bytes len + payload)
static int write_mqtt_string(uint8_t *buf, const char *s) {
    uint16_t len = (uint16_t)strlen(s);
    buf[0] = (len >> 8) & 0xFF;
    buf[1] = len & 0xFF;
    memcpy(buf + 2, s, len);
    return 2 + len;
}

// Encoda Remaining Length (MQTT var int)
static int encode_remaining_length(uint8_t *buf, int len) {
    int idx = 0;
    do {
        uint8_t digit = len % 128;
        len /= 128;
        if (len > 0) digit |= 0x80;
        buf[idx++] = digit;
    } while (len > 0 && idx < 4);
    return idx;
}

// Conecta ao broker MQTT e envia CONNECT com Will (LWT) retained
static int mqtt_connect_and_setup(const char *client_id) {
    struct sockaddr_in addr;
    mqtt_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (mqtt_sock < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mqtt_port);
    inet_aton(mqtt_server, &addr.sin_addr);

    if (connect(mqtt_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(mqtt_sock);
        mqtt_sock = -1;
        return -1;
    }

    uint8_t pkt[512];
    int pos = 0;

    // Variable header and payload
    uint8_t vh_payload[400];
    int vp = 0;
    // Protocol Name
    vp += write_mqtt_string(vh_payload + vp, "MQTT");
    // Protocol Level
    vh_payload[vp++] = 0x04; // 3.1.1
    // Connect Flags: CleanSession=1 (no Will, no username/password)
    vh_payload[vp++] = 0x02;
    // Keep Alive
    vh_payload[vp++] = 0x00;
    vh_payload[vp++] = 60; // 60s

    // Payload: ClientID
    vp += write_mqtt_string(vh_payload + vp, client_id);

    // Fixed header
    pkt[pos++] = 0x10; // CONNECT
    uint8_t rem[4];
    int rem_len = encode_remaining_length(rem, vp);
    memcpy(pkt + pos, rem, rem_len); pos += rem_len;
    memcpy(pkt + pos, vh_payload, vp); pos += vp;

    // send
    if (send(mqtt_sock, pkt, pos, 0) != pos) {
        close(mqtt_sock); mqtt_sock = -1; return -1;
    }

    // wait for CONNACK
    uint8_t resp[4];
    int r = recv(mqtt_sock, resp, sizeof(resp), 0);
    if (r < 4) { close(mqtt_sock); mqtt_sock = -1; return -1; }
    if (resp[0] != 0x20 || resp[1] != 0x02 || resp[3] != 0x00) {
        close(mqtt_sock); mqtt_sock = -1; return -1;
    }

    return 0;
}

// Publica QoS0 (retained opcional)
static int mqtt_publish_qos0(const char *topic, const char *payload, int retain) {
    if (mqtt_sock < 0) return -1;
    uint8_t pkt[1024]; int p = 0;
    uint8_t vh[512]; int vp = 0;
    vp += write_mqtt_string(vh + vp, topic);
    int payload_len = strlen(payload);
    memcpy(vh + vp, payload, payload_len); vp += payload_len;

    uint8_t rem[4]; int rem_len = encode_remaining_length(rem, vp);
    // fixed header
    uint8_t fh = 0x30 | (retain ? 0x01 : 0x00); // PUBLISH, QoS0
    pkt[p++] = fh;
    memcpy(pkt + p, rem, rem_len); p += rem_len;
    memcpy(pkt + p, vh, vp); p += vp;

    return send(mqtt_sock, pkt, p, 0);
}

// Subscribe to topic (QoS0)
static int mqtt_subscribe_qos0(const char *topic) {
    if (mqtt_sock < 0) return -1;
    uint8_t pkt[512]; int p = 0;
    // Variable header: Packet Identifier
    uint8_t vh[2]; vh[0] = 0x00; vh[1] = 0x01; // PacketId=1

    uint8_t payload[256]; int pp = 0;
    pp += write_mqtt_string(payload + pp, topic);
    payload[pp++] = 0x00; // QoS0

    uint8_t rem[4]; int rem_len = encode_remaining_length(rem, 2 + pp);
    pkt[p++] = 0x82; // SUBSCRIBE (QoS1)
    memcpy(pkt + p, rem, rem_len); p += rem_len;
    memcpy(pkt + p, vh, 2); p += 2;
    memcpy(pkt + p, payload, pp); p += pp;

    if (send(mqtt_sock, pkt, p, 0) != p) return -1;
    // Wait for SUBACK (simple blocking)
    uint8_t buf[16]; int r = recv(mqtt_sock, buf, sizeof(buf), 0);
    (void)r;
    return 0;
}

// Tenta ler pacotes MQTT (PUBLISH Qo0) e processa comandos
static void mqtt_process_incoming_and_handle_commands() {
    if (mqtt_sock < 0) return;
    uint8_t buf[512];
    int r = recv(mqtt_sock, buf, sizeof(buf), MSG_DONTWAIT);
    if (r <= 0) return;

    int idx = 0;
    while (idx < r) {
        uint8_t byte1 = buf[idx++];
        uint8_t pkt_type = (byte1 >> 4) & 0x0F;
        // read remaining length (simple, assume single byte)
        uint8_t rem = buf[idx++];
        if (pkt_type == 3) { // PUBLISH
            // topic
            uint16_t tlen = (buf[idx] << 8) | buf[idx+1]; idx += 2;
            char topic[128]; memcpy(topic, buf + idx, tlen); topic[tlen]=0; idx += tlen;
            int payload_len = rem - 2 - tlen;
            if (payload_len < 0) payload_len = 0;
            char payload[256]; int pl = payload_len;
            memcpy(payload, buf + idx, pl); payload[pl]=0; idx += pl;
            // process payload commands (same as TCP)
            if (strstr(payload, "M1 ON") != NULL) {
                mutex_enter_blocking(&motor_mutex);
                desiredLeft = 128;
                mutex_exit(&motor_mutex);
            } else if (strstr(payload, "M1 OFF") != NULL) {
                mutex_enter_blocking(&motor_mutex);
                desiredLeft = 0;
                mutex_exit(&motor_mutex);
            } else if (strstr(payload, "M2 ON") != NULL) {
                mutex_enter_blocking(&motor_mutex);
                desiredRight = 128;
                mutex_exit(&motor_mutex);
            } else if (strstr(payload, "M2 OFF") != NULL) {
                mutex_enter_blocking(&motor_mutex);
                desiredRight = 0;
                mutex_exit(&motor_mutex);
            } else if (strstr(payload, "STOP") != NULL) {
                mutex_enter_blocking(&motor_mutex);
                desiredLeft = 0; desiredRight = 0;
                mutex_exit(&motor_mutex);
            } else if (strncmp(payload, "SET M1", 6) == 0) {
                int v = atoi(payload + 6);
                if (v > 255) v = 255; if (v < -255) v = -255;
                mutex_enter_blocking(&motor_mutex); desiredLeft = v; mutex_exit(&motor_mutex);
            } else if (strncmp(payload, "SET M2", 6) == 0) {
                int v = atoi(payload + 6);
                if (v > 255) v = 255; if (v < -255) v = -255;
                mutex_enter_blocking(&motor_mutex); desiredRight = v; mutex_exit(&motor_mutex);
            }
        } else {
            // skip unknown
            idx += rem;
        }
    }
}

// --- Variáveis globais ---
char run_id[40] = "micromouse_001"; 

// Função auxiliar: obter tempo em milissegundos
inline uint32_t millis_pico() {
    return to_ms_since_boot(get_absolute_time());
}

int main() {
    // Inicialização do Pico W
    stdio_init_all();
    sleep_ms(100); // Aguarda inicialização do USB (se conectado)
    
    printf("\r\n========================================\r\n");
    printf("Micromouse - Firmware Pico W\r\n");
    printf("========================================\r\n");
    
    // Inicialização do Hardware
    printf("[INIT] Inicializando motores...\r\n");
    motorEsq.inicializar();
    motorDir.inicializar();
    printf("[OK] Motores inicializados\r\n");
    // Inicializa mutex para proteção de acesso às variáveis de velocidade
    mutex_init(&motor_mutex);

    // --- Inicializa Wi-Fi (Pico W) ---
    const char *ssid = "SEU_SSID"; // alterar SSID
    const char *password = "SUA_SENHA"; // alterar senha

    if (cyw43_arch_init()) {
        printf("[WIFI] Falha ao inicializar cyw43_arch\r\n");
    } else {
        printf("[WIFI] cyw43 iniciado\r\n");
        int rc = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000);
        if (rc == 0) {
            printf("[WIFI] Conectado à rede '%s'\r\n", ssid);
        } else {
            printf("[WIFI] Não conectado (rc=%d)\r\n", rc);
        }
    }

    // Lança Core 1 (placeholder para tarefas de tempo real)
    multicore_launch_core1([](){
        // Core1 ficará disponível para tarefas de tempo real (sensoriamento, ISR, etc.)
        while (true) {
            sleep_ms(1000);
        }
    });

    // --- Conecta ao broker MQTT ---
    if (mqtt_connect_and_setup(run_id) == 0) {
        printf("[MQTT] Conectado ao broker %s:%d\r\n", mqtt_server, mqtt_port);
        char cmd_topic[128];
        snprintf(cmd_topic, sizeof(cmd_topic), "micromouse/%s/cmd", run_id);
        mqtt_subscribe_qos0(cmd_topic);

        // publica status online (retained)
        char status_topic[128];
        snprintf(status_topic, sizeof(status_topic), "micromouse/%s/status", run_id);
        mqtt_publish_qos0(status_topic, "online", 1);
    } else {
        printf("[MQTT] Falha ao conectar ao broker\r\n");
    }

    // Variável global para controlar o tempo da telemetria
    uint32_t ultimaTelemetria = millis_pico();
    
    printf("[INIT] Sistema iniciado. Aguardando comandos...\r\n");
    
    // --- Loop principal ---
    while (true) {
        // 1. Atualiza a segurança
        watchdog.alimentar();
        watchdog.verificar();

        // 2. Execução física: aplica velocidades desejadas vindas do servidor TCP
        if (watchdog.isAtivo()) {
            // lê as velocidades desejadas protegidas por mutex
            mutex_enter_blocking(&motor_mutex);
            int l = desiredLeft;
            int r = desiredRight;
            mutex_exit(&motor_mutex);

            motorEsq.setVelocidade(l);
            motorDir.setVelocidade(r);
        }

        // 3. Telemetria (HU12) - Dispara a cada 100ms
        uint32_t agora = millis_pico();
        if (agora - ultimaTelemetria > 100) { 
            ultimaTelemetria = agora;
            
            // Simulação de dados
            float speed = 0.34; 
            int battery = 78;
            float voltage = 7.42;
            int pose_x = 0;
            int pose_y = 0;
            
            char payload[512];
            char topico[100];
            
            // Constrói o Tópico (Ex: micromouse/rato_01/telemetria)
            snprintf(topico, sizeof(topico), "micromouse/%s/telemetria", run_id);

            // Constrói o Payload do Guia (formato simplificado para teste)
            snprintf(payload, sizeof(payload), 
                "{\"ts\":\"2026-05-25T12:00:00Z\",\"run_id\":\"%s\",\"pose\":{\"x\":%d,\"y\":%d,\"heading\":\"N\"},\"maze_delta\":[],\"speed\":%.2f,\"battery\":%d,\"voltage\":%.2f}", 
                run_id, pose_x, pose_y, speed, battery, voltage);
            
            // Publica via MQTT (QoS0) e debug via printf
            mqtt_publish_qos0(topico, payload, 0);
            printf("[TELEMETRIA] %s: %s\r\n", topico, payload);

            // Processa mensagens de comando recebidas via MQTT
            mqtt_process_incoming_and_handle_commands();
        }

        // 4. Loop delay (evita busy-wait)
        sleep_ms(10); 
    }
    
    return 0;
}
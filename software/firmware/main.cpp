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
#include "MovimentacaoFrontal.h"
#include "Rotacao.h"
#include "MazeMapper.h"

// ─── Configurações de usuário ─────────────────────────────────────────────────
// Alterar antes de gravar no robô:
static const char *WIFI_SSID     = "SEU_SSID";
static const char *WIFI_PASSWORD = "SUA_SENHA";
static const char *MQTT_SERVER   = "192.168.1.100"; // IP do servidor rodando o broker
static const int   MQTT_PORT_NUM = 1883;

// ID fixo do micromouse — deve coincidir com MICROMOUSE_MQTT_ID no .env do backend.
// Tópicos usados:
//   Publica:  micromouse/<MOUSE_ID>/telemetria
//   Publica:  micromouse/<MOUSE_ID>/evento
//   Publica:  micromouse/<MOUSE_ID>/status  (retained, LWT = offline)
//   Assina:   micromouse/<MOUSE_ID>/comando
static const char *MOUSE_ID = "rato-01";

// ─── Configurações de pinos (hardware.md) ────────────────────────────────────
Motor motorEsq(15, 14, 13);
Motor motorDir(12, 11, 10);
WatchdogMotor watchdog(&motorEsq, &motorDir, 1000);
MovimentacaoFrontal movFrente(&motorEsq, &motorDir);
Rotacao rotacao(&motorEsq, &motorDir);

volatile int desiredLeft  = 0;
volatile int desiredRight = 0;
mutex_t motor_mutex;

// ─── Estado da corrida ────────────────────────────────────────────────────────
static char    current_run_id[48] = ""; // UUID recebido no comando "start"
static volatile bool running      = false;

// ─── Socket MQTT ─────────────────────────────────────────────────────────────
static int mqtt_sock = -1;

// ─── Helpers MQTT (nível de bytes) ───────────────────────────────────────────

static int write_mqtt_string(uint8_t *buf, const char *s) {
    uint16_t len = (uint16_t)strlen(s);
    buf[0] = (len >> 8) & 0xFF;
    buf[1] = len & 0xFF;
    memcpy(buf + 2, s, len);
    return 2 + len;
}

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

// Conecta ao broker com LWT configurado.
// LWT topic: micromouse/<MOUSE_ID>/status  payload: {"status":"offline"}  retained QoS1
static int mqtt_connect(void) {
    struct sockaddr_in addr;
    mqtt_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (mqtt_sock < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(MQTT_PORT_NUM);
    inet_aton(MQTT_SERVER, &addr.sin_addr);

    if (connect(mqtt_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(mqtt_sock);
        mqtt_sock = -1;
        return -1;
    }

    // ── Monta o CONNECT ──────────────────────────────────────────────────────
    uint8_t vh[512];
    int vp = 0;

    // Protocol Name "MQTT"
    vp += write_mqtt_string(vh + vp, "MQTT");
    // Protocol Level 3.1.1
    vh[vp++] = 0x04;
    // Connect Flags:
    //   bit7 Username=0, bit6 Password=0,
    //   bit5 WillRetain=1, bit4-3 WillQoS=01(QoS1), bit2 WillFlag=1,
    //   bit1 CleanSession=1, bit0 Reserved=0
    //   = 0b 0010 1110 = 0x2E
    vh[vp++] = 0x2E;
    // Keep Alive 60s
    vh[vp++] = 0x00;
    vh[vp++] = 60;

    // Payload: ClientID, WillTopic, WillMessage
    vp += write_mqtt_string(vh + vp, MOUSE_ID);

    char will_topic[128];
    snprintf(will_topic, sizeof(will_topic), "micromouse/%s/status", MOUSE_ID);
    vp += write_mqtt_string(vh + vp, will_topic);
    vp += write_mqtt_string(vh + vp, "{\"status\":\"offline\"}");

    // Fixed header
    uint8_t pkt[512];
    int p = 0;
    pkt[p++] = 0x10; // CONNECT
    uint8_t rem[4];
    int rem_len = encode_remaining_length(rem, vp);
    memcpy(pkt + p, rem, rem_len); p += rem_len;
    memcpy(pkt + p, vh, vp);      p += vp;

    if (send(mqtt_sock, pkt, p, 0) != p) {
        close(mqtt_sock); mqtt_sock = -1; return -1;
    }

    // Aguarda CONNACK
    uint8_t resp[4];
    int r = recv(mqtt_sock, resp, sizeof(resp), 0);
    if (r < 4 || resp[0] != 0x20 || resp[1] != 0x02 || resp[3] != 0x00) {
        close(mqtt_sock); mqtt_sock = -1; return -1;
    }

    return 0;
}

// Publica QoS0 (retained opcional).
static int mqtt_publish(const char *topic, const char *payload, int retain) {
    if (mqtt_sock < 0) return -1;
    uint8_t vh[768];
    int vp = 0;
    vp += write_mqtt_string(vh + vp, topic);
    int plen = strlen(payload);
    memcpy(vh + vp, payload, plen); vp += plen;

    uint8_t pkt[800]; int p = 0;
    uint8_t fh = 0x30 | (retain ? 0x01 : 0x00);
    pkt[p++] = fh;
    uint8_t rem[4]; int rlen = encode_remaining_length(rem, vp);
    memcpy(pkt + p, rem, rlen); p += rlen;
    memcpy(pkt + p, vh, vp);    p += vp;

    return send(mqtt_sock, pkt, p, 0);
}

// Subscribe a um tópico (QoS1).
static int mqtt_subscribe(const char *topic) {
    if (mqtt_sock < 0) return -1;
    uint8_t pkt[256]; int p = 0;
    uint8_t vh[2] = {0x00, 0x01}; // Packet ID = 1
    uint8_t payload[200]; int pp = 0;
    pp += write_mqtt_string(payload + pp, topic);
    payload[pp++] = 0x01; // QoS 1

    uint8_t rem[4]; int rlen = encode_remaining_length(rem, 2 + pp);
    pkt[p++] = 0x82; // SUBSCRIBE
    memcpy(pkt + p, rem, rlen); p += rlen;
    memcpy(pkt + p, vh, 2);     p += 2;
    memcpy(pkt + p, payload, pp); p += pp;

    if (send(mqtt_sock, pkt, p, 0) != p) return -1;
    uint8_t buf[16];
    recv(mqtt_sock, buf, sizeof(buf), 0); // SUBACK
    return 0;
}

// ─── Timestamp aproximado (segundos desde boot) ───────────────────────────────
// Sem RTC: usa tempo de boot + data base fixa. Suficiente para o backend aceitar.
static void fill_timestamp(char *buf, size_t size) {
    uint32_t s = to_ms_since_boot(get_absolute_time()) / 1000;
    snprintf(buf, size, "2026-06-16T%02u:%02u:%02uZ",
             (s / 3600) % 24, (s % 3600) / 60, s % 60);
}

// ─── Helpers de publicação de domínio ────────────────────────────────────────

static void publish_status_online(void) {
    char topic[128];
    snprintf(topic, sizeof(topic), "micromouse/%s/status", MOUSE_ID);
    mqtt_publish(topic, "{\"status\":\"online\"}", 1);
    printf("[STATUS] online publicado (retained)\r\n");
}

static void publish_evento(const char *tipo) {
    if (strlen(current_run_id) == 0) return;
    char topic[128], payload[256], ts[32];
    fill_timestamp(ts, sizeof(ts));
    snprintf(topic,   sizeof(topic),
             "micromouse/%s/evento", MOUSE_ID);
    snprintf(payload, sizeof(payload),
             "{\"ts\":\"%s\",\"run_id\":\"%s\",\"type\":\"%s\",\"detail\":\"\"}",
             ts, current_run_id, tipo);
    mqtt_publish(topic, payload, 1); // retained
    printf("[EVENTO] %s → %s\r\n", tipo, payload);
}

// Publica um pacote de telemetria.
// pose_x/y e maze_delta vêm da navegação; speed/battery/voltage dos sensores.
// TODO: integrar com MazeMapper para preencher com dados reais.
static void publish_telemetria(int pose_x, int pose_y, const char *heading,
                                float speed, int battery, float voltage) {
    if (strlen(current_run_id) == 0 || !running) return;
    char topic[128], payload[512], ts[32];
    fill_timestamp(ts, sizeof(ts));
    snprintf(topic, sizeof(topic), "micromouse/%s/telemetria", MOUSE_ID);
    // maze_delta vazio até MazeMapper estar integrado
    snprintf(payload, sizeof(payload),
             "{\"ts\":\"%s\",\"run_id\":\"%s\","
             "\"pose\":{\"x\":%d,\"y\":%d,\"heading\":\"%s\"},"
             "\"maze_delta\":[],"
             "\"speed\":%.3f,\"battery\":%d,\"voltage\":%.2f}",
             ts, current_run_id, pose_x, pose_y, heading,
             speed, battery, voltage);
    mqtt_publish(topic, payload, 0);
    printf("[TELEMETRIA] x=%d y=%d hdg=%s spd=%.2f bat=%d%%\r\n",
           pose_x, pose_y, heading, speed, battery);
}

// ─── Parser de comandos JSON ──────────────────────────────────────────────────
// Extrai o valor de uma chave string simples: "key":"value"
static int extract_string(const char *json, const char *key, char *out, int out_size) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return 0;
    p += strlen(search);
    while (*p == ' ' || *p == ':') p++;
    if (*p != '"') return 0;
    p++; // pula a aspa de abertura
    int i = 0;
    while (*p && *p != '"' && i < out_size - 1)
        out[i++] = *p++;
    out[i] = '\0';
    return i > 0;
}

static void handle_command(const char *payload) {
    printf("[CMD] Recebido: %s\r\n", payload);

    char acao[16] = "";
    extract_string(payload, "acao", acao, sizeof(acao));

    if (strcmp(acao, "start") == 0) {
        char run_id[48] = "";
        extract_string(payload, "run_id", run_id, sizeof(run_id));
        if (strlen(run_id) == 0) {
            printf("[CMD] Ignorado: start sem run_id\r\n");
            return;
        }
        strncpy(current_run_id, run_id, sizeof(current_run_id) - 1);
        current_run_id[sizeof(current_run_id) - 1] = '\0';
        running = true;
        printf("[CMD] Start → run_id=%s\r\n", current_run_id);
        publish_evento("inicio");

    } else if (strcmp(acao, "stop") == 0) {
        running = false;
        current_run_id[0] = '\0';
        mutex_enter_blocking(&motor_mutex);
        desiredLeft  = 0;
        desiredRight = 0;
        mutex_exit(&motor_mutex);
        printf("[CMD] Stop\r\n");

    } else {
        printf("[CMD] Acao desconhecida: '%s'\r\n", acao);
    }
}

// ─── Recebe e processa pacotes MQTT recebidos (não-bloqueante) ────────────────
static void mqtt_receive(void) {
    if (mqtt_sock < 0) return;
    uint8_t buf[512];
    int r = recv(mqtt_sock, buf, sizeof(buf), MSG_DONTWAIT);
    if (r <= 0) return;

    int idx = 0;
    while (idx < r) {
        uint8_t byte1 = buf[idx++];
        if (idx >= r) break;
        uint8_t pkt_type = (byte1 >> 4) & 0x0F;

        // Lê Remaining Length (assume 1 byte para payloads pequenos)
        uint8_t rem = buf[idx++];

        if (pkt_type == 3) { // PUBLISH
            if (idx + 2 > r) break;
            uint16_t tlen = ((uint16_t)buf[idx] << 8) | buf[idx + 1];
            idx += 2;
            if (idx + tlen > r) break;
            char topic[128] = "";
            int copy = tlen < (int)sizeof(topic) - 1 ? tlen : (int)sizeof(topic) - 1;
            memcpy(topic, buf + idx, copy);
            topic[copy] = '\0';
            idx += tlen;

            int payload_len = rem - 2 - tlen;
            if (payload_len < 0) payload_len = 0;
            if (payload_len > (int)(sizeof(buf) - idx)) payload_len = sizeof(buf) - idx;
            char payload[256] = "";
            int pl = payload_len < (int)sizeof(payload) - 1 ? payload_len : (int)sizeof(payload) - 1;
            memcpy(payload, buf + idx, pl);
            payload[pl] = '\0';
            idx += payload_len;

            handle_command(payload);
        } else {
            // Pacote de outro tipo (PINGRESP, etc.) — pula
            idx += rem;
        }
    }
}

// ─── Envio de PINGREQ para manter keepalive ───────────────────────────────────
static void mqtt_ping(void) {
    if (mqtt_sock < 0) return;
    uint8_t pkt[2] = {0xC0, 0x00}; // PINGREQ
    send(mqtt_sock, pkt, 2, 0);
}

// ─── Core 1: placeholder para navegação autônoma ─────────────────────────────
// A lógica real de flood-fill / wall-follow deve ser implementada aqui usando
// MazeMapper e os sensores de distância.
static void core1_navigation(void) {
    while (true) {
        if (running) {
            // TODO: ler sensores, atualizar MazeMapper, decidir próxima célula
            // Exemplo mínimo: manter velocidade constante se running
            // mutex_enter_blocking(&motor_mutex);
            // desiredLeft  = 100;
            // desiredRight = 100;
            // mutex_exit(&motor_mutex);
        } else {
            mutex_enter_blocking(&motor_mutex);
            desiredLeft  = 0;
            desiredRight = 0;
            mutex_exit(&motor_mutex);
        }
        sleep_ms(20);
    }
}

// ─── Função auxiliar: milissegundos desde boot ────────────────────────────────
inline uint32_t millis_pico(void) {
    return to_ms_since_boot(get_absolute_time());
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main(void) {
    stdio_init_all();
    sleep_ms(200);

    printf("\r\n========================================\r\n");
    printf("Micromouse — Firmware Pico W  [ID: %s]\r\n", MOUSE_ID);
    printf("========================================\r\n");

    // Motores
    motorEsq.inicializar();
    motorDir.inicializar();
    mutex_init(&motor_mutex);
    printf("[OK] Motores prontos\r\n");

    // Wi-Fi
    if (cyw43_arch_init()) {
        printf("[WIFI] Falha ao inicializar cyw43\r\n");
    } else {
        int rc = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 15000);
        if (rc == 0)
            printf("[WIFI] Conectado à rede '%s'\r\n", WIFI_SSID);
        else
            printf("[WIFI] Falha na conexão (rc=%d) — operando offline\r\n", rc);
    }

    // Lança Core 1 (navegação autônoma)
    multicore_launch_core1(core1_navigation);

    // MQTT
    if (mqtt_connect() == 0) {
        printf("[MQTT] Conectado ao broker %s:%d\r\n", MQTT_SERVER, MQTT_PORT_NUM);

        // Assina o tópico de comandos
        char cmd_topic[128];
        snprintf(cmd_topic, sizeof(cmd_topic), "micromouse/%s/comando", MOUSE_ID);
        mqtt_subscribe(cmd_topic);
        printf("[MQTT] Assinando %s\r\n", cmd_topic);

        // Anuncia presença (retained — persiste no broker)
        publish_status_online();

        printf("[MQTT] Aguardando comando 'start' do dashboard...\r\n");
    } else {
        printf("[MQTT] Falha ao conectar ao broker — sem telemetria\r\n");
    }

    // ─── Loop principal ───────────────────────────────────────────────────────
    uint32_t ultimaTelemetria = millis_pico();
    uint32_t ultimoPing       = millis_pico();
    uint32_t ultimaAtualizacaoFrente = 0;
    // Dados de pose/sensores — substituir com valores reais do MazeMapper
    int   pose_x   = 0, pose_y = 0;
    const char *heading = "N";
    float speed    = 0.0f;
    int   battery  = 100;
    float voltage  = 7.4f;

    printf("[INIT] Sistema iniciado. Aguardando comandos...\r\n");

    while (true) {
        uint32_t agora = millis_pico();

        // Watchdog
        watchdog.alimentar();
        watchdog.verificar();

        // 2. Execução física — prioridade: RF02 (rotação) > RF01 (frente) > manual
        if (watchdog.isAtivo()) {
            uint32_t agoraMotores = millis_pico();

            if (rotacao.emRotacao()) {
                rotacao.atualizar(agoraMotores);
                ultimaAtualizacaoFrente = 0;
            } else if (movFrente.ativo()) {
                float dt = (ultimaAtualizacaoFrente > 0)
                    ? (agoraMotores - ultimaAtualizacaoFrente) / 1000.0f
                    : 0.01f;
                ultimaAtualizacaoFrente = agoraMotores;
                // Correção PID de heading (erroHeading=0 até MPU6050 integrado)
                movFrente.atualizar(0.0f, dt);
            } else {
                ultimaAtualizacaoFrente = 0;
                mutex_enter_blocking(&motor_mutex);
                int l = desiredLeft;
                int r = desiredRight;
                mutex_exit(&motor_mutex);
                motorEsq.setVelocidade(l);
                motorDir.setVelocidade(r);
                // Estima velocidade (placeholder até sensor real)
                speed = (abs(l) + abs(r)) / 2.0f / 255.0f * 0.5f;
            }
        }

        // Processa mensagens MQTT recebidas (comandos)
        mqtt_receive();

        // Telemetria a 10 Hz somente durante corrida
        if (running && strlen(current_run_id) > 0 &&
            agora - ultimaTelemetria >= 100) {
            ultimaTelemetria = agora;
            publish_telemetria(pose_x, pose_y, heading, speed, battery, voltage);

            // Desce bateria levemente (placeholder — usar ADC real)
            if (battery > 0) battery--;
        }

        // PINGREQ a cada 30s para manter keepalive com o broker
        if (agora - ultimoPing >= 30000) {
            ultimoPing = agora;
            mqtt_ping();
        }

        sleep_ms(10);
    }

    return 0;
}

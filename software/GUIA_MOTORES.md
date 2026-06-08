# Guia de Teste de Motores — Pico W + MQTT


### Software no Host
- **Pico SDK** instalado e configurado
- **arm-none-eabi-gcc**
- **Broker MQTT** (Mosquitto)
- **Ferramentas MQTT**: `mosquitto_pub` e `mosquitto_sub`

#### Instalar Mosquitto (Linux)
```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
sudo systemctl start mosquitto
sudo systemctl enable mosquitto  # iniciar ao boot
mosquitto_sub -v -t '#'  # verificar conexão
```

#### Instalar Pico SDK (se não tiver)
```bash
mkdir -p ~/pico
cd ~/pico
git clone https://github.com/raspberrypi/pico-sdk.git
export PICO_SDK_PATH=~/pico/pico-sdk
echo "export PICO_SDK_PATH=~/pico/pico-sdk" >> ~/.bashrc
```

---

## Fiação e Esquema

### Mapeamento de Pinos Pico W

| Função | Pino GPIO | Pino Físico | Observação |
|--------|-----------|-------------|-----------|
| **Motor Esquerdo** |
| PWM_A | GP15 | 20 | PWM para controlar velocidade |
| DIR1_A | GP14 | 19 | Direção 1 |
| DIR2_A | GP13 | 17 | Direção 2 |
| **Motor Direito** |
| PWM_B | GP12 | 16 | PWM para controlar velocidade |
| DIR1_B | GP11 | 15 | Direção 1 |
| DIR2_B | GP10 | 14 | Direção 2 |
| **Power** |
| 3V3 (OUT) | — | 36 | Alimentação lógica 3.3V |
| GND | — | 38 | Terra |

### Conexões TB6612FNG

| TB6612 | Pico W | Função |
|--------|--------|--------|
| AIN1 | GP14 | Direção motor A |
| AIN2 | GP13 | Direção motor A |
| PWMA | GP15 | PWM motor A |
| BIN1 | GP11 | Direção motor B |
| BIN2 | GP10 | Direção motor B |
| PWMB | GP12 | PWM motor B |
| VCC | 3V3 | Alimentação lógica |
| GND | GND | Terra comum |
| VM | +6V/+12V | Alimentação motores (bateria/PSU) |
| STBY | 3V3 ou GP | Standby (HIGH = ativo) |


### Checklist de Fiação

- [ ] GP15, GP14, GP13 conectados a PWM_A, AIN1, AIN2 (motor esquerdo)
- [ ] GP12, GP11, GP10 conectados a PWM_B, BIN1, BIN2 (motor direito)
- [ ] TB6612 VCC em 3V3 (Pico)
- [ ] TB6612 GND em GND (Pico + PSU)
- [ ] TB6612 VM em +6V/+12V (bateria)
- [ ] TB6612 STBY em 3V3 (ou GPIO configurado HIGH)
- [ ] Motores conectados em OUT_A e OUT_B
- [ ] Protoboard com terminações seguras (sem fios frouxos)

---

## Configuração de Software

### 1. Clonar/Verificar Código

O firmware está em: `software/firmware/src/main.cpp`

### 2. Ajustar Credenciais Wi-Fi e MQTT

Abra `software/firmware/src/main.cpp` e localize:

```cpp
// --- Inicializa Wi-Fi (Pico W) ---
const char *ssid = "SEU_SSID";          // ← altere aqui
const char *password = "SUA_SENHA";      // ← altere aqui
```

E logo abaixo:

```cpp
static const char *mqtt_server = "192.168.1.100";  // ← coloque IP do broker
static const int mqtt_port = 1883;
```

**Como encontrar o IP do broker:**
```bash
# Se Mosquitto está no PC host
hostname -I
# Ex.: 192.168.1.100

# Se em outro servidor
ip addr show  # (em máquinas Linux/Mac)
```

### 3. Verificar Configuração de Pinos (Opcional)

Se usar pinos diferentes, edite `software/firmware/include/Motor.h` e `software/firmware/src/Motor.cpp`:

```cpp
// Motor.h
Motor motorEsq(15, 14, 13);  // PWM, DIR1, DIR2 (esquerdo)
Motor motorDir(12, 11, 10);  // PWM, DIR1, DIR2 (direito)
```

---

## Compilação e Flashear

### Compilar

```bash
cd /home/usuario/2026_1_PI1_Grupo02_Juliana/software/firmware

# criar diretório de build
mkdir -p build
cd build

# configurar com CMake (Pico W)
cmake .. -DPICO_BOARD=pico_w

# compilar
make -j4
```

**Saída esperada:**
```
[100%] Linking CXX executable firmware.elf
[100%] Built target firmware
```

O arquivo UF2 estará em: `build/firmware.uf2`

### Flashear no Pico W

#### Opção 1: Modo BOOTSEL (Drag & Drop)

1. Desconecte Pico W do USB
2. Mantenha pressionado o botão **BOOTSEL**
3. Conecte USB (mantendo botão pressionado)
4. Uma unidade `RPI-RP2` aparecerá no gerenciador de arquivos
5. Arraste `build/firmware.uf2` para a unidade
6. Pico reinicia automaticamente

#### Opção 2: picotool

```bash
# instalar picotool
sudo apt install picotool

# flashear
cd /home/usuario/2026_1_PI1_Grupo02_Juliana/software/firmware/build
picotool load firmware.uf2
picotool reboot
```

#### Opção 3: openocd (se tiver probe SWD)

```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
  -c "program build/firmware.elf verify reset exit"
```

---

## Configuração Wi-Fi e MQTT

### Verificar Conectividade

Após flashear, abra o monitor serial:

```bash
# Encontrar porta serial
ls /dev/ttyACM*

# Abrir com picocom (115200 baud)
picocom -b 115200 /dev/ttyACM0
```

**Saída esperada no serial:**
```
========================================
Micromouse - Firmware Pico W
========================================
[INIT] Inicializando motores...
[OK] Motores inicializados
[WIFI] cyw43 iniciado
[WIFI] Conectado à rede 'SEU_SSID'
[MQTT] Conectado ao broker 192.168.1.100:1883
[TELEMETRIA] micromouse/micromouse_001/telemetria: {...}
```

### Encontrar IP do Pico W

Se DHCP estiver ativo:

```bash
# No seu router (verificar tabela DHCP)
# Ou usar nmap
nmap -p 1883 192.168.1.0/24  # verifica porta MQTT

# Resultado
Nmap scan report for 192.168.1.50
Host is up (0.020s latency).
```

---

## Testes Práticos

### ⚠️ ANTES DE QUALQUER TESTE

1. **Eleve os motores** do chão (rodas fora do solo ou sem rodas).
2. **Limitar corrente** da fonte para ~1 A inicial.
3. **Ter "kill switch"** manual (desconectar bateria rapidamente).
4. **Ler instruções** deste guia completamente.

### Teste 1: Verificar Telemetria (MQTT)

Em outro terminal, assine o tópico de telemetria:

```bash
mosquitto_sub -h 192.168.1.100 -t "micromouse/micromouse_001/#" -v
```

**Resultado esperado** (a cada ~100ms):
```
micromouse/micromouse_001/telemetria {"ts":"2026-05-25T12:00:00Z",...}
micromouse/micromouse_001/status online
```

### Teste 2: Ligar Motor Esquerdo (M1 ON)

```bash
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "M1 ON"
```

**Esperado:**
- Motor esquerdo gira (velocidade 128 = 50% duty)
- Serial mostra: Watchdog alimentado, velocidade desejada atualizada

### Teste 3: Desligar Motor Esquerdo

```bash
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "M1 OFF"
```

**Esperado:** Motor esquerdo para.

### Teste 4: Controle de Velocidade com SET

```bash
# Motor direito a 50% (valor 128 de 255)
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "SET M2 128"

# Motor direito a 100% (valor 255)
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "SET M2 255"

# Motor direito em marcha ré (valor -128)
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "SET M2 -128"
```

### Teste 5: Parar Emergência

```bash
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "STOP"
```

**Esperado:** Ambos os motores param (velocidade = 0).

### Teste 6: Rampa de Velocidade (Teste de Aceleração)

Execute script bash:

```bash
#!/bin/bash
# test_ramp.sh

MQTT_HOST="192.168.1.100"
TOPIC="micromouse/micromouse_001/cmd"
BROKER_OPTS="-h $MQTT_HOST -t $TOPIC"

echo "Rampa de velocidade: M1 de 0→255 em 10 passos (1s cada)"

for speed in 25 50 75 100 125 150 175 200 225 255; do
  echo "SET M1 $speed"
  mosquitto_pub $BROKER_OPTS -m "SET M1 $speed"
  sleep 1
done

echo "STOP"
mosquitto_pub $BROKER_OPTS -m "STOP"
```

**Executar:**
```bash
chmod +x test_ramp.sh
./test_ramp.sh
```

**Observações:**
- Aumentar velocidade devagar para evitar saltos mecânicos.
- Monitorar temperatura do TB6612 e dos motores.
- Monitorar corrente da PSU (não deve atingir limite).

### Teste 7: Teste de Watchdog (Timeout de Segurança)

O watchdog desativa os motores após 1000 ms sem alimentação (`watchdog.alimentar()`).

**Teste manual:**
1. Envie `M1 ON` (motor liga)
2. Aguarde 1,5 segundos SEM enviar novo comando
3. Verifique serial: `[CRITICO] Watchdog desarmou os motores!`
4. Motor deve parar

### Teste 8: Inversão de Sentido

Se motor girar no sentido oposto ao esperado:

**Opção A: Trocar pinos DIR**
```cpp
// Em Motor.cpp, in setVelocidade():
if (velocidade > 0) {
    gpio_put(pinDir1, LOW);   // trocou HIGH por LOW
    gpio_put(pinDir2, HIGH);  // trocou LOW por HIGH
}
```

**Opção B: Enviar velocidade negativa**
```bash
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "SET M1 -128"  # marcha ré
```

---

## Troubleshooting

### Problema: Pico não aparece após flashear

**Solução:**
- Pressione reset (botão RUN) no Pico W
- Se ainda não conectar, tente BOOTSEL novamente
- Verifique USB com: `lsusb | grep Raspberry`

### Problema: Serial não mostra nada

**Causas possíveis:**
- Porta serial errada: `ls /dev/ttyACM*`
- Baud rate errado: use 115200
- USB mal conectado

**Solução:**
```bash
dmesg | tail -n 20  # ver eventos USB
picocom -b 115200 /dev/ttyACM0
```

### Problema: Wi-Fi não conecta

**Verificações:**
- SSID e senha corretos?
- Verificar no serial: `[WIFI] Não conectado (rc=X)`
- Testar manualmente em outro dispositivo

**Solução:**
```cpp
// Debug: aumentar verbosidade
printf("[WIFI] rc=%d (1=bad_auth, -1=no_net, etc.)\r\n", rc);
```

### Problema: Motores não giram

**Checklist:**
- [ ] VM tem tensão? (voltímetro no TB6612 VM vs GND)
- [ ] STBY está em 3V3 (HIGH)?
- [ ] Pinos DIR conectados corretamente?
- [ ] PWM tem sinal? (osciloscópio em PWM_A/PWM_B)
- [ ] Fiação sólida (não frouxo)?

**Teste PWM com osciloscópio:**
```
Expected: ~1 kHz quadrado
Duty = (desired_speed * 1000) / 255
```

### Problema: MQTT não conecta

**Verificar broker:**
```bash
sudo systemctl status mosquitto
sudo systemctl restart mosquitto
mosquitto_pub -h 127.0.0.1 -t "test" -m "hello"  # teste local
```

**Verificar rede:**
```bash
ping 192.168.1.100  # testar conectividade
netstat -tlnp | grep 1883  # verificar porta MQTT
```

### Problema: Motores vibram ou travam

**Causas:**
- Duty cycle muito baixo (< 50% pode não ter torque)
- Mecânica travada (verificar eixo, rolamentos)
- PWM muito lento (ajustar `pwm_set_clkdiv`)

**Solução:**
```cpp
// Em Motor.cpp, aumentar frequência PWM
pwm_set_clkdiv(pwm_slice_num, 62.5f);  // 2 kHz em vez de 1 kHz
```

---

## Segurança e Emergência

### Kill Switch Manual

Quando NÃO há comandos de parada remota:

1. **Desconectar USB** (Pico reinicia)
2. **Desconectar bateria dos motores** (interruptor física)
3. **Pressionar reset (RUN)** no Pico W

### Procedimento de Encerramento

```bash
# 1. Parar motores
mosquitto_pub -h 192.168.1.100 \
  -t "micromouse/micromouse_001/cmd" \
  -m "STOP"

# 2. Desconectar bateria/PSU
# (física)

# 3. Desconectar USB
# (ou deixar conectado para debug serial)
```

### Limites de Operação

| Parâmetro | Valor Máximo | Observação |
|-----------|-------------|-----------|
| Corrente | 2 A | PSU com limitador |
| Temperatura TB6612 | 70 °C | Usar dissipador se necessário |
| Temperatura Motor | 60 °C | Evitar sobre-aquecimento |
| Duty Cycle | 100% | Máximo 30s contínuo (heat) |
| Velocidade Máxima | 255 | Full-scale PWM |

### Monitoria Contínua

Durante testes, mantenha visível:
- Serial monitor (watchdog/erros)
- Telemetria MQTT (`mosquitto_sub`)
- PSU (corrente/tensão)
- Temperatura TB6612 (toque leve com dedo)

---

## C

### `test_motors.sh` — Testes Automatizados

Salve em `software/test_motors.sh`:

```bash
#!/bin/bash

MQTT_HOST="192.168.1.100"
TOPIC="micromouse/micromouse_001/cmd"
BROKER="-h $MQTT_HOST -t $TOPIC"

echo "=== TESTE DE MOTORES PICO W ==="
echo "Host: $MQTT_HOST"
echo ""

# Teste 1: Ligar M1
echo "[1] Ligando M1..."
mosquitto_pub $BROKER -m "M1 ON"
sleep 2

# Teste 2: Ligar M2
echo "[2] Ligando M2..."
mosquitto_pub $BROKER -m "M2 ON"
sleep 2

# Teste 3: Parar
echo "[3] Parando..."
mosquitto_pub $BROKER -m "STOP"
sleep 1

# Teste 4: Rampa M1
echo "[4] Rampa M1..."
for speed in 50 100 150 200 255; do
  echo "  Velocidade: $speed"
  mosquitto_pub $BROKER -m "SET M1 $speed"
  sleep 0.5
done

# Teste 5: Parar final
echo "[5] Parada final..."
mosquitto_pub $BROKER -m "STOP"
echo "=== FIM DO TESTE ==="
```

### `monitor_telemetry.sh` — Monitor de Telemetria

```bash
#!/bin/bash
MQTT_HOST="192.168.1.100"
echo "Monitorando telemetria de micromouse_001..."
mosquitto_sub -h $MQTT_HOST -t "micromouse/micromouse_001/#" -v
```

---

## Referências

- [Datasheet TB6612FNG](https://www.pololu.com/file/download/TB6612FNG.pdf)
- [Pico W Pinout](https://datasheets.raspberrypi.com/pico/Pico-R3-A4-Pinout.pdf)
- [MQTT Specification](https://mqtt.org/mqtt-specification)
- [Mosquitto Documentation](https://mosquitto.org/documentation/)

---

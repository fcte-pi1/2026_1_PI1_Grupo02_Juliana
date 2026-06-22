# Guia de Teste de Motores — Pico W

## Pré-requisitos

Você precisa de: **Pico SDK**, **arm-none-eabi-gcc** e **CMake**.
O Mosquitto (MQTT) só é necessário para o firmware de integração completo, não para o teste standalone de motores.

---

### Linux — Fedora

```bash
# Toolchain ARM + CMake
sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-newlib cmake git

# Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init
echo 'export PICO_SDK_PATH=~/pico-sdk' >> ~/.bashrc
source ~/.bashrc

# Mosquitto (só para testes MQTT)
sudo dnf install mosquitto mosquitto-clients
sudo systemctl start mosquitto && sudo systemctl enable mosquitto
```

---

### Linux — Ubuntu/Debian

```bash
# Toolchain ARM + CMake
sudo apt update
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi cmake git build-essential

# Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init
echo 'export PICO_SDK_PATH=~/pico-sdk' >> ~/.bashrc
source ~/.bashrc

# Mosquitto (só para testes MQTT)
sudo apt install mosquitto mosquitto-clients
sudo systemctl start mosquitto && sudo systemctl enable mosquitto
```

---

### Windows

**Recomendado:** usar a extensão oficial do VS Code, que instala tudo automaticamente.

#### Opção A: Extensão Raspberry Pi Pico para VS Code (mais fácil)

1. Instale o [VS Code](https://code.visualstudio.com/)
2. Instale a extensão **"Raspberry Pi Pico"** (ID: `raspberry-pi.raspberry-pi-pico`)
3. A extensão baixa automaticamente: SDK, toolchain ARM, CMake e Ninja
4. Abra a pasta `software/firmware/motores/` no VS Code
5. Use o botão **"Compile Project"** na barra inferior

#### Opção B: Instalação manual

Instale na ordem:

| Ferramenta | Download |
|------------|----------|
| Git | https://git-scm.com/download/win |
| CMake | https://cmake.org/download/ (marque "Add to PATH") |
| Ninja | https://github.com/ninja-build/ninja/releases (extraia e adicione ao PATH) |
| ARM Toolchain | https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads → `arm-none-eabi` Windows x86_64 |

Depois, no **Git Bash** ou **PowerShell**:

```powershell
# Clonar o Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git C:\pico-sdk
cd C:\pico-sdk
git submodule update --init

# Configurar variável de ambiente (permanente)
[System.Environment]::SetEnvironmentVariable("PICO_SDK_PATH", "C:\pico-sdk", "User")
```

Reinicie o terminal após configurar a variável.

#### Mosquitto no Windows (só para testes MQTT)

Baixe o instalador em: https://mosquitto.org/download/

```powershell
# Verificar instalação
mosquitto -v
mosquitto_pub --help
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
| VM | +6V | Alimentação motores (bateria) |
| STBY | 3V3 | Standby (HIGH = ativo) |

### Checklist de Fiação

- [ ] GP15, GP14, GP13 conectados a PWMA, AIN1, AIN2 (motor esquerdo)
- [ ] GP12, GP11, GP10 conectados a PWMB, BIN1, BIN2 (motor direito)
- [ ] TB6612 VCC em 3V3 (Pico)
- [ ] TB6612 GND em GND (Pico + bateria)
- [ ] TB6612 VM em +6V (bateria)
- [ ] TB6612 STBY em 3V3 (ou GPIO configurado HIGH)
- [ ] Motores conectados em OUTA e OUTB
- [ ] Sem fios frouxos na protoboard

---

## Compilação e Flash

### Linux (Fedora / Ubuntu)

```bash
cd software/firmware/motores
mkdir -p build && cd build
cmake ..
make -j4
```

### Windows — VS Code (Extensão Pico)

1. Abra a pasta `software/firmware/motores/` no VS Code
2. Clique em **"Compile Project"** na barra de status inferior
3. O `.uf2` é gerado em `build/motores.uf2`

### Windows — Manual (PowerShell)

```powershell
cd software\firmware\motores
mkdir build; cd build
cmake .. -G Ninja
ninja
```

**Saída esperada (todos os sistemas):**
```
[100%] Linking CXX executable motores.elf
[100%] Built target motores
```

O arquivo UF2 estará em: `build/motores.uf2`

---

### Flashear no Pico W

O processo é igual em todos os sistemas operacionais:

1. Desconecte a Pico W do USB
2. Mantenha pressionado o botão **BOOTSEL**
3. Conecte o USB (mantendo o botão)
4. Uma unidade `RPI-RP2` aparece no gerenciador de arquivos
5. Copie o `.uf2` para a unidade:

**Linux:**
```bash
cp build/motores.uf2 /media/$USER/RPI-RP2/
```

**Windows (PowerShell):**
```powershell
Copy-Item build\motores.uf2 D:\   # troque D: pela letra da unidade RPI-RP2
```

**Windows (Explorer):** arraste o arquivo `motores.uf2` para a unidade `RPI-RP2`.

6. A Pico reinicia automaticamente.

---

## Monitorar via USB Serial

**Saída esperada:**
```
=== Teste de Motores ===
Pinos: ESQ(PWM=GP15 D1=GP14 D2=GP13) DIR(PWM=GP12 D1=GP11 D2=GP10)

[1/5] Frente - velocidade 150
[2/5] Parar
[3/5] Re - velocidade -150
[4/5] Curva direita
[5/5] Curva esquerda
[CICLO] Reiniciando sequencia...
```

### Linux

```bash
# Encontrar porta
ls /dev/ttyACM*

# minicom
minicom -b 115200 -D /dev/ttyACM0

# ou picocom
picocom -b 115200 /dev/ttyACM0
```

### Windows

A Pico aparece como porta `COMx` no Gerenciador de Dispositivos.

**Opção A — PuTTY** (recomendado):
1. Baixe em https://www.putty.org/
2. Connection type: **Serial**
3. Serial line: `COM3` (verifique no Gerenciador de Dispositivos)
4. Speed: `115200`
5. Clique em **Open**

**Opção B — VS Code:**
Instale a extensão **Serial Monitor** (`ms-vscode.vscode-serial-monitor`) e conecte na porta COM correta a 115200 baud.

**Opção C — PowerShell:**
```powershell
# Requer PuTTY instalado e no PATH
putty -serial COM3 -sercfg 115200,8,n,1,N
```

---

## Testes Práticos

### ⚠️ ANTES DE QUALQUER TESTE

1. **Eleve os motores** do chão (rodas fora do solo).
2. **Limitar corrente** da fonte para ~1 A inicial.
3. **Ter kill switch** manual (desconectar bateria rapidamente).

### Teste 1: Sequência automática (firmware standalone)

O `main.cpp` executa em loop: frente → parar → ré → curva direita → curva esquerda.
Monitore pelo serial e observe os motores físicos.

### Teste 2: Controle via MQTT (firmware de integração)

Para controle remoto via MQTT, use o firmware em `software/firmware/` (raiz).
Configure SSID, senha e IP do broker em `software/firmware/main.cpp`:

```cpp
const char *ssid = "SEU_SSID";
const char *password = "SUA_SENHA";
static const char *mqtt_server = "192.168.1.100";
```

**Linux:**
```bash
mosquitto_pub -h 192.168.1.100 -t "micromouse/micromouse_001/cmd" -m "M1 ON"
mosquitto_pub -h 192.168.1.100 -t "micromouse/micromouse_001/cmd" -m "M2 ON"
mosquitto_pub -h 192.168.1.100 -t "micromouse/micromouse_001/cmd" -m "STOP"
mosquitto_pub -h 192.168.1.100 -t "micromouse/micromouse_001/cmd" -m "SET M1 200"
mosquitto_pub -h 192.168.1.100 -t "micromouse/micromouse_001/cmd" -m "SET M2 -128"
```

**Windows (PowerShell):**
```powershell
mosquitto_pub -h 192.168.1.100 -t "micromouse/micromouse_001/cmd" -m "M1 ON"
mosquitto_pub -h 192.168.1.100 -t "micromouse/micromouse_001/cmd" -m "STOP"
```

### Teste 3: Rampa de velocidade

**Linux (bash):**
```bash
MQTT_HOST="192.168.1.100"
TOPIC="micromouse/micromouse_001/cmd"
for speed in 25 50 75 100 125 150 175 200 225 255; do
  mosquitto_pub -h $MQTT_HOST -t $TOPIC -m "SET M1 $speed"
  sleep 1
done
mosquitto_pub -h $MQTT_HOST -t $TOPIC -m "STOP"
```

**Windows (PowerShell):**
```powershell
$host_mqtt = "192.168.1.100"
$topic = "micromouse/micromouse_001/cmd"
foreach ($speed in 25,50,75,100,125,150,175,200,225,255) {
  mosquitto_pub -h $host_mqtt -t $topic -m "SET M1 $speed"
  Start-Sleep -Seconds 1
}
mosquitto_pub -h $host_mqtt -t $topic -m "STOP"
```

### Teste 4: Watchdog (segurança)

O watchdog para os motores após 2 segundos sem `alimentar()`.
No firmware standalone, o watchdog é alimentado a cada ciclo — não deve disparar em operação normal.

**Verificar no serial:**
```
[CRITICO] Watchdog desarmou os motores!
```

### Teste 5: Inversão de sentido

Se o motor girar no sentido oposto ao esperado, edite `software/firmware/motores/Motor.cpp`:

```cpp
if (velocidade > 0) {
    gpio_put(pinDir1, 0);  // invertido
    gpio_put(pinDir2, 1);  // invertido
}
```

---

## Troubleshooting

### Pico não aparece após flashear

**Linux:**
```bash
lsusb | grep Raspberry
dmesg | tail -20
```

**Windows:** abra o Gerenciador de Dispositivos e procure por `RPI-RP2` em "Drives de disco" ou `COM` em "Portas".

Se não aparecer, pressione o botão **RUN** (reset) enquanto mantém BOOTSEL.

### Serial não mostra nada

- Confirme baud rate: **115200**
- **Linux:** verifique se o usuário está no grupo `dialout`: `sudo usermod -aG dialout $USER` (requer logout)
- **Windows:** verifique a letra da porta COM no Gerenciador de Dispositivos

### Motores não giram

- [ ] VM tem tensão? (voltímetro no TB6612 VM vs GND)
- [ ] STBY está em 3V3 (HIGH)?
- [ ] Pinos DIR conectados corretamente?
- [ ] PWM tem sinal? (osciloscópio em PWMA/PWMB — esperado ~1 kHz)

**Ajustar frequência PWM** (`software/firmware/motores/Motor.cpp`):
```cpp
pwm_set_clkdiv(pwm_slice_num, 62.5f);  // 2 kHz em vez de 1 kHz
```

### Motores vibram ou travam

- Duty cycle muito baixo (< 30% pode não ter torque suficiente)
- Mecânica travada — verificar eixo e rolamentos

### MQTT não conecta

**Linux:**
```bash
sudo systemctl status mosquitto
ping 192.168.1.100
```

**Windows:**
```powershell
Test-NetConnection -ComputerName 192.168.1.100 -Port 1883
```

---

## Segurança

### Kill Switch Manual

1. Enviar `STOP` via MQTT (ou desconectar USB para firmware standalone)
2. Desconectar bateria dos motores
3. Pressionar reset (botão RUN) no Pico W

### Limites de Operação

| Parâmetro | Valor Máximo | Observação |
|-----------|-------------|-----------|
| Corrente | 2 A | PSU com limitador |
| Temperatura TB6612 | 70 °C | Usar dissipador se necessário |
| Temperatura Motor | 60 °C | Evitar sobre-aquecimento |
| Duty Cycle máximo | 100% | Máximo 30s contínuo |
| Velocidade (código) | ±255 | Full-scale PWM |

---

## Referências

- [Datasheet TB6612FNG](https://www.pololu.com/file/download/TB6612FNG.pdf)
- [Pico W Pinout](https://datasheets.raspberrypi.com/pico/Pico-R3-A4-Pinout.pdf)
- [Extensão Pico para VS Code](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico)
- [MQTT Specification](https://mqtt.org/mqtt-specification)
- [Mosquitto Documentation](https://mosquitto.org/documentation/)

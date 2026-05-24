# Periféricos e Pinagem — Micromouse MPU-6050

## Microcontrolador

| Item | Detalhe |
|------|---------|
| Placa | Raspberry Pi Pico W |
| Chip | RP2040 (dual-core Arm Cortex-M0+, 133 MHz) |
| Tensão de operação | 3,3 V |
| Memória Flash | 2 MB (W25Q16JV) |
| SRAM | 264 KB |

---

## Sensor utilizado — MPU-6050

| Item | Detalhe |
|------|---------|
| Modelo | InvenSense MPU-6050 |
| Protocolo | I²C (até 400 kHz) |
| Endereço I²C | `0x68` (pino AD0 em GND) / `0x69` (AD0 em 3,3 V) |
| Tensão de alimentação (VCC) | 3,3 V (VDD e VLOGIC) |
| Giroscópio | 3 eixos, ±250 / ±500 / ±1000 / ±2000 °/s |
| Acelerômetro | 3 eixos (não usado nesta versão) |

---

## Conexões — Pico W ↔ MPU-6050

| Pino Pico W | GPIO | Função I²C | Pino MPU-6050 | Descrição |
|:-----------:|:----:|:----------:|:-------------:|-----------|
| **Pin 6**   | GP4  | I2C0 SDA   | SDA           | Dados I²C |
| **Pin 7**   | GP5  | I2C0 SCL   | SCL           | Clock I²C |
| **Pin 36**  | 3V3(OUT) | Alimentação | VCC / VDD | 3,3 V para o sensor |
| **Pin 38**  | GND  | GND        | GND           | Terra comum |
| *(não ligado)* | — | —        | AD0           | Deixar em GND → endereço 0x68 |
| *(não ligado)* | — | —        | INT           | Interrupção (não usado) |

> **Nota:** Os resistores de pull-up (tipicamente 4,7 kΩ) para SDA e SCL são ativados via software
> (`gpio_pull_up`) no código. Módulos GY-521 já incluem pull-ups na placa.

---

## Periférico de hardware do RP2040 utilizado

| Periférico | Instância | Configuração |
|-----------|-----------|-------------|
| **I²C** | `i2c0` | 400 kHz (Fast Mode), Master |
| **GPIO** | GP4, GP5 | Função I2C + pull-up interno |
| **Timer** | Sistema (`get_absolute_time`) | Base de tempo para integração |
| **USB CDC** | `stdio_usb` | Serial via USB para depuração |

---

## Registradores do MPU-6050 acessados

| Registrador | Endereço | Uso |
|------------|---------|-----|
| `WHO_AM_I` | `0x75` | Verificação de identidade (deve retornar `0x68`) |
| `PWR_MGMT_1` | `0x6B` | Wake-up do dispositivo (escreve `0x00`) |
| `SMPLRT_DIV` | `0x19` | Divisor de taxa de amostragem (`0x07` → 125 Hz) |
| `CONFIG` | `0x1A` | DLPF: filtro passa-baixa digital (`0x03` → 42 Hz) |
| `GYRO_CONFIG` | `0x1B` | Escala do giroscópio (`0x00` → ±250 °/s) |
| `GYRO_XOUT_H/L` | `0x43–0x44` | Dado bruto eixo X |
| `GYRO_YOUT_H/L` | `0x45–0x46` | Dado bruto eixo Y |
| `GYRO_ZOUT_H/L` | `0x47–0x48` | Dado bruto eixo Z (yaw / heading) |

---

## Parâmetros de compilação

| Item | Valor |
|------|-------|
| SDK | Pico SDK 2.2.0 |
| Board target | `pico_w` |
| Compilador | `arm-none-eabi-gcc 14.2` |
| Padrão C | C11 |
| Padrão C++ | C++17 |
| Saída | `.uf2` (arrastar para o Pico em modo BOOTSEL) |

---

## Diagrama de conexão (ASCII)

```
Raspberry Pi Pico W          MPU-6050 (GY-521)
┌──────────────────┐         ┌──────────────┐
│ GP4 (Pin 6) SDA  ├─────────┤ SDA          │
│ GP5 (Pin 7) SCL  ├─────────┤ SCL          │
│ 3V3 (Pin 36)     ├─────────┤ VCC          │
│ GND (Pin 38)     ├─────────┤ GND          │
│                  │    GND──┤ AD0          │
└──────────────────┘         └──────────────┘
```

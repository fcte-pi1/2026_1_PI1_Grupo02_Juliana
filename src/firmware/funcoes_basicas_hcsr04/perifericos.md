# Periféricos e Pinagem — Micromouse HC-SR04

## Sensor utilizado — HC-SR04

| Item | Detalhe |
|------|---------|
| Modelo | HC-SR04 (ultrassônico) |
| Princípio | Tempo de voo (Time-of-Flight) de pulso ultrassônico de 40 kHz |
| Tensão de alimentação (VCC) | **5 V DC** |
| Corrente em operação | 15 mA |
| Faixa de medição | 2 cm — 400 cm |
| Resolução | ~3 mm |
| Ângulo efetivo | 15° |
| Largura do pulso TRIG | 10 µs (TTL) |
| Saída ECHO | Pulso TTL **5 V** proporcional à distância |
| Ciclo mínimo entre medidas | 60 ms |
| Fórmula | `distância_cm = tempo_echo_us / 58` |

> **ATENÇÃO — Nível lógico:** o pino ECHO sai em 5 V, mas os GPIOs do Pico W
> são tolerantes apenas a 3,3 V. É **obrigatório** usar um divisor resistivo
> (R1 = 1 kΩ, R2 = 2 kΩ) ou um level shifter na linha ECHO → GP17, sob risco
> de danificar permanentemente o RP2040.

---

## Conexões — Pico W ↔ HC-SR04

| Pino Pico W | GPIO | Função | Pino HC-SR04 | Descrição |
|:-----------:|:----:|:------:|:------------:|-----------|
| **Pin 21**  | GP16 | Saída digital | TRIG | Pulso de disparo de 10 µs |
| **Pin 22**  | GP17 | Entrada digital | ECHO | Pulso de retorno (via divisor 5 V→3,3 V) |
| **Pin 40**  | VBUS (5 V) | Alimentação | VCC | 5 V para o sensor |
| **Pin 38**  | GND  | GND | GND | Terra comum |

### Divisor de tensão para o pino ECHO

```
HC-SR04 ECHO ──┬── R1 (1 kΩ) ──┬── GP17 (Pico W)
              (5 V)            │
                               R2 (2 kΩ)
                               │
                              GND
```

V_GP17 = V_ECHO × R2 / (R1 + R2) = 5 V × 2/3 ≈ 3,3 V ✅

---

## Periférico de hardware do RP2040 utilizado

| Periférico | Instância | Configuração |
|-----------|-----------|-------------|
| **GPIO** | GP16 (TRIG) | Saída digital, push-pull |
| **GPIO** | GP17 (ECHO) | Entrada digital, sem pull |
| **Timer** | Sistema (`time_us_32`) | Medição do pulso ECHO em µs |
| **USB CDC** | `stdio_usb` | Serial via USB para depuração |

---

## Parâmetros de tempo (datasheet)

| Parâmetro | Valor | Uso no código |
|----------|-------|---------------|
| Largura do pulso TRIG | 10 µs | `HCSR04_TRIG_PULSE_US` |
| Tempo de estabilização inicial | 50 ms | `HCSR04_SETTLE_MS` |
| Ciclo mínimo entre medidas | 60 ms | `HCSR04_MIN_CYCLE_MS` |
| Timeout do pulso ECHO | 30 ms (~5 m) | `HCSR04_ECHO_TIMEOUT_US` |

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
Raspberry Pi Pico W                  HC-SR04
┌──────────────────┐                ┌──────────────┐
│ VBUS (Pin 40, 5V)├─────────────── ┤ VCC          │
│ GP16 (Pin 21)    ├─────────────── ┤ TRIG         │
│ GP17 (Pin 22)    ├──[ resistor ]──┤ ECHO         │
│ GND  (Pin 38)    ├─────────────── ┤ GND          │
└──────────────────┘                └──────────────┘
```

# Hardware do Micromouse — Documentação (módulo MPU-6050)

## Microcontrolador
- **Raspberry Pi Pico W** (RP2040, dual-core ARM Cortex-M0+ @ 133 MHz)

## Sensor
- **MPU-6050** (acelerômetro + giroscópio de 3 eixos)
- Uso atual: **giroscópio nos 3 eixos (X, Y, Z)** → roll, pitch, yaw
- Comunicação: I²C @ 400 kHz (fast mode)
- Endereço I²C: `0x68` (pino AD0 em GND)

---

## Mapeamento de pinos (GPIO da Pico W)

### I²C — MPU-6050
| Sinal     | GPIO Pico W | Pino físico | Função |
|-----------|-------------|-------------|--------|
| SDA       | GP4         | 6           | I2C0 SDA (dados) |
| SCL       | GP5         | 7           | I2C0 SCL (clock) |
| VCC MPU   | 3V3 (OUT)   | 36          | Alimentação 3.3 V |
| GND MPU   | GND         | 38          | Terra |
| AD0 MPU   | GND         | —           | Define endereço I²C como 0x68 |

---

## Periféricos do RP2040 utilizados
| Periférico | Uso |
|------------|-----|
| I2C0 | Comunicação com o MPU-6050 (SDA=GP4, SCL=GP5) |
| Timer (via `absolute_time`) | Medir Δt entre leituras para integração da orientação |
| USB (stdio) | Saída de `printf` para debug no terminal serial |
| CYW43 (Wi-Fi chip) | Controle do LED onboard da Pico W via `cyw43_arch_gpio_put` |

---

## Registradores do MPU-6050 utilizados

| Endereço | Nome           | Função no código |
|----------|----------------|------------------|
| 0x19     | SMPLRT_DIV     | Divisor da taxa de amostragem: valor 7 → 125 Hz |
| 0x1A     | CONFIG         | Filtro digital DLPF_CFG=3 → banda 42 Hz, atraso 4.8 ms |
| 0x1B     | GYRO_CONFIG    | Escala do giroscópio (FS_SEL=0 → ±250 °/s) |
| 0x1C     | ACCEL_CONFIG   | Definido no header (reservado para uso futuro) |
| 0x38     | INT_ENABLE     | Definido no header (reservado para uso futuro) |
| 0x43-0x44| GYRO_XOUT_H/L  | Leitura do eixo X do giroscópio (roll) |
| 0x45-0x46| GYRO_YOUT_H/L  | Leitura do eixo Y do giroscópio (pitch) |
| 0x47-0x48| GYRO_ZOUT_H/L  | Leitura do eixo Z do giroscópio (yaw/heading) |
| 0x6B     | PWR_MGMT_1     | Acorda o sensor (limpa sleep bit) |
| 0x75     | WHO_AM_I       | Verificação de identidade — deve retornar `0x68` |

---

## Configuração do giroscópio

| Parâmetro | Valor | Descrição |
|-----------|-------|-----------|
| Full scale | ±250 °/s (`FS_SEL=0`) | Sensibilidade: 131 LSB/(°/s) |
| Taxa de amostragem | 125 Hz | `SMPLRT_DIV=7` → 1000 / (1+7) |
| DLPF | 42 Hz de banda | `DLPF_CFG=3`, reduz ruído de alta frequência |
| Loop principal | 8 ms (~125 Hz) | Definido por `LOOP_MS` em `main.cpp` |
| Amostras de calibração | 500 | Definido por `GYRO_CALIB_SAMPLES` em `mpu6050.h` |

---

## Organização do código

O módulo foi estruturado em três arquivos:

| Arquivo | Conteúdo |
|---------|----------|
| `mpu6050.h` | Definições de registradores, constantes de escala, structs e declarações da API pública |
| `mpu6050.cpp` | Implementação da API: init, leitura, calibração, integração de orientação |
| `main.cpp` | Loop de aplicação: demo com saída serial, LED de feedback e mapeamento cardinal |

---

## API pública (`mpu6050.h`)

### Structs

| Struct | Campos | Unidade |
|--------|--------|---------|
| `GyroData` | `x`, `y`, `z` | °/s (já convertido da leitura bruta) |
| `GyroBias` | `x`, `y`, `z` | °/s (offset medido na calibração) |
| `Orientation` | `yaw`, `pitch`, `roll` | graus (integrado ao longo do tempo) |

### Funções

| Função | Descrição |
|--------|-----------|
| `mpu6050_init()` | Inicializa I²C, verifica WHO_AM_I, configura DLPF e escala |
| `mpu6050_read_gyro(out)` | Lê os 6 bytes dos 3 eixos e converte para °/s |
| `mpu6050_calibrate(bias)` | Média de 500 amostras em repouso para calcular o bias |
| `mpu6050_update_orientation(gyro, bias, orient, dt)` | Integra `(gyro - bias) * dt` nos 3 ângulos |
| `mpu6050_reset_orientation(orient)` | Zera yaw, pitch e roll |
| `mpu6050_normalise_heading(deg)` | Normaliza o ângulo para o intervalo `[0, 360)` |

---

## Mapeamento de direções cardinais (main.cpp)

| Intervalo de heading | Direção |
|----------------------|---------|
| `[0°, 45°)` e `[315°, 360°)` | NORTH |
| `[45°, 135°)` | EAST |
| `[135°, 225°)` | SOUTH |
| `[225°, 315°)` | WEST |

---

## LED onboard — feedback visual

O LED da Pico W é controlado via chip CYW43 (não via GPIO direto).

| Evento | Comportamento do LED |
|--------|----------------------|
| Firmware iniciado | 3 blinks rápidos (100 ms on/off) |
| Sensor não encontrado | Blinks contínuos de 50 ms (loop de erro) |
| Calibrando (robô em repouso) | LED aceso continuamente |
| Calibração concluída | 2 blinks lentos (400 ms on / 200 ms off) |
| Loop em execução (heartbeat) | Toggle a cada ~125 iterações (~1 Hz) |

---

## Convenção de eixos

- **Eixo Z (yaw)**: perpendicular ao chão — giro do robô, usado como heading de navegação.
- **Eixo X (roll)**: inclinação lateral.
- **Eixo Y (pitch)**: inclinação frontal/traseira.

> Nota: a integração do yaw aplica sinal negativo (`yaw -= gz * dt`) para alinhar o sentido de rotação com a convenção de heading do labirinto.

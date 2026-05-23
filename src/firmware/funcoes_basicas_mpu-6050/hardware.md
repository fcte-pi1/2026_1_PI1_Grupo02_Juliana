# Hardware do Micromouse — Documentação (módulo Giroscópio)

## Microcontrolador
- **Raspberry Pi Pico W** (RP2040, dual-core ARM Cortex-M0+ @ 133 MHz)

## Sensor 
- **MPU-6050** (acelerômetro + giroscópio de 3 eixos)
- Uso atual: somente o **giroscópio no eixo Z** (yaw)
- Comunicação: I²C @ 400 kHz
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
| I2C0       | Comunicação com o MPU-6050 |
| Timer (via `absolute_time`) | Medir Δt entre leituras do giroscópio |
| USB (stdio) | Saída de `printf` para debug no terminal |

---

## Registradores do MPU-6050 utilizados

| Endereço | Nome           | Função no código |
|----------|----------------|------------------|
| 0x1A     | CONFIG         | Configura o filtro digital DLPF |
| 0x19     | SMPLRT_DIV     | Divisor da taxa de amostragem |
| 0x1B     | GYRO_CONFIG    | Escala do giroscópio (±250°/s) |
| 0x47-0x48| GYRO_ZOUT_H/L  | Leitura do eixo Z do giroscópio |
| 0x6B     | PWR_MGMT_1     | Acorda o sensor e seleciona clock |
| 0x75     | WHO_AM_I       | Verificação de identidade (deve retornar 0x68) |

## Convenção de eixos

- **Eixo Z**: vertical, perpendicular ao chão. Rotação no Z = giro do robô (yaw).
- **Eixos X e Y**: não são usados neste módulo (seriam pitch e roll).
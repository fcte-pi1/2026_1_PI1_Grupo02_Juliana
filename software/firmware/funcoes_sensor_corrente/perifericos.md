# Periféricos e Pinagem — Micromouse Sensor de Corrente (HW-872)

## Microcontrolador

| Item | Detalhe |
|------|---------|
| Placa | Raspberry Pi Pico W |
| Chip | RP2040 (dual-core Arm Cortex-M0+, 133 MHz) |
| Tensão de operação | 3,3 V |
| ADC | 12 bits, canais ADC0-ADC2 em GP26-GP28, Vref = 3,3 V |

---

## Sensor utilizado — HW-872A/B/C (efeito Hall)

| Item | Detalhe |
|------|---------|
| Modelo | HW-872A/B/C — sensor de corrente por efeito Hall (família ACS712) |
| Princípio | Efeito Hall: tensão de saída linear, proporcional à corrente que atravessa IP+/IP- |
| Faixa selecionável (jumper) | 5 A / 10 A / 20 A / 30 A (resistor de range soldado na placa) |
| Sensibilidade de referência | 185 mV/A (5A) · 100 mV/A (10A) · 66 mV/A (20A) · 40 mV/A (30A) |
| Saída em repouso (0 A) | ~VCC/2 (offset de zero, não é 0 V) |
| Isolação | Galvânica entre o caminho de potência (IP+/IP-) e o sinal (VCC/OUT/GND) |

> **ATENÇÃO — Nível lógico do pino OUT:** o ADC do RP2040 só aceita até 3,3 V.
> O código deste módulo assume o HW-872 alimentado em **3,3 V** (`VCC` ligado ao
> pino 3V3(OUT) do Pico), o que mantém a saída do sensor dentro da faixa do ADC.
> Se o seu módulo só funcionar corretamente em 5 V, será necessário um divisor
> resistivo (como o usado no ECHO do HC-SR04) ou level shifter na linha
> OUT → GPIO, e os valores de `ADC_VREF_MV` e `zero_offset_mv` precisam ser
> recalculados de acordo.

---

## Conexões — Pico W ↔ HW-872

| Pino Pico W | GPIO | Função | Pino HW-872 | Descrição |
|:-----------:|:----:|:------:|:-----------:|-----------|
| **Pin 32**  | GP27 (ADC1) | Entrada analógica | OUT | Tensão proporcional à corrente medida |
| **Pin 36**  | 3V3(OUT) | Alimentação | VCC | 3,3 V para o módulo |
| **Pin 38**  | GND | GND | GND | Terra comum |
| — | — | Caminho de potência | IP+ / IP- | Carga cuja corrente será medida, em série no terminal verde |

---

## Periférico de hardware do RP2040 utilizado

| Periférico | Instância | Configuração |
|-----------|-----------|---------------|
| **ADC** | Canal 1 (GP27) | 12 bits, leitura por software (`adc_read`) |
| **Timer** | `sleep_us` / `sleep_ms` | Espaçamento entre amostras e ciclo de leitura |
| **USB CDC** | `stdio_usb` | Serial via USB para debug/validação |

---

## Funcionamento do código

### Estrutura de arquivos

| Arquivo | Conteúdo |
|---------|----------|
| `current_sensor.h` | Struct `current_sensor_t`, constantes de sensibilidade por range e declarações da API pública |
| `current_sensor.c` | Implementação: inicialização do ADC, leitura crua, calibração de zero e conversão para corrente |
| `main.c` | Demo simples: imprime a corrente medida a cada 500 ms |
| `test_current_sensor.c` | Firmware de validação: calibra o zero, imprime RAW/mV/mA e alerta sobre leituras suspeitas |

### Struct `current_sensor_t`

| Campo | Significado |
|-------|--------------|
| `gpio` | Pino GPIO ligado ao OUT do sensor |
| `adc_channel` | Canal ADC correspondente (`gpio - 26`) |
| `sensitivity_mv_per_a` | Sensibilidade do range selecionado no jumper (mV por Ampere) |
| `zero_offset_mv` | Tensão de saída medida com corrente = 0 A (offset de zero) |

### Fluxo de leitura — `current_sensor_read_ma()`

1. `read_raw_avg()` faz 16 leituras do ADC com 100 µs de intervalo entre elas e tira a média, reduzindo ruído de leitura.
2. O valor médio (0–4095) é convertido para milivolts no pino: `mv = (raw * 3300) / 4095`.
3. Subtrai o offset de zero (`zero_offset_mv`) do resultado, isolando o delta de tensão causado apenas pela corrente.
4. Divide esse delta pela sensibilidade (`mV/A`) e multiplica por 1000 para obter o resultado em **miliamperes**. O sinal indica o sentido do fluxo de corrente entre IP+ e IP-.

### Calibração de zero — `current_sensor_calibrate_zero()`

Como a saída do sensor Hall não é 0 V em repouso (fica em torno de VCC/2), é necessário
medir esse offset real antes de confiar nas leituras de corrente — variações de
fabricação e temperatura deslocam esse valor. A função lê a tensão de saída com a carga
de potência desligada/desconectada de IP+/IP- e grava o resultado em `zero_offset_mv`,
substituindo o valor padrão passado em `current_sensor_init()`.

### `main.c` — demo

Inicializa o sensor no GPIO27 assumindo o jumper de range em 5 A
(`CURRENT_SENSOR_SENS_5A`) e um zero padrão de 1650 mV (metade de 3,3 V), e imprime a
corrente lida a cada 500 ms.

### `test_current_sensor.c` — validação

Além do que o `main.c` faz, este firmware:
- Executa `current_sensor_calibrate_zero()` no início e imprime o offset medido;
- Mostra a leitura crua do ADC e a tensão de saída em mV ao lado da corrente calculada;
- Emite avisos automáticos: `RAW = 0` (fio do OUT desconectado ou GND não comum) e
  `RAW no máximo` (possível saturação — corrente acima do range selecionado no jumper).

---

## Parâmetros de compilação

| Item | Valor |
|------|-------|
| SDK | Pico SDK 2.2.0 |
| Board target | `pico_w` |
| Compilador | `arm-none-eabi-gcc 14.2` |
| Padrão C | C11 |
| Saída | `.uf2` (arrastar para o Pico em modo BOOTSEL) |

---

## Diagrama de conexão (ASCII)

```
Raspberry Pi Pico W              HW-872 (efeito Hall)
┌──────────────────┐             ┌───────────────────┐
│ GP27 (Pin 32) OUT├─────────────┤ OUT                │
│ 3V3  (Pin 36)    ├─────────────┤ VCC                │
│ GND  (Pin 38)    ├─────────────┤ GND                │
└──────────────────┘             │                    │
                                  │  IP+ ──[ carga ]── IP- │
                                  └───────────────────┘
```

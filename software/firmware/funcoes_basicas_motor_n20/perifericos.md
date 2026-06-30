# Periféricos e Pinagem — Motor N20 com Encoder (ASLONG JGA12-N10B)

## Microcontrolador

| Item | Detalhe |
|------|---------|
| Placa | Raspberry Pi Pico W |
| Chip | RP2040 (dual-core Arm Cortex-M0+, 133 MHz) |
| Tensão de operação / nível lógico | 3,3 V (GPIO **não** é tolerante a 5 V) |

---

## Atuador utilizado — Motor N20 com encoder (datasheet ASLONG)

Modelo do datasheet enviado (*12MM Small Dc Gear Motor*): família **N20** da
ASLONG. A versão **com encoder** é a **JGA12-N10B** (página "Encoder connecting
Method" do PDF).

| Item | Detalhe |
|------|---------|
| Motor | DC com escovas, corpo 12 mm (N20) |
| Tensão de trabalho | 3–6 V (nominal **6 V**) |
| Corrente sem carga | ~40 mA |
| Corrente com carga | ~130 mA |
| Corrente de bloqueio (stall) | ~0,5 A (500 mA) |
| Reduções disponíveis (Ratio) | 10 · 30 · 50 · 100 · 150 · 210 · 298 · 380 (`:1`) |
| Encoder | Magnético (efeito Hall), **2 canais em quadratura** (C1 e C2) |
| Pulsos por volta do eixo do motor | 7 PPR (típico da família — ver nota abaixo) |

> **Nota sobre PPR:** o datasheet não especifica os pulsos por volta do encoder.
> O valor **7 PPR** por canal (no eixo do motor, antes da redução) é o padrão
> dessa família de encoder Hall e está definido em `ENCODER_PPR_MOTOR`. Se
> precisar de precisão, confirme contando os pulsos de uma volta completa do
> eixo do motor e ajuste a constante.

### Contagens por volta do eixo de saída

```
contagens_por_volta = ENCODER_PPR_MOTOR (7) × quadratura x4 (4) × Ratio
```

Ex.: redução 100:1 → 7 × 4 × 100 = **2800 contagens por volta do eixo de saída**.

---

## Fiação do motor (6 fios) — datasheet JGA12-N10B

| Cor do fio | Sinal | Função |
|:----------:|:-----:|--------|
| **Vermelho** | M+ (M2) | Alimentação do motor (+) — vai à saída da ponte H |
| **Branco**   | M− (M1) | Alimentação do motor (−) — vai à saída da ponte H |
| **Preto**    | VCC | Alimentação do encoder (+) — **3,3 V** |
| **Azul**     | GND | Terra do encoder (−) |
| **Amarelo**  | C2  | Saída do sensor Hall (canal B) |
| **Verde**    | C1  | Saída do sensor Hall (canal A) |

> **ATENÇÃO — alimentação do encoder:** alimente VCC do encoder em **3,3 V**
> para que as saídas C1/C2 fiquem em nível 3,3 V e seguras para o GPIO do
> RP2040. Se o encoder for alimentado em 5 V, as saídas chegarão a 5 V e
> exigirão divisor resistivo ou level shifter nas linhas C1/C2, igual ao que é
> feito no ECHO do HC-SR04.

> **ATENÇÃO — ponte H obrigatória:** o RP2040 não aciona o motor diretamente
> (corrente de stall ~0,5 A). Use uma ponte H (ex.: **TB6612FNG** ou DRV8833)
> entre os GPIOs e os fios M+/M−.

---

## Conexões — Pico W ↔ Ponte H (TB6612FNG) ↔ Motor/Encoder

| Pino Pico W | GPIO | Função | Destino |
|:-----------:|:----:|:------:|---------|
| **Pin 20**  | GP15 | PWM (velocidade) | PWMA da ponte H |
| **Pin 19**  | GP14 | Direção 1 | AIN1 da ponte H |
| **Pin 17**  | GP13 | Direção 2 | AIN2 da ponte H |
| **Pin 21**  | GP16 | Encoder canal A (entrada) | C1 (fio verde) |
| **Pin 22**  | GP17 | Encoder canal B (entrada) | C2 (fio amarelo) |
| **Pin 36**  | 3V3(OUT) | Alimentação encoder | VCC (fio preto) |
| **Pin 38**  | GND | Terra comum | GND (fio azul) + GND da ponte H |

> Os pinos acima são os usados no `main.c`/`test_motor_n20.c` e podem ser
> trocados. Se a contagem do encoder ficar com o sinal invertido em relação ao
> sentido comandado, basta trocar GP16↔GP17 (ou AIN1↔AIN2).

---

## Periféricos de hardware do RP2040 utilizados

| Periférico | Instância | Configuração |
|-----------|-----------|---------------|
| **PWM** | Slice do GP15 | ~20 kHz (fora da faixa audível), `wrap` = 255 |
| **GPIO (saída)** | GP14, GP13 | Linhas de direção da ponte H |
| **GPIO + IRQ** | GP16, GP17 | Entrada com pull-up, interrupção em ambas as bordas (quadratura x4) |
| **Timer** | `time_us_64` / `sleep_ms` | Base de tempo para cálculo de RPM e ciclo do demo |
| **USB CDC** | `stdio_usb` | Serial via USB para debug/validação |

---

## Funcionamento do código

### Estrutura de arquivos

| Arquivo | Conteúdo |
|---------|----------|
| `motor_n20.h` / `motor_n20.c` | Controle do motor via ponte H: PWM de velocidade e direção (`set_speed`, `stop`, `brake`) |
| `encoder.h` / `encoder.c` | Encoder em quadratura por IRQ: contagem, voltas e RPM |
| `main.c` | Demo: acelera frente/ré e imprime contagem, voltas e RPM |
| `test_motor_n20.c` | Validação: testa sentido, contagem do encoder, RPM e coerência sinal↔direção |

### Motor — `motor_n20_set_speed()`

Recebe um valor de `-255` a `+255` (mesma convenção da classe `Motor` já usada
no firmware). O **sinal** define o sentido (positivo = frente via IN1=1/IN2=0;
negativo = ré via IN1=0/IN2=1) e o **módulo** vira o nível do PWM diretamente
(o `wrap` do PWM é 255). `motor_n20_stop()` deixa o motor em roda-livre
(IN1=IN2=0) e `motor_n20_brake()` faz frenagem ativa (IN1=IN2=1).

### Encoder — decodificação em quadratura x4

1. Os canais A (C1) e B (C2) são lidos como entrada com pull-up.
2. Cada borda (subida ou descida) de qualquer um dos canais dispara uma IRQ.
3. O callback monta o índice `(estado_anterior << 2) | estado_atual` e consulta
   uma **tabela de transição** que retorna `+1`, `-1` ou `0`, acumulando em
   `count`. Assim contam-se as 4 transições de cada ciclo (resolução x4).
4. O RP2040 tem **um único callback de IRQ de GPIO por núcleo**; por isso os
   encoders são registrados num vetor estático (`ENCODER_MAX`) e o callback
   compartilhado despacha para o encoder cujo pino gerou o evento. Isso permite
   usar 2 encoders (motor esquerdo e direito) simultaneamente.

### Cálculo de RPM — `encoder_get_rpm()`

Calcula a velocidade a partir da **variação de contagem** desde a última chamada
e do tempo decorrido (`time_us_64`). Deve ser chamada periodicamente (ex.: a
cada 100–200 ms); intervalos muito curtos aumentam o ruído da medida.

---

## Parâmetros de compilação

| Item | Valor |
|------|-------|
| SDK | Pico SDK 2.2.0 |
| Board target | `pico_w` |
| Compilador | `arm-none-eabi-gcc 14.2` |
| Padrão C | C11 |
| Saída | `.uf2` (arrastar para o Pico em modo BOOTSEL) |

Gera dois executáveis: `MOTORN20` (demo) e `MOTORN20_TEST` (validação).

---

## Diagrama de conexão (ASCII)

```
Raspberry Pi Pico W          TB6612FNG (ponte H)        Motor N20 JGA12-N10B
┌──────────────────┐         ┌────────────────┐         ┌──────────────────┐
│ GP15 (Pin 20) PWM├─────────┤PWMA       AO1   ├─────────┤ M+ (vermelho)    │
│ GP14 (Pin 19)    ├─────────┤AIN1       AO2   ├─────────┤ M- (branco)      │
│ GP13 (Pin 17)    ├─────────┤AIN2            │          │                  │
│ GP16 (Pin 21) ◄──┼─────────────────────────────────────┤ C1 (verde)       │
│ GP17 (Pin 22) ◄──┼─────────────────────────────────────┤ C2 (amarelo)     │
│ 3V3  (Pin 36)    ├─────────────────────────────────────┤ VCC (preto)      │
│ GND  (Pin 38)    ├──────────┬──────────────────────────┤ GND (azul)       │
└──────────────────┘          └── GND da ponte H          └──────────────────┘
                         (Vmotor da ponte H = bateria 3–6 V)
```

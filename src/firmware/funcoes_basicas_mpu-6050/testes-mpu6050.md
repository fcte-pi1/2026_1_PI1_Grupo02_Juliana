# Relatório de Testes — MPU-6050 (Micromouse)

**Responsável:** Equipe de Eletrônica — Jorge Andrés Vásquez Vega
**Firmware:** `main.cpp` + `mpu6050.cpp` / `mpu6050.h`
**Hardware:** Raspberry Pi Pico W + MPU-6050

---

## 1. Objetivo

Validar o funcionamento do módulo MPU-6050 integrado à Raspberry Pi Pico W para uso no Micromouse, verificando:

- Inicialização I²C e detecção do sensor (WHO_AM_I)
- Calibração do bias do giroscópio
- Integração angular (yaw/pitch/roll) ao longo do tempo
- Mapeamento correto de heading para as quatro direções cardinais (NORTH, EAST, SOUTH, WEST)

---

## 2. Configuração do Hardware

### Montagem

O teste foi realizado com o sensor MPU-6050 conectado à Raspberry Pi Pico W em uma protoboard, conforme foto abaixo:

- **Microcontrolador:** Raspberry Pi Pico W (RP2040 @ 133 MHz)
- **Sensor:** módulo MPU-6050 (giroscópio + acelerômetro 3 eixos)
- **Alimentação:** fonte de bancada 3.3 V via módulo de alimentação para protoboard
- **Comunicação:** I²C @ 400 kHz (fast mode)

### Mapeamento de pinos

| Sinal MPU-6050 | GPIO Pico W | Pino físico |
|----------------|-------------|-------------|
| SDA            | GP4         | 6           |
| SCL            | GP5         | 7           |
| VCC            | 3V3 (OUT)   | 36          |
| GND            | GND         | 38          |
| AD0            | GND         | —           |

### Parâmetros de configuração do giroscópio

| Parâmetro           | Valor          |
|---------------------|----------------|
| Full scale          | ±250 °/s       |
| Sensibilidade       | 131 LSB/(°/s)  |
| Taxa de amostragem  | 125 Hz         |
| Filtro DLPF         | 42 Hz banda    |
| Loop principal      | 8 ms (~125 Hz) |
| Amostras calibração | 500            |

---

## 3. Procedimento de Teste

1. Firmware gravado na Pico W (arquivo `.uf2`), conectada por USB.
2. Monitor serial aberto na porta `COM5` (Bluetooth link) em modo texto.
3. Ao ligar, o firmware aguarda 2 s para estabilização da USB e executa:
   - **3 blinks rápidos** — confirmação de boot.
   - **LED aceso continuamente** — calibração em andamento (robô em repouso).
   - **2 blinks lentos** — calibração concluída.
4. Com o sensor em repouso, a calibração coletou 500 amostras e calculou o bias dos três eixos.
5. Após a calibração, o conjunto (protoboard + sensor) foi rotacionado manualmente sobre o eixo Z em sentido horário, percorrendo aproximadamente 360°, passando pelas quatro direções cardinais.
6. Os dados impressos pelo `printf` foram capturados no Serial Monitor (VS Code).

---

## 4. Resultados — Saída Serial

O firmware imprime a cada iteração os valores de `Yaw`, `Pitch`, `Roll`, o `Heading` normalizado em [0°, 360°) e a direção cardinal classificada.

### Trecho 1 — NORTH → EAST

```
Yaw(deg)   Pitch(deg)  Roll(deg)  | Heading    Cardinal
  +28.00    -39.07      +2.91     |   28.00    NORTH
  +26.05    -35.83      +1.83     |   26.05    NORTH
  +25.87    -35.23      +1.76     |   25.87    NORTH
  +26.14    -37.21      +2.00     |   26.14    NORTH
  +26.72    -37.20      +1.77     |   26.72    NORTH
  +27.61    -37.59      +0.84     |   27.61    NORTH
  +27.62    -36.27      +1.85     |   27.62    NORTH
  +27.48    -37.56      +1.11     |   27.48    NORTH
  +28.15    -37.99      +0.86     |   28.15    NORTH
  +29.15    -39.69      +1.17     |   29.15    NORTH
  +31.24    -42.76      +1.82     |   31.24    NORTH
  +32.44    -40.22      +3.33     |   32.44    NORTH
  +32.36    -39.78      -1.68     |   32.36    NORTH
  +40.30    -37.69      -3.80     |   40.30    NORTH
  +51.13    -35.94      -4.13     |   51.13    EAST
  +60.87    -35.00      -6.16     |   60.87    EAST
  +71.34    -33.32      -5.41     |   71.34    EAST
  +79.55    -33.34      -5.85     |   79.55    EAST
  +85.29    -33.73      -6.53     |   85.29    EAST
  +89.18    -34.42      -6.39     |   89.18    EAST
  +92.09    -35.38      -7.40     |   92.09    EAST
  +95.81    -35.01      -7.19     |   95.81    EAST
  +98.86    -36.19      -8.25     |   98.86    EAST
 +101.13    -36.70      -8.26     |  101.13    EAST
 +102.32    -37.08      -8.67     |  102.32    EAST
 +104.05    -36.90      -8.72     |  104.05    EAST
 +106.16    -36.94      -9.20     |  106.16    EAST
 +106.71    -36.90      -9.12     |  106.71    EAST
 +108.64    -37.44      -9.69     |  108.64    EAST
 +110.05    -38.99     -10.73     |  110.05    EAST
 +110.40    -38.96     -10.73     |  110.40    EAST
 +111.28    -39.54     -10.63     |  111.28    EAST
```

### Trecho 2 — EAST → SOUTH → WEST

```
Yaw(deg)   Pitch(deg)  Roll(deg)  | Heading    Cardinal
 +130.99    -45.39      -6.00     |  130.99    EAST
 +141.20    -40.55      -6.27     |  141.20    SOUTH
 +148.75    -40.03      -4.35     |  148.75    SOUTH
 +161.41    -37.56      -6.85     |  161.41    SOUTH
 +170.55    -39.21      -7.63     |  170.55    SOUTH
 +175.72    -40.54      -8.06     |  175.72    SOUTH
 +178.89    -39.85      -8.88     |  178.89    SOUTH
 +181.29    -40.86      -9.82     |  181.29    SOUTH
 +182.87    -41.15     -10.27     |  182.87    SOUTH
 +184.35    -41.37     -10.74     |  184.35    SOUTH
 +184.72    -41.11     -10.89     |  184.72    SOUTH
 +188.47    -42.57     -10.73     |  188.47    SOUTH
 +189.26    -39.96     -10.32     |  189.26    SOUTH
 +192.40    -39.13      -9.62     |  192.40    SOUTH
 +198.97    -36.18      -7.01     |  198.97    SOUTH
 +210.13    -33.82      -4.95     |  210.13    SOUTH
 +222.10    -34.73      -3.23     |  222.10    SOUTH
 +237.41    -32.64      -4.75     |  237.41    WEST
 +253.07    -33.45      -7.36     |  253.07    WEST
 +262.93    -35.04      -7.34     |  262.93    WEST
 +271.32    -34.41      -6.19     |  271.32    WEST
 +277.98    -35.64      -8.67     |  277.98    WEST
 +281.05    -37.14     -10.25     |  281.05    WEST
 +282.56    -38.51      -9.83     |  282.56    WEST
 +282.50    -39.27      -9.71     |  282.50    WEST
 +279.85    -38.11     -11.01     |  279.85    WEST
 +276.98    -25.93      -4.40     |  276.98    WEST
 +274.07    -14.35      +4.92     |  274.07    WEST
 +274.82     -8.26      +9.67     |  274.82    WEST
 +273.86     -2.00     +12.34     |  273.86    WEST
 +273.85     +1.73     +13.75     |  273.85    WEST
 +274.22     +2.86     +13.71     |  274.22    WEST
```

---

## 5. Análise dos Resultados

### 5.1 Detecção e inicialização

O sensor respondeu corretamente ao endereço I²C `0x68` (WHO_AM_I = `0x68`), confirmando a detecção sem erros de hardware.

### 5.2 Calibração do bias

A calibração executou 500 amostras com o sensor em repouso, calculando os offsets dos três eixos. O LED permaneceu aceso durante o processo, sinalizando corretamente o período de imobilidade necessário.

### 5.3 Integração angular (yaw) e transições cardinais

A tabela a seguir resume as transições observadas:

| Transição         | Yaw (saída) | Heading (entrada) | Esperado  | Obtido   |
|-------------------|-------------|-------------------|-----------|----------|
| NORTH → EAST      | ~40.30°     | 40.30°            | > 45°     | 51.13°   |
| EAST → SOUTH      | ~130.99°    | 130.99°           | > 135°    | 141.20°  |
| SOUTH → WEST      | ~222.10°    | 222.10°           | > 225°    | 237.41°  |

As transições ocorreram nas faixas corretas definidas no código:

| Intervalo de heading | Direção mapeada |
|----------------------|-----------------|
| [0°, 45°) e [315°, 360°) | NORTH |
| [45°, 135°)          | EAST            |
| [135°, 225°)         | SOUTH           |
| [225°, 315°)         | WEST            |

### 5.4 Drift de pitch e roll durante rotação no eixo Z

Durante a rotação intencional no eixo Z, os eixos pitch e roll apresentaram variação gradual (pitch chegou a ~-45°, roll a ~-11°). Isso é esperado: como o conjunto foi rotacionado manualmente sobre a protoboard, é difícil manter o plano perfeitamente horizontal. O drift também é parcialmente atribuído à acumulação de erro de integração (drift intrínseco ao giroscópio sem fusão com acelerômetro).

---

## 6. Conclusões

| Critério de validação                    | Resultado |
|------------------------------------------|-----------|
| Sensor detectado via I²C (WHO_AM_I)      | Aprovado  |
| Calibração executada com sucesso         | Aprovado  |
| Integração de yaw coerente com rotação real | Aprovado  |
| Transições NORTH → EAST → SOUTH → WEST  | Aprovadas |
| LED de feedback em cada etapa            | Aprovado  |
| Saída serial formatada corretamente      | Aprovado  |

O módulo MPU-6050 funcionou corretamente para a aplicação de orientação do Micromouse. A integração por giroscópio é adequada para rotações rápidas e de curta duração típicas do labirinto, mas acumula drift em operações longas — ponto a ser tratado em versões futuras com fusão de dados do acelerômetro.

---

## 7. Anexos

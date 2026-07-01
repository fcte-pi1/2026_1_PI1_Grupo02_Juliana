# Módulo de Sensores de Distância

Camada de aplicação sobre os drivers de distância (VL53L0X e HC-SR04). Agrega os
três sensores do micromouse, amostra a cada 50 ms e entrega a distância por
direção e o alerta de colisão iminente. Não mexe em mapa nem navegação.

Cobre a História de Usuário:

| HU | Requisito | Estado |
|----|-----------|--------|
| **HU04** | RF04 - Medição de distância | implementado |

> HU03 (reconhecimento de paredes) e HU08 (prevenção de colisões) dependem do
> mapa/navegação (FloodFill), que está em refatoração na branch
> `navegacao-labirinto`, e ficam para depois dessa integração.

## Sensores e alocação

| Direção | Sensor | Barramento / pinos | Observação |
|---------|--------|--------------------|------------|
| Frente | VL53L0X | i2c1 (GP6 SDA / GP7 SCL) | ToF, mede bem abaixo de 2 cm |
| Esquerda | VL53L0X | i2c0 (GP4 SDA / GP5 SCL) | compartilha o barramento com o MPU6050 (0x29 vs 0x68) |
| Direita | HC-SR04 | TRIG GP16 / ECHO GP17 | ultrassom; ECHO em 5 V exige divisor resistivo pra 3,3 V |

A alocação é ajustável em `sensores_config.h`. A frente usa VL53L0X de propósito:
o HC-SR04 tem piso físico de ~2 cm e não detecta bem a colisão frontal iminente
(< 2 cm), então essa faixa fica com os sensores ToF.

### Posse do barramento i2c0 (compartilhado com o MPU6050)

O VL53L0X da esquerda fica no i2c0, o mesmo barramento do MPU6050. `i2c_init` faz
reset de hardware do periférico (não é idempotente), então o barramento precisa
de **um dono único** que o inicializa uma vez no boot. Na integração com o MPU,
chame `inicializar(true)` para o módulo **não** reiniciar o i2c0 (o dono do MPU já
o fez); ele apenas configura os pinos e o sensor. Standalone (demo), `inicializar()`
com o default `false` inicializa o i2c0 normalmente. A frequência (400 kHz) é a
mesma usada pelo MPU6050, então não há conflito de baudrate.

## HU04 - Medição de distância

`DistanceSensors::atualizar()` amostra os três sensores a cada 50 ms e cumpre os
critérios de aceitação:

1. **Precisão de ±1 cm entre 1 e 20 cm** - os VL53L0X (ToF) medem em milímetros;
   o HC-SR04 tem resolução na casa do cm. `distancia_cm()` expõe o valor por
   direção; leitura inválida (timeout / fora de range) retorna `DIST_INVALIDA_CM`.
2. **Alerta de colisão iminente < 2 cm** - `colisao_iminente()` fica true quando
   alguma direção com leitura válida está abaixo de `DIST_COLISAO_CM`, e
   `direcao_colisao()` diz qual. Leitura inválida não dispara colisão.

   > **Limitação conhecida:** o HC-SR04 (direita) tem piso físico de ~2 cm e não
   > mede abaixo disso, então a colisão iminente < 2 cm é coberta pelos VL53L0X
   > (frente e esquerda), não pela direita. Como a frente é a direção crítica no
   > avanço, o VL53L0X está alocado lá de propósito. Se a colisão < 2 cm na
   > direita virar requisito, é preciso um terceiro VL53L0X (decisão de hardware).
3. **Novas leituras a cada 50 ms no mínimo** - o intervalo é `DIST_INTERVALO_MS`.
   Com o timing budget dos VL53L0X em 20 ms cada mais o HC-SR04, o ciclo dos três
   sensores cabe dentro dos 50 ms.

## Orçamento de tempo

2 x VL53L0X a 20 ms (`DIST_VL_TIMING_BUDGET_US`) + HC-SR04 (timeout de
`DIST_HC_TIMEOUT_US` = 8 ms no pior caso) somam menos de 50 ms. Se algum sensor
for adicionado ou o budget subir, revisar pra não estourar o intervalo.

## Configuração - `sensores_config.h`

| Parâmetro | Valor | Papel |
|-----------|-------|-------|
| `DIST_INTERVALO_MS` | 50 ms | cadência de amostragem (HU04) |
| `DIST_COLISAO_CM` | 2,0 cm | limiar de colisão iminente (HU04) |
| `DIST_VL_TIMING_BUDGET_US` | 20000 | budget por VL53L0X |
| `DIST_HC_TIMEOUT_US` | 8000 | timeout do eco do HC-SR04 |
| `DIST_ALCANCE_UTIL_CM` | 20 cm | faixa útil de detecção de parede |

## Build / validação

```bash
cd sensores
mkdir build && cd build
cmake .. && make
# grave sensores_demo.uf2 no Pico (modo BOOTSEL) e abra o serial USB
```

O `main.cpp` (`sensores_demo`) imprime as três distâncias e o alerta de colisão a
cada 50 ms no terminal serial.

# Módulo de Energia - Monitoramento de tensão, bateria e consumo

Camada de aplicação sobre os drivers de sensor (`voltage_sensor`, `current_sensor`).
Encapsula a leitura periódica, os limiares de alerta e a integração de energia,
entregando ao firmware principal os valores prontos pra telemetria MQTT.

Cobre as Histórias de Usuário:

| HU | Requisito | Estado |
|----|-----------|--------|
| **HU21** | RF21 - Monitoramento da alimentação elétrica | implementado |
| **HU16** | RF16 - Monitoramento da bateria | implementado |
| **HU10** | RF10 - Cálculo de consumo energético | implementado |

## Configuração - `energia_config.h`

Todos os parâmetros calibráveis ficam em `energia_config.h` (um lugar só pra ajustar
hardware, bateria e limiares). Os marcados **(a confirmar)** dependem de medição
real da eletrônica.

| Parâmetro | Valor | HU |
|-----------|-------|----|
| `ENERGIA_TENSAO_GPIO` | GP26 (ADC0) | HU21 |
| `ENERGIA_TENSAO_DIVIDER_RATIO` | 5.0 *(a confirmar)* | HU21 |
| `ENERGIA_TENSAO_MIN_OP_V` | 6,5 V | HU21 |
| `ENERGIA_TENSAO_OSCILACAO_V` | 0,5 V | HU21 |
| `ENERGIA_BAT_TENSAO_CHEIA_V` | 8,4 V (100%) | HU16 |
| `ENERGIA_BAT_TENSAO_VAZIA_V` | 6,0 V (0%) | HU16 |
| `ENERGIA_BAT_ALERTA_PCT` | 20% | HU16 |
| `ENERGIA_BAT_CRITICO_PCT` | 5% | HU16 |
| `ENERGIA_CORRENTE_GPIO` | GP27 (ADC1) | HU10 |
| `ENERGIA_CORRENTE_SENS_MV_A` | 185 mV/A (range 5A) | HU10 |
| `ENERGIA_BAT_CAPACIDADE_MAH` | 1000 *(a confirmar)* | HU10 |
| `ENERGIA_CONSUMO_ALERTA_PCT` | 80% | HU10 |
| `ENERGIA_AMOSTRAGEM_MS` | 500 ms | todas |

## HU21 - Monitoramento da alimentação elétrica

`EnergyMonitor::atualizar()` amostra a tensão a cada 500 ms e cumpre os três
critérios de aceitação:

1. **Valor em volts a cada 500 ms** - `tensao_v()` expõe a última leitura, que o
   firmware principal publica na telemetria.
2. **Alerta de tensão baixa** - abaixo de `ENERGIA_TENSAO_MIN_OP_V` (6,5 V para
   bateria 2S de 7,4 V nominal) dispara o evento `EVT_TENSAO_BAIXA`. Usa
   histerese (`ENERGIA_HISTERESE_V`) pra o alerta não piscar em torno do limiar.
3. **Registro de oscilação** - variação entre amostras acima de
   `ENERGIA_TENSAO_OSCILACAO_V` dispara `EVT_TENSAO_OSCILACAO` com o valor da
   tensão; o firmware principal acrescenta o timestamp ao gravar no log.

## HU16 - Monitoramento da bateria

`bateria_pct()` converte a tensão lida em nível percentual (`tensao_para_pct()`,
aproximação linear entre `VAZIA` = 0% e `CHEIA` = 100%, calibrável). Cumpre os
critérios:

1. **Nível percentual a cada 500 ms** - exposto pra telemetria.
2. **Alerta de bateria fraca** - em 20% (`ENERGIA_BAT_ALERTA_PCT`) dispara
   `EVT_BATERIA_BAIXA`.
3. **Alerta crítico + log** - em 5% (`ENERGIA_BAT_CRITICO_PCT`) dispara
   `EVT_BATERIA_CRITICA`, que o firmware principal registra no log de operação.

Ambos os alertas têm histerese (`ENERGIA_HISTERESE_PCT`) pra não piscar em torno
do limiar quando a leitura oscila.

## HU10 - Cálculo de consumo energético

`atualizar()` integra a energia consumida na corrida: a cada amostra calcula
`P = tensão × |corrente|` e acumula `P × Δt` em Wh (usa o Δt real entre amostras,
não o nominal). Cumpre os critérios:

1. **Consumo acumulado em Wh a cada 500 ms** - `consumo_wh()` exposto pra
   telemetria.
2. **Alerta ao passar 80% da capacidade** - `EVT_CONSUMO_ALTO` dispara quando o
   acumulado atinge `ENERGIA_CONSUMO_ALERTA_PCT` da capacidade
   (`ENERGIA_BAT_CAPACIDADE_WH`, derivada de mAh × 7,4 V).
3. **Consolidação no fim** - `iniciar_corrida()` zera o acumulador no `start`;
   o consumo só é integrado enquanto a corrida está ativa (parado não conta como
   consumo da tentativa). No `stop`, o total é lido (`consumo_wh()`) e publicado
   no evento de fim, e `encerrar_corrida()` interrompe a integração.

> `iniciar_corrida()` também **rearma os alertas** de tensão e bateria: se uma
> condição de alerta latchou com o robô parado (idle), a corrida nova reavalia a
> condição atual do zero, garantindo que uma bateria já baixa dispare o aviso ao
> iniciar. Sem isso o alerta edge-triggered ficaria preso e não redispararia.

O zero do sensor Hall é calibrado no boot (`current_sensor_calibrate_zero()`),
com a carga de potência desligada.

### Eventos (edge-triggered, com fila)

`consumir_evento()` devolve um evento **uma vez** por transição, evitando
enxurrada de alertas repetidos enquanto a condição persiste. Como um mesmo ciclo
pode gerar mais de um alerta (ex. tensão baixa + bateria crítica), os eventos vão
para uma fila; o firmware principal drena chamando `consumir_evento()` em laço até
`EVT_NENHUM`. `ultimo_detalhe()` traz o texto com o valor medido, pronto pro
log/MQTT.

## Pinagem

| Pico W | GPIO | Sensor | Observação |
|:------:|:----:|:------:|------------|
| Pin 31 | GP26 (ADC0) | Tensão (divisor) | entrada analógica após divisor resistivo |
| Pin 32 | GP27 (ADC1) | Corrente (Hall HW-872) | saída OUT do sensor; zero calibrado no boot |

> GP28 (ADC2) livre. O sensor de corrente assume alimentação em 3,3 V (zero
> ≈ 1650 mV) - **(a confirmar com eletrônica)**: se o módulo for alimentado em
> 5 V, ajustar `ENERGIA_CORRENTE_ZERO_MV` em `energia_config.h` (a calibração de zero no
> boot absorve o desvio, mas o valor nominal deve bater).

## Build / validação

```bash
cd energia
mkdir build && cd build
cmake .. && make
# grave energia_demo.uf2 no Pico (modo BOOTSEL) e abra o serial USB
```

O `main.cpp` (`energia_demo`) imprime a tensão a cada 500 ms e mostra os alertas
disparando no terminal serial - validação sem depender do robô se locomover.

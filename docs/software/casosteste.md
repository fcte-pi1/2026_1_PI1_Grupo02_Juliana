# Detalhamento dos Casos de Teste

### CT01 - Validação do Sistema Sensorial e Precisão
* **Nome do caso de teste:** CT01 - Leitura e Calibração dos Sensores de Distância (RF03, RF04, RF10, RNF07).
* **Objetivo do caso de teste:** Validar se o sistema reconhece as paredes, adquire os dados corretamente e mede a distância com a precisão mínima exigida.
* **Pré-condições:** Micromouse montado e posicionado em uma célula de teste; monitor serial ou telemetria ativos.
* **Descrição dos procedimentos:**
    1. Posicionar o robô a distâncias 6cm, 12cm, 18cm (que corresponde ao tamanho de uma célula) de uma parede.
    2. Registrar o valor reportado pelo sistema para cada sensor.
    3. Verificar se o software identifica a presença de parede quando a distância for inferior ao limite da célula.
* **Resultado esperado:** O erro de medição deve ser inferior a 1 cm, conforme o requisito RNF07. O sistema deve sinalizar corretamente a existência de obstruções à frente e nas laterais.

### CT02 - Controle de Trajetória e Estabilidade Elétrica
* **Nome do caso de teste:** CT02 - Deslocamento Retilíneo e Controle de Motores (RF01, RF08, RF09, RNF10).
* **Objetivo do caso de teste:** Validar a capacidade de mover-se para frente controlando os motores para evitar colisões sem reinicializações elétricas.
* **Pré-condições:** Bateria carregada; corredor reto livre de obstáculos imprevistos.
* **Descrição dos procedimentos:**
    1. Acionar o comando de avanço para percorrer três células consecutivas.
    2. Observar se o robô mantém a trajetória centralizada usando o controle dos motores.
    3. Monitorar se ocorrem quedas de tensão que causem o reset do microcontrolador.
* **Resultado esperado:** O robô deve completar o percurso sem tocar nas paredes laterais e o sistema deve permanecer estável, sem oscilações de tensão que interrompam a execução.

### CT03 - Precisão de Manobra e Alteração de Direção
* **Nome do caso de teste:** CT03 - Rotação e Mudança de Orientação (RF02, RF09).
* **Objetivo do caso de teste:** Garantir que o micromouse realize rotações de até 180° através do controle preciso dos motores.
* **Pré-condições:** Robô parado no centro de uma célula.
* **Descrição dos procedimentos:**
    1. Comandar uma rotação de 90° para a esquerda e medir o ângulo final.
    2. Comandar uma rotação de 90° para a direita e medir o ângulo final.
    3. Comandar uma rotação de 180° (meia-volta).
* **Resultado esperado:** O robô deve realizar as rotações solicitadas com erro angular mínimo, permitindo que ele continue a navegação alinhado ao novo corredor.

### CT04 - Inteligência e Navegação Autônoma
* **Nome do caso de teste:** CT04 - Mapeamento e Busca do Centro (RF05, RF06, RF07, RF14, RNF01).
* **Objetivo do caso de teste:** Validar o funcionamento autônomo na identificação de caminhos, geração do mapa e alcance do objetivo central.
* **Pré-condições:** Labirinto desconhecido pelo robô; algoritmo de navegação (ex: Flood Fill) carregado.
* **Descrição dos procedimentos:**
    1. Iniciar o robô na célula inicial (0,0) em modo autônomo.
    2. Observar se o robô identifica caminhos livres e mapeia as paredes encontradas.
    3. Verificar se o sistema detecta a chegada ao objetivo central do labirinto.
* **Resultado esperado:** O micromouse deve encontrar o caminho até o centro de forma autônoma e o mapa gerado internamente deve corresponder à realidade física do labirinto.

### CT05 - Telemetria e Comunicação sem Fio
* **Nome do caso de teste:** CT05 - Validação de Telemetria em Tempo Real (RF16, RF17, RNF04).
* **Objetivo:** Verificar se os dados de navegação são enviados e exibidos na interface web com a latência exigida.
* **Pré-condições:** Micromouse conectado à rede sem fio; sistema web de telemetria ativo e aguardando conexão.
* **Descrição dos procedimentos:**
    1. Iniciar a movimentação do robô no labirinto.
    2. Observar a atualização dos campos de velocidade e posição na interface web.
    3. Cronometrar o tempo entre um evento físico (ex: detecção de parede) e sua exibição na tela.
* **Resultado esperado:** Os dados devem ser atualizados a cada 500 ms (RNF05) com latência inferior a 1 segundo (RNF04). A interface deve exibir corretamente a posição X e Y enviada pelo robô.

### CT06 - Mapeamento e Persistência no Banco de Dados
* **Nome do caso de teste:** CT06 - Armazenamento de Tentativas e Mapas (RF13, RF15).
* **Objetivo:** Garantir que os dados da "Tentativa" e as "Posições" percorridas sejam salvos corretamente no banco de dados seguindo o MER.
* **Pré-condições:** Banco de dados operacional; robô ter finalizado uma execução completa.
* **Descrição dos procedimentos:**
    1. Executar uma tentativa de travessia do labirinto até o centro.
    2. Acessar o banco de dados e realizar uma consulta na tabela "Tentativa" e "Posicao".
    3. Verificar se os IDs (PK/FK) estão relacionados corretamente entre Micromouse e Tentativa.
* **Resultado esperado:** O banco de dados deve conter o registro da tentativa com tempo de início/fim e todas as coordenadas (X, Y) percorridas vinculadas ao ID da tentativa realizada.

### CT07 - Monitoramento Energético e Estabilidade
* **Nome do caso de teste:** CT07 - Monitoramento de Tensão e Consumo (RF12, RF18, RF21).
* **Objetivo:** Validar se o sistema monitora a bateria e se mantém estável sob carga (RNF10).
* **Pré-condições:** Robô com bateria em carga parcial (aprox. 50%).
* **Descrição dos procedimentos:**
    1. Iniciar o robô e monitorar o log de tensão da bateria via telemetria.
    2. Simular um esforço maior (aceleração máxima) para observar quedas de tensão (voltage sag).
    3. Verificar se o software registra o consumo estimado em Joules conforme a fórmula E = V * I * t definida no projeto.
* **Resultado esperado:** O sistema não deve reiniciar durante picos de corrente (RNF10). O nível da bateria deve ser reportado com precisão e o cálculo de consumo energético deve ser salvo ao final da tentativa.

### CT08 - Conformidade Física e Operação Autônoma
* **Nome do caso de teste:** CT08 - Verificação de Dimensões e Autonomia de Início (RNF09, RNF01).
* **Objetivo:** Garantir que o robô respeite os limites físicos do labirinto e inicie sem intervenção humana física.
* **Pré-condições:** Labirinto padrão de 18 cm de largura por célula.
* **Descrição dos procedimentos:**
    1. Medir o robô com um paquímetro ou régua.
    2. Posicionar o robô na célula inicial.
    3. Acionar o comando "Iniciar" remotamente via sistema web (RF22).
* **Resultado esperado:** O robô deve medir menos de 18 cm x 18 cm (RNF09). Após o comando remoto, o robô deve iniciar a exploração de forma totalmente autônoma (RNF01) sem auxílio manual para partida.

### CT09 - Salvamento e Recuperação do Caminho Percorrido
* **Nome do caso de teste:** CT09 - Persistência de Trajetória em Memória (RF11, RF13).
* **Objetivo:** Validar que o caminho completo percorrido pelo robô é salvo corretamente em memória e recuperável após desligamento.
* **Pré-condições:** Robô com memória flash disponível; sistema de arquivos operacional no microcontrolador.
* **Descrição dos procedimentos:**
    1. Iniciar uma execução do robô no labirinto e deixá-lo navegar por um tempo determinado.
    2. Desligar o robô durante a navegação.
    3. Religar o robô e verificar se consegue acessar o histórico de posições armazenadas.
    4. Comparar o caminho salvo com o caminho esperado baseado nas movimentações registradas.
* **Resultado esperado:** O sistema deve recuperar o histórico completo de posições e orientações antes do desligamento, permitindo análise posterior da trajetória percorrida.

### CT10 - Integração de Múltiplos Sensores
* **Nome do caso de teste:** CT10 - Validação de Fusão de Dados Sensoriais (RF20, RF04, RF03).
* **Objetivo:** Garantir que todos os sensores de distância funcionam simultaneamente e fornecem dados consistentes durante a navegação.
* **Pré-condições:** Todos os sensores calibrados; célula de teste com configuração conhecida.
* **Descrição dos procedimentos:**
    1. Posicionar o robô com diferentes distâncias em relação às paredes (frente, laterais).
    2. Registrar as leituras de todos os sensores simultaneamente.
    3. Verificar a coerência entre os dados (ex: lateral esquerda e direita em corredor simétrico devem ser aproximadamente iguais).
* **Resultado esperado:** Todos os sensores devem funcionar sem travamentos e fornecer valores consistentes sem discrepâncias anormais entre eles.

### CT11 - Robustez sob Variações de Iluminação
* **Nome do caso de teste:** CT11 - Operação em Diferentes Condições de Luz (RNF06).
* **Objetivo:** Validar que o sistema sensorial do robô funciona corretamente sob variações de iluminação do ambiente.
* **Pré-condições:** Robô em ambiente controlável com possibilidade de ajuste de iluminação; labirinto montado.
* **Descrição dos procedimentos:**
    1. Executar uma travessia do labirinto com iluminação ambiente normal.
    2. Repetir a mesma travessia com luz reduzida (50% de iluminação).
    3. Repetir com luz mínima (aceso apenas com iluminação do robô).
    4. Comparar os resultados das três execuções.
* **Resultado esperado:** O robô deve completar a trajetória com sucesso em todas as condições de iluminação testadas sem perda de orientação ou detecção incorreta de paredes.

### CT12 - Autonomia Energética Completa
* **Nome do caso de teste:** CT12 - Teste de Duração Máxima (RF10, RNF08).
* **Objetivo:** Validar que a bateria fornece autonomia suficiente para completar o desafio do labirinto em menos de 30 minutos conforme RNF08.
* **Pré-condições:** Bateria carregada completamente; labirinto de tamanho competitivo (16x16 células).
* **Descrição dos procedimentos:**
    1. Registrar a tensão inicial da bateria.
    2. Iniciar o robô em modo autônomo para explorar o labirinto completamente.
    3. Cronometrar o tempo total até a conclusão.
    4. Registrar a tensão final da bateria e o consumo energético total calculado.
* **Resultado esperado:** O robô deve completar a exploração e navegação até o centro em menos de 30 minutos, e a bateria deve manter nível superior a 10% de carga ao final.

### CT13 - Resistência Estrutural a Impactos
* **Nome do caso de teste:** CT13 - Durabilidade Mecânica e Funcionabilidade (RNF11).
* **Objetivo:** Garantir que o robô continua operacional após sofrer múltiplas colisões leves conforme RNF11.
* **Pré-condições:** Robô montado e testado; parede de teste ou célula de colisão preparada.
* **Descrição dos procedimentos:**
    1. Realizar 10 colisões leves consecutivas contra uma parede a velocidade reduzida.
    2. Inspecionar visualmente o robô para detectar danos óbvios.
    3. Executar um teste de navegação pós-colisão para validar funcionalidade.
    4. Verificar se sensores e motores ainda respondem corretamente.
* **Resultado esperado:** O robô deve manter todas as funcionalidades após as colisões, sem danos estruturais relevantes ou perda de capacidade sensorial/motora.

### CT14 - Comunicação Sem Fio em Longo Alcance
* **Nome do caso de teste:** CT14 - Conectividade Wireless em Diferentes Distâncias (RNF02, RNF04).
* **Objetivo:** Validar que a comunicação sem fio permanece estável e com latência aceitável em diferentes distâncias entre o robô e o ponto de acesso.
* **Pré-condições:** Sistema de telemetria web ativo; medidor de sinal wireless disponível; labirinto em área com WiFi.
* **Descrição dos procedimentos:**
    1. Iniciar transmissão de telemetria com o robô a 1 metro do ponto de acesso.
    2. Afastar progressivamente o robô (3m, 5m, 10m) e monitorar a qualidade do sinal.
    3. Registrar a latência e perdas de pacotes em cada distância.
    4. Testar com obstáculos entre o robô e o ponto de acesso (paredes, móveis).
* **Resultado esperado:** Latência deve permanecer inferior a 1 segundo em até 10 metros de distância com sinal mínimo de -70 dBm. Com obstáculos, deve manter comunicação funcional até 5 metros.

### CT15 - Tempo de Carregamento da Interface de Telemetria
* **Nome do caso de teste:** CT15 - Performance da Página Web (RNF03).
* **Objetivo:** Verificar que a página de telemetria carrega em até 5 segundos conforme RNF03.
* **Pré-condições:** Servidor web em execução; conexão WiFi estável; navegador web atualizado.
* **Descrição dos procedimentos:**
    1. Acessar a página de telemetria via URL no navegador.
    2. Medir o tempo desde o carregamento até a exibição completa (primeiros dados visíveis).
    3. Repetir o teste 5 vezes e calcular a média.
    4. Verificar também o carregamento com cache limpo.
* **Resultado esperado:** Tempo de carregamento completo deve ser inferior a 5 segundos, incluindo renderização de todos os elementos gráficos principais.

### CT16 - Frequência de Atualização de Telemetria
* **Nome do caso de teste:** CT16 - Taxa de Refresh dos Dados (RNF05).
* **Objetivo:** Validar que os dados de telemetria são atualizados a cada 500 ms conforme RNF05.
* **Pré-condições:** Robô em movimento no labirinto; página de telemetria aberta; ferramenta de monitoramento de rede disponível.
* **Descrição dos procedimentos:**
    1. Iniciar captura de pacotes de rede durante execução do robô.
    2. Medir o intervalo entre atualizações consecutivas de dados de posição.
    3. Registrar a variação no intervalo (jitter).
    4. Verificar se todas as atualizações contêm dados válidos e íntegros.
* **Resultado esperado:** Intervalo entre atualizações deve ser 500 ms ± 50 ms. Não deve haver perdas de pacotes que causem lacunas maiores que 1 segundo.

### CT17 - Controle Remoto de Execução via Interface Web
* **Nome do caso de teste:** CT17 - Controle de Início e Parada Remota (RF22).
* **Objetivo:** Garantir que o robô pode ser iniciado e parado remotamente pela interface web sem atrasos significativos.
* **Pré-condições:** Robô posicionado na célula inicial; interface web de controle acessível; WiFi conectado.
* **Descrição dos procedimentos:**
    1. Clicar no botão "Iniciar" na interface web e cronometrar a resposta do robô.
    2. Deixar o robô navegar por 30 segundos.
    3. Clicar no botão "Parar" e verificar se o robô para em tempo aceitável.
    4. Repetir o ciclo de início e parada 5 vezes.
* **Resultado esperado:** Tempo de resposta entre clique no botão e ação do robô deve ser inferior a 1 segundo. Parada deve ocorrer em menos de 100 ms após comando.

### CT18 - Recuperação Após Falha de Sensor
* **Nome do caso de teste:** CT18 - Tratamento de Erro de Sensor (RF20, RF04, RF05).
* **Objetivo:** Validar o comportamento do sistema quando um sensor falha ou fornece dados inconsistentes durante a navegação.
* **Pré-condições:** Robô em movimento; possibilidade de bloquear manualmente um sensor; sistema com validação de dados.
* **Descrição dos procedimentos:**
    1. Iniciar navegação do robô no labirinto.
    2. Durante a navegação, obstruir físico um dos sensores de distância.
    3. Observar como o sistema reage ao valor anômalo.
    4. Verificar se o robô continua funcionando ou ativa protocolo de segurança.
* **Resultado esperado:** O sistema deve detectar a leitura anômala, descartar ou suavizar o valor, e continuar navegando de forma segura ou registrar um aviso sem parar completamente.

### CT19 - Variação de Temperatura Operacional
* **Nome do caso de teste:** CT19 - Estabilidade em Diferentes Temperaturas (RNF10).
* **Objetivo:** Validar que o sistema permanece estável e funcional sob variações de temperatura que possam afetar componentes eletrônicos.
* **Pré-condições:** Câmara climática disponível ou ambiente com controle de temperatura; robô totalmente montado.
* **Descrição dos procedimentos:**
    1. Executar teste de navegação em temperatura ambiente (20-25°C).
    2. Repetir em ambiente aquecido (30-35°C) simulando competição com iluminação.
    3. Repetir em ambiente mais frio (10-15°C).
    4. Registrar consumo de energia e tensão da bateria em cada condição.
* **Resultado esperado:** O robô deve funcionar sem erros em todas as condições testadas. Consumo de energia pode variar, mas o sistema não deve sofrer reset ou perda de comunicação devido à temperatura.

### CT20 - Validação Completa do Sistema Integrado
* **Nome do caso de teste:** CT20 - Teste de Aceitação Final (Todos os RF/RNF).
* **Objetivo:** Realizar um teste final integrado que valida o sistema completo funcionando de forma holística, cobrindo todos os requisitos funcionais e não-funcionais.
* **Pré-condições:** Sistema totalmente montado e testado; labirinto padrão de competição; interface web operacional; todas as dependências resolvidas.
* **Descrição dos procedimentos:**
    1. Carregar bateria completamente.
    2. Posicionar robô na célula inicial (0,0).
    3. Iniciar via comando remoto na interface web.
    4. Monitorar telemetria em tempo real durante toda a execução.
    5. Registrar o tempo de conclusão, consumo energético e qualidade dos dados.
    6. Validar que o mapa foi armazenado corretamente no banco de dados.
* **Resultado esperado:** O robô deve completar uma execução bem-sucedida do labirinto com todos os requisitos atendidos: navegação autônoma, coleta de dados correta, comunicação sem perdas significativas, e conclusão em menos de 30 minutos.
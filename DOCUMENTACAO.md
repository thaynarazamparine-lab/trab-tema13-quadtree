# Documentação do Projeto

## 1. Introdução
Este projeto desenvolve uma solução para o Tema 13 do Trabalho Interdisciplinar de Estrutura de Dados (C) e Sistemas Operacionais. O sistema integra a estrutura de dados Quadtree com paralelismo multi-thread para acelerar a detecção de colisões em uma simulação 2D de partículas.

## 2. Objetivos
- Implementar uma Quadtree eficiente para gerenciar partículas em um espaço 2D.
- Integrar o gerenciamento espacial com mecanismos de SO baseados em `pthread`.
- Garantir correção e desempenho por meio de testes unitários, de concorrência e de estresse.
- Gerar métricas e gráficos para análise de desempenho e balanceamento.

## 3. Arquitetura de Dados (ED)
### 3.1 Estrutura Quadtree
A Quadtree central do projeto é usada para indexação espacial de partículas. Cada nó armazena até `MAX_PARTICLES` partículas antes de subdividir-se em quatro filhos:
- `nw`, `ne`, `sw`, `se`
- `bounds` define a região do nó
- `particles[]` guarda ponteiros para partículas
- `count` e `is_divided` controlam o estado do nó

### 3.2 Operações implementadas
- `create_node()` — cria um nó com limites definidos
- `insert_particle()` — insere partículas recursivamente, subdividindo conforme necessário
- `remove_particle()` — remove partícula e realiza fusão condicional
- `subdivide()` / `merge()` — mantém a árvore compacta e balanceada
- `query_range()` / `query_region()` — consultas espaciais rápidas em área retangular
- `query_radius()` / `find_nearest()` — consultas com poda eficiente
- `check_collisions()` — detecção de colisões locais usando busca espacial

### 3.3 Validação de corretude
A base de testes em `tests/test_fundacao.c` cobre:
- criação, inserção, subdivisão e remoção
- consultas por região e raio
- comportamento de fusão após remoção
- consistência da Quadtree em diferentes cenários

## 4. Integração de Sistemas Operacionais (SO)
### 4.1 Paralelismo e divisão de trabalho
O módulo `src/sim_parallel.c` cria uma simulação paralela que:
- divide o mundo 800x600 em faixas horizontais, uma por thread
- mantém uma Quadtree local por thread
- usa filas de transferência (`TransferQueue`) entre threads para migrar partículas

### 4.2 Sincronização
A sincronização é realizada por:
- `pthread_barrier_t` — divide a execução em três fases claras
  1. movimento e detecção de migração
  2. drenagem de filas de transferência e reconstrução das Quadtrees
  3. coleta de métricas
- `pthread_mutex_t` — protege o enfileiramento nas filas de transferência,
  garantindo ausência de condições de corrida entre produtor e consumidor.

### 4.3 Validação de concorrência
O teste `tests/test_concorrente.c` cobre a execução simultânea de consultas sobre uma Quadtree imutável, buscando:
- ausência de data races com Thread Sanitizer (TSan)
- ausência de vazamentos de memória com AddressSanitizer/Valgrind

## 5. Persistência e métricas
### 5.1 Persistência de métricas
O projeto grava métricas de simulação em disco em formato CSV por frame:
- `time.csv`
- `tree.csv`
- `threads.csv`
- `transfers.csv`
- `balance.csv`

O módulo `src/metrics.c` cria esses arquivos em diretórios de saída e escreve as linhas de cabeçalho e dados.

### 5.2 Limitações de persistência
- Não há persistência transacional da Quadtree ou do estado completo da simulação.
- O sistema atual não implementa recuperação após morte abrupta do processo (`kill -9`).
- A persistência em disco existe apenas para resultados analíticos, não para recuperação de estado.

## 6. Testes e scripts
### 6.1 Testes unitários e de estresse
- `tests/test_fundacao.c` — cobertura de corretude da Quadtree
- `tests/test_parallel.c` — testes de métricas e cenários imbalanced
- `tests/test_concorrente.c` — teste de leitura concorrente e validação de TSAN/ASAN
- `tests/test_stress.c` — comparação de tempo entre força bruta e Quadtree

### 6.2 Geração de gráficos
- `scripts/coleta_speedup.sh` — coleta tempos de execução para 1, 2, 4 e 8 threads
- `scripts/plot_metrics.py` — plota gráficos de speedup e desempenho

## 7. Resultados e conclusões
- A Quadtree provê ganho significativo de desempenho frente à abordagem `O(n^2)` de força bruta.
- O paradigma de partição espacial horizontal permite escalabilidade com múltiplas threads.
- Métricas de balanceamento mostram como a carga pode ser distribuída entre threads.

## 8. Aspectos acadêmicos
O projeto atende ao requisito de cruzamento entre uma árvore espacial (ED) e mecanismo de SO (paralelismo e sincronização). O uso de barricadas e filas de transferência demonstra compreensão das duas disciplinas.

## 9. Recomendações para entrega final
- Preencher `DIARIO.md` com registro das quatro etapas E1–E4.
- Completar `README.md` e `DOCUMENTACAO.md` com instruções de execução e resultados obtidos.
- Se o edital exigir persistência transacional, adicionar salvamento e recuperação de estado.

# Trabalho Interdisciplinar: Estrutura de Dados (C) & Sistemas Operacionais

## INSTITUTO FEDERAL DO ESPÍRITO SANTO - CAMPUS CACHOEIRO DE ITAPEMIRIM
Cursos de Sistemas de Informação e Técnico em Informática

## Integrantes
- Thaynara Zamparini Xavier
- [INSERIR NOME DOS OUTROS INTEGRANTES AQUI]

## Tema Escolhido
- Tema identificado: **Tema 13**
- Árvore Central (ED): **Quadtree**
- Mecanismo de SO: **POSIX Threads (`pthread`)** com sincronização por **barreiras** e **mutexes**.

## Visão Geral do Sistema
O sistema simula um conjunto de partículas em um espaço 2D usando uma Quadtree para acelerar a detecção de colisões. A implementação integra Estrutura de Dados e Sistemas Operacionais por meio de:
- Particionamento do mundo em faixas horizontais, cada uma processada por uma thread distinta.
- Cada thread mantém uma Quadtree local para as partículas da sua região.
- Partículas que cruzam a fronteira são encaminhadas para a thread de destino através de filas de transferência protegidas por mutex.
- Três fases sincronizadas por barreiras garantem consistência entre movimentação, migração e reconstrução de Quadtrees.

## Arquivos Principais
- `src/quadtree.c` / `include/quadtree.h` — implementação da Quadtree, inserção, remoção, subdivisão, fusão, consultas e busca de colisões.
- `src/sim_parallel.c` — simulação paralela de partículas com divisão de espaço, filas de transferência e coleta de métricas.
- `src/transfer.c` / `include/transfer.h` — fila thread-safe para migração de partículas entre threads.
- `src/metrics.c` / `include/metrics.h` — coleta e gravação de métricas em CSV.
- `tests/` — bateria de testes unitários e de concorrência.
- `scripts/` — geração de métricas e gráficos para o relatório.

## Compilação e Execução
### Requisitos
- `gcc` compatível com `-std=c11`
- `make`
- `python3` e `matplotlib` para geração de gráficos
- `raylib` para compilação da simulação visual opcional

### Comandos principais
```bash
make all         # Compila os executáveis principais
make test        # Executa o runner de testes de fundamentação
make stress      # Executa o runner de estresse
make clean       # Remove binários e arquivos de build
```

### Outros alvos úteis
```bash
make test_parallel   # Compila e executa os testes paralelos adicionais
make conc            # Compila e executa o teste de concorrência
make graphs          # Gera gráficos a partir dos resultados de métricas CSV
make tsan            # Compila e executa o teste com Thread Sanitizer
make asan            # Compila e executa o teste com AddressSanitizer/UndefinedBehavior
```

### Geração de gráficos
```bash
python3 scripts/plot_metrics.py both
```

## Arquitetura
### Estrutura de Dados Central
A Quadtree é a estrutura fundamental para armazenar partículas em subdivisões espaciais. Ela oferece:
- inserção adaptativa de até `MAX_PARTICLES` por nó,
- subdivisão quando o nó ultrapassa a capacidade,
- fusão quando o conteúdo total dos quatro filhos volta a ficar abaixo do limite,
- consultas espaciais por retângulo e por raio,
- detecção de colisões locais entre partículas.

### Mecanismo de SO
A parte de Sistemas Operacionais é implementada por:
- `pthread_barrier_t` para separar fases de simulação e evitar condições de corrida entre threads.
- `pthread_mutex_t` em `TransferQueue` para proteger enfileiramento e desenfileiramento de partículas.
- divisão de trabalho por região do mundo para reduzir contenção e permitir paralelismo.
- geração de métricas em CSV, garantindo persistência de resultado de execução para análise posterior.

### Persistência e Validação
- O projeto grava métricas de execução em disco na forma de arquivos CSV em `results/csv/`.
- Não há persistência transacional da Quadtree ou do estado completo da simulação.
- A sobrevivência a `kill -9` não está implementada no estado atual.

## Status de Conformidade com o Edital
### Itens cumpridos
- Compilação C11 com `-Wall -Wextra -Werror` confirmada em ambiente WSL.
- `Makefile` com alvos `all`, `test`, `stress` e `clean` presentes.
- Bateria de testes própria em `tests/`.
- Scripts de métricas e gráficos em `scripts/`.
- Mecanismo concorrente baseado em `pthread` com barreiras e mutex.

### Itens incompletos / ausentes
- `README.md` original estava vazio e foi atualizado.
- `DIARIO.md` existe, mas está vazio e precisa ser preenchido com o diário de engenharia.
- `DOCUMENTACAO.md` apresenta conteúdo muito limitado para um relatório acadêmico completo.
- Não existe persistência real em disco da estrutura de dados principal.
- Não existe suporte à recuperação após morte abrupta do processo (`kill -9`).
- O `Makefile` não possui um alvo único que execute toda a bateria de testes e validações automaticamente.

## Observações Finais
Este projeto atende ao tema de Quadtree e paralelismo (Tema 13) com uma arquitetura clara de ED + SO. A maior lacuna para nota máxima é a camada de persistência / durabilidade, além do preenchimento do diário e da documentação acadêmica completa.

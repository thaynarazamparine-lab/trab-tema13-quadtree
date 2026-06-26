# Diário de Engenharia

## E1 — Definição do Tema e Pesquisa Inicial
- Tema escolhido: **Tema 13** — Quadtree + Paralelismo
- Pesquisas realizadas:
  - conceitos de Quadtree e subdivisão espacial
  - uso de `pthread` para sincronização por barreira e mutex
  - benchmarks de detecção de colisão em 2D
- Entregáveis iniciais:
  - definição da estrutura de `QuadTreeNode`
  - criação dos testes básicos de inserção, remoção e consultas

## E2 — Implementação da Quadtree e Testes de Fundamento
- Implementado `src/quadtree.c` com:
  - criação e destruição de nós
  - subdivisão automática
  - fusão condicional após remoção
  - consultas espaciais e por raio
  - detecção de colisões local usando `check_collisions()`
- Criado `tests/test_fundacao.c` para validar:
  - comportamento de subdivisão
  - inserção de partículas dentro e fora dos limites
  - remoção e fusão
  - consultas e estrutura de dados
- Observação: a estrutura foi projetada para evitar vazamentos com `free_tree()`.

## E3 — Paralelismo e Filas de Transferência
- Desenvolvido `src/transfer.c` para filas thread-safe de partículas.
- Implementada a simulação paralela em `src/sim_parallel.c`:
  - divisão do espaço em faixas horizontais
  - cada thread mantém uma Quadtree local
  - partículas migrantes são transferidas entre threads por filas protegidas por mutex
  - sincronização com `pthread_barrier_t` em três fases
- Testes adicionados em `tests/test_parallel.c`:
  - validação de métricas de threads e balanceamento
  - verificação de cenário de desbalanceamento reproduzível
  - geração de CSV para análise de resultados

## E4 — Validação de Concorrência e Métricas
- Criado `tests/test_concorrente.c` para provar:
  - consultas concorrentes seguras em uma Quadtree imutável
  - ausência de data races via `Thread Sanitizer`
  - ausência de vazamentos via `AddressSanitizer`
- Adicionado suporte a grafos de métrica com `scripts/plot_metrics.py`.
- Validado compilação com `-Wall -Wextra -Werror -std=c11` no ambiente WSL.

## Pendências e Próximos Passos
- Preencher `DIARIO.md` durante a entrega final com datas e horas de cada passo.
- Adicionar documentação detalhada de cada bug encontrado e correção aplicada.
- Acrescentar teste automatizado no `Makefile` que roda sanitizadores e gera gráficos em um fluxo único.
- Se exigido, implementar persistência transacional e tolerância a `kill -9`.

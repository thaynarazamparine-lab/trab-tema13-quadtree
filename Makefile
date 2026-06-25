CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude
LIBS   = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Objetos da Quadtree base
OBJ_QT = obj/quadtree.o

# Objetos dos módulos paralelos
OBJ_PAR = obj/quadtree.o obj/metrics.o obj/transfer.o

# ── Alvo padrão ────────────────────────────────────────────
all: test_runner stress_runner parallel_runner imbalance_runner

# ── Compilação dos objetos ─────────────────────────────────
obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# ── Testes unitários originais ─────────────────────────────
test: tests/test_fundacao.c $(OBJ_QT)
	$(CC) $(CFLAGS) tests/test_fundacao.c $(OBJ_QT) -lm -o test_runner
	./test_runner

# ── Testes de stress ───────────────────────────────────────
stress: tests/test_stress.c $(OBJ_QT)
	$(CC) $(CFLAGS) tests/test_stress.c $(OBJ_QT) -lm -o stress_runner
	./stress_runner

# ── Testes paralelos (transferência, métricas, CSV) ────────
test_parallel: tests/test_parallel.c $(OBJ_PAR)
	$(CC) $(CFLAGS) tests/test_parallel.c $(OBJ_PAR) -lm -lpthread -o parallel_test_runner
	./parallel_test_runner

# ── Simulação paralela — cenário normal ───────────────────
parallel: src/sim_parallel.c $(OBJ_PAR)
	$(CC) $(CFLAGS) src/sim_parallel.c $(OBJ_PAR) -lm -lpthread -o parallel_runner
	./parallel_runner

# ── Simulação paralela — cenário de desbalanceamento ──────
imbalance_test: src/sim_parallel.c $(OBJ_PAR)
	$(CC) $(CFLAGS) src/sim_parallel.c $(OBJ_PAR) -lm -lpthread -o imbalance_runner
	./imbalance_runner imbalance

# ── Gráficos (requer Python 3 + matplotlib + pandas) ──────
graphs:
	python3 scripts/plot_metrics.py both

# ── Visualização Raylib ────────────────────────────────────
visual: src/main_visual.c $(OBJ_QT)
	$(CC) $(CFLAGS) src/main_visual.c $(OBJ_QT) $(LIBS) -o sim_visual
	./sim_visual

# ── Limpeza ────────────────────────────────────────────────
clean:
	rm -rf obj test_runner stress_runner parallel_runner \
	       parallel_test_runner imbalance_runner sim_visual

clean_results:
	rm -rf results/

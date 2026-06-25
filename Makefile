CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude
LIBS   = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Objetos da Quadtree base
OBJ_QT = obj/quadtree.o

# Objetos dos módulos paralelos
OBJ_PAR = obj/quadtree.o obj/metrics.o obj/transfer.o

# ── Alvo padrão ────────────────────────────────────────────
all: test_runner stress_runner parallel_runner imbalance_runner test_conc

# ── Compilação dos objetos ─────────────────────────────────
obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# ── Regras explícitas dos binários (necessário para make all) ──
test_runner: tests/test_fundacao.c $(OBJ_QT)
	$(CC) $(CFLAGS) tests/test_fundacao.c $(OBJ_QT) -lm -o test_runner

stress_runner: tests/test_stress.c $(OBJ_QT)
	$(CC) $(CFLAGS) tests/test_stress.c $(OBJ_QT) -lm -o stress_runner

parallel_runner: src/sim_parallel.c $(OBJ_PAR)
	$(CC) $(CFLAGS) src/sim_parallel.c $(OBJ_PAR) -lm -lpthread -o parallel_runner

imbalance_runner: src/sim_parallel.c $(OBJ_PAR)
	$(CC) $(CFLAGS) src/sim_parallel.c $(OBJ_PAR) -lm -lpthread -o imbalance_runner

test_conc: tests/test_concorrente.c $(OBJ_QT)
	$(CC) $(CFLAGS) tests/test_concorrente.c $(OBJ_QT) -lpthread -o test_conc

# ── Alvos de execução ──────────────────────────────────────
test: test_runner
	./test_runner

stress: stress_runner
	./stress_runner

test_parallel: tests/test_parallel.c $(OBJ_PAR)
	$(CC) $(CFLAGS) tests/test_parallel.c $(OBJ_PAR) -lm -lpthread -o parallel_test_runner
	./parallel_test_runner

parallel: parallel_runner
	./parallel_runner

imbalance_test: imbalance_runner
	./imbalance_runner imbalance

conc: test_conc
	./test_conc

# ── Gráficos ───────────────────────────────────────────────
graphs:
	python3 scripts/plot_metrics.py both

# ── Visualização Raylib ────────────────────────────────────
visual: src/main_visual.c $(OBJ_QT)
	$(CC) $(CFLAGS) src/main_visual.c $(OBJ_QT) $(LIBS) -o sim_visual
	./sim_visual

# ── ThreadSanitizer — prova ausência de data races ─────────
tsan: tests/test_concorrente.c src/quadtree.c
	$(CC) $(CFLAGS) -fsanitize=thread -g \
		tests/test_concorrente.c src/quadtree.c \
		-lpthread -o test_conc_tsan
	./test_conc_tsan

# ── AddressSanitizer + UBSan — sem vazamentos nem UB ───────
asan: tests/test_fundacao.c tests/test_concorrente.c src/quadtree.c
	$(CC) $(CFLAGS) -fsanitize=address,undefined -g \
		tests/test_fundacao.c src/quadtree.c \
		-lm -o test_fund_asan && ./test_fund_asan
	$(CC) $(CFLAGS) -fsanitize=address,undefined -g \
		tests/test_concorrente.c src/quadtree.c \
		-lpthread -o test_conc_asan && ./test_conc_asan

# ── Limpeza ────────────────────────────────────────────────
clean:
	rm -rf obj test_runner stress_runner parallel_runner \
	       parallel_test_runner imbalance_runner sim_visual \
	       test_conc test_conc_tsan test_fund_asan test_conc_asan

clean_results:
	rm -rf results/

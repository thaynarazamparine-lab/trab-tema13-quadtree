CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude
LIBS   = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC = src/quadtree.c
OBJ = obj/quadtree.o

# -------------------------------------------------------------------
# all: compila tudo que não depende de Raylib (padrão para avaliação)
# -------------------------------------------------------------------
all: test_runner stress_runner test_conc

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# -------------------------------------------------------------------
# Testes unitários da fundação (E1/E2)
# -------------------------------------------------------------------
test: test_runner
	./test_runner

test_runner: tests/test_fundacao.c $(OBJ)
	$(CC) $(CFLAGS) tests/test_fundacao.c $(OBJ) -o test_runner

# -------------------------------------------------------------------
# Testes de stress sequencial (E2)
# -------------------------------------------------------------------
stress: stress_runner
	./stress_runner

stress_runner: tests/test_stress.c $(OBJ)
	$(CC) $(CFLAGS) tests/test_stress.c $(OBJ) -o stress_runner

# -------------------------------------------------------------------
# Testes de concorrência (E3) — Rafael Zoppé
# -------------------------------------------------------------------
conc: test_conc
	./test_conc

test_conc: tests/test_concorrente.c $(OBJ)
	$(CC) $(CFLAGS) tests/test_concorrente.c $(OBJ) -lpthread -o test_conc

# -------------------------------------------------------------------
# ThreadSanitizer — prova ausência de data races
# -------------------------------------------------------------------
tsan: tests/test_concorrente.c src/quadtree.c
	$(CC) $(CFLAGS) -fsanitize=thread -g \
		tests/test_concorrente.c src/quadtree.c \
		-lpthread -o test_conc_tsan
	./test_conc_tsan

# -------------------------------------------------------------------
# AddressSanitizer + UBSan — prova ausência de vazamentos e UB
# -------------------------------------------------------------------
asan: tests/test_fundacao.c tests/test_concorrente.c src/quadtree.c
	$(CC) $(CFLAGS) -fsanitize=address,undefined -g \
		tests/test_fundacao.c src/quadtree.c \
		-o test_fund_asan && ./test_fund_asan
	$(CC) $(CFLAGS) -fsanitize=address,undefined -g \
		tests/test_concorrente.c src/quadtree.c \
		-lpthread -o test_conc_asan && ./test_conc_asan

# -------------------------------------------------------------------
# Visualização com Raylib (requer Raylib instalada)
# -------------------------------------------------------------------
visual: src/main_visual.c $(OBJ)
	$(CC) $(CFLAGS) src/main_visual.c $(OBJ) $(LIBS) -o sim_visual
	./sim_visual

# -------------------------------------------------------------------
# Limpeza
# -------------------------------------------------------------------
clean:
	rm -rf obj test_runner stress_runner test_conc \
	       test_conc_tsan test_fund_asan test_conc_asan sim_visual

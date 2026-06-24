CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude
LIBS   = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC = src/quadtree.c
OBJ = obj/quadtree.o

all: test_runner sim_visual

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

test: tests/test_fundacao.c $(OBJ)
	$(CC) $(CFLAGS) tests/test_fundacao.c $(OBJ) -o test_runner
	./test_runner

stress: tests/test_stress.c $(OBJ)
	$(CC) $(CFLAGS) tests/test_stress.c $(OBJ) -o stress_runner
	./stress_runner

visual: src/main_visual.c $(OBJ)
	$(CC) $(CFLAGS) src/main_visual.c $(OBJ) $(LIBS) -o sim_visual
	./sim_visual

clean:
	rm -rf obj test_runner stress_runner sim_visual

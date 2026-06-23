CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude
LDFLAGS = -pthread -lm

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
TEST_DIR = tests

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

TARGET = sim_colisoes

all: $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test:
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_fundacao.c -o test_runner
	./test_runner

stress: $(TARGET)
	./$(TARGET) --stress

clean:
	rm -rf $(OBJ_DIR) $(TARGET) test_runner
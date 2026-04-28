# Makefile — Neander Assembler + Simulator

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra
TARGET  = neander
SRC_DIR = src
INC_DIR = include

SRCS = $(SRC_DIR)/main.c      \
       $(SRC_DIR)/assembler.c  \
       $(SRC_DIR)/executor.c   \
       $(SRC_DIR)/display.c

# Regra padrão: compila tudo
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I $(INC_DIR) -o $(TARGET) $(SRCS)

# Remove o binário compilado
clean:
	rm -f $(TARGET)

# Atalhos de teste (ajuste o nome dos arquivos conforme necessário)
test-asm:
	./$(TARGET) asm exemplos/soma.asm

test-run:
	./$(TARGET) run exemplos/soma.mem

test-step:
	./$(TARGET) step exemplos/soma.mem

.PHONY: all clean test-asm test-run test-step
#ifndef DISPLAY_H
#define DISPLAY_H

#include "../include/executor.h"
#include <stdbool.h>

// PROTÓTIPOS DAS FUNÇÕES DE DISPLAY

// Imprime o banner inicial do programa 
void display_banner(void);

// Imprime o mapa completo de memória (256 posições)
void display_memory(const uint8_t mem[MEM_SIZE], bool hex_mode, const char *title);

// Imprime o estado final: AC, PC, flags, contadores 
void display_state(const NeanderState *state, bool hex_mode);

// Imprime o estado de uma instrução (usado no passo a passo) 
void display_step(const NeanderState *state, uint8_t opcode, uint8_t operand,
                  bool has_operand, bool hex_mode);

// Imprime as instruções de uso do programa 
void display_usage(const char *prog);

#endif
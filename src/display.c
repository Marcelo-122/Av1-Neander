#include "../include/display.h"
#include "../include/executor.h"
#include <stdio.h>

void display_banner(void) {
    printf("Neander Assembler + Simulator\n");
    printf("Maquina Hipotetica UFRGS\n\n");
}

void display_usage(const char *prog) {
    printf("Uso:\n");
    printf("  %s asm <arquivo.asm>           Monta o assembly e gera .mem\n", prog);
    printf("  %s run <arquivo.mem>           Executa em modo continuo\n", prog);
    printf("  %s run <arquivo.mem> --hex     Executa e exibe em hexadecimal\n", prog);
    printf("  %s step <arquivo.mem>          Executa passo a passo\n", prog);
    printf("  %s step <arquivo.mem> --hex    Passo a passo em hexadecimal\n\n", prog);
}

/*
display_memory — Exibe as 256 posições em grade 16x16.
Cada linha representa 16 endereços consecutivos.
O endereço real de uma célula = endereço da linha + offset da coluna.
Ex: linha 32, coluna 3 → endereço 35.
*/
void display_memory(const uint8_t mem[MEM_SIZE], bool hex_mode, const char *title) {
    printf("%s\n", title);

    /* Cabeçalho das colunas */
    printf("     ");
    for (int col = 0; col < 16; col++) {
        if (hex_mode) printf(" +%X ", col);
        else          printf(" %2d ", col);
    }
    printf("\n");

    /* Linhas de dados */
    for (int row = 0; row < MEM_SIZE; row += 16) {
        if (hex_mode) printf("%02X  ", row);
        else          printf("%3d  ", row);

        for (int col = 0; col < 16; col++) {
            if (hex_mode) printf(" %02X ", mem[row + col]);
            else          printf(" %3d", mem[row + col]);
        }
        printf("\n");
    }
    printf("\n");
}

/*
display_state — Exibe o estado final da máquina.
Mostra AC, PC, flags N e Z, e os dois contadores
exigidos pela especificação (acessos à memória e instruções).
*/
void display_state(const NeanderState *state, bool hex_mode) {
    printf("--- Estado final ---\n");

    if (hex_mode) {
        printf("AC: 0x%02X\n", state->AC);
        printf("PC: 0x%02X\n", state->PC);
    } else {
        printf("AC: %d\n", state->AC);
        printf("PC: %d\n", state->PC);
    }

    printf("Flag N: %d\n", state->flag_N ? 1 : 0);
    printf("Flag Z: %d\n", state->flag_Z ? 1 : 0);
    printf("Acessos a memoria: %llu\n", (unsigned long long)state->mem_accesses);
    printf("Instrucoes executadas: %llu\n\n", (unsigned long long)state->instr_count);
}

/*
display_step — Exibe uma linha por instrução no modo passo a passo.
Formato: "LDA 16  =>  AC=5  PC=4  N=0  Z=0"
has_operand distingue instruções com endereço (LDA, ADD, STA...)
das implícitas (NOP, NOT, HLT), que não têm operando.
*/
void display_step(const NeanderState *state, uint8_t opcode, uint8_t operand,
                  bool has_operand, bool hex_mode) {

    /* Instrução executada */
    if (has_operand) {
        if (hex_mode) printf("%s 0x%02X", executor_opcode_name(opcode), operand);
        else          printf("%s %d",     executor_opcode_name(opcode), operand);
    } else {
        printf("%s", executor_opcode_name(opcode));
    }

    /* Estado após execução */
    if (hex_mode)
        printf("  =>  AC=0x%02X  PC=0x%02X  N=%d  Z=%d\n",
               state->AC, state->PC,
               state->flag_N ? 1 : 0, state->flag_Z ? 1 : 0);
    else
        printf("  =>  AC=%-3d  PC=%-3d  N=%d  Z=%d\n",
               state->AC, state->PC,
               state->flag_N ? 1 : 0, state->flag_Z ? 1 : 0);
}
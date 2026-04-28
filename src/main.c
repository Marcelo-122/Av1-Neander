/*
 * main.c — Ponto de entrada do Neander Assembler + Simulator
 *
 * Modos de uso:
 *   ./neander asm  <arquivo.asm>           monta o assembly
 *   ./neander run  <arquivo.mem>           executa em modo continuo
 *   ./neander run  <arquivo.mem> --hex     executa e exibe em hex
 *   ./neander step <arquivo.mem>           executa passo a passo
 *   ./neander step <arquivo.mem> --hex     passo a passo em hex
 */

#include "../include/assembler.h"
#include "../include/executor.h"
#include "../include/display.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* =========================================================
 * INSTRUÇÕES QUE TÊM OPERANDO
 *
 * O display_step precisa saber se a instrução tem operando
 * para exibir corretamente. Esta função centraliza essa lógica.
 * ========================================================= */
static bool has_operand(uint8_t opcode) {
    switch (opcode & 0xF0) {
        case 0x10: /* STA */
        case 0x20: /* LDA */
        case 0x30: /* ADD */
        case 0x50: /* OR  */
        case 0x60: /* AND */
        case 0x80: /* JMP */
        case 0x90: /* JN  */
        case 0xA0: /* JZ  */
            return true;
        default:
            return false;
    }
}

/* =========================================================
 * MODO: asm
 *
 * Monta um arquivo .asm e gera o .mem correspondente.
 * O nome do arquivo de saída é o mesmo do .asm mas com
 * a extensão trocada para .mem.
 *
 * Ex: "programa.asm" → "programa.mem"
 * ========================================================= */
static int mode_asm(const char *asm_file) {
    /* Monta o nome do arquivo de saída */
    char out_file[256];
    strncpy(out_file, asm_file, sizeof(out_file) - 5);
    out_file[sizeof(out_file) - 5] = '\0';

    /* Troca a extensão: remove ".asm" e coloca ".mem" */
    char *dot = strrchr(out_file, '.');
    if (dot) *dot = '\0';
    strcat(out_file, ".mem");

    /* Executa o assembler */
    AssemblerResult result = assembler_run(asm_file);
    if (!result.success)
        return 1;

    /* Salva o .mem */
    if (!assembler_save(&result, out_file))
        return 1;

    return 0;
}

/* =========================================================
 * MODO: run
 *
 * Carrega um .mem e executa o programa até o HLT.
 * Exibe o mapa de memória antes e depois, e o estado final.
 * ========================================================= */
static int mode_run(const char *mem_file, bool hex_mode) {
    NeanderState state;
    executor_init(&state);

    if (!executor_load(&state, mem_file))
        return 1;

    /* Salva cópia da memória antes de executar */
    uint8_t mem_before[MEM_SIZE];
    memcpy(mem_before, state.mem, MEM_SIZE);

    display_memory(mem_before, hex_mode, "Memoria antes da execucao");

    printf("Executando...\n\n");
    executor_run(&state);

    display_memory(state.mem, hex_mode, "Memoria depois da execucao");
    display_state(&state, hex_mode);

    return 0;
}

/* =========================================================
 * MODO: step
 *
 * Executa o programa instrução por instrução.
 * A cada passo, aguarda o usuário pressionar ENTER.
 *
 * Como funciona:
 *   Antes de chamar executor_step(), precisamos salvar o
 *   opcode e operando atuais para exibição — porque o step
 *   vai consumir o PC e não teremos mais esses valores depois.
 * ========================================================= */
static int mode_step(const char *mem_file, bool hex_mode) {
    NeanderState state;
    executor_init(&state);

    if (!executor_load(&state, mem_file))
        return 1;

    /* Salva cópia da memória antes */
    uint8_t mem_before[MEM_SIZE];
    memcpy(mem_before, state.mem, MEM_SIZE);

    display_memory(mem_before, hex_mode, "Memoria antes da execucao");

    printf("Modo passo a passo. Pressione ENTER para avancar.\n\n");

    bool running = true;
    while (running) {
        /*
         * Captura o opcode e operando ANTES do step,
         * enquanto o PC ainda aponta para eles.
         */
        uint8_t op      = state.mem[state.PC];
        bool    tem_op  = has_operand(op);
        uint8_t operand = tem_op ? state.mem[(uint8_t)(state.PC + 1)] : 0;

        /* Aguarda ENTER do usuário */
        getchar();

        /* Executa uma instrução */
        running = executor_step(&state);

        /* Exibe o que aconteceu */
        display_step(&state, op, operand, tem_op, hex_mode);

        /* Se chegou no HLT, ainda exibe o estado final */
        if (!running) {
            printf("\nHLT alcancado.\n\n");
        }

        /* Proteção contra loop infinito */
        if (state.instr_count > 1000000ULL) {
            printf("Limite de instrucoes atingido.\n");
            break;
        }
    }

    display_memory(state.mem, hex_mode, "Memoria depois da execucao");
    display_state(&state, hex_mode);

    return 0;
}

int main(int argc, char *argv[]) {
    display_banner();

    if (argc < 3) {
        display_usage(argv[0]);
        return 1;
    }

    const char *mode     = argv[1];
    const char *file     = argv[2];
    bool        hex_mode = (argc >= 4 && strcmp(argv[3], "--hex") == 0);

    if (strcmp(mode, "asm") == 0)
        return mode_asm(file);

    if (strcmp(mode, "run") == 0)
        return mode_run(file, hex_mode);

    if (strcmp(mode, "step") == 0)
        return mode_step(file, hex_mode);

    fprintf(stderr, "Modo desconhecido: '%s'\n\n", mode);
    display_usage(argv[0]);
    return 1;
}
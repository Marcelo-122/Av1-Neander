/*
 * Compilar: gcc -std=c11 -Wall -Wextra -o neander neander.c
 * Uso: ./neander <arquivo.mem> [--hex]
 * ./neander programa.mem          # saída em decimal
 * ./neander programa.mem --hex    # saída em hexadecimal
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Constantes
#define MEM_SIZE       256
#define NEANDER_MAGIC  "NEANDER"
#define MAGIC_LEN      7

// Opcodes do Neander
typedef enum {
    NOP = 0x00,
    STA = 0x10,
    LDA = 0x20,
    ADD = 0x30,
    OR  = 0x40,
    AND = 0x50,
    NOT = 0x60,
    JMP = 0x80,
    JN  = 0x90,
    JZ  = 0xA0,
    HLT = 0xF0
} Opcode;

// Estado da Máquina
typedef struct {
    uint8_t  AC;          /* Acumulador          */
    uint8_t  PC;          /* Program Counter      */
    bool     flag_N;      /* Flag Negativo        */
    bool     flag_Z;      /* Flag Zero            */
    uint8_t  mem[MEM_SIZE];
    uint64_t mem_accesses;   /* Acessos à memória   */
    uint64_t instr_count;    /* Instruções executadas */
} NeanderState;

// Protótipos
bool   load_binary(const char *filename, NeanderState *state);
void   run(NeanderState *state);
void   update_flags(NeanderState *state);
void   print_state(const NeanderState *state, bool hex_mode);
void   print_memory(const uint8_t mem[MEM_SIZE], bool hex_mode, const char *title);
void   print_usage(const char *prog);
const char *opcode_name(uint8_t opcode);
uint8_t mem_read(NeanderState *state, uint8_t addr);
void    mem_write(NeanderState *state, uint8_t addr, uint8_t val);

/* ========== Leitura de memória com contagem ========== */
uint8_t mem_read(NeanderState *state, uint8_t addr) {
    state->mem_accesses++;
    return state->mem[addr];
}

void mem_write(NeanderState *state, uint8_t addr, uint8_t val) {
    state->mem_accesses++;
    state->mem[addr] = val;
}

/* ========== Atualiza Flags ========== */
void update_flags(NeanderState *state) {
    state->flag_N = (state->AC & 0x80) != 0;
    state->flag_Z = (state->AC == 0);
}

/* ========== Carrega arquivo binário do Neander ========== */
bool load_binary(const char *filename, NeanderState *state) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Erro: Não foi possível abrir o arquivo '%s'\n", filename);
        return false;
    }

    /*
     * Deteccao do formato do arquivo .mem do Neander.
     *
     * Variantes conhecidas (identificadas pelo tamanho do arquivo):
     *   520 bytes: header "NEANDER\0" (8 bytes) + 512 bytes de dados
     *   516 bytes: header "03 4E 44 52" (4 bytes) - GUI UFRGS
     *   512 bytes: raw sem header
     */
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);

    long data_offset = 0;
    if (filesize == 520) {
        data_offset = 8;
        printf("Formato detectado: Neander padrao (header 8 bytes)\n");
    } else if (filesize == 516) {
        data_offset = 4;
        printf("Formato detectado: Neander GUI UFRGS (header 4 bytes)\n");
    } else if (filesize == 512) {
        data_offset = 0;
        printf("Formato detectado: Raw sem header (512 bytes)\n");
    } else {
        uint8_t probe[8] = {0};
        fread(probe, 1, 8, f);
        if (strncmp((char *)probe, NEANDER_MAGIC, MAGIC_LEN) == 0) {
            data_offset = 8;
            printf("Formato detectado: Neander padrao (magic encontrado)\n");
        } else {
            fprintf(stderr, "Aviso: Formato desconhecido (%ld bytes). Tentando sem header...\n", filesize);
            data_offset = 0;
        }
        rewind(f);
    }

    fseek(f, data_offset, SEEK_SET);

    /*
     * O formato do Neander GUI salva 512 bytes:
     * cada posição de memória ocupa 2 bytes (byte de dado + byte ignorado).
     * Lemos os 256 pares e pegamos apenas o primeiro byte de cada par.
     */
    uint8_t buf[2];
    int loaded = 0;
    for (int i = 0; i < MEM_SIZE; i++) {
        size_t r = fread(buf, 1, 2, f);
        if (r == 2) {
            state->mem[i] = buf[0];
            loaded++;
        } else if (r == 1) {
            state->mem[i] = buf[0];
            loaded++;
            break;
        } else {
            state->mem[i] = 0;
        }
    }

    fclose(f);

    if (loaded == 0) {
        fprintf(stderr, "Erro: Nenhum dado de memória carregado.\n");
        return false;
    }

    printf("Arquivo '%s' carregado com sucesso. (%d posições)\n\n", filename, loaded);
    return true;
}

/* ========== Nome do opcode ========== */
const char *opcode_name(uint8_t opcode) {
    switch (opcode & 0xF0) {
        case NOP: return "NOP";
        case STA: return "STA";
        case LDA: return "LDA";
        case ADD: return "ADD";
        case OR:  return "OR ";
        case AND: return "AND";
        case NOT: return "NOT";
        case JMP: return "JMP";
        case JN:  return "JN ";
        case JZ:  return "JZ ";
        case HLT: return "HLT";
        default:  return "???";
    }
}

/* ========== Executa o programa ========== */
void run(NeanderState *state) {
    bool halted = false;

    while (!halted) {
        uint8_t instr = mem_read(state, state->PC);
        state->PC++;
        state->instr_count++;

        uint8_t op = instr & 0xF0;
        uint8_t addr;

        switch (op) {
            case NOP:
                /* Nenhuma operação */
                break;

            case LDA:
                addr = mem_read(state, state->PC);
                state->PC++;
                state->AC = mem_read(state, addr);
                update_flags(state);
                break;

            case STA:
                addr = mem_read(state, state->PC);
                state->PC++;
                mem_write(state, addr, state->AC);
                break;

            case ADD:
                addr = mem_read(state, state->PC);
                state->PC++;
                state->AC = (uint8_t)(state->AC + mem_read(state, addr));
                update_flags(state);
                break;

            case OR:
                addr = mem_read(state, state->PC);
                state->PC++;
                state->AC = state->AC | mem_read(state, addr);
                update_flags(state);
                break;

            case AND:
                addr = mem_read(state, state->PC);
                state->PC++;
                state->AC = state->AC & mem_read(state, addr);
                update_flags(state);
                break;

            case NOT:
                state->AC = ~state->AC;
                update_flags(state);
                break;

            case JMP:
                addr = mem_read(state, state->PC);
                state->PC = addr;
                break;

            case JN:
                addr = mem_read(state, state->PC);
                state->PC++;
                if (state->flag_N) {
                    state->PC = addr;
                }
                break;

            case JZ:
                addr = mem_read(state, state->PC);
                state->PC++;
                if (state->flag_Z) {
                    state->PC = addr;
                }
                break;

            case HLT:
                halted = true;
                break;

            default:
                fprintf(stderr, "Aviso: Opcode desconhecido 0x%02X no PC=0x%02X. Tratado como HLT.\n",
                        instr, (uint8_t)(state->PC - 1));
                halted = true;
                break;
        }

        /* Proteção contra loop infinito */
        if (state->instr_count > 1000000ULL) {
            fprintf(stderr, "Aviso: Limite de instruções atingido (1.000.000). Execução interrompida.\n");
            break;
        }
    }
}

/* ========== Imprime mapa de memória ========== */
void print_memory(const uint8_t mem[MEM_SIZE], bool hex_mode, const char *title) {
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║  %-64s║\n", title);
    printf("╠══════╦══════════════════════════════════════════════════════════╣\n");
    printf("║  End ║  +0   +1   +2   +3   +4   +5   +6   +7   "
           "+8   +9   +A   +B   +C   +D   +E   +F  ║\n");
    printf("╠══════╬══════════════════════════════════════════════════════════╣\n");

    for (int row = 0; row < MEM_SIZE; row += 16) {
        if (hex_mode)
            printf("║  %02X  ║", row);
        else
            printf("║ %3d  ║", row);

        for (int col = 0; col < 16; col++) {
            int idx = row + col;
            if (idx < MEM_SIZE) {
                if (hex_mode)
                    printf(" %02X ", mem[idx]);
                else
                    printf(" %3d", mem[idx]);
            }
        }
        printf("  ║\n");
    }

    printf("╚══════╩══════════════════════════════════════════════════════════╝\n\n");
}

/* ========== Imprime estado final ========== */
void print_state(const NeanderState *state, bool hex_mode) {
    printf("╔══════════════════════════════════════╗\n");
    printf("║        ESTADO FINAL DA MÁQUINA       ║\n");
    printf("╠══════════════════════════════════════╣\n");

    if (hex_mode) {
        printf("║  AC  (Acumulador)   : 0x%02X           ║\n", state->AC);
        printf("║  PC  (Program Ctr)  : 0x%02X           ║\n", state->PC);
    } else {
        printf("║  AC  (Acumulador)   : %-3d            ║\n", state->AC);
        printf("║  PC  (Program Ctr)  : %-3d            ║\n", state->PC);
    }

    printf("║  Flag N (Negativo)  : %s              ║\n", state->flag_N ? "1 (SET)  " : "0 (CLEAR)");
    printf("║  Flag Z (Zero)      : %s              ║\n", state->flag_Z ? "1 (SET)  " : "0 (CLEAR)");
    printf("╠══════════════════════════════════════╣\n");
    printf("║  Acessos à memória  : %-14llu║\n", (unsigned long long)state->mem_accesses);
    printf("║  Instruções exec.   : %-14llu║\n", (unsigned long long)state->instr_count);
    printf("╚══════════════════════════════════════╝\n\n");
}

/* ========== Uso ========== */
void print_usage(const char *prog) {
    printf("Uso: %s <arquivo.mem> [opcoes]\n\n", prog);
    printf("Opcoes:\n");
    printf("  --hex      Exibe valores em hexadecimal (padrão: decimal)\n");
    printf("  --help     Exibe esta mensagem de ajuda\n\n");
    printf("Exemplos:\n");
    printf("  %s programa.mem\n", prog);
    printf("  %s programa.mem --hex\n\n", prog);
}

/* ========== Main ========== */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Parse argumentos */
    const char *filename = NULL;
    bool hex_mode = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--hex") == 0) {
            hex_mode = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            fprintf(stderr, "Opção desconhecida: %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!filename) {
        fprintf(stderr, "Erro: Nenhum arquivo especificado.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Banner */
    printf("╔══════════════════════════════════════╗\n");
    printf("║     NEANDER CLI SIMULATOR  v1.0      ║\n");
    printf("║  Máquina Hipotética UFRGS - Neander  ║\n");
    printf("╚══════════════════════════════════════╝\n\n");
    printf("Modo de exibição: %s\n\n", hex_mode ? "HEXADECIMAL" : "DECIMAL");

    /* Inicializa estado */
    NeanderState state = {0};
    state.flag_Z = true; /* AC = 0 → Z ativo inicialmente */

    /* Carrega binário */
    if (!load_binary(filename, &state)) {
        return EXIT_FAILURE;
    }

    /* Salva memória inicial para exibir antes */
    uint8_t mem_before[MEM_SIZE];
    memcpy(mem_before, state.mem, MEM_SIZE);

    /* Exibe memória ANTES */
    print_memory(mem_before, hex_mode, "MAPA DE MEMÓRIA — ANTES DA EXECUÇÃO");

    /* Executa */
    printf(">>> Iniciando execução...\n\n");
    run(&state);
    printf(">>> Execução concluída.\n\n");

    /* Exibe memória DEPOIS */
    print_memory(state.mem, hex_mode, "MAPA DE MEMÓRIA — DEPOIS DA EXECUÇÃO");

    /* Exibe estado final */
    print_state(&state, hex_mode);

    return EXIT_SUCCESS;
}
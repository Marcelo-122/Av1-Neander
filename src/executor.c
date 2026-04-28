/*
executor.c — Simulador da Máquina Neander
Este arquivo implementa o núcleo do simulador:
    - Carregamento do arquivo binário (.mem)
    - Ciclo fetch-decode-execute
    - Execução contínua e passo a passo

Baseado na ADO1 (neander.c), com:
    - Opcodes corrigidos conforme especificação do novo trabalho
    - Função executor_step() adicionada (passo a passo)
    - Código reorganizado em módulo separado
 */

#include "../include/executor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//FUNÇÕES INTERNAS (static = visíveis só neste arquivo)

/*
mem_read — Lê um byte da memória e conta o acesso.
Cada leitura ou escrita conta como 1 acesso, isso inclui
buscar a instrução (fetch), buscar o operando, e ler/gravar dados.
 */
static uint8_t mem_read(NeanderState *state, uint8_t addr) {
    state->mem_accesses++;
    return state->mem[addr];
}

// mem_write — Escreve um byte na memória e conta o acesso.
static void mem_write(NeanderState *state, uint8_t addr, uint8_t val) {
    state->mem_accesses++;
    state->mem[addr] = val;
}

/* 
update_flags Atualiza as flags N e Z com base no valor do AC.
Flag N (Negativo): ativada quando o bit 7 do AC é 1.
Em 8 bits com complemento de dois, bit 7 = 1 significa número negativo.
Ex: AC = 0b10000000 = 128 decimal → N = 1

Flag Z (Zero): ativada quando AC == 0.

Essas flags são usadas pelas instruções JN e JZ.
*/
static void update_flags(NeanderState *state) {
    state->flag_N = (state->AC & 0x80) != 0;
    state->flag_Z = (state->AC == 0);
}

// Carregamento do arquivo binário

bool executor_load(NeanderState *state, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Erro: Não foi possível abrir '%s'\n", filename);
        return false;
    }

    // Descobre o tamanho do arquivo para identificar o formato 
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);

    long data_offset = 0;

    if (filesize == 520) {
        // Header de 8 bytes ("NEANDER\0") + 512 bytes de dados 
        data_offset = 8;
    } else if (filesize == 516) {
        // Header de 4 bytes (formato GUI UFRGS) + 512 bytes 
        data_offset = 4;
    } else if (filesize == 512) {
        // Sem header: arquivo gerado por esse assembler 
        data_offset = 0;
    } else {
        /*
        Formato desconhecido: tenta detectar o magic "NEANDER"
        no início do arquivo. Se não achar, assumimos sem header.
        */
        uint8_t probe[8] = {0};
        fread(probe, 1, 8, f);
        if (strncmp((char *)probe, "NEANDER", 7) == 0) {
            data_offset = 8;
        } else {
            fprintf(stderr, "Aviso: Formato desconhecido (%ld bytes). Tentando sem header...\n", filesize);
            data_offset = 0;
        }
        rewind(f);
    }

    fseek(f, data_offset, SEEK_SET);

    /*
    Lê os pares de bytes. Para arquivos de 512 bytes (esse assembler),
    cada posição de memória ocupa 1 byte — não há par.
    Para o formato da GUI, cada posição ocupa 2 bytes (lemos e descartamos o segundo).
    */
    if (filesize == 512 || data_offset == 0) {
        // Formato simples: 1 byte por posição 
        for (int i = 0; i < MEM_SIZE; i++) {
            uint8_t b = 0;
            if (fread(&b, 1, 1, f) == 1)
                state->mem[i] = b;
            else
                state->mem[i] = 0;
        }
    } else {
        // Formato GUI: 2 bytes por posição, pega só o primeiro
        uint8_t buf[2];
        for (int i = 0; i < MEM_SIZE; i++) {
            if (fread(buf, 1, 2, f) == 2)
                state->mem[i] = buf[0];
            else
                state->mem[i] = 0;
        }
    }

    fclose(f);
    printf("Arquivo '%s' carregado com sucesso.\n\n", filename);
    return true;
}

/* 
Inicialização - Zera todos os campos. A flag Z começa ativa porque AC = 0.
*/
void executor_init(NeanderState *state) {
    memset(state, 0, sizeof(NeanderState));
    state->flag_Z = true; /* AC = 0 → Z ativo desde o início */
}

// NOME DO OPCODE (para debug e display)*/
const char *executor_opcode_name(uint8_t opcode) {
    switch (opcode & 0xF0) {
        case OP_NOP: return "NOP";
        case OP_STA: return "STA";
        case OP_LDA: return "LDA";
        case OP_ADD: return "ADD";
        case OP_OR:  return "OR ";
        case OP_AND: return "AND";
        case OP_NOT: return "NOT";
        case OP_JMP: return "JMP";
        case OP_JN:  return "JN ";
        case OP_JZ:  return "JZ ";
        case OP_HLT: return "HLT";
        default:     return "???";
    }
}

/* 
EXECUTOR_STEP — Executa UMA instrução (passo a passo)
Esta é a nova função adicionada em relação a ADO1.
Retorna:
    true  → instrução executada, pode continuar
    false → HLT encontrado (ou erro), deve parar

Como funciona o ciclo de uma instrução:
    1. FETCH: lê o opcode da memória[PC], incrementa PC
    2. DECODE: identifica qual instrução é (switch no opcode)
    3. EXECUTE: realiza a operação, atualiza AC/flags/memória
    */
bool executor_step(NeanderState *state) {

    // FASE 1: FETCH 
    uint8_t instr = mem_read(state, state->PC);
    state->PC++;          /* PC agora aponta para depois do opcode */
    state->instr_count++;

    // FASE 2: DECODE 
    uint8_t op   = instr & 0xF0;
    uint8_t addr = 0;     /* Endereço do operando (quando necessário) */

    // FASE 3: EXECUTE
    switch (op) {

        case OP_NOP:
            // Não faz nada. PC já foi incrementado. 
            break;

        case OP_LDA:
            addr = mem_read(state, state->PC);   /* lê o operando */
            state->PC++;
            state->AC = mem_read(state, addr);   /* lê o dado     */
            update_flags(state);
            break;

        case OP_STA:
            addr = mem_read(state, state->PC);
            state->PC++;
            mem_write(state, addr, state->AC);
            break;

        case OP_ADD:
            addr = mem_read(state, state->PC);
            state->PC++;
            state->AC = (uint8_t)(state->AC + mem_read(state, addr));
            update_flags(state);
            break;

        case OP_OR:
            addr = mem_read(state, state->PC);
            state->PC++;
            state->AC = state->AC | mem_read(state, addr);
            update_flags(state);
            break;

        case OP_AND:
            addr = mem_read(state, state->PC);
            state->PC++;
            state->AC = state->AC & mem_read(state, addr);
            update_flags(state);
            break;

        case OP_NOT:
            state->AC = ~state->AC;
            update_flags(state);
            break;

        case OP_JMP:
            addr = mem_read(state, state->PC);
            state->PC = addr;   /* PC não é incrementado: vai direto para addr */
            break;

        case OP_JN:
            addr = mem_read(state, state->PC);
            state->PC++;
            if (state->flag_N) {
                state->PC = addr;
            }
            break;

        case OP_JZ:
            addr = mem_read(state, state->PC);
            state->PC++;
            if (state->flag_Z) {
                state->PC = addr;
            }
            break;

        case OP_HLT:
            // Para a execução. Retorna false para sinalizar fim.
            return false;

        default:
            /*
             Opcode desconhecido: avisa e trata como HLT.
             Isso evita loops infinitos por lixo na memória.
            */
            fprintf(stderr,
                "Aviso: Opcode desconhecido 0x%02X no PC=0x%02X. Tratado como HLT.\n",
                instr, (uint8_t)(state->PC - 1));
            return false;
    }

    // Instrução executada com sucesso, pode continuar
    return true;
}

/*
EXECUTOR_RUN — Execução contínua (loop até HLT)
Simplesmente chama executor_step() em loop.
A proteção contra loop infinito evita travamentos quando o programa não tem HLT.
*/
void executor_run(NeanderState *state) {
    while (executor_step(state)) {
        /* Proteção: para após 1 milhão de instruções */
        if (state->instr_count > 1000000ULL) {
            fprintf(stderr,
                "Aviso: Limite de instruções atingido (1.000.000). Execução interrompida.\n");
            break;
        }
    }
}
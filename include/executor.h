#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdint.h>
#include <stdbool.h>

/* 
CONSTANTES DA MÁQUINA NEANDER
O Neander tem memória de 256 posições (endereços 0x00 a 0xFF).
Cada posição armazena 1 byte (8 bits).
*/
#define MEM_SIZE 256

// Tabela dos OPCODES
typedef enum {
    OP_NOP = 0x00,   // Nenhuma operação                    
    OP_STA = 0x10,   // Armazena AC na memória          
    OP_LDA = 0x20,   // Carrega valor da memória no AC  
    OP_ADD = 0x30,   // Soma valor da memória ao AC     
    OP_OR  = 0x50,   // OR lógico entre AC e memória    
    OP_AND = 0x60,   // AND lógico entre AC e memória   
    OP_NOT = 0x70,   // Complemento bit a bit do AC     
    OP_JMP = 0x80,   // Salta incondicionalmente        
    OP_JN  = 0x90,   // Salta se Flag N estiver ativa   
    OP_JZ  = 0xA0,   // Salta se Flag Z estiver ativa   
    OP_HLT = 0xF0    // Para a execução                 
} Opcode;

/* 
ESTADO DA MÁQUINA NEANDER
É um retrato da máquina em um instante.
*/
typedef struct {
    uint8_t  AC;             // Acumulador (8 bits)          
    uint8_t  PC;             // Program Counter (8 bits)     
    bool     flag_N;         // Flag Negativo: AC >= 128     
    bool     flag_Z;         // Flag Zero: AC == 0           
    uint8_t  mem[MEM_SIZE];  // Memória: 256 bytes           
    uint64_t mem_accesses;   // Contador de acessos à mem.   
    uint64_t instr_count;    // Contador de instruções exec. 
} NeanderState;

/*
PROTÓTIPOS DAS FUNÇÕES PÚBLICAS DO EXECUTOR
*/

/* Inicializa o estado da máquina com valores padrão */
void executor_init(NeanderState *state);

/* Carrega um arquivo .mem na memória simulada */
bool executor_load(NeanderState *state, const char *filename);

/* Executa o programa completo (modo contínuo) */
void executor_run(NeanderState *state);

/* Executa uma única instrução (modo passo a passo) */
bool executor_step(NeanderState *state);

/* Retorna o nome textual de um opcode (ex: 0x20 → "LDA") */
const char *executor_opcode_name(uint8_t opcode);

#endif /* EXECUTOR_H */
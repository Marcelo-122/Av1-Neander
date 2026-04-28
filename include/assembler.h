#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stdbool.h>
#include "../include/executor.h"  /* para MEM_SIZE */

/* 
TABELA DE SÍMBOLOS
A tabela de símbolos mapeia nomes de rótulos para seus
endereços de memória, descobertos na primeira passagem.
Exemplo após a 1ª passagem:
"VALOR" → 10
"LOOP"  → 4
"FIM"   → 8
MAX_SYMBOLS: limite de rótulos por programa.
MAX_LABEL_LEN: tamanho máximo do nome de um rótulo.
*/
#define MAX_SYMBOLS   128
#define MAX_LABEL_LEN 32

typedef struct {
    char    name[MAX_LABEL_LEN];  // nome do rótulo (ex: "VALOR") 
    uint8_t address;              // endereço na memória          
} Symbol;

typedef struct {
    Symbol entries[MAX_SYMBOLS];  // array de símbolos            
    int    count;                 // quantos símbolos foram adicionados 
} SymbolTable;

/* 
Após as duas passagens, o assembler preenche esta struct:
- mem: os 256 bytes de saída (código de máquina)
- success: true se não houve erros
*/
typedef struct {
    uint8_t mem[MEM_SIZE];  // código de máquina gerado     
    bool    success;        // false se houve qualquer erro      
} AssemblerResult;

// Função principal: recebe o caminho do .asm, retorna o resultado 
AssemblerResult assembler_run(const char *filename);

// Salva o resultado em um arquivo .mem (512 bytes no formato GUI) 
bool assembler_save(const AssemblerResult *result, const char *filename);

#endif /* ASSEMBLER_H */
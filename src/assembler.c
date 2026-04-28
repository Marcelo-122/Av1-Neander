#include "../include/assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// CONSTANTES INTERNAS
#define MAX_LINE_LEN 256  /* tamanho máximo de uma linha do .asm */

/* 
TABELA DE MNEMÔNICOS
Associa cada nome de instrução ao seu opcode e se ela
tem operando (endereço) ou não (instrução implícita).
has_operand:
true  → instrução ocupa 2 bytes: [opcode][endereço]
false → instrução ocupa 1 byte:  [opcode]
*/
typedef struct {
    const char *mnemonic;   /* ex: "LDA"  */
    uint8_t     opcode;     /* ex: 0x20   */
    bool        has_operand;
} MnemonicEntry;

static const MnemonicEntry MNEMONICS[] = {
    { "NOP", 0x00, false },
    { "STA", 0x10, true  },
    { "LDA", 0x20, true  },
    { "ADD", 0x30, true  },
    { "OR",  0x50, true  },
    { "AND", 0x60, true  },
    { "NOT", 0x70, false },
    { "JMP", 0x80, true  },
    { "JN",  0x90, true  },
    { "JZ",  0xA0, true  },
    { "HLT", 0xF0, false },
    { NULL,  0x00, false }  // sentinela: marca o fim da tabela 
};

/* =========================================================
 * FUNÇÕES AUXILIARES — TABELA DE SÍMBOLOS
 * ========================================================= */

/*
 * symbol_add — Adiciona um rótulo à tabela de símbolos.
 *
 * Retorna false se:
 *   - O rótulo já existe (duplicado → erro)
 *   - A tabela está cheia
 */
static bool symbol_add(SymbolTable *table, const char *name, uint8_t address) {
    /* Verifica duplicata */
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].name, name) == 0) {
            fprintf(stderr, "Erro: rotulo duplicado '%s'\n", name);
            return false;
        }
    }

    if (table->count >= MAX_SYMBOLS) {
        fprintf(stderr, "Erro: tabela de simbolos cheia\n");
        return false;
    }

    strncpy(table->entries[table->count].name, name, MAX_LABEL_LEN - 1);
    table->entries[table->count].address = address;
    table->count++;
    return true;
}

/*
 * symbol_find — Busca um rótulo e retorna seu endereço.
 *
 * found é setado para true se o rótulo existe, false se não.
 */
static uint8_t symbol_find(const SymbolTable *table, const char *name, bool *found) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].name, name) == 0) {
            *found = true;
            return table->entries[i].address;
        }
    }
    *found = false;
    return 0;
}

/* =========================================================
 * FUNÇÕES AUXILIARES — PARSING DE LINHA
 * ========================================================= */

/*
 * to_upper_str — Converte uma string para maiúsculas no lugar.
 * Usamos isso para aceitar "lda", "Lda" e "LDA" como equivalentes.
 */
static void to_upper_str(char *str) {
    for (int i = 0; str[i]; i++)
        str[i] = (char)toupper((unsigned char)str[i]);
}

/*
 * trim_left — Avança o ponteiro pulando espaços e tabs.
 * Não aloca memória: só move o ponteiro dentro da string original.
 */
static char *trim_left(char *str) {
    while (*str == ' ' || *str == '\t')
        str++;
    return str;
}

/*
 * strip_comment — Remove tudo a partir do ';' (inclusive).
 * Modifica a string no lugar colocando '\0' onde achar ';'.
 */
static void strip_comment(char *str) {
    char *p = strchr(str, ';');
    if (p) *p = '\0';
}

/*
 * strip_newline — Remove '\n' e '\r' do final da string.
 */
static void strip_newline(char *str) {
    int len = (int)strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r'))
        str[--len] = '\0';
}

/*
 * find_mnemonic — Busca um mnemônico na tabela MNEMONICS.
 * Retorna o ponteiro para a entrada, ou NULL se não encontrar.
 */
static const MnemonicEntry *find_mnemonic(const char *word) {
    for (int i = 0; MNEMONICS[i].mnemonic != NULL; i++) {
        if (strcmp(MNEMONICS[i].mnemonic, word) == 0)
            return &MNEMONICS[i];
    }
    return NULL;
}

/*
 * parse_line — Analisa uma linha e extrai suas partes.
 *
 * Preenche os ponteiros de saída (que apontam para dentro
 * do buffer 'line', não há cópia extra):
 *
 *   label     → nome do rótulo, ou NULL se não houver
 *   mnemonic  → nome da instrução/diretiva, ou NULL se linha vazia
 *   operand   → texto do operando, ou NULL se não houver
 *
 * A linha é modificada no lugar (strtok insere '\0').
 *
 * Exemplos de parsing:
 *   "INICIO: LDA VALOR"  → label="INICIO", mnemonic="LDA", operand="VALOR"
 *   "        HLT"        → label=NULL,      mnemonic="HLT", operand=NULL
 *   "DADO:   DATA 42"    → label="DADO",    mnemonic="DATA", operand="42"
 *   "; comentario"       → label=NULL,      mnemonic=NULL,   operand=NULL
 */
static void parse_line(char *line, char **label, char **mnemonic, char **operand) {
    *label    = NULL;
    *mnemonic = NULL;
    *operand  = NULL;

    strip_newline(line);
    strip_comment(line);
    to_upper_str(line);

    char *p = trim_left(line);

    /* Linha vazia após limpeza */
    if (*p == '\0')
        return;

    /*
     * Verifica se a linha começa com um rótulo.
     * Rótulos terminam com ':'.
     * Estratégia: procura ':' na string.
     */
    char *colon = strchr(p, ':');
    if (colon != NULL) {
        *colon = '\0';      /* termina a string do rótulo */
        *label = p;
        p = trim_left(colon + 1);  /* avança para depois do ':' */
    }

    /* Pega o mnemônico (primeira palavra após o rótulo) */
    if (*p == '\0')
        return;  /* linha só tinha rótulo, sem instrução */

    *mnemonic = p;

    /* Avança até o próximo espaço (fim do mnemônico) */
    while (*p && *p != ' ' && *p != '\t')
        p++;

    if (*p != '\0') {
        *p = '\0';    /* termina a string do mnemônico */
        p++;
        p = trim_left(p);

        /* O que sobrou é o operando */
        if (*p != '\0') {
            /* Remove espaços do final do operando */
            char *end = p + strlen(p) - 1;
            while (end > p && (*end == ' ' || *end == '\t'))
                *end-- = '\0';
            *operand = p;
        }
    }
}

/* 
RIMEIRA PASSAGEM Objetivo: construir a tabela de símbolos.
Percorre o arquivo linha por linha simulando a alocação de memória (PC imaginário), sem gerar bytes de verdade.
A cada instrução, avança o PC pelo tamanho da instrução:
  - Instrução com operando:  PC += 2
  - Instrução sem operando:  PC += 1
  - DATA:                    PC += 1
  - SPACE N:                 PC += N
  - ORG N:                   PC  = N (muda o PC)
Quando encontra um rótulo, registra: nome → PC atual.
Retorna true se não houve erros.
*/
static bool first_pass(FILE *f, SymbolTable *table) {
    char    line[MAX_LINE_LEN];
    char   *label, *mnemonic, *operand;
    uint8_t pc      = 0;     /* endereço atual simulado */
    int     linenum = 0;
    bool    ok      = true;

    rewind(f);  /* garante que começa do início */

    while (fgets(line, sizeof(line), f)) {
        linenum++;
        parse_line(line, &label, &mnemonic, &operand);

        /* Registra o rótulo ANTES de processar a instrução,
         * pois o rótulo aponta para o endereço atual do PC */
        if (label != NULL) {
            if (!symbol_add(table, label, pc)) {
                fprintf(stderr, "  (linha %d)\n", linenum);
                ok = false;
            }
        }

        /* Linha sem instrução (só rótulo, comentário ou vazia) */
        if (mnemonic == NULL)
            continue;

        /* Diretiva ORG: reposiciona o PC */
        if (strcmp(mnemonic, "ORG") == 0) {
            if (operand == NULL) {
                fprintf(stderr, "Erro linha %d: ORG sem endereco\n", linenum);
                ok = false;
                continue;
            }
            pc = (uint8_t)atoi(operand);
            continue;
        }

        /* Diretiva DATA: reserva 1 byte */
        if (strcmp(mnemonic, "DATA") == 0) {
            pc += 1;
            continue;
        }

        /* Diretiva SPACE N: reserva N bytes */
        if (strcmp(mnemonic, "SPACE") == 0) {
            if (operand == NULL) {
                fprintf(stderr, "Erro linha %d: SPACE sem tamanho\n", linenum);
                ok = false;
                continue;
            }
            pc += (uint8_t)atoi(operand);
            continue;
        }

        // Instrução normal: busca na tabela de mnemônicos 
        const MnemonicEntry *entry = find_mnemonic(mnemonic);
        if (entry == NULL) {
            fprintf(stderr, "Erro linha %d: mnemonico desconhecido '%s'\n",
                    linenum, mnemonic);
            ok = false;
            continue;
        }
        // Avança o PC pelo tamanho da instrução 
        pc += entry->has_operand ? 2 : 1;
    }

    return ok;
}

/* 
SEGUNDA PASSAGEM
Objetivo: gerar o código de máquina.
Percorre o arquivo de novo, agora com a tabela de símbolos
completa. Para cada instrução:
- Escreve o opcode em mem[pc]
- Se tem operando, resolve o endereço (número ou rótulo) e escreve em mem[pc+1]
Retorna true se não houve erros.
*/
static bool second_pass(FILE *f, const SymbolTable *table,
                         uint8_t mem[MEM_SIZE]) {
    char    line[MAX_LINE_LEN];
    char   *label, *mnemonic, *operand;
    uint8_t pc      = 0;
    int     linenum = 0;
    bool    ok      = true;

    rewind(f);
    memset(mem, 0, MEM_SIZE);  // zera toda a memória antes de escrever 

    while (fgets(line, sizeof(line), f)) {
        linenum++;
        parse_line(line, &label, &mnemonic, &operand);

        if (mnemonic == NULL)
            continue;

        // ORG: atualiza o PC sem gerar bytes 
        if (strcmp(mnemonic, "ORG") == 0) {
            pc = (uint8_t)atoi(operand);
            continue;
        }

        // DATA: escreve o valor literal na memória 
        if (strcmp(mnemonic, "DATA") == 0) {
            if (operand == NULL) {
                fprintf(stderr, "Erro linha %d: DATA sem valor\n", linenum);
                ok = false;
                continue;
            }
            mem[pc] = (uint8_t)atoi(operand);
            pc += 1;
            continue;
        }

        // SPACE: deixa N bytes zerados (já estão zerados pelo memset) 
        if (strcmp(mnemonic, "SPACE") == 0) {
            pc += (uint8_t)atoi(operand);
            continue;
        }

        // Instrução normal
        const MnemonicEntry *entry = find_mnemonic(mnemonic);
        if (entry == NULL) {
            fprintf(stderr, "Erro linha %d: mnemonico desconhecido '%s'\n",
                    linenum, mnemonic);
            ok = false;
            continue;
        }

        // Escreve o opcode 
        mem[pc] = entry->opcode;
        pc += 1;

        // Se tem operando, resolve e escreve o endereço 
        if (entry->has_operand) {
            if (operand == NULL) {
                fprintf(stderr, "Erro linha %d: '%s' requer um operando\n",
                        linenum, mnemonic);
                ok = false;
                continue;
            }

            uint8_t addr = 0;

            /*
            O operando pode ser:
            a) Um número literal: "42", "0xFF" → converte direto
            b) Um rótulo: "VALOR", "LOOP" → busca na tabela de símbolos
            
            Distinguimos pelo primeiro caractere:
            isdigit → número; caso contrário → rótulo
            */
            if (isdigit((unsigned char)operand[0])) {
                // Suporta decimal (42) e hexadecimal (0x2A) 
                addr = (uint8_t)strtol(operand, NULL, 0);
            } else {
                bool found;
                addr = symbol_find(table, operand, &found);
                if (!found) {
                    fprintf(stderr, "Erro linha %d: rotulo indefinido '%s'\n",
                            linenum, operand);
                    ok = false;
                    continue;
                }
            }

            mem[pc] = addr;
            pc += 1;
        }
    }

    return ok;
}

// ASSEMBLER_RUN — Função pública principal
AssemblerResult assembler_run(const char *filename) {
    AssemblerResult result = { .success = false };

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", filename);
        return result;
    }

    SymbolTable table = { .count = 0 };

    printf("Passagem 1: construindo tabela de simbolos...\n");
    bool ok = first_pass(f, &table);

    if (!ok) {
        fprintf(stderr, "Montagem abortada na passagem 1.\n");
        fclose(f);
        return result;
    }

    // Imprime a tabela de símbolos para o usuário acompanhar 
    printf("Tabela de simbolos (%d entradas):\n", table.count);
    for (int i = 0; i < table.count; i++)
        printf("  %-16s -> endereco %d (0x%02X)\n",
               table.entries[i].name,
               table.entries[i].address,
               table.entries[i].address);
    printf("\n");

    printf("Passagem 2: gerando codigo de maquina...\n");
    ok = second_pass(f, &table, result.mem);

    fclose(f);

    if (!ok) {
        fprintf(stderr, "Montagem abortada na passagem 2.\n");
        return result;
    }

    printf("Montagem concluida com sucesso.\n\n");
    result.success = true;
    return result;
}

/* 
ASSEMBLER_SAVE — Salva o .mem no formato da GUI Neander

Formato: 4 bytes de header + 512 bytes de dados.
Os 512 bytes são 256 pares: [valor][0x00].
O segundo byte de cada par é ignorado pelo executor.

Usamos esse formato para manter compatibilidade com a
GUI do Neander e com o executor que já lida com ele.
*/
bool assembler_save(const AssemblerResult *result, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel criar '%s'\n", filename);
        return false;
    }

    // Header de 4 bytes (formato GUI UFRGS) 
    uint8_t header[4] = { 0x03, 0x4E, 0x44, 0x52 };
    fwrite(header, 1, 4, f);

    // 256 pares de bytes 
    for (int i = 0; i < MEM_SIZE; i++) {
        uint8_t pair[2] = { result->mem[i], 0x00 };
        fwrite(pair, 1, 2, f);
    }

    fclose(f);
    printf("Arquivo '%s' gerado com sucesso.\n", filename);
    return true;
}
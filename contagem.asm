; Teste 2: Contagem regressiva com salto condicional
;
; Conta de N ate 0, somando 255 (-1 em complemento de 2) a cada iteracao.
; O laco termina quando AC chega a 0 (flag Z ativada) e JZ pula para FIM.
;
; Instrucoes usadas: LDA, ADD, STA, JZ, JMP, HLT
; Resultado esperado: CONT = 0, AC = 0, Flag Z = 1

        ORG  0

LOOP:   LDA  CONT    ; AC <- valor atual do contador
        ADD  NEG1    ; AC <- AC + 255  (equivale a AC - 1)
        STA  CONT    ; salva o novo valor
        JZ   FIM     ; se AC == 0, pula para FIM
        JMP  LOOP    ; senao, repete o laco

FIM:    HLT

CONT:   DATA 4       ; contador inicial: 4
NEG1:   DATA 255     ; 255 = -1 em complemento de 2 (8 bits)
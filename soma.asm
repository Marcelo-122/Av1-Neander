; Teste 1: Soma simples
; Calcula RESULT = A + B  (5 + 3 = 8)
; Instrucoes usadas: LDA, ADD, STA, HLT

        ORG  0

        LDA  A       ; AC <- 5
        ADD  B       ; AC <- 5 + 3 = 8
        STA  RESULT  ; mem[RESULT] <- 8
        HLT

A:      DATA 5
B:      DATA 3
RESULT: DATA 0       ; sera 8 apos a execucao
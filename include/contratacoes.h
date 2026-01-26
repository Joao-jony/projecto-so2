contratacoes.h

#ifndef CONTRATACOES_H
#define CONTRATACOES_H

#include <time.h>

/* Estrutura da lista encadeada */
typedef struct Contratacao {
    int id;
    time_t data;
    struct Contratacao *prox;
} Contratacao;

/* Interface do módulo */
void adicionar_contratacao(int id);
void remover_contratacao(void);
void exibir_contratacoes(void);
void limpar_lista(void);

int get_contratacoes(void);
int get_demissoes(void);

#endif



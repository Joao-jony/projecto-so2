contratacoes.c

#include "contratacoes.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* Variáveis internas ao módulo */
static Contratacao *lista = NULL;
static int contratacoes = 0;
static int demissoes = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Adiciona contratação */
void adicionar_contratacao(int id) {
    Contratacao *novo = malloc(sizeof(Contratacao));
    if (!novo) return;

    novo->id = id;
    novo->data = time(NULL);

    pthread_mutex_lock(&mutex);
    novo->prox = lista;
    lista = novo;
    contratacoes++;
    pthread_mutex_unlock(&mutex);
}

/* Remove contratação (demissão) */
void remover_contratacao(void) {
    pthread_mutex_lock(&mutex);

    if (lista) {
        Contratacao *tmp = lista;
        lista = lista->prox;
        free(tmp);
        demissoes++;
    }

    pthread_mutex_unlock(&mutex);
}

/* Exibe relatório */
void exibir_contratacoes(void) {
    pthread_mutex_lock(&mutex);

    Contratacao *aux = lista;
    char buffer[26];

    printf("\n=== RELATÓRIO DE CONTRATAÇÕES ===\n");
    while (aux) {
        ctime_r(&aux->data, buffer);
        printf("ID: %03d | Data: %s", aux->id, buffer);
        aux = aux->prox;
    }

    printf("Ativos: %d | Contratações: %d | Demissões: %d\n",
           contratacoes - demissoes, contratacoes, demissoes);
    printf("================================\n");

    pthread_mutex_unlock(&mutex);
}

/* Liberta memória */
void limpar_lista(void) {
    pthread_mutex_lock(&mutex);

    while (lista) {
        Contratacao *tmp = lista;
        lista = lista->prox;
        free(tmp);
    }

    contratacoes = 0;
    demissoes = 0;

    pthread_mutex_unlock(&mutex);
}

/* Getters protegidos */
int get_contratacoes(void) {
    pthread_mutex_lock(&mutex);
    int c = contratacoes;
    pthread_mutex_unlock(&mutex);
    return c;
}

int get_demissoes(void) {
    pthread_mutex_lock(&mutex);
    int d = demissoes;
    pthread_mutex_unlock(&mutex);
    return d;
}

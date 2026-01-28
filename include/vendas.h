#ifndef VENDAS_H
#define VENDAS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

// Estrutura de Venda
typedef struct {
    int id;
    char tipo[20];      // "empresa" ou "publico"
    char status[20];    // "disponivel" ou "vendido"
} Venda;

// Locks dos integrantes 1 e 2
extern pthread_mutex_t lock1;
extern pthread_mutex_t lock2;

// Array de vendas
extern Venda vendas[100];

// Função principal
void* vender_cartao(void* arg);

#endif

#ifndef ESTOQUE_H
#define ESTOQUE_H

#include <pthread.h>

/* Número total de cartões SIM */
#define TOTAL_CARTOES 100

/* Estrutura do cartão SIM */
typedef struct {
    int id;        // Identificador único
    int vendido;   // 0 = disponível | 1 = vendido
} CartaoSIM;

/* Recursos globais compartilhados */
extern CartaoSIM estoque[TOTAL_CARTOES];
extern pthread_mutex_t estoque_lock;

/* Funções de gestão do estoque */
void inicializar_estoque();
void liberar_estoque();

int reservar_cartao(int id);
int liberar_cartao(int id);

#endif


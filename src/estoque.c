#include "estoque.h"

/* Estoque global de cartões SIM */
CartaoSIM estoque[TOTAL_CARTOES];

/* Mutex global para sincronização */
pthread_mutex_t estoque_lock;

/* Inicializa o estoque e o mutex */
void inicializar_estoque() {
    pthread_mutex_init(&estoque_lock, NULL);

    for (int i = 0; i < TOTAL_CARTOES; i++) {
        estoque[i].id = i;
        estoque[i].vendido = 0; // todos disponíveis no início
    }
}

/* Liberta os recursos do estoque */
void liberar_estoque() {
    pthread_mutex_destroy(&estoque_lock);
}

/* Reserva (vende) um cartão SIM */
int reservar_cartao(int id) {
    int sucesso = 0;

    pthread_mutex_lock(&estoque_lock); // região crítica

    if (id >= 0 && id < TOTAL_CARTOES) {
        if (estoque[id].vendido == 0) {
            estoque[id].vendido = 1;
            sucesso = 1;
        }
    }

    pthread_mutex_unlock(&estoque_lock); // fim da região crítica

    return sucesso;
}

/* Libera um cartão SIM */
int liberar_cartao(int id) {
    int sucesso = 0;

    pthread_mutex_lock(&estoque_lock);

    if (id >= 0 && id < TOTAL_CARTOES) {
        if (estoque[id].vendido == 1) {
            estoque[id].vendido = 0;
            sucesso = 1;
        }
    }

    pthread_mutex_unlock(&estoque_lock);

    return sucesso;
}


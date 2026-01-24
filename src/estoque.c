#include "estoque.h" 
#include <stdio.h>
#include <time.h>
#include <string.h>

/* Estoque global de cartões SIM */
CartaoSIM estoque[TOTAL_CARTOES];

/* Mutex global para sincronização */
pthread_mutex_t estoque_lock;

/* Estatísticas */
int vendas_realizadas = 0;
int vendas_empresas = 0;
int vendas_publico = 0;

/* Inicializa o estoque e o mutex */
void inicializar_estoque() {
    pthread_mutex_init(&estoque_lock, NULL);

    for (int i = 0; i < TOTAL_CARTOES; i++) {
        estoque[i].id = i;
        estoque[i].vendido = 0; // todos disponíveis no início
        estoque[i].hora_venda = 0;
    }
}

/* Liberta os recursos do estoque */
void liberar_estoque() {
    pthread_mutex_destroy(&estoque_lock);
}

/* Reserva o próximo cartão SIM disponível */
int reservar_proximo_cartao() {
    int cartao_id = -1;

    pthread_mutex_lock(&estoque_lock);

    for (int i = 0; i < TOTAL_CARTOES; i++) {
        if (estoque[i].vendido == 0) {
            estoque[i].vendido = 1;
            estoque[i].hora_venda = time(NULL);
            cartao_id = i;
            vendas_realizadas++;
            break;
        }
    }

    pthread_mutex_unlock(&estoque_lock);
    return cartao_id;  // -1 se estoque esgotado
}

/* Reserva um cartão SIM específico (para casos especiais) */
int reservar_cartao_especifico(int id) {
    int sucesso = 0;

    pthread_mutex_lock(&estoque_lock);

    if (id >= 0 && id < TOTAL_CARTOES) {
        if (estoque[id].vendido == 0) {
            estoque[id].vendido = 1;
            estoque[id].hora_venda = time(NULL);
            vendas_realizadas++;
            sucesso = 1;
        }
    }

    pthread_mutex_unlock(&estoque_lock);
    return sucesso;
}

/* Libera um cartão SIM */
int liberar_cartao(int id) {
    int sucesso = 0;

    pthread_mutex_lock(&estoque_lock);

    if (id >= 0 && id < TOTAL_CARTOES) {
        if (estoque[id].vendido == 1) {
            estoque[id].vendido = 0;
            estoque[id].hora_venda = 0;
            vendas_realizadas--;
            sucesso = 1;
        }
    }

    pthread_mutex_unlock(&estoque_lock);
    return sucesso;
}

/* Retorna quantidade de cartões disponíveis */
int estoque_disponivel() {
    int disponiveis = 0;

    pthread_mutex_lock(&estoque_lock);
    for (int i = 0; i < TOTAL_CARTOES; i++) {
        if (estoque[i].vendido == 0) disponiveis++;
    }
    pthread_mutex_unlock(&estoque_lock);

    return disponiveis;
}

/* Retorna quantidade de cartões vendidos */
int estoque_vendido() {
    int vendidos = 0;

    pthread_mutex_lock(&estoque_lock);
    for (int i = 0; i < TOTAL_CARTOES; i++) {
        if (estoque[i].vendido == 1) vendidos++;
    }
    pthread_mutex_unlock(&estoque_lock);

    return vendidos;
}

/* Imprime status do estoque (para debugging) */
void imprimir_estoque() {
    pthread_mutex_lock(&estoque_lock);
    
    printf("\n=== ESTOQUE DE CARTÕES ===\n");
    
    // Calcular disponíveis manualmente para evitar chamar estoque_disponivel()
    int disponiveis = 0, vendidos = 0;
    for (int i = 0; i < TOTAL_CARTOES; i++) {
        if (estoque[i].vendido == 0) disponiveis++;
        else vendidos++;
    }
    
    printf("Total: %d | Disponíveis: %d | Vendidos: %d\n", 
           TOTAL_CARTOES, disponiveis, vendidos);
    
    printf("\nCartões vendidos:\n");
    int count_vendidos = 0;
    for (int i = 0; i < TOTAL_CARTOES; i++) {
        if (estoque[i].vendido == 1) {
            count_vendidos++;
            if (estoque[i].hora_venda > 0) {
                char* time_str = ctime(&estoque[i].hora_venda);
                if (time_str) {
                    // Remover newline do ctime
                    time_str[strcspn(time_str, "\n")] = 0;
                    printf("  Cartão %03d - Vendido em: %s\n", i, time_str);
                } else {
                    printf("  Cartão %03d - Vendido em: (data inválida)\n", i);
                }
            } else {
                printf("  Cartão %03d - Vendido em: (sem data)\n", i);
            }
        }
    }
    
    if (count_vendidos == 0) {
        printf("  (nenhum cartão vendido ainda)\n");
    }
    
    pthread_mutex_unlock(&estoque_lock);
}
#include "vendas.h"

// Locks
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

// Array de vendas (100 cartões)
Venda vendas[100];

// Inicializar vendas
void inicializar_vendas() {
    for (int i = 0; i < 100; i++) {
        vendas[i].id = i + 1;
        strcpy(vendas[i].tipo, "");
        strcpy(vendas[i].status, "disponivel");
    }
}

// Função vender_cartao (chamada por threads)
void* vender_cartao(void* arg) {
    char* periodo = (char*)arg;
    
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&lock1);  // Lock do integrante 1
        
        if (strcmp(vendas[i].status, "disponivel") == 0) {
            // Define tipo (empresa ou publico)
            if (i % 3 == 0) {
                strcpy(vendas[i].tipo, "empresa");
            } else {
                strcpy(vendas[i].tipo, "publico");
            }
            
            strcpy(vendas[i].status, "vendido");
            
            printf("Cartao #%d vendido - Tipo: %s - Periodo: %s\n", 
                   vendas[i].id, vendas[i].tipo, periodo);
            
            pthread_mutex_unlock(&lock1);
            
            // Intervalo de ~5s entre vendas
            sleep(5);
            break;
        }
        
        pthread_mutex_unlock(&lock1);
    }
    
    return NULL;
}

int main() {
    // Inicializa vendas
    inicializar_vendas();
    
    printf("Sistema de Vendas UNITEL - Iniciado\n");
    printf("=====================================\n\n");
    
    // Criar threads para os 3 períodos
    pthread_t threads_manha[35];
    pthread_t threads_tarde[35];
    pthread_t threads_noite[30];
    
    // Threads manhã (35 vendas)
    for (int i = 0; i < 35; i++) {
        pthread_create(&threads_manha[i], NULL, vender_cartao, "manha");
    }
    
    // Threads tarde (35 vendas)
    for (int i = 0; i < 35; i++) {
        pthread_create(&threads_tarde[i], NULL, vender_cartao, "tarde");
    }
    
    // Threads noite (30 vendas)
    for (int i = 0; i < 30; i++) {
        pthread_create(&threads_noite[i], NULL, vender_cartao, "noite");
    }
    
    // Aguardar threads
    for (int i = 0; i < 35; i++) {
        pthread_join(threads_manha[i], NULL);
    }
    for (int i = 0; i < 35; i++) {
        pthread_join(threads_tarde[i], NULL);
    }
    for (int i = 0; i < 30; i++) {
        pthread_join(threads_noite[i], NULL);
    }
    
    printf("\n=====================================\n");
    printf("Todas as vendas concluidas!\n");
    
    return 0;
}

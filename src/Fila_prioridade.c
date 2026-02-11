#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "Fila_prioridade.h"
#include "estoque.h"

#define MAX_FILA 200  // Tamanho máximo da fila

/* Inicializa fila com mutex e semáforos */
FilaPrioridade* inicializar_fila() {
    FilaPrioridade* fila = (FilaPrioridade*)malloc(sizeof(FilaPrioridade));
    if (!fila) return NULL;
    
    fila->frente = NULL;
    fila->fim = NULL;
    fila->tamanho = 0;
    
    // Inicializar mutex
    pthread_mutex_init(&fila->lock, NULL);
    
    // Inicializar semáforos
    sem_init(&fila->semaforo_clientes, 0, 0);
    sem_init(&fila->semaforo_espaco, 0, MAX_FILA);
    
    return fila;
}

/* Libera toda a memória da fila */
void liberar_fila(FilaPrioridade* fila) {
    if (!fila) return;
    
    pthread_mutex_lock(&fila->lock);
    
    Node* atual = fila->frente;
    while (atual != NULL) {
        Node* prox = atual->next;
        free(atual);
        atual = prox;
    }
    
    pthread_mutex_unlock(&fila->lock);
    
    pthread_mutex_destroy(&fila->lock);
    sem_destroy(&fila->semaforo_clientes);
    sem_destroy(&fila->semaforo_espaco);
    
    free(fila);
}

/* Retorna o tamanho atual da fila */
int tamanho_fila(FilaPrioridade* fila) {
    if (!fila) return 0;
    
    pthread_mutex_lock(&fila->lock);
    int tamanho = fila->tamanho;
    pthread_mutex_unlock(&fila->lock);
    
    return tamanho;
}

/* Calcula prioridade dinâmica com AGING */
int calcular_prioridade_cliente(Cliente* cliente) {
    if (!cliente) return 0;
    
    int prioridade_base = (cliente->tipo == EMPRESA) ? 10 : 1;
    
    time_t agora = time(NULL);
    double tempo_espera = difftime(agora, cliente->timestamp);
    int bonus_aging = (int)(tempo_espera / 30);
    
    if (cliente->tipo == PUBLICO && bonus_aging > 8) {
        bonus_aging = 8; 
    }
    
    int prioridade_total = prioridade_base + bonus_aging;
    cliente->prioridade_calculada = prioridade_total;
    
    return prioridade_total;
}

/* Insere cliente na posição correta baseado na prioridade */
void inserir_cliente(FilaPrioridade* fila, int id_cliente, TipoCliente tipo) {
    if (!fila) return;
    
    sem_wait(&fila->semaforo_espaco);
    
    pthread_mutex_lock(&fila->lock);
    
    Node* novo = (Node*)malloc(sizeof(Node));
    if (!novo) {
        pthread_mutex_unlock(&fila->lock);
        sem_post(&fila->semaforo_espaco);
        return;
    }
    
    novo->cliente.id_cliente = id_cliente;
    novo->cliente.tipo = tipo;
    novo->cliente.timestamp = time(NULL);
    novo->cliente.prioridade_calculada = 0;
    novo->next = NULL;
    
    calcular_prioridade_cliente(&novo->cliente);
    
    if (fila->frente == NULL) {
        fila->frente = fila->fim = novo;
        fila->tamanho++;
        sem_post(&fila->semaforo_clientes);
        pthread_mutex_unlock(&fila->lock);
        return;
    }
    
    Node* atual = fila->frente;
    Node* anterior = NULL;
    int prioridade_novo = novo->cliente.prioridade_calculada;
    
    while (atual != NULL && 
           calcular_prioridade_cliente(&atual->cliente) >= prioridade_novo) {
        anterior = atual;
        atual = atual->next;
    }
    
    if (anterior == NULL) {
        novo->next = fila->frente;
        fila->frente = novo;
    } else {
        anterior->next = novo;
        novo->next = atual;
        
        if (atual == NULL) {
            fila->fim = novo;
        }
    }
    
    fila->tamanho++;
    sem_post(&fila->semaforo_clientes);
    
    pthread_mutex_unlock(&fila->lock);
}

/* Obtém próximo cliente sem remover */
Cliente* obter_proximo_cliente(FilaPrioridade* fila) {
    if (!fila) return NULL;
    
    sem_wait(&fila->semaforo_clientes);
    
    pthread_mutex_lock(&fila->lock);
    
    if (fila->frente == NULL) {
        pthread_mutex_unlock(&fila->lock);
        sem_post(&fila->semaforo_clientes);
        return NULL;
    }
    
    Cliente* cliente = &(fila->frente->cliente);
    
    pthread_mutex_unlock(&fila->lock);
    
    return cliente;
}

/* Remove cliente específico após processamento */
void remover_cliente_processado(FilaPrioridade* fila, int id_cliente) {
    if (!fila) return;
    
    pthread_mutex_lock(&fila->lock);
    
    if (fila->frente == NULL) {
        pthread_mutex_unlock(&fila->lock);
        return;
    }
    
    Node* atual = fila->frente;
    Node* anterior = NULL;
    
    while (atual != NULL && atual->cliente.id_cliente != id_cliente) {
        anterior = atual;
        atual = atual->next;
    }
    
    if (atual == NULL) {
        pthread_mutex_unlock(&fila->lock);
        return;
    }
    
    if (anterior == NULL) {
        fila->frente = atual->next;
        if (fila->frente == NULL) {
            fila->fim = NULL;
        }
    } else {
        anterior->next = atual->next;
        if (atual->next == NULL) {
            fila->fim = anterior;
        }
    }
    
    free(atual);
    fila->tamanho--;
    
    sem_post(&fila->semaforo_espaco);
    
    pthread_mutex_unlock(&fila->lock);
}

/* Processa vendas para um turno */
int processar_vendas_turno(FilaPrioridade* fila, Turno turno_atual) {
    if (!fila) return 0;
    
    int limite, vendas_realizadas = 0;
    
    switch (turno_atual) {
        case MANHA: limite = 35; break;
        case TARDE: limite = 35; break;
        case NOITE: limite = 30; break;
        default: return 0;
    }
    
    printf("\n=== INICIANDO TURNO ===\n");
    printf("Turno: %s | Limite: %d vendas\n", 
           (turno_atual == MANHA) ? "MANHÃ" : 
           (turno_atual == TARDE) ? "TARDE" : "NOITE", 
           limite);
    
    while (vendas_realizadas < limite) {
        int disponivel = estoque_disponivel();
        if (disponivel <= 0) {
            printf("[ESTOQUE ESGOTADO] Vendidos: %d/%d\n", 
                   vendas_realizadas, limite);
            break;
        }
        
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 2;
        
        if (sem_timedwait(&fila->semaforo_clientes, &ts) == -1) {
            printf("[FILA VAZIA - TIMEOUT] Vendidos: %d/%d\n", 
                   vendas_realizadas, limite);
            break;
        }
        
        pthread_mutex_lock(&fila->lock);
        
        if (fila->frente == NULL) {
            pthread_mutex_unlock(&fila->lock);
            sem_post(&fila->semaforo_clientes);
            continue;
        }
        
        Cliente* proximo = &(fila->frente->cliente);
        int id_cliente = proximo->id_cliente;
        TipoCliente tipo = proximo->tipo;
        time_t chegada = proximo->timestamp;
        int prioridade = calcular_prioridade_cliente(proximo);
        
        pthread_mutex_unlock(&fila->lock);
        
        int cartao_id = reservar_proximo_cartao();
        if (cartao_id == -1) {
            printf("[ERRO] Não foi possível reservar cartão\n");
            sem_post(&fila->semaforo_clientes);
            break;
        }
        
        char* tipo_str = (tipo == EMPRESA) ? "EMPRESA" : "PUBLICO";
        char tempo_espera_str[50];
        time_t agora = time(NULL);
        double espera = difftime(agora, chegada);
        
        if (espera < 60) {
            sprintf(tempo_espera_str, "%.0f segundos", espera);
        } else {
            sprintf(tempo_espera_str, "%.1f minutos", espera/60.0);
        }
        
        printf("[VENDA %02d/%d] %-7s | Cliente: %03d | ", 
               vendas_realizadas + 1, limite, tipo_str, id_cliente);
        printf("Cartão: %03d | Prioridade: %d | Espera: %s\n",
               cartao_id, prioridade, tempo_espera_str);
        
        remover_cliente_processado(fila, id_cliente);
        
        vendas_realizadas++;
        
        usleep(100000);
        
        if (disponivel == 1 && vendas_realizadas < limite) {
            printf("[ALERTA] Último cartão disponível!\n");
        }
    }
    
    printf("=== FIM DO TURNO ===\n");
    printf("Total vendido: %d/%d\n", vendas_realizadas, limite);
    printf("Estoque restante: %d cartões\n\n", estoque_disponivel());
    
    return vendas_realizadas;
}

/* Imprime estado atual da fila */
void imprimir_fila(FilaPrioridade* fila) {
    if (!fila) return;
    
    pthread_mutex_lock(&fila->lock);
    
    printf("\n=== FILA DE VENDAS (Ordenada por Prioridade) ===\n");
    printf("Tamanho: %d clientes (Máx: %d)\n", fila->tamanho, MAX_FILA);
    
    int sem_valor_clientes, sem_valor_espaco;
    sem_getvalue(&fila->semaforo_clientes, &sem_valor_clientes);
    sem_getvalue(&fila->semaforo_espaco, &sem_valor_espaco);
    printf("Semáforo Clientes: %d | Semáforo Espaço: %d\n", 
           sem_valor_clientes, sem_valor_espaco);
    
    Node* atual = fila->frente;
    int pos = 1;
    
    while (atual != NULL) {
        Cliente* c = &atual->cliente;
        int prioridade = calcular_prioridade_cliente(c);
        time_t agora = time(NULL);
        double espera = difftime(agora, c->timestamp);
        
        printf("%2d. [%s] Cliente %03d | ", pos++,
               (c->tipo == EMPRESA) ? "EMP" : "PUB",
               c->id_cliente);
        printf("Prioridade: %2d | Espera: ", prioridade);
        
        if (espera < 60) printf("%.0fs\n", espera);
        else printf("%.1fm\n", espera/60.0);
        
        atual = atual->next;
    }
    
    if (fila->tamanho == 0) {
        printf("(fila vazia)\n");
    }
    
    pthread_mutex_unlock(&fila->lock);
}

/* Bloquear vendas públicas */
void bloquear_vendas_publico(FilaPrioridade* fila) {
    if (!fila) return;
    
    pthread_mutex_lock(&fila->lock);
    
    Node* atual = fila->frente;
    Node* anterior = NULL;
    int removidos = 0;
    
    while (atual != NULL) {
        if (atual->cliente.tipo == PUBLICO) {
            Node* remover = atual;
            
            if (anterior == NULL) {
                fila->frente = atual->next;
                atual = fila->frente;
            } else {
                anterior->next = atual->next;
                atual = anterior->next;
            }
            
            if (remover == fila->fim) {
                fila->fim = anterior;
            }
            
            free(remover);
            fila->tamanho--;
            removidos++;
            
            sem_post(&fila->semaforo_espaco);
        } else {
            anterior = atual;
            atual = atual->next;
        }
    }
    
    pthread_mutex_unlock(&fila->lock);
    
    if (removidos > 0) {
        printf("[BLOQUEIO] %d clientes públicos removidos da fila\n", removidos);
    }
}

/* Adiciona lote de empresas */
void adicionar_lote_empresas(FilaPrioridade* fila, int quantidade) {
    if (!fila || quantidade <= 0) return;
    
    printf("[LOTE] Adicionando %d solicitações de empresas...\n", quantidade);
    
    static int next_client_id = 1000;
    
    for (int i = 0; i < quantidade; i++) {
        inserir_cliente(fila, next_client_id++, EMPRESA);
    }
    
    printf("[LOTE] %d empresas adicionadas à fila\n", quantidade);
}
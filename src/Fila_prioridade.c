#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "Fila_prioridade.h"
#include "estoque.h"

#define MAX_FILA 200  // Tamanho máximo da fila

/* Getter para tamanho máximo */
int get_max_fila(void) {
    return MAX_FILA;
}

/* Inicializa fila com mutex e semáforos */
FilaPrioridade* inicializar_fila(void) {
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
    
    // Destruir mutex e semáforos
    pthread_mutex_destroy(&fila->lock);
    sem_destroy(&fila->semaforo_clientes);
    sem_destroy(&fila->semaforo_espaco);
    
    free(fila);
}

/* Calcula prioridade dinâmica com AGING */
int calcular_prioridade_cliente(Cliente* cliente) {
    if (!cliente) return 0;
    
    // Prioridade base: Empresa = 10, Público = 1
    int prioridade_base = (cliente->tipo == EMPRESA) ? 10 : 1;
    
    // AGING: aumenta 1 ponto a cada 30 segundos de espera
    time_t agora = time(NULL);
    double tempo_espera = difftime(agora, cliente->timestamp);
    int bonus_aging = (int)(tempo_espera / 30);
    
    // Limita o aging para não ultrapassar prioridade de empresa
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
    
    // Aguardar espaço disponível na fila
    sem_wait(&fila->semaforo_espaco);
    
    pthread_mutex_lock(&fila->lock);
    
    // Criar novo nó
    Node* novo = (Node*)malloc(sizeof(Node));
    if (!novo) {
        pthread_mutex_unlock(&fila->lock);
        sem_post(&fila->semaforo_espaco);
        return;
    }
    
    // Preenche dados do cliente
    novo->cliente.id_cliente = id_cliente;
    novo->cliente.tipo = tipo;
    novo->cliente.timestamp = time(NULL);
    novo->cliente.prioridade_calculada = 0;
    novo->next = NULL;
    
    // Calcula prioridade
    calcular_prioridade_cliente(&novo->cliente);
    
    // Caso 1: Fila vazia
    if (fila->frente == NULL) {
        fila->frente = fila->fim = novo;
        fila->tamanho++;
        
        sem_post(&fila->semaforo_clientes);
        pthread_mutex_unlock(&fila->lock);
        return;
    }
    
    // Caso 2: Inserir ordenado por prioridade (maior primeiro)
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

/* Obtém próximo cliente (maior prioridade) sem remover */
Cliente* obter_proximo_cliente(FilaPrioridade* fila) {
    if (!fila) return NULL;
    
    // Aguardar cliente disponível
    if (sem_wait(&fila->semaforo_clientes) == -1) {
        return NULL;
    }
    
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
    
    int limite;
    switch (turno_atual) {
        case MANHA: limite = 35; break;
        case TARDE: limite = 35; break;
        case NOITE: limite = 30; break;
        default: return 0;
    }
    
    printf("\n=== INICIANDO TURNO ===\n");
    printf("Turno: %s | Limite: %d vendas\n", 
           turno_atual == MANHA ? "MANHÃ" : 
           turno_atual == TARDE ? "TARDE" : "NOITE", 
           limite);
    
    int vendas_realizadas = 0;
    
    while (vendas_realizadas < limite) {
        int disponivel = estoque_disponivel();
        if (disponivel <= 0) {
            printf("[ESTOQUE ESGOTADO] Vendidos: %d/%d\n", 
                   vendas_realizadas, limite);
            break;
        }
        
        // Aguardar cliente
        if (sem_wait(&fila->semaforo_clientes) == -1) {
            printf("[FILA VAZIA] Vendidos: %d/%d\n", 
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
        double espera = difftime(time(NULL), chegada);
        
        printf("[VENDA %02d/%d] %-7s | Cliente: %03d | ", 
               vendas_realizadas + 1, limite, tipo_str, id_cliente);
        printf("Cartão: %03d | Prioridade: %d | Espera: %.0fs\n",
               cartao_id, prioridade, espera);
        
        remover_cliente_processado(fila, id_cliente);
        vendas_realizadas++;
        
        usleep(100000);
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
    
    Node* atual = fila->frente;
    int pos = 1;
    
    while (atual != NULL) {
        Cliente* c = &atual->cliente;
        int prioridade = calcular_prioridade_cliente(c);
        double espera = difftime(time(NULL), c->timestamp);
        
        printf("%2d. [%s] Cliente %03d | ", pos++,
               c->tipo == EMPRESA ? "EMP" : "PUB",
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "Fila_prioridade.h"
#include "estoque.h"  

/* Inicializa fila com mutex */
FilaPrioridade* inicializar_fila() {
    FilaPrioridade* fila = (FilaPrioridade*)malloc(sizeof(FilaPrioridade));
    if (!fila) return NULL;
    
    fila->frente = NULL;
    fila->fim = NULL;
    fila->tamanho = 0;
    pthread_mutex_init(&fila->lock, NULL);
    
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
    cliente->prioridade_calculada = prioridade_total;  // Cache
    
    return prioridade_total;
}

/* Insere cliente na posição correta baseado na prioridade */
void inserir_cliente(FilaPrioridade* fila, int id_cliente, TipoCliente tipo) {
    if (!fila) return;
    
    Node* novo = (Node*)malloc(sizeof(Node));
    if (!novo) return;
    
    // Preenche dados do cliente
    novo->cliente.id_cliente = id_cliente;
    novo->cliente.tipo = tipo;
    novo->cliente.timestamp = time(NULL);
    novo->cliente.prioridade_calculada = 0;
    novo->next = NULL;
    
    pthread_mutex_lock(&fila->lock);
    
    // Calcula prioridade
    calcular_prioridade_cliente(&novo->cliente);
    
    // Caso 1: Fila vazia
    if (fila->frente == NULL) {
        fila->frente = fila->fim = novo;
        fila->tamanho++;
        pthread_mutex_unlock(&fila->lock);
        return;
    }
    
    // Caso 2: Inserir ordenado por prioridade (maior primeiro)
    Node* atual = fila->frente;
    Node* anterior = NULL;
    int prioridade_novo = novo->cliente.prioridade_calculada;
    
    // Encontra posição correta
    while (atual != NULL && 
           calcular_prioridade_cliente(&atual->cliente) >= prioridade_novo) {
        anterior = atual;
        atual = atual->next;
    }
    
    // Insere na posição encontrada
    if (anterior == NULL) {
        // Insere no início
        novo->next = fila->frente;
        fila->frente = novo;
    } else {
        // Insere no meio/fim
        anterior->next = novo;
        novo->next = atual;
        
        if (atual == NULL) {
            fila->fim = novo;
        }
    }
    
    fila->tamanho++;
    pthread_mutex_unlock(&fila->lock);
}

/* Obtém próximo cliente (maior prioridade) sem remover */
Cliente* obter_proximo_cliente(FilaPrioridade* fila) {
    if (!fila || fila->frente == NULL) return NULL;
    
    pthread_mutex_lock(&fila->lock);
    Cliente* cliente = &(fila->frente->cliente);
    pthread_mutex_unlock(&fila->lock);
    
    return cliente;
}

/* Remove cliente específico após processamento */
void remover_cliente_processado(FilaPrioridade* fila, int id_cliente) {
    if (!fila || fila->frente == NULL) return;
    
    pthread_mutex_lock(&fila->lock);
    
    Node* atual = fila->frente;
    Node* anterior = NULL;
    
    while (atual != NULL && atual->cliente.id_cliente != id_cliente) {
        anterior = atual;
        atual = atual->next;
    }
    
    if (atual == NULL) {
        pthread_mutex_unlock(&fila->lock);
        return;  // Cliente não encontrado
    }
    
    // Remove o nó
    if (anterior == NULL) {
        fila->frente = atual->next;
        if (fila->frente == NULL) fila->fim = NULL;
    } else {
        anterior->next = atual->next;
        if (atual->next == NULL) fila->fim = anterior;
    }
    
    free(atual);
    fila->tamanho--;
    
    pthread_mutex_unlock(&fila->lock);
}

/* Processa vendas para um turno (INTEGRAÇÃO COM ESTOQUE) */
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
        // 1. Verificar se há estoque disponível
        int disponivel = estoque_disponivel();
        if (disponivel <= 0) {
            printf("[ESTOQUE ESGOTADO] Vendidos: %d/%d\n", 
                   vendas_realizadas, limite);
            break;
        }
        
        // 2. Pegar próximo cliente por prioridade
        pthread_mutex_lock(&fila->lock);
        if (fila->frente == NULL) {
            pthread_mutex_unlock(&fila->lock);
            printf("[FILA VAZIA] Vendidos: %d/%d\n", 
                   vendas_realizadas, limite);
            break;
        }
        
        Cliente* proximo = &(fila->frente->cliente);
        int id_cliente = proximo->id_cliente;
        TipoCliente tipo = proximo->tipo;
        time_t chegada = proximo->timestamp;
        int prioridade = calcular_prioridade_cliente(proximo);
        pthread_mutex_unlock(&fila->lock);
        
        // 3. Tentar reservar cartão
        int cartao_id = reservar_proximo_cartao();
        if (cartao_id == -1) {
            printf("[ERRO] Não foi possível reservar cartão\n");
            break;
        }
        
        // 4. Registrar venda
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
        
        // 5. Atualizar estatísticas (simulação)
        static int empresa_count = 0;
        static int publico_count = 0;
        
        if (tipo == EMPRESA) empresa_count++;
        else publico_count++;
        
        // 6. Remover cliente da fila
        remover_cliente_processado(fila, id_cliente);
        
        vendas_realizadas++;
        
        
        sleep(1); 
        
        // 8. Verificar cenário especial (30 empresas após estoque vazio)
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
    printf("Tamanho: %d clientes\n", fila->tamanho);
    
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

/* Bloqueia vendas para público (cenário do enunciado) */
void bloquear_vendas_publico(FilaPrioridade* fila) {
    if (!fila) return;
    
    pthread_mutex_lock(&fila->lock);
    
    // Remove todos os clientes públicos da fila
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

/* Adiciona lote de empresas (cenário do enunciado - 30 empresas) */
void adicionar_lote_empresas(FilaPrioridade* fila, int quantidade) {
    if (!fila || quantidade <= 0) return;
    
    printf("[LOTE] Adicionando %d solicitações de empresas...\n", quantidade);
    
    static int next_client_id = 1000;  
    
    for (int i = 0; i < quantidade; i++) {
        inserir_cliente(fila, next_client_id++, EMPRESA);
    }
    
    printf("[LOTE] %d empresas adicionadas à fila\n", quantidade);
}
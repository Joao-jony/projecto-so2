#ifndef FILA_PRIORIDADE_H
#define FILA_PRIORIDADE_H

#include <pthread.h>
#include <time.h>

typedef enum {
    PUBLICO = 0,
    EMPRESA = 1
} TipoCliente;

typedef enum {
    MANHA,   // Limite 35
    TARDE,   // Limite 35
    NOITE    // Limite 30
} Turno;

typedef struct {
    int id_cliente;
    TipoCliente tipo;
    time_t timestamp;       // Quando entrou na fila
    int prioridade_calculada; // Cache da prioridade
} Cliente;

typedef struct Node {
    Cliente cliente;
    struct Node* next;
} Node;

typedef struct {
    Node* frente;      // Fila única ordenada por prioridade
    Node* fim;
    int tamanho;
    pthread_mutex_t lock;  // Mutex para thread-safety
} FilaPrioridade;

// Protótipos
FilaPrioridade* inicializar_fila();
void liberar_fila(FilaPrioridade* fila);

void inserir_cliente(FilaPrioridade* fila, int id_cliente, TipoCliente tipo);
int calcular_prioridade_cliente(Cliente* cliente);

Cliente* obter_proximo_cliente(FilaPrioridade* fila);
void remover_cliente_processado(FilaPrioridade* fila, int id_cliente);

int processar_vendas_turno(FilaPrioridade* fila, Turno turno_atual);
void imprimir_fila(FilaPrioridade* fila);

// Funções auxiliares para o cenário do enunciado
void bloquear_vendas_publico(FilaPrioridade* fila);
void adicionar_lote_empresas(FilaPrioridade* fila, int quantidade);

#endif
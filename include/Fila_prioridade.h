#ifndef FILA_PRIORIDADE_H
#define FILA_PRIORIDADE_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

typedef enum {
    EMPRESA,
    PUBLICO
} TipoCliente;

typedef enum {
    MANHA,
    TARDE,
    NOITE
} Turno;

typedef struct {
    int id_cliente;
    TipoCliente tipo;
    time_t timestamp;
    int prioridade_calculada;
} Cliente;

typedef struct Node {
    Cliente cliente;
    struct Node* next;
} Node;

typedef struct {
    Node* frente;
    Node* fim;
    int tamanho;
    pthread_mutex_t lock;       // Mutex para exclusão mútua
    sem_t semaforo_clientes;    // Semáforo para controle de clientes disponíveis
    sem_t semaforo_espaco;      // Semáforo para controle de espaço na fila
} FilaPrioridade;

// Protótipos das funções
FilaPrioridade* inicializar_fila(void);
void liberar_fila(FilaPrioridade* fila);
void inserir_cliente(FilaPrioridade* fila, int id_cliente, TipoCliente tipo);
Cliente* obter_proximo_cliente(FilaPrioridade* fila);
void remover_cliente_processado(FilaPrioridade* fila, int id_cliente);
int processar_vendas_turno(FilaPrioridade* fila, Turno turno_atual);
void imprimir_fila(FilaPrioridade* fila);
void bloquear_vendas_publico(FilaPrioridade* fila);
void adicionar_lote_empresas(FilaPrioridade* fila, int quantidade);
int calcular_prioridade_cliente(Cliente* cliente);

#endif
#ifndef FILA_PRIORIDADE_H
#define FILA_PRIORIDADE_H

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
    int id;
    TipoCliente tipo;
    time_t timestamp;
} Cliente;

typedef struct Node {
    Cliente cliente;
    struct Node* next;
} Node;

typedef struct {
    Node* frente_empresas;
    Node* fim_empresas;
    Node* frente_publico;
    Node* fim_publico;
} FilaVendas;

// Protótipos
FilaVendas* inicializar_fila();
void inserir_cliente(FilaVendas* fila, int id, TipoCliente tipo);
void processar_vendas_turno(FilaVendas* fila, Turno turno_atual);
void realizar_atendimento(Node** frente, Node** fim);

#endif

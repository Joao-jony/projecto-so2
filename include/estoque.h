#ifndef ESTOQUE_H
#define ESTOQUE_H

#include <pthread.h>

/* Número total de cartões SIM */
#define TOTAL_CARTOES 100

/* Estrutura do cartão SIM */
typedef struct {
    int id;        // Identificador único
    int vendido;   // 0 = disponível | 1 = vendido
    time_t hora_venda; // Timestamp da venda (opcional para logs)
} CartaoSIM;

/* Recursos globais compartilhados */
extern CartaoSIM estoque[TOTAL_CARTOES];
extern pthread_mutex_t estoque_lock;

/* Estatísticas */
extern int vendas_realizadas;
extern int vendas_empresas;
extern int vendas_publico;

/* Funções de gestão do estoque */
void inicializar_estoque();
void liberar_estoque();

int reservar_proximo_cartao();  // NOVA: escolhe automaticamente
int reservar_cartao_especifico(int id);  // Renomeada
int liberar_cartao(int id);

int estoque_disponivel();  // NOVA: conta disponíveis
int estoque_vendido();     // NOVA: conta vendidos
void imprimir_estoque();   // NOVA: para debugging

#endif

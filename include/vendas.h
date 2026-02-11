#ifndef VENDAS_H
#define VENDAS_H

#include "Fila_prioridade.h"
#include <pthread.h>
#include <time.h>

#define NUM_AGENCIAS 2
#define TEMPO_VENDA_SIMULADO 5
#define TEMPO_VENDA_REAL 1

// Estrutura de uma agência
typedef struct {
    int id;
    char nome[50];
    pthread_t thread;
    int vendas_realizadas;
    int clientes_atendidos;
    int ativa;
    pthread_mutex_t lock;
} Agencia;

// Estrutura para estatísticas
typedef struct {
    int total_vendas;
    int vendas_empresas;
    int vendas_publico;
    int vendas_por_turno[3];
} EstatisticasVendas;

// Protótipos
void inicializar_sistema_vendas(FilaPrioridade* fila_global);
void iniciar_turno_vendas(Turno turno);
void iniciar_vendas_concorrentes(Turno turno);
void parar_todas_agencias(void);
void exibir_relatorio_vendas(void);
void exibir_relatorio_agencias(void);
void exportar_vendas_csv(const char* filename);
void reinicializar_vendas(void);

// Funções auxiliares (para testes)
int get_vendas_totais(void);
int get_vendas_empresas(void);
int get_vendas_publico(void);

#endif
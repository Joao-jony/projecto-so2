#ifndef VENDAS_H
#define VENDAS_H

#include "Fila_prioridade.h"
#include <pthread.h>
#include <time.h>

#define NUM_AGENCIAS 2
#define TEMPO_VENDA_SIMULADO 5  // 5 segundos simulados
#define TEMPO_VENDA_REAL 1       // 1 segundo real = 5 simulados

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
    int vendas_por_turno[3]; // 0=MANHA, 1=TARDE, 2=NOITE
} EstatisticasVendas;

// Protótipos
void inicializar_sistema_vendas(FilaPrioridade* fila_global);
void iniciar_turno_vendas(Turno turno);
void iniciar_vendas_concorrentes(Turno turno);
void parar_todas_agencias();
void exibir_relatorio_vendas();
void exibir_relatorio_agencias();
void exportar_vendas_csv(const char* filename);
void reinicializar_vendas();

// Funções auxiliares (para testes)
int get_vendas_totais();
int get_vendas_empresas();
int get_vendas_publico();

#endif

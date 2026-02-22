#ifndef CONTRATACOES_H
#define CONTRATACOES_H

#include <time.h>
#include <pthread.h>
#include <signal.h>

#define LIMITE_CONTRATACOES 100
#define LIMITE_PROCESSOS_SIMULTANEOS 50
#define TEMPO_CONTRATACAO_MIN 10  
#define TEMPO_CONTRATACAO_MAX 20 
#define INTERVALO_EXIBICAO 30     

/* Estados do processo de contratação */
typedef enum {
    PENDENTE,
    EM_ANALISE,
    APROVADO,
    CONTRATADO,
    REJEITADO,
    CANCELADO
} EstadoProcesso;

/* Estrutura de um funcionário contratado */
typedef struct Contratacao {
    int id;
    char nome[100];
    char cargo[50];
    float salario;
    time_t data_contratacao;
    struct Contratacao *prox;
} Contratacao;

/* Estrutura de um processo de contratação */
typedef struct ProcessoContratacao {
    int id;
    char nome[100];
    char cargo[50];
    float salario;
    EstadoProcesso estado;
    time_t data_inicio;
    time_t data_conclusao;
    pthread_t thread_processo;
    struct ProcessoContratacao *prox;
} ProcessoContratacao;

/* Interface do módulo */
// Inicialização/Encerramento
void inicializar_sistema_rh(void);
void encerrar_sistema_rh(void);

// Sistema de interrupções
void configurar_interrupcoes_rh(void);
void iniciar_thread_verificacao(void);
void parar_thread_verificacao(void);
void verificar_interrupcao_rh(void);
void tratar_interrupcao_rh(int sinal);

// Timer periódico
void iniciar_timer_exibicao(void);
void parar_timer_exibicao(void);

// Processos de contratação
void iniciar_processo_contratacao(const char *nome, const char *cargo, float salario);
void analisar_processo(int id);
void concluir_contratacao(int id);
void rejeitar_processo(int id);
void cancelar_processo(int id);

// Gestão de funcionários
void adicionar_contratado(int id, const char *nome, const char *cargo, float salario);
void demitir_funcionario(int id);

// Relatórios
void exibir_processos_pendentes(void);
void exibir_contratacoes(void);
void exibir_estatisticas(void);
void exibir_contratacoes_periodicas(void);

// Thread para processo
void* thread_processo_contratacao(void* arg);

// Utilitários
int get_vagas_disponiveis(void);
int get_total_contratacoes(void);
int get_total_demissoes(void);
int get_funcionarios_ativos(void);
void limpar_lista(void);

#endif
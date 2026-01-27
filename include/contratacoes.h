#ifndef CONTRATACOES_H
contratacoes.h

#define CONTRATACOES_H

#include <time.h>

/* Estados do processo de contratação */
typedef enum {
    PENDENTE,
    EM_ANALISE,
    APROVADO,
    CONTRATADO,
    REJEITADO,
    CANCELADO
} EstadoContratacao;

/* Estrutura do processo de contratação */
typedef struct ProcessoContratacao {
    int id;
    char nome[100];
    char cargo[50];
    float salario;
    EstadoContratacao estado;
    time_t data_inicio;
    time_t data_conclusao;
    struct ProcessoContratacao *prox;
} ProcessoContratacao;

/* Estrutura da lista encadeada de contratados */
typedef struct Contratacao {
    int id;
    time_t data;
    char nome[100];
    char cargo[50];
    float salario;
    struct Contratacao *prox;
} Contratacao;

/* Interface do módulo */
// Processos de contratação
void iniciar_processo_contratacao(const char *nome, const char *cargo, float salario);
void analisar_processo(int id);
void aprovar_processo(int id);
void concluir_contratacao(int id);
void rejeitar_processo(int id);
void cancelar_processo(int id);

// Gerenciamento de contratados
void adicionar_contratado(int id, const char *nome, const char *cargo, float salario);
void demitir_funcionario(int id);
void exibir_contratacoes(void);
void limpar_lista(void);

// Relatórios e estatísticas
void exibir_processos_pendentes(void);
void exibir_estatisticas(void);
int get_contratacoes(void);
int get_demissoes(void);
int get_vagas_disponiveis(void);

#endif

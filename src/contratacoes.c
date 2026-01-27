contratacoes.c

#include "contratacoes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/* Variáveis internas ao módulo */
static Contratacao *lista_contratados = NULL;
static ProcessoContratacao *lista_processos = NULL;
static int total_contratacoes = 0;
static int total_demissoes = 0;
static int total_processos = 0;
static int processos_aprovados = 0;
static int processos_rejeitados = 0;

#define LIMITE_CONTRATACOES 100
#define LIMITE_PROCESSOS_SIMULTANEOS 50

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* ========== FUNÇÕES DE PROCESSO DE CONTRATAÇÃO ========== */

/* Inicia um novo processo de contratação */
void iniciar_processo_contratacao(const char *nome, const char *cargo, float salario) {
    pthread_mutex_lock(&mutex);
    
    // Verifica limite de processos simultâneos
    ProcessoContratacao *aux_proc = lista_processos;
    int processos_ativos = 0;
    while (aux_proc) {
        if (aux_proc->estado == PENDENTE || aux_proc->estado == EM_ANALISE) {
            processos_ativos++;
        }
        aux_proc = aux_proc->prox;
    }
    
    if (processos_ativos >= LIMITE_PROCESSOS_SIMULTANEOS) {
        printf("ERRO: Limite de %d processos simultâneos atingido!\n", 
               LIMITE_PROCESSOS_SIMULTANEOS);
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    // Verifica se já há vagas disponíveis
    int ativos = total_contratacoes - total_demissoes;
    if (ativos >= LIMITE_CONTRATACOES) {
        printf("AVISO: Limite de %d funcionários atingido! ", LIMITE_CONTRATACOES);
        printf("Processo será iniciado mas só poderá ser concluído após demissão.\n");
    }
    
    // Cria novo processo
    ProcessoContratacao *novo = malloc(sizeof(ProcessoContratacao));
    if (!novo) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    novo->id = ++total_processos;
    strncpy(novo->nome, nome, sizeof(novo->nome) - 1);
    strncpy(novo->cargo, cargo, sizeof(novo->cargo) - 1);
    novo->salario = salario;
    novo->estado = PENDENTE;
    novo->data_inicio = time(NULL);
    novo->data_conclusao = 0;
    novo->prox = lista_processos;
    lista_processos = novo;
    
    printf("=== NOVO PROCESSO INICIADO ===\n");
    printf("ID: %03d\n", novo->id);
    printf("Nome: %s\n", novo->nome);
    printf("Cargo: %s\n", novo->cargo);
    printf("Salário pretendido: %.2f\n", novo->salario);
    printf("Estado: PENDENTE\n");
    printf("=============================\n");
    
    pthread_mutex_unlock(&mutex);
}

/* Coloca processo em análise */
void analisar_processo(int id) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id) {
            if (proc->estado == PENDENTE) {
                proc->estado = EM_ANALISE;
                printf("Processo %03d agora está EM ANÁLISE\n", id);
            } else {
                printf("Processo %03d não pode ser analisado (estado: %d)\n", 
                       id, proc->estado);
            }
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("Processo %03d não encontrado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* Aprova um processo */
void aprovar_processo(int id) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id) {
            if (proc->estado == EM_ANALISE) {
                // Verifica se há vagas disponíveis
                int ativos = total_contratacoes - total_demissoes;
                if (ativos >= LIMITE_CONTRATACOES) {
                    printf("Processo %03d APROVADO mas AGUARDANDO VAGA\n", id);
                    printf("Vagas disponíveis: 0/%d\n", LIMITE_CONTRATACOES);
                    proc->estado = APROVADO;
                } else {
                    proc->estado = APROVADO;
                    printf("Processo %03d APROVADO\n", id);
                    printf("Vagas disponíveis: %d/%d\n", 
                           LIMITE_CONTRATACOES - ativos - 1, LIMITE_CONTRATACOES);
                }
                processos_aprovados++;
            } else {
                printf("Processo %03d não pode ser aprovado (estado: %d)\n", 
                       id, proc->estado);
            }
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("Processo %03d não encontrado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* Conclui a contratação de um processo aprovado */
void concluir_contratacao(int id) {
    pthread_mutex_lock(&mutex);
    
    // Verifica se há vagas disponíveis
    int ativos = total_contratacoes - total_demissoes;
    if (ativos >= LIMITE_CONTRATACOES) {
        printf("ERRO: Não há vagas disponíveis! (%d/%d)\n", 
               ativos, LIMITE_CONTRATACOES);
        printf("É necessário uma demissão antes de concluir a contratação.\n");
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id && proc->estado == APROVADO) {
            // Conclui o processo
            proc->estado = CONTRATADO;
            proc->data_conclusao = time(NULL);
            
            // Adiciona à lista de contratados
            adicionar_contratado(id, proc->nome, proc->cargo, proc->salario);
            
            printf("=== CONTRATAÇÃO CONCLUÍDA ===\n");
            printf("ID: %03d\n", id);
            printf("Nome: %s\n", proc->nome);
            printf("Cargo: %s\n", proc->cargo);
            printf("Salário: %.2f\n", proc->salario);
            printf("============================\n");
            
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("Processo %03d não encontrado ou não está aprovado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* Rejeita um processo */
void rejeitar_processo(int id) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id && 
           (proc->estado == PENDENTE || proc->estado == EM_ANALISE)) {
            proc->estado = REJEITADO;
            proc->data_conclusao = time(NULL);
            processos_rejeitados++;
            
            printf("Processo %03d REJEITADO\n", id);
            printf("Motivo: Não atende aos requisitos\n");
            
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("Processo %03d não encontrado ou não pode ser rejeitado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* Cancela um processo */
void cancelar_processo(int id) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id) {
            proc->estado = CANCELADO;
            proc->data_conclusao = time(NULL);
            
            printf("Processo %03d CANCELADO\n", id);
            
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("Processo %03d não encontrado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* ========== FUNÇÕES DE GERENCIAMENTO DE CONTRATADOS ========== */

/* Adiciona funcionário contratado */
void adicionar_contratado(int id, const char *nome, const char *cargo, float salario) {
    Contratacao *novo = malloc(sizeof(Contratacao));
    if (!novo) return;

    novo->id = id;
    novo->data = time(NULL);
    strncpy(novo->nome, nome, sizeof(novo->nome) - 1);
    strncpy(novo->cargo, cargo, sizeof(novo->cargo) - 1);
    novo->salario = salario;

    novo->prox = lista_contratados;
    lista_contratados = novo;
    total_contratacoes++;
}

/* Demite um funcionário */
void demitir_funcionario(int id) {
    pthread_mutex_lock(&mutex);

    Contratacao *anterior = NULL;
    Contratacao *atual = lista_contratados;
    
    while (atual) {
        if (atual->id == id) {
            if (anterior) {
                anterior->prox = atual->prox;
            } else {
                lista_contratados = atual->prox;
            }
            
            printf("=== DEMISSÃO REALIZADA ===\n");
            printf("ID: %03d\n", id);
            printf("Nome: %s\n", atual->nome);
            printf("Cargo: %s\n", atual->cargo);
            printf("=========================\n");
            
            free(atual);
            total_demissoes++;
            
            // Verifica se há processos aprovados aguardando vaga
            ProcessoContratacao *proc = lista_processos;
            while (proc) {
                if (proc->estado == APROVADO) {
                    printf("AVISO: Processo %03d (%s) pode ser concluído agora!\n",
                           proc->id, proc->nome);
                }
                proc = proc->prox;
            }
            
            pthread_mutex_unlock(&mutex);
            return;
        }
        anterior = atual;
        atual = atual->prox;
    }

    printf("Funcionário ID %03d não encontrado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* ========== FUNÇÕES DE RELATÓRIO ========== */

/* Exibe processos pendentes/aprovados */
void exibir_processos_pendentes(void) {
    pthread_mutex_lock(&mutex);
    
    printf("\n=== PROCESSOS DE CONTRATAÇÃO ===\n");
    
    if (!lista_processos) {
        printf("Nenhum processo em andamento.\n");
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    ProcessoContratacao *proc = lista_processos;
    char buffer[26];
    
    while (proc) {
        printf("\nID: %03d\n", proc->id);
        printf("Nome: %s\n", proc->nome);
        printf("Cargo: %s\n", proc->cargo);
        printf("Salário: %.2f\n", proc->salario);
        
        printf("Estado: ");
        switch(proc->estado) {
            case PENDENTE: printf("PENDENTE"); break;
            case EM_ANALISE: printf("EM ANÁLISE"); break;
            case APROVADO: printf("APROVADO"); break;
            case CONTRATADO: printf("CONTRATADO"); break;
            case REJEITADO: printf("REJEITADO"); break;
            case CANCELADO: printf("CANCELADO"); break;
        }
        printf("\n");
        
        ctime_r(&proc->data_inicio, buffer);
        printf("Início: %s", buffer);
        
        if (proc->data_conclusao > 0) {
            ctime_r(&proc->data_conclusao, buffer);
            printf("Conclusão: %s", buffer);
        }
        
        proc = proc->prox;
    }
    
    printf("================================\n");
    
    pthread_mutex_unlock(&mutex);
}

/* Exibe lista de contratados */
void exibir_contratacoes(void) {
    pthread_mutex_lock(&mutex);

    Contratacao *aux = lista_contratados;
    char buffer[26];
    int ativos = total_contratacoes - total_demissoes;

    printf("\n=== FUNCIONÁRIOS CONTRATADOS ===\n");
    printf("Limite máximo: %d | Ativos: %d | Vagas: %d\n",
           LIMITE_CONTRATACOES, ativos, LIMITE_CONTRATACOES - ativos);
    
    if (!aux) {
        printf("Nenhum funcionário contratado.\n");
    } else {
        while (aux) {
            ctime_r(&aux->data, buffer);
            printf("\nID: %03d\n", aux->id);
            printf("Nome: %s\n", aux->nome);
            printf("Cargo: %s\n", aux->cargo);
            printf("Salário: %.2f\n", aux->salario);
            printf("Data contratação: %s", buffer);
            aux = aux->prox;
        }
    }

    printf("\n--- Estatísticas ---\n");
    printf("Total contratações: %d\n", total_contratacoes);
    printf("Total demissões: %d\n", total_demissoes);
    printf("Ativos atualmente: %d\n", ativos);
    printf("==============================\n");

    pthread_mutex_unlock(&mutex);
}

/* Exibe estatísticas completas */
void exibir_estatisticas(void) {
    pthread_mutex_lock(&mutex);
    
    int ativos = total_contratacoes - total_demissoes;
    
    printf("\n=== ESTATÍSTICAS COMPLETAS ===\n");
    printf("\n1. FUNCIONÁRIOS:\n");
    printf("   Limite máximo: %d\n", LIMITE_CONTRATACOES);
    printf("   Ativos: %d\n", ativos);
    printf("   Vagas disponíveis: %d\n", LIMITE_CONTRATACOES - ativos);
    printf("   Total contratações: %d\n", total_contratacoes);
    printf("   Total demissões: %d\n", total_demissoes);
    
    printf("\n2. PROCESSOS:\n");
    printf("   Total processos: %d\n", total_processos);
    printf("   Processos aprovados: %d\n", processos_aprovados);
    printf("   Processos rejeitados: %d\n", processos_rejeitados);
    printf("   Processos pendentes: ");
    
    int pendentes = 0, em_analise = 0, aprovados = 0;
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        switch(proc->estado) {
            case PENDENTE: pendentes++; break;
            case EM_ANALISE: em_analise++; break;
            case APROVADO: aprovados++; break;
            default: break;
        }
        proc = proc->prox;
    }
    
    printf("%d\n", pendentes);
    printf("   Processos em análise: %d\n", em_analise);
    printf("   Processos aprovados aguardando: %d\n", aprovados);
    
    printf("\n3. TAXAS:\n");
    if (total_processos > 0) {
        printf("   Taxa de aprovação: %.1f%%\n", 
               (processos_aprovados * 100.0) / total_processos);
        printf("   Taxa de rejeição: %.1f%%\n", 
               (processos_rejeitados * 100.0) / total_processos);
    }
    
    printf("===============================\n");
    
    pthread_mutex_unlock(&mutex);
}

/* ========== FUNÇÕES AUXILIARES ========== */

/* Retorna vagas disponíveis */
int get_vagas_disponiveis(void) {
    pthread_mutex_lock(&mutex);
    int ativos = total_contratacoes - total_demissoes;
    int vagas = LIMITE_CONTRATACOES - ativos;
    vagas = (vagas > 0) ? vagas : 0;
    pthread_mutex_unlock(&mutex);
    return vagas;
}

/* Liberta toda a memória */
void limpar_lista(void) {
    pthread_mutex_lock(&mutex);

    // Limpa lista de contratados
    while (lista_contratados) {
        Contratacao *tmp = lista_contratados;
        lista_contratados = lista_contratados->prox;
        free(tmp);
    }
    
    // Limpa lista de processos
    while (lista_processos) {
        ProcessoContratacao *tmp = lista_processos;
        lista_processos = lista_processos->prox;
        free(tmp);
    }

    total_contratacoes = 0;
    total_demissoes = 0;
    total_processos = 0;
    processos_aprovados = 0;
    processos_rejeitados = 0;

    pthread_mutex_unlock(&mutex);
}

/* Getters */
int get_contratacoes(void) {
    pthread_mutex_lock(&mutex);
    int c = total_contratacoes;
    pthread_mutex_unlock(&mutex);
    return c;
}

int get_demissoes(void) {
    pthread_mutex_lock(&mutex);
    int d = total_demissoes;
    pthread_mutex_unlock(&mutex);
    return d;
}

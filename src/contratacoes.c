#include "contratacoes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static Contratacao *lista_contratados = NULL;
static ProcessoContratacao *lista_processos = NULL;
static int total_contratacoes = 0;
static int total_demissoes = 0;
static int total_processos = 0;
static int processos_aprovados = 0;
static int processos_rejeitados = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t thread_timer;
static pthread_t thread_verificacao;
static volatile int timer_ativa = 0;
static volatile int verificacao_ativa = 0;
static volatile int interrupcao_ativa = 0;

/* ========== INICIALIZAÇÃO/ENCERRAMENTO ========== */

void inicializar_sistema_rh(void) {
    printf("[RH] Sistema de Recursos Humanos inicializando...\n");
    
    configurar_interrupcoes_rh();
    iniciar_timer_exibicao();
    iniciar_thread_verificacao();
    
    printf("[RH] Sistema pronto. Limite: %d funcionários\n", LIMITE_CONTRATACOES);
    printf("[RH] Timer de exibição configurado a cada %d segundos\n", INTERVALO_EXIBICAO);
}

void encerrar_sistema_rh(void) {
    printf("[RH] Encerrando sistema...\n");
    
    parar_timer_exibicao();
    parar_thread_verificacao();
    limpar_lista();
    
    pthread_mutex_destroy(&mutex);
    
    printf("[RH] Sistema encerrado\n");
}

/* ========== SISTEMA DE INTERRUPÇÕES ========== */

void handler_interrupcao_rh(int sig) {
    (void)sig;
    interrupcao_ativa = 1;
    
    pthread_mutex_lock(&mutex);
    int contratacoes = total_contratacoes;
    int demissoes = total_demissoes;
    pthread_mutex_unlock(&mutex);
    
    printf("\n  [INTERRUPÇÃO DO SISTEMA] \n");
    printf("ALERTA: Contratações (%d) > Demissões (%d)\n", 
           contratacoes, demissoes);
    printf("Diferença: %d funcionário(s)\n", contratacoes - demissoes);
    printf("Ação necessária: Verificar necessidade de demissões\n");
    printf("  -------------------------- \n\n");
}

void configurar_interrupcoes_rh(void) {
    signal(SIGUSR1, handler_interrupcao_rh);
    printf("[RH] Sistema de interrupções configurado (usando thread)\n");
}

static void* thread_verificacao_periodica(void* arg) {
    (void)arg;
    
    printf("[RH VERIFICAÇÃO] Thread de verificação iniciada\n");
    
    while (verificacao_ativa) {
        sleep(10);
        
        if (verificacao_ativa) {
            verificar_interrupcao_rh();
        }
    }
    
    printf("[RH VERIFICAÇÃO] Thread de verificação encerrada\n");
    return NULL;
}

void iniciar_thread_verificacao(void) {
    verificacao_ativa = 1;
    pthread_create(&thread_verificacao, NULL, thread_verificacao_periodica, NULL);
    printf("[RH] Thread de verificação periódica iniciada (10s)\n");
}

void parar_thread_verificacao(void) {
    verificacao_ativa = 0;
    pthread_join(thread_verificacao, NULL);
    printf("[RH] Thread de verificação periódica parada\n");
}

void verificar_interrupcao_rh(void) {
    pthread_mutex_lock(&mutex);
    int contratacoes = total_contratacoes;
    int demissoes = total_demissoes;
    pthread_mutex_unlock(&mutex);
    
    if (contratacoes > demissoes) {
        printf("[RH VERIFICAÇÃO] Condição de interrupção detectada!\n");
        handler_interrupcao_rh(SIGUSR1);
    }
}

/* ========== TIMER DE EXIBIÇÃO PERIÓDICA ========== */

static void* thread_exibicao_periodica(void* arg) {
    (void)arg;
    
    while (timer_ativa) {
        sleep(INTERVALO_EXIBICAO);
        
        if (timer_ativa) {
            printf("\n [EXIBIÇÃO PERIÓDICA] \n");
            printf("Intervalo: %d segundos (simula 3 minutos)\n", INTERVALO_EXIBICAO);
            exibir_contratacoes_periodicas();
        }
    }
    
    return NULL;
}

void iniciar_timer_exibicao(void) {
    timer_ativa = 1;
    pthread_create(&thread_timer, NULL, thread_exibicao_periodica, NULL);
    printf("[RH] Timer de exibição periódica iniciado (%ds)\n", INTERVALO_EXIBICAO);
}

void parar_timer_exibicao(void) {
    timer_ativa = 0;
    pthread_join(thread_timer, NULL);
    printf("[RH] Timer de exibição periódica parado\n");
}

/* ========== PROCESSO DE CONTRATAÇÃO ========== */

void* thread_processo_contratacao(void* arg) {
    ProcessoContratacao* processo = (ProcessoContratacao*)arg;
    
    printf("[RH PROCESSO %03d] Iniciando análise...\n", processo->id);
    
    int tempo_analise = TEMPO_CONTRATACAO_MIN + 
                       (rand() % (TEMPO_CONTRATACAO_MAX - TEMPO_CONTRATACAO_MIN + 1));
    
    printf("[RH PROCESSO %03d] Tempo estimado: %d segundos\n", 
           processo->id, tempo_analise);
    
    for (int i = 1; i <= tempo_analise; i++) {
        if (processo->estado == CANCELADO || processo->estado == REJEITADO) {
            printf("[RH PROCESSO %03d] Processo interrompido\n", processo->id);
            return NULL;
        }
        sleep(1);
        
        if (i % 10 == 0) {
            printf("[RH PROCESSO %03d] Em análise... %d/%d segundos\n", 
                   processo->id, i, tempo_analise);
        }
    }
    
    pthread_mutex_lock(&mutex);
    
    if (processo->estado == EM_ANALISE) {
        processo->estado = APROVADO;
        processo->data_conclusao = time(NULL);
        processos_aprovados++;
        
        printf("[RH PROCESSO %03d] ANÁLISE CONCLUÍDA - APROVADO\n", processo->id);
        printf("[RH PROCESSO %03d] Aguardando conclusão da contratação...\n", processo->id);
        
        int ativos = total_contratacoes - total_demissoes;
        if (ativos < LIMITE_CONTRATACOES) {
            printf("[RH PROCESSO %03d] Vaga disponível! Concluindo automaticamente...\n", processo->id);
            processo->estado = CONTRATADO;
            adicionar_contratado(processo->id, processo->nome, processo->cargo, processo->salario);
        }
    }
    
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

void iniciar_processo_contratacao(const char *nome, const char *cargo, float salario) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *aux_proc = lista_processos;
    int processos_ativos = 0;
    while (aux_proc) {
        if (aux_proc->estado == PENDENTE || aux_proc->estado == EM_ANALISE) {
            processos_ativos++;
        }
        aux_proc = aux_proc->prox;
    }
    
    if (processos_ativos >= LIMITE_PROCESSOS_SIMULTANEOS) {
        printf("[RH ERRO] Limite de %d processos simultâneos atingido!\n", 
               LIMITE_PROCESSOS_SIMULTANEOS);
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    int ativos = total_contratacoes - total_demissoes;
    if (ativos >= LIMITE_CONTRATACOES) {
        printf("[RH AVISO] Limite de %d funcionários atingido! ", LIMITE_CONTRATACOES);
        printf("Processo será iniciado mas só poderá ser concluído após demissão.\n");
    }
    
    ProcessoContratacao *novo = malloc(sizeof(ProcessoContratacao));
    if (!novo) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    novo->id = ++total_processos;
    strncpy(novo->nome, nome, sizeof(novo->nome) - 1);
    novo->nome[sizeof(novo->nome) - 1] = '\0';
    strncpy(novo->cargo, cargo, sizeof(novo->cargo) - 1);
    novo->cargo[sizeof(novo->cargo) - 1] = '\0';
    novo->salario = salario;
    novo->estado = PENDENTE;
    novo->data_inicio = time(NULL);
    novo->data_conclusao = 0;
    novo->prox = lista_processos;
    lista_processos = novo;
    
    printf("[RH] === NOVO PROCESSO INICIADO ===\n");
    printf("[RH] ID: %03d\n", novo->id);
    printf("[RH] Nome: %s\n", novo->nome);
    printf("[RH] Cargo: %s\n", novo->cargo);
    printf("[RH] Salário pretendido: %.2f\n", novo->salario);
    printf("[RH] Estado: PENDENTE\n");
    printf("[RH] =============================\n");
    
    pthread_mutex_unlock(&mutex);
    
    sleep(2);
    analisar_processo(novo->id);
}

void analisar_processo(int id) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id) {
            if (proc->estado == PENDENTE) {
                proc->estado = EM_ANALISE;
                printf("[RH PROCESSO %03d] Estado: EM ANÁLISE\n", id);
                
                pthread_create(&proc->thread_processo, NULL, 
                              thread_processo_contratacao, proc);
                pthread_detach(proc->thread_processo);
            } else {
                printf("[RH PROCESSO %03d] Não pode ser analisado (estado: %d)\n", 
                       id, proc->estado);
            }
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("[RH ERRO] Processo %03d não encontrado!\n", id);
    pthread_mutex_unlock(&mutex);
}

void concluir_contratacao(int id) {
    pthread_mutex_lock(&mutex);
    
    int ativos = total_contratacoes - total_demissoes;
    if (ativos >= LIMITE_CONTRATACOES) {
        printf("[RH ERRO] Não há vagas disponíveis! (%d/%d)\n", 
               ativos, LIMITE_CONTRATACOES);
        printf("[RH] É necessário uma demissão antes de concluir.\n");
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id && proc->estado == APROVADO) {
            proc->estado = CONTRATADO;
            proc->data_conclusao = time(NULL);
            
            adicionar_contratado(id, proc->nome, proc->cargo, proc->salario);
            
            printf("[RH] === CONTRATAÇÃO CONCLUÍDA ===\n");
            printf("[RH] ID: %03d\n", id);
            printf("[RH] Nome: %s\n", proc->nome);
            printf("[RH] Cargo: %s\n", proc->cargo);
            printf("[RH] Salário: %.2f\n", proc->salario);
            printf("[RH] Vagas restantes: %d/%d\n", 
                   LIMITE_CONTRATACOES - ativos - 1, LIMITE_CONTRATACOES);
            printf("[RH] ============================\n");
            
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("[RH ERRO] Processo %03d não encontrado ou não aprovado!\n", id);
    pthread_mutex_unlock(&mutex);
}

void rejeitar_processo(int id) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id && 
           (proc->estado == PENDENTE || proc->estado == EM_ANALISE)) {
            proc->estado = REJEITADO;
            proc->data_conclusao = time(NULL);
            processos_rejeitados++;
            
            printf("[RH PROCESSO %03d] ❌ REJEITADO\n", id);
            printf("[RH] Motivo: Não atende aos requisitos\n");
            
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("[RH ERRO] Processo %03d não encontrado ou não pode ser rejeitado!\n", id);
    pthread_mutex_unlock(&mutex);
}

void cancelar_processo(int id) {
    pthread_mutex_lock(&mutex);
    
    ProcessoContratacao *proc = lista_processos;
    while (proc) {
        if (proc->id == id) {
            proc->estado = CANCELADO;
            proc->data_conclusao = time(NULL);
            
            printf("[RH PROCESSO %03d] CANCELADO\n", id);
            
            pthread_mutex_unlock(&mutex);
            return;
        }
        proc = proc->prox;
    }
    
    printf("[RH ERRO] Processo %03d não encontrado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* ========== FUNÇÕES DE CONTRATADOS ========== */

void adicionar_contratado(int id, const char *nome, const char *cargo, float salario) {
    Contratacao *novo = malloc(sizeof(Contratacao));
    if (!novo) return;

    novo->id = id;
    novo->data_contratacao = time(NULL);
    strncpy(novo->nome, nome, sizeof(novo->nome) - 1);
    novo->nome[sizeof(novo->nome) - 1] = '\0';
    strncpy(novo->cargo, cargo, sizeof(novo->cargo) - 1);
    novo->cargo[sizeof(novo->cargo) - 1] = '\0';
    novo->salario = salario;
    novo->prox = lista_contratados;
    lista_contratados = novo;
    total_contratacoes++;
    
    verificar_interrupcao_rh();
}

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
            
            printf("[RH] === DEMISSÃO REALIZADA ===\n");
            printf("[RH] ID: %03d\n", id);
            printf("[RH] Nome: %s\n", atual->nome);
            printf("[RH] Cargo: %s\n", atual->cargo);
            printf("[RH] =========================\n");
            
            free(atual);
            total_demissoes++;
            
            ProcessoContratacao *proc = lista_processos;
            while (proc) {
                if (proc->estado == APROVADO) {
                    printf("[RH AVISO] Processo %03d (%s) pode ser concluído agora!\n",
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

    printf("[RH ERRO] Funcionário ID %03d não encontrado!\n", id);
    pthread_mutex_unlock(&mutex);
}

/* ========== FUNÇÕES DE RELATÓRIO ========== */

void exibir_processos_pendentes(void) {
    pthread_mutex_lock(&mutex);
    
    printf("\n[RH] === PROCESSOS DE CONTRATAÇÃO ===\n");
    
    if (!lista_processos) {
        printf("[RH] Nenhum processo em andamento.\n");
        pthread_mutex_unlock(&mutex);
        return;
    }
    
    ProcessoContratacao *proc = lista_processos;
    int count = 0;
    
    while (proc && count < 10) {
        printf("\n[RH] ID: %03d\n", proc->id);
        printf("[RH] Nome: %s\n", proc->nome);
        printf("[RH] Cargo: %s\n", proc->cargo);
        printf("[RH] Salário: %.2f\n", proc->salario);
        
        printf("[RH] Estado: ");
        switch(proc->estado) {
            case PENDENTE: printf("PENDENTE"); break;
            case EM_ANALISE: printf("EM ANÁLISE"); break;
            case APROVADO: printf("APROVADO"); break;
            case CONTRATADO: printf("CONTRATADO"); break;
            case REJEITADO: printf("REJEITADO"); break;
            case CANCELADO: printf("CANCELADO"); break;
        }
        printf("\n");
        
        char buffer[26];
        struct tm *tm_info = localtime(&proc->data_inicio);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        printf("[RH] Início: %s\n", buffer);
        
        if (proc->data_conclusao > 0) {
            tm_info = localtime(&proc->data_conclusao);
            strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            printf("[RH] Conclusão: %s\n", buffer);
        }
        
        proc = proc->prox;
        count++;
    }
    
    if (count == 10 && proc != NULL) {
        printf("[RH] ... (%d processos mais antigos não exibidos)\n", 
               total_processos - 10);
    }
    
    printf("[RH] ================================\n");
    
    pthread_mutex_unlock(&mutex);
}

void exibir_contratacoes(void) {
    pthread_mutex_lock(&mutex);

    Contratacao *aux = lista_contratados;
    int ativos = total_contratacoes - total_demissoes;

    printf("\n[RH] === FUNCIONÁRIOS CONTRATADOS ===\n");
    printf("[RH] Limite máximo: %d | Ativos: %d | Vagas: %d\n",
           LIMITE_CONTRATACOES, ativos, LIMITE_CONTRATACOES - ativos);
    
    if (!aux) {
        printf("[RH] Nenhum funcionário contratado.\n");
    } else {
        int count = 0;
        while (aux && count < 5) {
            char buffer[26];
            struct tm *tm_info = localtime(&aux->data_contratacao);
            strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            
            printf("\n[RH] ID: %03d\n", aux->id);
            printf("[RH] Nome: %s\n", aux->nome);
            printf("[RH] Cargo: %s\n", aux->cargo);
            printf("[RH] Salário: %.2f\n", aux->salario);
            printf("[RH] Data: %s\n", buffer);
            
            aux = aux->prox;
            count++;
        }
        
        if (count == 5 && aux != NULL) {
            printf("[RH] ... (%d funcionários mais antigos não exibidos)\n", 
                   total_contratacoes - 5);
        }
    }

    printf("\n[RH] --- Estatísticas ---\n");
    printf("[RH] Total contratações: %d\n", total_contratacoes);
    printf("[RH] Total demissões: %d\n", total_demissoes);
    printf("[RH] Ativos atualmente: %d\n", ativos);
    printf("[RH] ==============================\n");

    pthread_mutex_unlock(&mutex);
}

void exibir_contratacoes_periodicas(void) {
    pthread_mutex_lock(&mutex);
    
    int ativos = total_contratacoes - total_demissoes;
    
    printf("\n═══════════════════════════════════\n");
    printf("   RELATÓRIO PERIÓDICO DE RH\n");
    printf("═══════════════════════════════════\n");
    printf("  Funcionários ativos: %d/%d\n", ativos, LIMITE_CONTRATACOES);
    printf("  Vagas disponíveis:   %d\n", LIMITE_CONTRATACOES - ativos);
    printf("  Total contratações:  %d\n", total_contratacoes);
    printf("  Total demissões:     %d\n", total_demissoes);
    printf("═══════════════════════════════════\n\n");
    
    pthread_mutex_unlock(&mutex);
}

void exibir_estatisticas(void) {
    pthread_mutex_lock(&mutex);
    
    int ativos = total_contratacoes - total_demissoes;
    
    printf("\n[RH] === ESTATÍSTICAS COMPLETAS ===\n");
    printf("\n[RH] 1. FUNCIONÁRIOS:\n");
    printf("[RH]    Limite máximo: %d\n", LIMITE_CONTRATACOES);
    printf("[RH]    Ativos: %d\n", ativos);
    printf("[RH]    Vagas disponíveis: %d\n", LIMITE_CONTRATACOES - ativos);
    printf("[RH]    Total contratações: %d\n", total_contratacoes);
    printf("[RH]    Total demissões: %d\n", total_demissoes);
    
    printf("\n[RH] 2. PROCESSOS:\n");
    printf("[RH]    Total processos: %d\n", total_processos);
    printf("[RH]    Processos aprovados: %d\n", processos_aprovados);
    printf("[RH]    Processos rejeitados: %d\n", processos_rejeitados);
    
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
    
    printf("[RH]    Processos pendentes: %d\n", pendentes);
    printf("[RH]    Processos em análise: %d\n", em_analise);
    printf("[RH]    Processos aprovados aguardando: %d\n", aprovados);
    
    printf("\n[RH] 3. INTERRUPÇÕES:\n");
    printf("[RH]    Interrupções ativas: %s\n", interrupcao_ativa ? "SIM" : "NÃO");
    if (interrupcao_ativa) {
        printf("[RH]    Contratações > Demissões: %d > %d\n", 
               total_contratacoes, total_demissoes);
    }
    
    printf("[RH] ===============================\n");
    
    pthread_mutex_unlock(&mutex);
}

/* ========== FUNÇÕES AUXILIARES ========== */

int get_vagas_disponiveis(void) {
    pthread_mutex_lock(&mutex);
    int ativos = total_contratacoes - total_demissoes;
    int vagas = LIMITE_CONTRATACOES - ativos;
    vagas = (vagas > 0) ? vagas : 0;
    pthread_mutex_unlock(&mutex);
    return vagas;
}

int get_total_contratacoes(void) {
    pthread_mutex_lock(&mutex);
    int c = total_contratacoes;
    pthread_mutex_unlock(&mutex);
    return c;
}

int get_total_demissoes(void) {
    pthread_mutex_lock(&mutex);
    int d = total_demissoes;
    pthread_mutex_unlock(&mutex);
    return d;
}

int get_funcionarios_ativos(void) {
    return get_total_contratacoes() - get_total_demissoes();
}

void limpar_lista(void) {
    pthread_mutex_lock(&mutex);

    while (lista_contratados) {
        Contratacao *tmp = lista_contratados;
        lista_contratados = lista_contratados->prox;
        free(tmp);
    }
    
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
    interrupcao_ativa = 0;

    pthread_mutex_unlock(&mutex);
}

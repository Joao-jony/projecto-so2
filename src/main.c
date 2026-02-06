#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include "estoque.h"
#include "Fila_prioridade.h"
#include "vendas.h"
#include "contratacoes.h"

#define SIMULACAO_ATIVA 1
#define TEMPO_TOTAL_SIMULACAO 30 

static volatile int sistema_executando = 1;
static FilaPrioridade* fila_global = NULL;

void handler_sigint(int sig) {
    (void)sig;
    printf("\n\n⚠️  Interrupção recebida. Encerrando sistema...\n");
    sistema_executando = 0;
}

/* Inicializar todos os módulos */
int inicializar_sistema(void) {
    printf("========================================\n");
    printf("   SISTEMA OPERACIONAL UNITEL v1.0\n");
    printf("   Simulador de Multiprocessador\n");
    printf("========================================\n\n");
    
    // Configurar handler para Ctrl+C
    signal(SIGINT, handler_sigint);
    
    printf("[SISTEMA] Inicializando módulos...\n");
    
    // 1. Inicializar estoque
    inicializar_estoque();
    
    // 2. Inicializar fila de prioridade
    fila_global = inicializar_fila();
    if (!fila_global) {
        printf("  Erro ao inicializar fila de prioridade\n");
        return 0;
    }
    
    // 3. Inicializar sistema de vendas (CORRIGIDO)
    inicializar_sistema_vendas(fila_global); 
    inicializar_sistema_rh();
    
    printf("\n[SISTEMA] Todos os módulos inicializados com sucesso!\n");
    return 1;
}

void popular_dados_iniciais(void) {
    printf(" Adicionando clientes à fila...\n");
    
    // 20 empresas
    for (int i = 1; i <= 30; i++) {
        inserir_cliente(fila_global, 1000 + i, EMPRESA);
    }
    
    // 50 clientes público
    for (int i = 1; i <= 40; i++) {
        inserir_cliente(fila_global, 2000 + i, PUBLICO);
    }
    
    printf("  • 70 clientes adicionados (20 empresas, 50 público)\n");
    
    printf("  contratações iniciais...\n");
    for (int i = 0; i < 3; i++) {
        // Usar funções corretas do módulo de contratações
        iniciar_processo_contratacao("Funcionário Inicial", "Cargo Inicial", 50000 + i * 10000);
        sleep(1);
    }
}

static void* wrapper_contratacao(void* arg) {
    (void)arg;
    // Não existe simular_contratacao_thread, então usamos funções diretas
    printf("[THREAD] Processo de contratação iniciado\n");
    iniciar_processo_contratacao("Novo Funcionário", "Analista", 60000);
    return NULL;
}

static void* wrapper_demissao(void* arg) {
    (void)arg;
    printf("[THREAD] Processo de demissão iniciado\n");
    if (get_funcionarios_ativos() > 0) {
        printf("[THREAD] Demissão simulada \n");
    }
    return NULL;
}

/* Executar cenários de teste */
void executar_cenarios_teste(void) {
    printf("\n[SISTEMA] Executando cenários de teste...\n");
    
    // Cenário 1: Vendas normais por turno
    printf("\n--- CENÁRIO 1: Vendas por Turno ---\n");
    printf("Turno da Manhã (35 vendas):\n");
    iniciar_turno_vendas(MANHA);
    
    printf("\nTurno da Tarde (35 vendas):\n");
    iniciar_turno_vendas(TARDE);
    
    printf("\nTurno da Noite (30 vendas):\n");
    iniciar_turno_vendas(NOITE);
    
    // Cenário 2: Estoque vazio + 30 empresas
    printf("\n--- CENÁRIO 2: Estoque Esgotado + 30 Empresas ---\n");
    
    // Primeiro esgotar estoque
    while (estoque_disponivel() > 0) {
        reservar_proximo_cartao();
    }
    
    printf("Estoque esgotado! Bloqueando vendas públicas...\n");
    bloquear_vendas_publico(fila_global);
    
    printf("Adicionando 30 solicitações de empresas...\n");
    adicionar_lote_empresas(fila_global, 30);
    
    // Liberar alguns cartões para teste
    for (int i = 0; i < 10; i++) {
        liberar_cartao(i);
    }
    
    printf("Processando novas solicitações...\n");
    iniciar_turno_vendas(MANHA);

    printf("\n--- CENÁRIO 3: Sistema de RH ---\n");
    printf("Simulando fluxo de contratações/demissoes...\n");
    
    for (int i = 0; i < 5; i++) {
        pthread_t thread_contratacao;
        pthread_create(&thread_contratacao, NULL, 
                      wrapper_contratacao, NULL);  
        pthread_detach(thread_contratacao);
        
        usleep(500000); 
    }
    
    for (int i = 0; i < 2; i++) {
        pthread_t thread_demissao;
        pthread_create(&thread_demissao, NULL, 
                      wrapper_demissao, NULL); 
        pthread_detach(thread_demissao);
        
        usleep(500000);
    }
}

/* Loop principal do sistema */
void loop_principal(void) {
    printf("\n[SISTEMA] Entrando no loop principal de execução...\n");
    printf("Pressione Ctrl+C para encerrar o sistema.\n\n");
    
    time_t inicio = time(NULL);
    int ciclos = 0;
    
    while (sistema_executando && 
           difftime(time(NULL), inicio) < TEMPO_TOTAL_SIMULACAO) {
        
        ciclos++;
        
        // Mostrar status a cada 5 ciclos
        if (ciclos % 5 == 0) {
            printf("\n--- STATUS DO SISTEMA [Ciclo %d] ---\n", ciclos);
            
            // Estoque
            printf("Estoque: %d/%d cartões disponíveis\n", 
                   estoque_disponivel(), TOTAL_CARTOES);
            
            // Fila
            imprimir_fila(fila_global); 
            
            // Agências
            exibir_relatorio_agencias();  
            
            // RH
            printf("RH: %d funcionários ativos\n", get_funcionarios_ativos());
            
            printf("--------------------------------\n");
        }
        
        // Processar algumas vendas
        if (ciclos % 3 == 0 && estoque_disponivel() > 0) {
            printf("[SISTEMA] Processando vendas concorrentes...\n");
            iniciar_vendas_concorrentes(MANHA);
        }
        
        // Simular operações de RH aleatórias
        if (rand() % 4 == 0) {
            if (rand() % 2 == 0) {
                pthread_t thread;
                pthread_create(&thread, NULL, 
                              wrapper_contratacao, NULL); 
                pthread_detach(thread);
            } else {
                pthread_t thread;
                pthread_create(&thread, NULL, 
                              wrapper_demissao, NULL);  
                pthread_detach(thread);
            }
        }
        
        sleep(1); // 1 segundo entre ciclos
    }
    
    printf("\n[SISTEMA] Tempo de simulação concluído.\n");
}

/* Encerrar todos os módulos */
void encerrar_sistema(void) {
    printf("\n[SISTEMA] Encerrando módulos...\n");
    
    // 1. Parar agências
    printf("  • Parando sistema de agências...\n");
    parar_todas_agencias();
    
    // 2. Encerrar sistema de RH
    printf("  • Encerrando sistema de RH...\n");
    encerrar_sistema_rh();
    
    // 3. Liberar fila
    printf("  • Liberando fila de prioridade...\n");
    if (fila_global) {
        liberar_fila(fila_global);
    }
    
    // 4. Liberar estoque
    printf("  • Liberando estoque...\n");
    liberar_estoque();
    
    printf("\n[SISTEMA] Todos os módulos encerrados com sucesso!\n");
}

void gerar_relatorio_final(void) {
    printf("\n========================================\n");
    printf("        RELATÓRIO FINAL DO SISTEMA\n");
    printf("========================================\n\n");
    
    // Estoque
    printf("ESTOQUE:\n");
    imprimir_estoque();
    
    // Vendas
    printf("\n VENDAS:\n");
    exibir_relatorio_agencias();  
    
    // RH
    printf("\nRECURSOS HUMANOS:\n");
    exibir_contratacoes();  
    
    // Estatísticas
    printf("\nESTATÍSTICAS GERAIS:\n");
    printf("  • Cartões vendidos: %d/%d\n", 
           estoque_vendido(), TOTAL_CARTOES);
    printf("  • Funcionários ativos: %d\n", 
           get_funcionarios_ativos());
    
    printf("\n========================================\n");
}

int main(void) {
    printf("Iniciando Sistema Operacional UNITEL...\n");
    
    srand(time(NULL));
    
    if (!inicializar_sistema()) {
        printf("Erro na inicialização do sistema.\n");
        return 1;
    }
    
    popular_dados_iniciais();
    
    executar_cenarios_teste();
    
    loop_principal();
    
    gerar_relatorio_final();
    
    encerrar_sistema();
    
    printf("\n========================================\n");
    printf("   SISTEMA DE VENDAS UNITEL FINALIZADO\n");
    printf("========================================\n");
    
    return 0;
}
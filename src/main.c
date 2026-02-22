#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "estoque.h"
#include "Fila_prioridade.h"
#include "vendas.h"
#include "contratacoes.h"
#include "webserver.h"

#define SIMULACAO_ATIVA 1
#define TEMPO_TOTAL_SIMULACAO 60
#define INTERVALO_ENTRE_TURNOS 5

static volatile int sistema_executando = 1;
static volatile int modo_interativo = 0;

// Variáveis globais exportadas
FilaPrioridade* fila_global = NULL;
static pthread_mutex_t system_lock = PTHREAD_MUTEX_INITIALIZER;

/* Handler para Ctrl+C */
void handler_sigint(int sig) {
    (void)sig;
    printf("\n\n⚠️  INTERRUPÇÃO RECEBIDA - Ctrl+C\n");
    printf("Encerrando sistema de forma segura...\n");
    sistema_executando = 0;
}

/* Handler para SIGTERM */
void handler_sigterm(int sig) {
    (void)sig;
    printf("\n\n⚠️  SINAL DE TÉRMINO RECEBIDO\n");
    sistema_executando = 0;
}

/* Configurar handlers de sinal */
void configurar_handlers(void) {
    struct sigaction sa;
    
    sa.sa_handler = handler_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    sa.sa_handler = handler_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    
    signal(SIGPIPE, SIG_IGN);
}

/* Wrapper para contratação */
static void* wrapper_contratacao(void* arg) {
    (void)arg;
    
    pthread_mutex_lock(&system_lock);
    
    const char* nomes[] = {"João Silva", "Maria Santos", "Carlos Oliveira", 
                          "Ana Costa", "Pedro Almeida"};
    const char* cargos[] = {"Analista", "Desenvolvedor", "Gerente", 
                           "Coordenador", "Consultor"};
    
    int idx = rand() % 5;
    float salario = 50000 + (rand() % 10) * 5000;
    
    pthread_mutex_unlock(&system_lock);
    
    iniciar_processo_contratacao(nomes[idx], cargos[idx], salario);
    return NULL;
}

/* Wrapper para demissão */
static void* wrapper_demissao(void* arg) {
    (void)arg;
    
    int ativos = get_funcionarios_ativos();
    if (ativos > 0) {
        printf("[THREAD] %d funcionários ativos disponíveis para demissão\n", ativos);
    } else {
        printf("[THREAD] Nenhum funcionário ativo para demitir\n");
    }
    return NULL;
}

/* Inicializar todos os módulos */
int inicializar_sistema(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════════\n");
    printf("           SISTEMA OPERACIONAL UNITEL v2.0\n");
    printf("══════════════════════════════════════════════════════════\n\n");
    
    configurar_handlers();
    
    printf("[SISTEMA] 🔧 Inicializando módulos...\n");
    printf("────────────────────────────────────────────────────\n");
    
    printf("[SISTEMA] 📦 Inicializando estoque... ");
    fflush(stdout);
    inicializar_estoque();
    printf("OK (%d cartões)\n", TOTAL_CARTOES);
    
    printf("[SISTEMA] 👥 Inicializando fila de prioridade... ");
    fflush(stdout);
    fila_global = inicializar_fila();
    if (!fila_global) {
        printf("FALHA!\n");
        return 0;
    }
    printf("OK\n");
    
    printf("[SISTEMA] 🏢 Inicializando agências... ");
    fflush(stdout);
    inicializar_sistema_vendas(fila_global);
    printf("OK (%d agências)\n", NUM_AGENCIAS);
    
    printf("[SISTEMA] 👔 Inicializando RH... ");
    fflush(stdout);
    inicializar_sistema_rh();
    printf("OK (limite: %d funcionários)\n", LIMITE_CONTRATACOES);
    
    printf("[SISTEMA] 🌐 Inicializando web server... ");
    fflush(stdout);
    if (iniciar_webserver()) {
        printf("OK\n");
    } else {
        printf("FALHA - Continuando sem interface web\n");
    }
    
    printf("────────────────────────────────────────────────────\n");
    printf("[SISTEMA] ✅ Todos os módulos inicializados com sucesso!\n\n");
    
    return 1;
}

/* Popular dados iniciais */
void popular_dados_iniciais(void) {
    printf("[SISTEMA] 📊 Populando dados iniciais...\n");
    printf("────────────────────────────────────────────────────\n");
    
    printf("[FILA] 🏢 Adicionando 30 empresas... ");
    fflush(stdout);
    for (int i = 1; i <= 30; i++) {
        inserir_cliente(fila_global, 1000 + i, EMPRESA);
    }
    printf("OK\n");
    
    printf("[FILA] 👤 Adicionando 40 clientes público... ");
    fflush(stdout);
    for (int i = 1; i <= 40; i++) {
        inserir_cliente(fila_global, 2000 + i, PUBLICO);
    }
    printf("OK\n");
    
    printf("[FILA] 📈 Total: 70 clientes na fila\n");
    printf("────────────────────────────────────────────────────\n\n");
}

/* Loop principal */
void loop_principal(void) {
    printf("\n══════════════════════════════════════════════════════════\n");
    printf("           LOOP PRINCIPAL DE EXECUÇÃO\n");
    printf("══════════════════════════════════════════════════════════\n\n");
    
    printf("📌 Sistema em execução - Pressione Ctrl+C para encerrar\n\n");
    
    time_t inicio = time(NULL);
    int ciclos = 0;
    
    while (sistema_executando) {
        if (difftime(time(NULL), inicio) >= TEMPO_TOTAL_SIMULACAO) {
            printf("\n⏰ Tempo de simulação concluído (%d segundos)\n", 
                   TEMPO_TOTAL_SIMULACAO);
            break;
        }
        
        ciclos++;
        
        if (ciclos % 10 == 0) {
            printf("\n════════════════════════════════════════════\n");
            printf("     STATUS DO SISTEMA [Ciclo %d]\n", ciclos);
            printf("════════════════════════════════════════════\n");
            
            printf("📦 Estoque: %d/%d disponíveis\n", 
                   estoque_disponivel(), TOTAL_CARTOES);
            printf("👔 RH: %d/%d funcionários\n", 
                   get_funcionarios_ativos(), LIMITE_CONTRATACOES);
            printf("👥 Fila: %d clientes\n", fila_global->tamanho);
            printf("💰 Vendas: %d total\n", get_vendas_totais());
            printf("════════════════════════════════════════════\n");
        }
        
        sleep(1);
    }
}

/* Gerar relatório final */
void gerar_relatorio_final(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════════\n");
    printf("              RELATÓRIO FINAL DO SISTEMA\n");
    printf("══════════════════════════════════════════════════════════\n\n");
    
    time_t agora = time(NULL);
    struct tm* tm_info = localtime(&agora);
    char timestamp[26];
    strftime(timestamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("📅 Data/Hora: %s\n\n", timestamp);
    
    printf("📦 ESTOQUE:\n");
    printf("   • Disponíveis: %d/%d\n", estoque_disponivel(), TOTAL_CARTOES);
    printf("   • Vendidos: %d/%d\n", estoque_vendido(), TOTAL_CARTOES);
    
    printf("\n👔 RECURSOS HUMANOS:\n");
    printf("   • Funcionários ativos: %d/%d\n", 
           get_funcionarios_ativos(), LIMITE_CONTRATACOES);
    printf("   • Contratações: %d\n", get_total_contratacoes());
    printf("   • Demissões: %d\n", get_total_demissoes());
    
    printf("\n💰 VENDAS:\n");
    printf("   • Total: %d\n", get_vendas_totais());
    printf("   • Empresas: %d\n", get_vendas_empresas());
    printf("   • Público: %d\n", get_vendas_publico());
    
    printf("\n══════════════════════════════════════════════════════════\n");
}

/* Encerrar sistema */
void encerrar_sistema(void) {
    printf("\n");
    printf("══════════════════════════════════════════════════════════\n");
    printf("           ENCERRANDO SISTEMA\n");
    printf("══════════════════════════════════════════════════════════\n\n");
    
    printf("[SISTEMA] 🌐 Parando web server... ");
    fflush(stdout);
    parar_webserver();
    printf("OK\n");
    
    printf("[SISTEMA] 🏢 Parando agências... ");
    fflush(stdout);
    parar_todas_agencias();
    printf("OK\n");
    
    printf("[SISTEMA] 👔 Encerrando RH... ");
    fflush(stdout);
    encerrar_sistema_rh();
    printf("OK\n");
    
    printf("[SISTEMA] 👥 Liberando fila... ");
    fflush(stdout);
    if (fila_global) {
        liberar_fila(fila_global);
        fila_global = NULL;
    }
    printf("OK\n");
    
    printf("[SISTEMA] 📦 Liberando estoque... ");
    fflush(stdout);
    liberar_estoque();
    printf("OK\n");
    
    pthread_mutex_destroy(&system_lock);
    
    printf("\n[SISTEMA] ✅ Sistema encerrado com sucesso!\n");
}

/* Main */
int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                                                          ║\n");
    printf("║     SISTEMA OPERACIONAL UNITEL - SIMULADOR v2.0         ║\n");
    printf("║                                                          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    srand(time(NULL));
    
    if (!inicializar_sistema()) {
        printf("[SISTEMA] ❌ Erro fatal na inicialização. Abortando.\n");
        return 1;
    }
    
    popular_dados_iniciais();
    
    // Executar um turno de exemplo
    printf("\n--- EXECUTANDO TURNO DE TESTE ---\n");
    iniciar_turno_vendas(MANHA);
    
    loop_principal();
    gerar_relatorio_final();
    encerrar_sistema();
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                                                          ║\n");
    printf("║     SISTEMA FINALIZADO COM SUCESSO                      ║\n");
    printf("║                                                          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    return 0;
}
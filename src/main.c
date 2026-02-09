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
#define INTERVALO_ENTRE_TURNOS 3 // Intervalo de 3 segundos entre turnos

static volatile int sistema_executando = 1;
static FilaPrioridade* fila_global = NULL;

void handler_sigint(int sig) {
    (void)sig;
    printf("\n\n⚠️  Interrupção recebida. Encerrando sistema...\n");
    sistema_executando = 0;
}

/* Funções wrapper para contratações */
static void* wrapper_contratacao(void* arg) {
    (void)arg;
    printf("[THREAD] Processo de contratação iniciado\n");
    
    // Simular diferentes tipos de funcionários
    const char* nomes[] = {"João Silva", "Maria Santos", "Carlos Oliveira", 
                          "Ana Costa", "Pedro Almeida"};
    const char* cargos[] = {"Analista", "Desenvolvedor", "Gerente", 
                           "Coordenador", "Consultor"};
    
    int idx = rand() % 5;
    float salario = 50000 + (rand() % 10) * 5000;
    
    iniciar_processo_contratacao(nomes[idx], cargos[idx], salario);
    return NULL;
}

static void* wrapper_demissao(void* arg) {
    (void)arg;
    printf("[THREAD] Processo de demissão iniciado\n");
    
    // Verificar se há funcionários para demitir
    // Em um sistema real, você precisaria de lógica para selecionar qual funcionário demitir
    // Por enquanto, apenas exibimos uma mensagem
    int ativos = get_funcionarios_ativos();
    if (ativos > 0) {
        printf("[THREAD] %d funcionários ativos disponíveis para demissão\n", ativos);
        // Nota: Para realmente demitir, precisaria do ID do funcionário
        // demitir_funcionario(id_do_funcionario);
    } else {
        printf("[THREAD] Nenhum funcionário ativo para demitir\n");
    }
    return NULL;
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
    
    // 3. Inicializar sistema de vendas
    inicializar_sistema_vendas(fila_global); 
    inicializar_sistema_rh();
    
    printf("\n[SISTEMA] Todos os módulos inicializados com sucesso!\n");
    return 1;
}

void popular_dados_iniciais(void) {
    printf(" Adicionando clientes à fila...\n");
    
    // 30 empresas
    for (int i = 1; i <= 30; i++) {
        inserir_cliente(fila_global, 1000 + i, EMPRESA);
    }
    
    // 40 clientes público
    for (int i = 1; i <= 40; i++) {
        inserir_cliente(fila_global, 2000 + i, PUBLICO);
    }
    
    printf("  • 70 clientes adicionados (30 empresas, 40 público)\n");
    
    // NÃO iniciar contratações aqui - apenas nos intervalos entre turnos
    printf("  • Contratações serão realizadas apenas nos intervalos entre turnos\n");
}

/* Executar turnos com intervalos para contratações */
void executar_cenarios_teste_com_intervalos(void) {
    printf("\n[SISTEMA] Executando cenários de teste com intervalos...\n");
    
    // Cenário 1: Vendas normais por turno com intervalos
    printf("\n--- CENÁRIO 1: Vendas por Turno com Intervalos ---\n");
    
    // Turno da Manhã
    printf("\nTurno da Manhã (35 vendas):\n");
    iniciar_turno_vendas(MANHA);
    
    // INTERVALO PARA CONTRATAÇÕES
    printf("\n--- INTERVALO ENTRE TURNOS (3 segundos) ---\n");
    printf("Processando contratações no intervalo...\n");
    for (int i = 0; i < 2; i++) {
        pthread_t thread_contratacao;
        pthread_create(&thread_contratacao, NULL, wrapper_contratacao, NULL);
        pthread_detach(thread_contratacao);
        usleep(500000); // 0.5 segundos entre contratações
    }
    sleep(INTERVALO_ENTRE_TURNOS);
    
    // Turno da Tarde
    printf("\nTurno da Tarde (35 vendas):\n");
    iniciar_turno_vendas(TARDE);
    
    // INTERVALO PARA CONTRATAÇÕES
    printf("\n--- INTERVALO ENTRE TURNOS (3 segundos) ---\n");
    printf("Processando contratações no intervalo...\n");
    for (int i = 0; i < 3; i++) {
        pthread_t thread_contratacao;
        pthread_create(&thread_contratacao, NULL, wrapper_contratacao, NULL);
        pthread_detach(thread_contratacao);
        usleep(500000);
    }
    sleep(INTERVALO_ENTRE_TURNOS);
    
    // Turno da Noite
    printf("\nTurno da Noite (30 vendas):\n");
    iniciar_turno_vendas(NOITE);
    
    // INTERVALO FINAL PARA CONTRATAÇÕES
    printf("\n--- INTERVALO FINAL (3 segundos) ---\n");
    printf("Processando contratações no intervalo...\n");
    for (int i = 0; i < 2; i++) {
        pthread_t thread_contratacao;
        pthread_create(&thread_contratacao, NULL, wrapper_contratacao, NULL);
        pthread_detach(thread_contratacao);
        usleep(500000);
    }
    sleep(INTERVALO_ENTRE_TURNOS);
    
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
    
    // INTERVALO PARA DEMISSÕES
    printf("\n--- INTERVALO PARA AJUSTES DE RH ---\n");
    printf("Processando demissões no intervalo...\n");
    for (int i = 0; i < 2; i++) {
        pthread_t thread_demissao;
        pthread_create(&thread_demissao, NULL, wrapper_demissao, NULL);
        pthread_detach(thread_demissao);
        usleep(500000);
    }
    sleep(INTERVALO_ENTRE_TURNOS);
}

/* Loop principal do sistema */
void loop_principal(void) {
    printf("\n[SISTEMA] Entrando no loop principal de execução...\n");
    printf("Pressione Ctrl+C para encerrar o sistema.\n\n");
    
    time_t inicio = time(NULL);
    int ciclos = 0;
    int ultimo_turno_processado = -1;
    
    while (sistema_executando && 
           difftime(time(NULL), inicio) < TEMPO_TOTAL_SIMULACAO) {
        
        ciclos++;
        
        // A cada 5 ciclos, processar um "turno" simulado
        if (ciclos % 5 == 0) {
            int turno_atual = (ciclos / 5) % 3; // Alterna entre 0, 1, 2 (cast para int)
            
            if (turno_atual != ultimo_turno_processado) {
                // Se houve mudança de turno, processar intervalo primeiro
                if (ultimo_turno_processado != -1) {
                    printf("\n--- INTERVALO ENTRE TURNOS ---\n");
                    printf("Processando operações de RH no intervalo...\n");
                    
                    // Executar algumas operações de RH no intervalo
                    int num_operacoes = 1 + (rand() % 3);
                    for (int i = 0; i < num_operacoes; i++) {
                        if (rand() % 2 == 0) {
                            pthread_t thread_contratacao;
                            pthread_create(&thread_contratacao, NULL, 
                                          wrapper_contratacao, NULL);
                            pthread_detach(thread_contratacao);
                        } else {
                            pthread_t thread_demissao;
                            pthread_create(&thread_demissao, NULL, 
                                          wrapper_demissao, NULL);
                            pthread_detach(thread_demissao);
                        }
                        usleep(300000); // 0.3 segundos entre operações
                    }
                    sleep(INTERVALO_ENTRE_TURNOS);
                }
                
                // Processar o turno atual
                const char* nomes_turnos[] = {"Manhã", "Tarde", "Noite"};
                printf("\n=== INICIANDO TURNO DA %s ===\n", nomes_turnos[turno_atual]);
                
                // Converter int para Turno enum
                Turno turno_enum;
                switch(turno_atual) {
                    case 0: turno_enum = MANHA; break;
                    case 1: turno_enum = TARDE; break;
                    case 2: turno_enum = NOITE; break;
                    default: turno_enum = MANHA; break;
                }
                
                // Processar vendas do turno
                iniciar_turno_vendas(turno_enum);
                
                ultimo_turno_processado = turno_atual;
            }
        }
        
        // Mostrar status a cada 3 ciclos
        if (ciclos % 3 == 0) {
            printf("\n--- STATUS DO SISTEMA [Ciclo %d] ---\n", ciclos);
            
            // Estoque
            printf("Estoque: %d/%d cartões disponíveis\n", 
                   estoque_disponivel(), TOTAL_CARTOES);
            
            // RH
            printf("RH: %d funcionários ativos | %d vagas disponíveis\n", 
                   get_funcionarios_ativos(), get_vagas_disponiveis());
            
            printf("--------------------------------\n");
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
    printf("\nVENDAS:\n");
    exibir_relatorio_agencias();
    
    // RH
    printf("\nRECURSOS HUMANOS:\n");
    exibir_contratacoes();
    exibir_estatisticas();
    
    // Estatísticas
    printf("\nESTATÍSTICAS GERAIS:\n");
    printf("  • Cartões vendidos: %d/%d\n", 
           estoque_vendido(), TOTAL_CARTOES);
    printf("  • Funcionários ativos: %d/%d\n", 
           get_funcionarios_ativos(), LIMITE_CONTRATACOES);
    printf("  • Vagas disponíveis: %d\n", 
           get_vagas_disponiveis());
    
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
    
    // Usar nova função com intervalos
    executar_cenarios_teste_com_intervalos();
    
    loop_principal();
    
    gerar_relatorio_final();
    
    encerrar_sistema();
    
    printf("\n========================================\n");
    printf("   SISTEMA DE VENDAS UNITEL FINALIZADO\n");
    printf("========================================\n");
    
    return 0;
}
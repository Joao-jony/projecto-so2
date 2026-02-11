#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#ifdef USE_NCURSES
#include <ncurses.h>
#include <locale.h>
#endif

#include "estoque.h"
#include "Fila_prioridade.h"
#include "vendas.h"
#include "contratacoes.h"

#define SIMULACAO_ATIVA 1
#define TEMPO_TOTAL_SIMULACAO 30 
#define INTERVALO_ENTRE_TURNOS 3

static volatile int sistema_executando = 1;
static FilaPrioridade* fila_global = NULL;
static int total_vendas_realizadas = 0;

// Declaração da função tamanho_fila
int tamanho_fila(FilaPrioridade* fila);

#ifdef USE_NCURSES
static WINDOW *win_principal, *win_estoque, *win_rh, *win_vendas, *win_fila, *win_status, *win_log;
static pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;
static int linha_log = 1;

void init_ncurses_windows(void) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(100);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_BLUE, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLUE);
        init_pair(8, COLOR_WHITE, COLOR_RED);
    }
    
    clear();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    win_principal = newwin(max_y - 3, max_x, 0, 0);
    win_status = newwin(1, max_x, max_y - 3, 0);
    win_log = newwin(2, max_x, max_y - 2, 0);
    
    wattron(win_principal, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win_principal, 0, (max_x - 42) / 2, "+------------------------------------------+");
    mvwprintw(win_principal, 1, (max_x - 42) / 2, "|     SISTEMA OPERACIONAL UNITEL v1.0      |");
    mvwprintw(win_principal, 2, (max_x - 42) / 2, "|       Simulador de Multiprocessador      |");
    mvwprintw(win_principal, 3, (max_x - 42) / 2, "+------------------------------------------+");
    wattroff(win_principal, COLOR_PAIR(4) | A_BOLD);
    
    int info_y = 5;
    int info_width = (max_x - 8) / 3;
    
    win_estoque = derwin(win_principal, 8, info_width, info_y, 2);
    win_rh = derwin(win_principal, 8, info_width, info_y, info_width + 4);
    win_vendas = derwin(win_principal, 8, info_width, info_y, 2 * info_width + 6);
    
    win_fila = derwin(win_principal, 10, max_x - 4, info_y + 9, 2);
    
    // Usando caracteres ASCII em vez de ACS
    box(win_estoque, 0, 0);
    box(win_rh, 0, 0);
    box(win_vendas, 0, 0);
    box(win_fila, 0, 0);
    
    wattron(win_estoque, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win_estoque, 0, 3, "ESTOQUE");
    wattroff(win_estoque, COLOR_PAIR(1) | A_BOLD);
    
    wattron(win_rh, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win_rh, 0, 3, "RECURSOS HUMANOS");
    wattroff(win_rh, COLOR_PAIR(5) | A_BOLD);
    
    wattron(win_vendas, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win_vendas, 0, 3, "VENDAS");
    wattroff(win_vendas, COLOR_PAIR(2) | A_BOLD);
    
    wattron(win_fila, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(win_fila, 0, 3, "FILA DE CLIENTES");
    wattroff(win_fila, COLOR_PAIR(6) | A_BOLD);
    
    wattron(win_status, COLOR_PAIR(7) | A_REVERSE);
    mvwhline(win_status, 0, 0, ' ', max_x);
    mvwprintw(win_status, 0, 2, " UNITEL Simulator ");
    wattroff(win_status, COLOR_PAIR(7) | A_REVERSE);
    
    wattron(win_status, COLOR_PAIR(2));
    mvwprintw(win_status, 0, 25, "| [Q] Sair | [R] Atualizar | [P] Pausar/Continuar |");
    wattroff(win_status, COLOR_PAIR(2));
    
    wattron(win_log, COLOR_PAIR(3) | A_BOLD);
    mvwhline(win_log, 0, 0, ' ', max_x);
    mvwprintw(win_log, 0, 2, "LOG DO SISTEMA:");
    wattroff(win_log, COLOR_PAIR(3) | A_BOLD);
    
    refresh();
    wrefresh(win_principal);
    wrefresh(win_status);
    wrefresh(win_log);
}

void atualizar_interface(void) {
    pthread_mutex_lock(&ncurses_mutex);
    
    int max_y, max_x;
    getmaxyx(win_principal, max_y, max_x);
    
    int info_width = (max_x - 8) / 3;
    
    werase(win_estoque);
    box(win_estoque, 0, 0);
    
    wattron(win_estoque, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win_estoque, 0, 3, "ESTOQUE");
    wattroff(win_estoque, COLOR_PAIR(1) | A_BOLD);
    
    int disponivel = estoque_disponivel();
    int vendidos = estoque_vendido();
    float percentual = (float)vendidos / TOTAL_CARTOES * 100;
    
    wattron(win_estoque, COLOR_PAIR(2));
    mvwprintw(win_estoque, 2, 2, "Disponivel: ");
    wattron(win_estoque, A_BOLD);
    wprintw(win_estoque, "%d", disponivel);
    wattroff(win_estoque, A_BOLD);
    
    mvwprintw(win_estoque, 3, 2, "Vendidos:   ");
    wattron(win_estoque, A_BOLD);
    wprintw(win_estoque, "%d", vendidos);
    wattroff(win_estoque, A_BOLD);
    
    mvwprintw(win_estoque, 4, 2, "Total:      ");
    wattron(win_estoque, A_BOLD);
    wprintw(win_estoque, "%d", TOTAL_CARTOES);
    wattroff(win_estoque, A_BOLD);
    
    mvwprintw(win_estoque, 6, 2, "Progresso: ");
    int bar_width = info_width - 15;
    int filled = (int)((float)vendidos / TOTAL_CARTOES * bar_width);
    
    wattron(win_estoque, COLOR_PAIR(1));
    for (int i = 0; i < bar_width; i++) {
        if (i < filled)
            mvwaddch(win_estoque, 6, 13 + i, '#');
        else
            mvwaddch(win_estoque, 6, 13 + i, '-');
    }
    wattroff(win_estoque, COLOR_PAIR(1));
    
    mvwprintw(win_estoque, 6, 13 + bar_width + 1, "%d%%", (int)percentual);
    wattroff(win_estoque, COLOR_PAIR(2));
    wrefresh(win_estoque);
    
    werase(win_rh);
    box(win_rh, 0, 0);
    
    wattron(win_rh, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win_rh, 0, 3, "RECURSOS HUMANOS");
    wattroff(win_rh, COLOR_PAIR(5) | A_BOLD);
    
    int ativos = get_funcionarios_ativos();
    int vagas = get_vagas_disponiveis();
    
    wattron(win_rh, COLOR_PAIR(2));
    mvwprintw(win_rh, 2, 2, "Funcionarios: ");
    wattron(win_rh, A_BOLD);
    wprintw(win_rh, "%d", ativos);
    wattroff(win_rh, A_BOLD);
    
    mvwprintw(win_rh, 3, 2, "Vagas:        ");
    wattron(win_rh, A_BOLD);
    wprintw(win_rh, "%d", vagas);
    wattroff(win_rh, A_BOLD);
    
    mvwprintw(win_rh, 4, 2, "Limite:       ");
    wattron(win_rh, A_BOLD);
    wprintw(win_rh, "%d", LIMITE_CONTRATACOES);
    wattroff(win_rh, A_BOLD);
    
    if (vagas > 0) {
        wattron(win_rh, COLOR_PAIR(1));
        mvwprintw(win_rh, 6, 2, "Contratando");
        wattroff(win_rh, COLOR_PAIR(1));
    } else {
        wattron(win_rh, COLOR_PAIR(3));
        mvwprintw(win_rh, 6, 2, "Sem vagas");
        wattroff(win_rh, COLOR_PAIR(3));
    }
    wattroff(win_rh, COLOR_PAIR(2));
    wrefresh(win_rh);
    
    werase(win_vendas);
    box(win_vendas, 0, 0);
    
    wattron(win_vendas, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win_vendas, 0, 3, "VENDAS");
    wattroff(win_vendas, COLOR_PAIR(2) | A_BOLD);
    
    wattron(win_vendas, COLOR_PAIR(4));
    mvwprintw(win_vendas, 2, 2, "Total vendas:  ");
    wattron(win_vendas, A_BOLD);
    wprintw(win_vendas, "%d", total_vendas_realizadas);
    wattroff(win_vendas, A_BOLD);
    
    mvwprintw(win_vendas, 3, 2, "Receita:       ");
    wattron(win_vendas, A_BOLD);
    wprintw(win_vendas, "R$ %.2f", total_vendas_realizadas * 50.0);
    wattroff(win_vendas, A_BOLD);
    
    mvwprintw(win_vendas, 5, 2, "Media/turno:   ");
    wattron(win_vendas, A_BOLD);
    wprintw(win_vendas, "%.1f", total_vendas_realizadas / 3.0);
    wattroff(win_vendas, A_BOLD);
    wattroff(win_vendas, COLOR_PAIR(4));
    wrefresh(win_vendas);
    
    werase(win_fila);
    box(win_fila, 0, 0);
    
    wattron(win_fila, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(win_fila, 0, 3, "FILA DE CLIENTES");
    wattroff(win_fila, COLOR_PAIR(6) | A_BOLD);
    
    if (fila_global) {
        int tamanho = tamanho_fila(fila_global);
        
        wattron(win_fila, COLOR_PAIR(4));
        mvwprintw(win_fila, 2, 2, "Total na fila: ");
        wattron(win_fila, A_BOLD);
        wattron(win_fila, COLOR_PAIR(2));
        wprintw(win_fila, "%d", tamanho);
        wattroff(win_fila, COLOR_PAIR(2));
        wattroff(win_fila, A_BOLD);
        
        mvwprintw(win_fila, 4, 2, "Proximos clientes:");
        
        for (int i = 0; i < 5 && i < tamanho; i++) {
            if (i % 2 == 0) {
                wattron(win_fila, COLOR_PAIR(2));
                mvwprintw(win_fila, 5 + i, 4, "Empresa %d", 1001 + i);
                wattroff(win_fila, COLOR_PAIR(2));
            } else {
                wattron(win_fila, COLOR_PAIR(4));
                mvwprintw(win_fila, 5 + i, 4, "Publico %d", 2001 + i);
                wattroff(win_fila, COLOR_PAIR(4));
            }
        }
        wattroff(win_fila, COLOR_PAIR(4));
    }
    wrefresh(win_fila);
    wrefresh(win_principal);
    
    pthread_mutex_unlock(&ncurses_mutex);
}

void ncurses_log(const char *msg) {
    pthread_mutex_lock(&ncurses_mutex);
    
    int max_y, max_x;
    getmaxyx(win_log, max_y, max_x);
    
    (void)max_y;  // Evita warning de variável não utilizada
    (void)max_x;  // Evita warning de variável não utilizada
    
    if (linha_log > 0) {
        wscrl(win_log, 1);
        linha_log--;
    }
    
    wattron(win_log, COLOR_PAIR(4));
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    mvwprintw(win_log, linha_log + 1, 2, "[%02d:%02d:%02d] > %s", 
              tm->tm_hour, tm->tm_min, tm->tm_sec, msg);
    wattroff(win_log, COLOR_PAIR(4));
    
    wrefresh(win_log);
    linha_log++;
    
    pthread_mutex_unlock(&ncurses_mutex);
}

#else
void log_texto(const char *msg) {
    printf("%s\n", msg);
}
#endif

void handler_sigint(int sig) {
    (void)sig;
#ifdef USE_NCURSES
    ncurses_log("Interrupcao recebida. Encerrando sistema...");
#else
    printf("\n\nInterrupcao recebida. Encerrando sistema...\n");
#endif
    sistema_executando = 0;
}

static void* wrapper_contratacao(void* arg) {
    (void)arg;
    const char* nomes[] = {"Joao Silva", "Maria Santos", "Carlos Oliveira", 
                          "Ana Costa", "Pedro Almeida", "Fernanda Lima",
                          "Roberto Souza", "Juliana Mendes", "Paulo Ferreira",
                          "Camila Rocha"};
    
    const char* cargos[] = {"Analista", "Desenvolvedor", "Gerente", 
                           "Coordenador", "Consultor", "Arquiteto",
                           "Suporte", "Testador", "DevOps", "Scrum Master"};
    
    int idx = rand() % 10;
    float salario = 3000 + (rand() % 15) * 1000;
    
#ifdef USE_NCURSES
    char msg[200];
    snprintf(msg, sizeof(msg), "[RH] Contratando: %s como %s (R$ %.2f)", 
             nomes[idx], cargos[idx], salario);
    ncurses_log(msg);
#else
    printf("[THREAD] Contratando: %s como %s (R$ %.2f)\n", 
           nomes[idx], cargos[idx], salario);
#endif
    
    iniciar_processo_contratacao(nomes[idx], cargos[idx], salario);
    return NULL;
}

static void* wrapper_demissao(void* arg) {
    (void)arg;
    int ativos = get_funcionarios_ativos();
    
    if (ativos > 0) {
        int id_demitir = 1 + (rand() % ativos);
#ifdef USE_NCURSES
        char msg[100];
        snprintf(msg, sizeof(msg), "[RH] Demitindo funcionario ID: %d", id_demitir);
        ncurses_log(msg);
#else
        printf("[THREAD] Demitindo funcionario ID: %d\n", id_demitir);
#endif
        demitir_funcionario(id_demitir);
    }
    return NULL;
}

int inicializar_sistema(void) {
#ifdef USE_NCURSES
    init_ncurses_windows();
    ncurses_log("SISTEMA OPERACIONAL UNITEL v1.0");
    ncurses_log("Inicializando modulos...");
#else
    printf("========================================\n");
    printf("   SISTEMA OPERACIONAL UNITEL v1.0\n");
    printf("   Simulador de Multiprocessador\n");
    printf("========================================\n\n");
    
    signal(SIGINT, handler_sigint);
    printf("[SISTEMA] Inicializando modulos...\n");
#endif
    
    inicializar_estoque();
    
    fila_global = inicializar_fila();
    if (!fila_global) {
#ifdef USE_NCURSES
        ncurses_log("Erro ao inicializar fila de prioridade");
        endwin();
#else
        printf("Erro ao inicializar fila de prioridade\n");
#endif
        return 0;
    }
    
    inicializar_sistema_vendas(fila_global);
    inicializar_sistema_rh();
    
#ifdef USE_NCURSES
    ncurses_log("Todos os modulos inicializados com sucesso!");
#else
    printf("Todos os modulos inicializados com sucesso!\n");
#endif
    return 1;
}

void popular_dados_iniciais(void) {
#ifdef USE_NCURSES
    ncurses_log("Adicionando clientes a fila...");
#else
    printf("Adicionando clientes a fila...\n");
#endif
    
    for (int i = 1; i <= 30; i++) {
        inserir_cliente(fila_global, 1000 + i, EMPRESA);
    }
    
    for (int i = 1; i <= 40; i++) {
        inserir_cliente(fila_global, 2000 + i, PUBLICO);
    }
    
#ifdef USE_NCURSES
    char msg[100];
    snprintf(msg, sizeof(msg), "70 clientes adicionados (30 empresas, 40 publico)");
    ncurses_log(msg);
    atualizar_interface();
#else
    printf("70 clientes adicionados (30 empresas, 40 publico)\n");
#endif
}

void executar_cenarios_teste_com_intervalos(void) {
#ifdef USE_NCURSES
    ncurses_log("Executando cenarios de teste...");
#else
    printf("\n[SISTEMA] Executando cenarios de teste...\n");
#endif
    
#ifdef USE_NCURSES
    ncurses_log("CENARIO 1: Vendas por Turno");
#else
    printf("\n--- CENARIO 1: Vendas por Turno ---\n");
#endif
    
#ifdef USE_NCURSES
    ncurses_log("Turno da Manha (35 vendas):");
#else
    printf("\nTurno da Manha (35 vendas):\n");
#endif
    iniciar_turno_vendas(MANHA);
    total_vendas_realizadas += 35;
    
#ifdef USE_NCURSES
    atualizar_interface();
    ncurses_log("Intervalo entre turnos...");
#endif
    sleep(INTERVALO_ENTRE_TURNOS);
    
#ifdef USE_NCURSES
    ncurses_log("Turno da Tarde (35 vendas):");
#else
    printf("\nTurno da Tarde (35 vendas):\n");
#endif
    iniciar_turno_vendas(TARDE);
    total_vendas_realizadas += 35;
    
#ifdef USE_NCURSES
    atualizar_interface();
    ncurses_log("Intervalo entre turnos...");
#endif
    sleep(INTERVALO_ENTRE_TURNOS);
    
#ifdef USE_NCURSES
    ncurses_log("Turno da Noite (30 vendas):");
#else
    printf("\nTurno da Noite (30 vendas):\n");
#endif
    iniciar_turno_vendas(NOITE);
    total_vendas_realizadas += 30;
    
#ifdef USE_NCURSES
    atualizar_interface();
    ncurses_log("Cenario 1 concluido!");
#endif
    sleep(INTERVALO_ENTRE_TURNOS);
    
#ifdef USE_NCURSES
    ncurses_log("CENARIO 2: Estoque Esgotado");
#else
    printf("\n--- CENARIO 2: Estoque Esgotado ---\n");
#endif
    
    while (estoque_disponivel() > 0) {
        reservar_proximo_cartao();
    }
    
#ifdef USE_NCURSES
    ncurses_log("Estoque esgotado! Bloqueando vendas publicas...");
    ncurses_log("Adicionando 30 solicitacoes de empresas...");
#else
    printf("Estoque esgotado! Bloqueando vendas publicas...\n");
    printf("Adicionando 30 solicitacoes de empresas...\n");
#endif
    
    bloquear_vendas_publico(fila_global);
    adicionar_lote_empresas(fila_global, 30);
    
    for (int i = 0; i < 10; i++) {
        liberar_cartao(i);
    }
    
#ifdef USE_NCURSES
    ncurses_log("Processando novas solicitacoes...");
    atualizar_interface();
#else
    printf("Processando novas solicitacoes...\n");
#endif
    
    iniciar_turno_vendas(MANHA);
    total_vendas_realizadas += 10;
    
#ifdef USE_NCURSES
    atualizar_interface();
    ncurses_log("Cenario 2 concluido!");
#endif
}

void loop_principal(void) {
#ifdef USE_NCURSES
    ncurses_log("Entrando no loop principal...");
    ncurses_log("Pressione 'q' para sair, 'r' para atualizar");
#else
    printf("\n[SISTEMA] Entrando no loop principal...\n");
    printf("Pressione Ctrl+C para encerrar.\n\n");
#endif
    
    time_t inicio = time(NULL);
    int ciclos = 0;
    int ultimo_turno_processado = -1;
#ifdef USE_NCURSES
    int pausado = 0;
#endif
    
    while (sistema_executando && 
           difftime(time(NULL), inicio) < TEMPO_TOTAL_SIMULACAO) {
        
        ciclos++;
        
#ifdef USE_NCURSES
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            ncurses_log("Comando de saida recebido");
            sistema_executando = 0;
            break;
        } else if (ch == 'r' || ch == 'R') {
            atualizar_interface();
            ncurses_log("Interface atualizada");
        } else if (ch == 'p' || ch == 'P') {
            pausado = !pausado;
            if (pausado) {
                ncurses_log("Sistema pausado");
            } else {
                ncurses_log("Sistema continuando");
            }
        }
        
        if (pausado) {
            sleep(1);
            continue;
        }
#endif
        
        if (ciclos % 5 == 0) {
            int turno_atual = (ciclos / 5) % 3;
            
            if (turno_atual != ultimo_turno_processado) {
                if (ultimo_turno_processado != -1) {
#ifdef USE_NCURSES
                    ncurses_log("INTERVALO ENTRE TURNOS");
                    ncurses_log("Processando operacoes de RH...");
#else
                    printf("\n--- INTERVALO ENTRE TURNOS ---\n");
                    printf("Processando operacoes de RH...\n");
#endif
                    
                    int num_operacoes = 1 + (rand() % 4);
                    for (int i = 0; i < num_operacoes; i++) {
                        if (rand() % 3 != 0) {
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
                        usleep(300000);
                    }
                    sleep(INTERVALO_ENTRE_TURNOS);
#ifdef USE_NCURSES
                    atualizar_interface();
#endif
                }
                
                const char* nomes_turnos[] = {"MANHA", "TARDE", "NOITE"};
#ifdef USE_NCURSES
                char msg[100];
                snprintf(msg, sizeof(msg), "INICIANDO TURNO DA %s", nomes_turnos[turno_atual]);
                ncurses_log(msg);
#else
                printf("\n=== INICIANDO TURNO DA %s ===\n", nomes_turnos[turno_atual]);
#endif
                
                Turno turno_enum;
                switch(turno_atual) {
                    case 0: turno_enum = MANHA; break;
                    case 1: turno_enum = TARDE; break;
                    case 2: turno_enum = NOITE; break;
                    default: turno_enum = MANHA; break;
                }
                
                iniciar_turno_vendas(turno_enum);
                total_vendas_realizadas += 5 + (rand() % 10);
                
#ifdef USE_NCURSES
                atualizar_interface();
#endif
                ultimo_turno_processado = turno_atual;
            }
        }
        
        if (ciclos % 3 == 0) {
#ifdef USE_NCURSES
            atualizar_interface();
#else
            printf("\n--- STATUS [Ciclo %d] ---\n", ciclos);
            printf("Estoque: %d/%d\n", estoque_disponivel(), TOTAL_CARTOES);
            printf("RH: %d funcionarios\n", get_funcionarios_ativos());
            printf("Vendas: %d\n", total_vendas_realizadas);
            printf("------------------------\n");
#endif
        }
        
        sleep(1);
    }
    
#ifdef USE_NCURSES
    ncurses_log("Tempo de simulacao concluido");
#else
    printf("\n[SISTEMA] Tempo de simulacao concluido.\n");
#endif
}

void encerrar_sistema(void) {
#ifdef USE_NCURSES
    ncurses_log("Encerrando modulos...");
#else
    printf("\n[SISTEMA] Encerrando modulos...\n");
#endif
    
#ifdef USE_NCURSES
    ncurses_log("  * Parando sistema de agencias...");
#else
    printf("  * Parando sistema de agencias...\n");
#endif
    parar_todas_agencias();
    
#ifdef USE_NCURSES
    ncurses_log("  * Encerrando sistema de RH...");
#else
    printf("  * Encerrando sistema de RH...\n");
#endif
    encerrar_sistema_rh();
    
#ifdef USE_NCURSES
    ncurses_log("  * Liberando fila de prioridade...");
#else
    printf("  * Liberando fila de prioridade...\n");
#endif
    if (fila_global) {
        liberar_fila(fila_global);
    }
    
#ifdef USE_NCURSES
    ncurses_log("  * Liberando estoque...");
#else
    printf("  * Liberando estoque...\n");
#endif
    liberar_estoque();
    
#ifdef USE_NCURSES
    ncurses_log("Sistema encerrado com sucesso!");
    sleep(2);
    endwin();
#else
    printf("\n[SISTEMA] Sistema encerrado com sucesso!\n");
#endif
}

void gerar_relatorio_final(void) {
#ifdef USE_NCURSES
    ncurses_log("═══════════════════════════════════");
    ncurses_log("     RELATORIO FINAL DO SISTEMA    ");
    ncurses_log("═══════════════════════════════════");
    
    char msg[200];
    snprintf(msg, sizeof(msg), "Cartoes vendidos: %d/%d (%.1f%%)", 
             estoque_vendido(), TOTAL_CARTOES, 
             (float)estoque_vendido() / TOTAL_CARTOES * 100);
    ncurses_log(msg);
    
    snprintf(msg, sizeof(msg), "Funcionarios ativos: %d/%d", 
             get_funcionarios_ativos(), LIMITE_CONTRATACOES);
    ncurses_log(msg);
    
    snprintf(msg, sizeof(msg), "Total de vendas: %d", total_vendas_realizadas);
    ncurses_log(msg);
    
    snprintf(msg, sizeof(msg), "Receita total: R$ %.2f", 
             total_vendas_realizadas * 50.0);
    ncurses_log(msg);
    
    ncurses_log("═══════════════════════════════════");
    
    sleep(4);
#else
    printf("\n========================================\n");
    printf("        RELATORIO FINAL DO SISTEMA\n");
    printf("========================================\n\n");
    
    printf("ESTOQUE:\n");
    imprimir_estoque();
    
    printf("\nVENDAS:\n");
    printf("  * Total de vendas: %d\n", total_vendas_realizadas);
    printf("  * Receita total: R$ %.2f\n", total_vendas_realizadas * 50.0);
    exibir_relatorio_agencias();
    
    printf("\nRECURSOS HUMANOS:\n");
    exibir_contratacoes();
    exibir_estatisticas();
    
    printf("\nESTATISTICAS GERAIS:\n");
    printf("  * Cartoes vendidos: %d/%d (%.1f%%)\n", 
           estoque_vendido(), TOTAL_CARTOES,
           (float)estoque_vendido() / TOTAL_CARTOES * 100);
    printf("  * Funcionarios ativos: %d/%d\n", 
           get_funcionarios_ativos(), LIMITE_CONTRATACOES);
    printf("  * Vagas disponiveis: %d\n", 
           get_vagas_disponiveis());
    printf("  * Total de vendas realizadas: %d\n", total_vendas_realizadas);
    printf("  * Receita total: R$ %.2f\n", total_vendas_realizadas * 50.0);
    
    printf("\n========================================\n");
#endif
}

int main(void) {
#ifdef USE_NCURSES
#else
    printf("Iniciando Sistema Operacional UNITEL...\n\n");
#endif
    
    srand(time(NULL));
    
    if (!inicializar_sistema()) {
#ifdef USE_NCURSES
        endwin();
#endif
        printf("Erro na inicializacao do sistema.\n");
        return 1;
    }
    
    popular_dados_iniciais();
    executar_cenarios_teste_com_intervalos();
    loop_principal();
    gerar_relatorio_final();
    encerrar_sistema();
    
#ifndef USE_NCURSES
    printf("\n========================================\n");
    printf("   SISTEMA UNITEL FINALIZADO COM SUCESSO\n");
    printf("========================================\n");
#endif
    
    return 0;
}
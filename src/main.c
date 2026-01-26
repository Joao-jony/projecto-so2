main.c

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdatomic.h>
#include "contratacoes.h"

#define MAX_FUNCIONARIOS 4
#define SIG_CONTRATACAO SIGUSR1

/* Flags seguras */
static atomic_int bloqueado = 0;
static atomic_int pedido_relatorio = 0;
static atomic_int sair = 0;

/* ================= SIGNAL HANDLERS ================= */

/* Limite atingido */
void handler_contratacao(int sig) {
    atomic_store(&bloqueado, 1);
}

/* Timer apenas sinaliza */
void handler_timer(int sig) {
    atomic_store(&pedido_relatorio, 1);
}

/* Ctrl+C */
void handler_sair(int sig) {
    atomic_store(&sair, 1);
}

/* ================= THREAD CONTRATAR ================= */
void *thread_contratar(void *arg) {
    int id = 1;

    while (!atomic_load(&sair)) {
        sleep(2);

        int ativos = get_contratacoes() - get_demissoes();

        if (!atomic_load(&bloqueado) && ativos < MAX_FUNCIONARIOS) {
            adicionar_contratacao(id++);
            printf("[+] Nova contratação\n");

            if (ativos + 1 == MAX_FUNCIONARIOS) {
                raise(SIG_CONTRATACAO);
                printf("[!] Limite atingido. Contratações bloqueadas\n");
            }
        }
    }
    return NULL;
}

/* ================= THREAD DEMITIR ================= */
void *thread_demitir(void *arg) {
    while (!atomic_load(&sair)) {
        sleep(7);

        if (get_contratacoes() - get_demissoes() > 0) {
            remover_contratacao();
            printf("[-] Funcionário demitido\n");
            atomic_store(&bloqueado, 0);
        }
    }
    return NULL;
}

/* ================= THREAD RELATÓRIO ================= */
void *thread_relatorio(void *arg) {
    while (!atomic_load(&sair)) {
        if (atomic_exchange(&pedido_relatorio, 0)) {
            exibir_contratacoes();
        }
        usleep(100000);
    }
    return NULL;
}

/* ================= MAIN ================= */
int main(void) {
    pthread_t t_contrata, t_demite, t_relatorio;
    struct sigaction sa;

    /* SIGUSR1 */
    sa.sa_handler = handler_contratacao;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIG_CONTRATACAO, &sa, NULL);

    /* SIGALRM */
    sa.sa_handler = handler_timer;
    sigaction(SIGALRM, &sa, NULL);

    /* SIGINT */
    sa.sa_handler = handler_sair;
    sigaction(SIGINT, &sa, NULL);

    /* Timer periódico */
    struct itimerval timer = {
        .it_value = {5, 0},
        .it_interval = {5, 0}
    };
    setitimer(ITIMER_REAL, &timer, NULL);

    pthread_create(&t_contrata, NULL, thread_contratar, NULL);
    pthread_create(&t_demite, NULL, thread_demitir, NULL);
    pthread_create(&t_relatorio, NULL, thread_relatorio, NULL);

    printf("Sistema iniciado. Ctrl+C para sair.\n");

    pthread_join(t_contrata, NULL);
    pthread_join(t_demite, NULL);
    pthread_join(t_relatorio, NULL);

    limpar_lista();
    printf("Sistema encerrado corretamente.\n");

    return 0;
}

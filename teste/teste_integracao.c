#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "estoque.h"
#include "Fila_prioridade.h"

void* thread_vendas_manha(void* arg) {
    FilaPrioridade* fila = (FilaPrioridade*)arg;
    printf("\n>>> THREAD: Iniciando vendas da MANHÃ\n");
    processar_vendas_turno(fila, MANHA);
    printf(">>> THREAD: Vendas da MANHÃ concluídas\n");
    return NULL;
}

void* thread_vendas_tarde(void* arg) {
    FilaPrioridade* fila = (FilaPrioridade*)arg;
    printf("\n>>> THREAD: Iniciando vendas da TARDE\n");
    processar_vendas_turno(fila, TARDE);
    printf(">>> THREAD: Vendas da TARDE concluídas\n");
    return NULL;
}

void* thread_vendas_noite(void* arg) {
    FilaPrioridade* fila = (FilaPrioridade*)arg;
    printf("\n>>> THREAD: Iniciando vendas da NOITE\n");
    processar_vendas_turno(fila, NOITE);
    printf(">>> THREAD: Vendas da NOITE concluídas\n");
    return NULL;
}

int main() {
    // Desabilita buffer para ver prints imediatamente
    setbuf(stdout, NULL);
    
    printf("=== SIMULAÇÃO UNITEL - SISTEMA OPERATIVO ===\n");
    
    // 1. Inicializar módulos
    printf("\n1. Inicializando sistema...\n");
    inicializar_estoque();
    printf("   DEBUG: Estoque inicializado\n");
    
    FilaPrioridade* fila = inicializar_fila();
    if (!fila) {
        printf("ERRO: Não foi possível inicializar fila\n");
        return 1;
    }
    printf("   DEBUG: Fila inicializada\n");
    
    // 2. Adicionar clientes à fila (simulação)
    printf("\n2. Simulando chegada de clientes...\n");
    
    // 20 empresas
    for (int i = 1; i <= 20; i++) {
        inserir_cliente(fila, i * 10, EMPRESA);
    }
    printf("   DEBUG: 20 empresas adicionadas\n");
    
    // 50 público
    for (int i = 1; i <= 50; i++) {
        inserir_cliente(fila, i * 20, PUBLICO);
    }
    printf("   DEBUG: 50 clientes público adicionados\n");
    
    // 3. Mostrar estado inicial
    printf("\n3. Estado inicial do sistema:\n");
    imprimir_estoque();
    imprimir_fila(fila);
    
    // 4. TESTE SIMPLES primeiro (sem threads)
    printf("\n4. Testando processamento SIMPLES (sem threads)...\n");
    int resultado = processar_vendas_turno(fila, MANHA);
    printf("   Resultado: %d vendas realizadas\n", resultado);
    
    // 5. Mostrar estado após teste
    printf("\n5. Estado após teste simples:\n");
    imprimir_estoque();
    imprimir_fila(fila);
    
    
    // 6. Teste com múltiplas threads (comentar se o simples não funcionar)
    printf("\n6. Testando com múltiplas threads (agências)...\n");
    
    pthread_t t1, t2, t3;
    
    // Reset estoque para teste
    for (int i = 0; i < TOTAL_CARTOES; i++) {
        liberar_cartao(i);
    }
    
    // Criar threads para diferentes turnos
    pthread_create(&t1, NULL, thread_vendas_manha, fila);
    pthread_create(&t2, NULL, thread_vendas_tarde, fila);
    pthread_create(&t3, NULL, thread_vendas_noite, fila);
    
    // Esperar threads terminarem
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    
    // 7. Resultados finais
    printf("\n=== RESULTADOS FINAIS ===\n");
    imprimir_estoque();
    
    // 8. Liberar recursos
    liberar_fila(fila);
    liberar_estoque();
    
    printf("\n=== SIMULAÇÃO CONCLUÍDA ===\n");
    return 0;
}
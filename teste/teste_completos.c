#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "estoque.h"
#include "Fila_prioridade.h"
#include "vendas.h"
#include "contratacoes.h"

void test_estoque(void) {
    printf("Testando módulo Estoque...\n");
    
    inicializar_estoque();
    
    // Teste 1: Estoque inicial
    assert(estoque_disponivel() == TOTAL_CARTOES);
    assert(estoque_vendido() == 0);
    
    // Teste 2: Reservar cartão
    int cartao = reservar_proximo_cartao();
    assert(cartao >= 0 && cartao < TOTAL_CARTOES);
    assert(estoque_disponivel() == TOTAL_CARTOES - 1);
    
    // Teste 3: Liberar cartão
    assert(liberar_cartao(cartao) == 1);
    assert(estoque_disponivel() == TOTAL_CARTOES);
    
    printf("Estoque: OK\n");
}

void test_fila_prioridade(void) {
    printf("Testando módulo Fila de Prioridade...\n");
    
    FilaPrioridade* fila = inicializar_fila();
    assert(fila != NULL);
    
    // Teste 1: Inserção
    inserir_cliente(fila, 1001, EMPRESA);
    inserir_cliente(fila, 1002, PUBLICO);
    assert(fila->tamanho == 2);
    
    // Teste 2: Prioridade (empresa deve vir primeiro)
    Cliente* cliente = obter_proximo_cliente(fila);
    assert(cliente != NULL);
    assert(cliente->tipo == EMPRESA);
    
    // Teste 3: Remoção
    remover_cliente_processado(fila, 1001);
    assert(fila->tamanho == 1);
    
    liberar_fila(fila);
    printf("Fila Prioridade: OK\n");
}

void test_vendas(void) {
    printf("🧪 Testando módulo Vendas...\n");
    
    FilaPrioridade* fila = inicializar_fila();
    inicializar_sistema_agencias(fila);
    
    // Adicionar clientes
    for (int i = 0; i < 10; i++) {
        inserir_cliente(fila, 2000 + i, i % 3 == 0 ? EMPRESA : PUBLICO);
    }
    
    // Testar turno
    int vendas = processar_vendas_turno(fila, MANHA);
    assert(vendas >= 0 && vendas <= 35);
    
    parar_todas_agencias();
    liberar_fila(fila);
    printf("Vendas: OK\n");
}

void test_contratacoes(void) {
    printf("Testando módulo Contratações...\n");
    
    inicializar_sistema_rh();
    
    // Teste 1: Contratação
    adicionar_contratacao(5001, 75000);
    assert(get_total_contratacoes() == 1);
    assert(get_funcionarios_ativos() == 1);
    
    // Teste 2: Demissão
    remover_ultima_contratacao();
    assert(get_total_demissoes() == 1);
    assert(get_funcionarios_ativos() == 0);
    
    // Teste 3: Exibição
    exibir_contratacoes_periodicas();
    
    encerrar_sistema_rh();
    printf("Contratações: OK\n");
}

void test_integracao(void) {
    printf("Testando integração completa...\n");
    
    // Inicializar tudo
    inicializar_estoque();
    FilaPrioridade* fila = inicializar_fila();
    inicializar_sistema_agencias(fila);
    inicializar_sistema_rh();
    
    // Cenário integrado
    printf("  • Adicionando dados...\n");
    for (int i = 0; i < 5; i++) {
        inserir_cliente(fila, 3000 + i, EMPRESA);
        adicionar_contratacao(6000 + i, 50000 + i * 10000);
    }
    
    printf("  • Processando vendas...\n");
    processar_vendas_turno(fila, MANHA);
    
    printf("  • Verificando consistência...\n");
    assert(estoque_vendido() <= TOTAL_CARTOES);
    assert(get_funcionarios_ativos() >= 0);
    
    // Limpar
    parar_todas_agencias();
    encerrar_sistema_rh();
    liberar_fila(fila);
    liberar_estoque();
    
    printf("  Integração: OK\n");
}

int main(void) {
    printf("========================================\n");
    printf("   TESTES COMPLETOS - SISTEMA UNITEL\n");
    printf("========================================\n\n");
    
    test_estoque();
    test_fila_prioridade();
    test_vendas();
    test_contratacoes();
    test_integracao();
    
    printf("\n========================================\n");
    printf("   TODOS OS TESTES PASSARAM!\n");
    printf("========================================\n");
    
    return 0;
}
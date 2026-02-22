#include "vendas.h"
#include "estoque.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Variáveis globais do módulo - AGORA NÃO SÃO STATIC para serem acessíveis
Agencia agencias[NUM_AGENCIAS];
EstatisticasVendas estatisticas_vendas;  // Renomeado de 'estatisticas' para 'estatisticas_vendas'
pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

static FilaPrioridade* fila_global = NULL;
static int sistema_ativa = 0;

// Nomes das agências
static const char* nomes_agencias[NUM_AGENCIAS] = {
    "Agência Centro", "Agência Sul"
};

/* Declaração antecipada */
static void processar_venda_agencia(Agencia* agencia);

/* ========== FUNÇÕES INTERNAS ========== */

/* Processar uma venda em uma agência */
static void processar_venda_agencia(Agencia* agencia) {
    pthread_mutex_lock(&agencia->lock);
    
    // 1. Verificar se há estoque
    if (estoque_disponivel() <= 0) {
        printf("[AGÊNCIA %d] Sem estoque disponível\n", agencia->id);
        pthread_mutex_unlock(&agencia->lock);
        return;
    }
    
    // 2. Verificar se há clientes na fila
    Cliente* proximo = obter_proximo_cliente(fila_global);
    if (!proximo) {
        printf("[AGÊNCIA %d] Nenhum cliente na fila\n", agencia->id);
        pthread_mutex_unlock(&agencia->lock);
        return;
    }
    
    // 3. Reservar cartão
    int cartao_id = reservar_proximo_cartao();
    if (cartao_id == -1) {
        printf("[AGÊNCIA %d] Falha ao reservar cartão\n", agencia->id);
        pthread_mutex_unlock(&agencia->lock);
        return;
    }
    
    // 4. Registrar venda
    char* tipo_str = (proximo->tipo == EMPRESA) ? "EMPRESA" : "PUBLICO";
    
    printf("[AGÊNCIA %d] Venda realizada: Cartão %03d para %s (Cliente %d)\n",
           agencia->id, cartao_id, tipo_str, proximo->id_cliente);
    
    // 5. Atualizar estatísticas da agência
    agencia->vendas_realizadas++;
    agencia->clientes_atendidos++;
    
    // 6. Atualizar estatísticas globais
    pthread_mutex_lock(&stats_lock);
    estatisticas_vendas.total_vendas++;
    if (proximo->tipo == EMPRESA) {
        estatisticas_vendas.vendas_empresas++;
    } else {
        estatisticas_vendas.vendas_publico++;
    }
    pthread_mutex_unlock(&stats_lock);
    
    // 7. Remover cliente da fila
    remover_cliente_processado(fila_global, proximo->id_cliente);
    
    pthread_mutex_unlock(&agencia->lock);
}

/* Thread de uma agência */
static void* thread_agencia(void* arg) {
    Agencia* agencia = (Agencia*)arg;
    
    printf("[AGÊNCIA %d] %s iniciou operações\n", 
           agencia->id, agencia->nome);
    
    while (agencia->ativa && sistema_ativa) {
        // Processar uma venda
        processar_venda_agencia(agencia);
        
        // Esperar tempo simulado
        sleep(TEMPO_VENDA_REAL);
    }
    
    printf("[AGÊNCIA %d] %s encerrou operações\n", 
           agencia->id, agencia->nome);
    
    return NULL;
}

/* ========== FUNÇÕES PÚBLICAS ========== */

/* Inicializar sistema de vendas */
void inicializar_sistema_vendas(FilaPrioridade* fila) {
    fila_global = fila;
    
    // Inicializar estatísticas
    memset(&estatisticas_vendas, 0, sizeof(EstatisticasVendas));
    
    // Inicializar agências
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        agencias[i].id = i + 1;
        snprintf(agencias[i].nome, sizeof(agencias[i].nome), "%s", nomes_agencias[i]);
        agencias[i].vendas_realizadas = 0;
        agencias[i].clientes_atendidos = 0;
        agencias[i].ativa = 0;
        agencias[i].thread = 0;
        pthread_mutex_init(&agencias[i].lock, NULL);
    }
    
    sistema_ativa = 1;
    printf("[VENDAS] Sistema inicializado com %d agências\n", NUM_AGENCIAS);
}

/* Iniciar turno de vendas */
void iniciar_turno_vendas(Turno turno) {
    if (!fila_global || !sistema_ativa) {
        printf("[VENDAS] Sistema não inicializado\n");
        return;
    }
    
    printf("\n=== INICIANDO VENDAS ===\n");
    printf("Turno: %s\n", 
           turno == MANHA ? "MANHÃ" : turno == TARDE ? "TARDE" : "NOITE");
    
    // Verificar estoque antes de iniciar
    int estoque_inicial = estoque_disponivel();
    printf("Estoque inicial: %d cartões\n", estoque_inicial);
    
    // Usar função do módulo FilaPrioridade
    int vendas = processar_vendas_turno(fila_global, turno);
    
    // Atualizar estatísticas do turno
    pthread_mutex_lock(&stats_lock);
    estatisticas_vendas.vendas_por_turno[turno] += vendas;
    pthread_mutex_unlock(&stats_lock);
    
    printf("Vendas realizadas no turno: %d\n", vendas);
    printf("Estoque final: %d cartões\n", estoque_disponivel());
    printf("=== FIM DO TURNO ===\n");
}

/* Iniciar vendas concorrentes (todas agências) */
void iniciar_vendas_concorrentes(Turno turno) {
    (void)turno; // Parâmetro não usado
    
    if (!sistema_ativa) {
        printf("[VENDAS] Sistema não está ativo\n");
        return;
    }
    
    printf("\n=== VENDAS CONCORRENTES ===\n");
    printf("Iniciando %d agências simultaneamente...\n", NUM_AGENCIAS);
    
    // Ativar todas as agências
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        agencias[i].ativa = 1;
        int result = pthread_create(&agencias[i].thread, NULL, thread_agencia, &agencias[i]);
        if (result != 0) {
            printf("[ERRO] Falha ao criar thread para agência %d\n", agencias[i].id);
            agencias[i].ativa = 0;
        }
    }
    
    printf("Agências iniciadas. Operando por 10 segundos...\n");
    sleep(10);
    
    // Parar agências
    parar_todas_agencias();
    printf("Vendas concorrentes concluídas.\n");
}

/* Parar todas as agências */
void parar_todas_agencias() {
    printf("[VENDAS] Parando todas as agências...\n");
    
    // Sinalizar para parar
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        agencias[i].ativa = 0;
    }
    
    // Esperar threads terminarem
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        if (agencias[i].thread != 0) {
            pthread_join(agencias[i].thread, NULL);
            agencias[i].thread = 0;
        }
    }
    
    printf("[VENDAS] Todas as agências paradas\n");
}

/* Exibir relatório de vendas */
void exibir_relatorio_vendas() {
    pthread_mutex_lock(&stats_lock);
    
    printf("\n════════════════════════════════════════════\n");
    printf("        RELATÓRIO DE VENDAS\n");
    printf("════════════════════════════════════════════\n");
    
    // Estatísticas gerais
    printf("\n📊 ESTATÍSTICAS GERAIS:\n");
    printf("   • Total de vendas:      %d\n", estatisticas_vendas.total_vendas);
    printf("   • Vendas para empresas: %d (%.1f%%)\n", 
           estatisticas_vendas.vendas_empresas,
           estatisticas_vendas.total_vendas > 0 ? 
           (estatisticas_vendas.vendas_empresas * 100.0) / estatisticas_vendas.total_vendas : 0);
    printf("   • Vendas para público:  %d (%.1f%%)\n", 
           estatisticas_vendas.vendas_publico,
           estatisticas_vendas.total_vendas > 0 ? 
           (estatisticas_vendas.vendas_publico * 100.0) / estatisticas_vendas.total_vendas : 0);
    
    // Vendas por turno
    printf("\n🕒 VENDAS POR TURNO:\n");
    printf("   • Manhã:  %d vendas\n", estatisticas_vendas.vendas_por_turno[MANHA]);
    printf("   • Tarde:  %d vendas\n", estatisticas_vendas.vendas_por_turno[TARDE]);
    printf("   • Noite:  %d vendas\n", estatisticas_vendas.vendas_por_turno[NOITE]);
    
    // Média por turno
    int total_turnos = 0;
    for (int i = 0; i < 3; i++) total_turnos += estatisticas_vendas.vendas_por_turno[i];
    if (total_turnos > 0) {
        printf("\n📈 MÉDIA POR TURNO:\n");
        printf("   • %.1f vendas/turno\n", total_turnos / 3.0);
    }
    
    // Estoque atual
    printf("\n📦 ESTOQUE ATUAL:\n");
    printf("   • Disponíveis: %d/%d (%.1f%%)\n", 
           estoque_disponivel(), TOTAL_CARTOES,
           (float)estoque_disponivel() / TOTAL_CARTOES * 100);
    printf("   • Vendidos:    %d/%d (%.1f%%)\n", 
           estoque_vendido(), TOTAL_CARTOES,
           (float)estoque_vendido() / TOTAL_CARTOES * 100);
    
    printf("\n════════════════════════════════════════════\n");
    
    pthread_mutex_unlock(&stats_lock);
}

/* Exibir relatório por agência */
void exibir_relatorio_agencias() {
    printf("\n════════════════════════════════════════════\n");
    printf("        RELATÓRIO POR AGÊNCIA\n");
    printf("════════════════════════════════════════════\n");
    
    int total_vendas_agencias = 0;
    int total_clientes = 0;
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        pthread_mutex_lock(&agencias[i].lock);
        
        printf("\n🏢 %s (ID: %d)\n", agencias[i].nome, agencias[i].id);
        printf("   • Vendas realizadas:    %d\n", agencias[i].vendas_realizadas);
        printf("   • Clientes atendidos:   %d\n", agencias[i].clientes_atendidos);
        printf("   • Status:               %s\n", 
               agencias[i].ativa ? "ATIVA" : "INATIVA");
        printf("   • Eficiência:           %.1f vendas/cliente\n",
               agencias[i].clientes_atendidos > 0 ? 
               (float)agencias[i].vendas_realizadas / agencias[i].clientes_atendidos : 0);
        
        total_vendas_agencias += agencias[i].vendas_realizadas;
        total_clientes += agencias[i].clientes_atendidos;
        
        pthread_mutex_unlock(&agencias[i].lock);
    }
    
    printf("\n📊 TOTAL DAS AGÊNCIAS:\n");
    printf("   • Vendas: %d\n", total_vendas_agencias);
    printf("   • Clientes: %d\n", total_clientes);
    printf("   • Média: %.1f vendas/agência\n", 
           (float)total_vendas_agencias / NUM_AGENCIAS);
    
    printf("\n════════════════════════════════════════════\n");
}

/* Exportar vendas para CSV */
void exportar_vendas_csv(const char* filename) {
    if (!filename) {
        printf("[ERRO] Nome de arquivo inválido\n");
        return;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("[ERRO] Erro ao abrir arquivo CSV");
        return;
    }
    
    // Cabeçalho
    fprintf(file, "agencia_id,agencia_nome,vendas_realizadas,clientes_atendidos,status\n");
    
    // Dados das agências
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        pthread_mutex_lock(&agencias[i].lock);
        
        fprintf(file, "%d,\"%s\",%d,%d,%s\n",
                agencias[i].id,
                agencias[i].nome,
                agencias[i].vendas_realizadas,
                agencias[i].clientes_atendidos,
                agencias[i].ativa ? "ATIVA" : "INATIVA");
        
        pthread_mutex_unlock(&agencias[i].lock);
    }
    
    // Estatísticas gerais
    pthread_mutex_lock(&stats_lock);
    fprintf(file, "\n# Estatísticas Gerais\n");
    fprintf(file, "# Total Vendas,%d\n", estatisticas_vendas.total_vendas);
    fprintf(file, "# Vendas Empresas,%d\n", estatisticas_vendas.vendas_empresas);
    fprintf(file, "# Vendas Público,%d\n", estatisticas_vendas.vendas_publico);
    fprintf(file, "# Vendas Manhã,%d\n", estatisticas_vendas.vendas_por_turno[MANHA]);
    fprintf(file, "# Vendas Tarde,%d\n", estatisticas_vendas.vendas_por_turno[TARDE]);
    fprintf(file, "# Vendas Noite,%d\n", estatisticas_vendas.vendas_por_turno[NOITE]);
    pthread_mutex_unlock(&stats_lock);
    
    fclose(file);
    printf("[VENDAS] Dados exportados para '%s'\n", filename);
}

/* ========== GETTERS PARA OUTROS MÓDULOS ========== */

int get_vendas_totais() {
    pthread_mutex_lock(&stats_lock);
    int total = estatisticas_vendas.total_vendas;
    pthread_mutex_unlock(&stats_lock);
    return total;
}

int get_vendas_empresas() {
    pthread_mutex_lock(&stats_lock);
    int empresas = estatisticas_vendas.vendas_empresas;
    pthread_mutex_unlock(&stats_lock);
    return empresas;
}

int get_vendas_publico() {
    pthread_mutex_lock(&stats_lock);
    int publico = estatisticas_vendas.vendas_publico;
    pthread_mutex_unlock(&stats_lock);
    return publico;
}

/* ========== FUNÇÕES DE RESET ========== */

/* Reinicializar vendas (para testes) */
void reinicializar_vendas() {
    printf("[VENDAS] Reinicializando sistema...\n");
    
    // Parar todas as agências
    parar_todas_agencias();
    
    // Resetar estatísticas
    pthread_mutex_lock(&stats_lock);
    memset(&estatisticas_vendas, 0, sizeof(EstatisticasVendas));
    pthread_mutex_unlock(&stats_lock);
    
    // Resetar agências
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        pthread_mutex_lock(&agencias[i].lock);
        agencias[i].vendas_realizadas = 0;
        agencias[i].clientes_atendidos = 0;
        agencias[i].ativa = 0;
        agencias[i].thread = 0;
        pthread_mutex_unlock(&agencias[i].lock);
    }
    
    printf("[VENDAS] Sistema reinicializado com sucesso\n");
}

/* Status do sistema */
int vendas_sistema_ativo(void) {
    return sistema_ativa;
}

/* Obter fila global (para outros módulos) */
FilaPrioridade* get_fila_global(void) {
    return fila_global;
}
#include "vendas.h"
#include "estoque.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Variáveis globais do módulo
static Agencia agencias[NUM_AGENCIAS];
static EstatisticasVendas estatisticas;
static FilaPrioridade* fila_global = NULL;
static pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;
static int sistema_ativa = 0;

// Nomes das agências
static const char* nomes_agencias[NUM_AGENCIAS] = {
    "Agência Centro", "Agência Sul"
};

static void processar_venda_agencia(Agencia* agencia);

/* Processar uma venda em uma agência */
static void processar_venda_agencia(Agencia* agencia) {
    pthread_mutex_lock(&agencia->lock);
    
    if (estoque_disponivel() <= 0) {
        pthread_mutex_unlock(&agencia->lock);
        return;
    }
    
    Cliente* proximo = obter_proximo_cliente(fila_global);
    if (!proximo) {
        pthread_mutex_unlock(&agencia->lock);
        return;
    }
    
    int cartao_id = reservar_proximo_cartao();
    if (cartao_id == -1) {
        pthread_mutex_unlock(&agencia->lock);
        return;
    }
    
    char* tipo_str = (proximo->tipo == EMPRESA) ? "EMPRESA" : "PUBLICO";
    
    printf("[AGÊNCIA %d] Venda realizada: Cartão %03d para %s (Cliente %d)\n",
           agencia->id, cartao_id, tipo_str, proximo->id_cliente);
    
    agencia->vendas_realizadas++;
    agencia->clientes_atendidos++;
    
    pthread_mutex_lock(&stats_lock);
    estatisticas.total_vendas++;
    if (proximo->tipo == EMPRESA) {
        estatisticas.vendas_empresas++;
    } else {
        estatisticas.vendas_publico++;
    }
    pthread_mutex_unlock(&stats_lock);
    
    remover_cliente_processado(fila_global, proximo->id_cliente);
    
    pthread_mutex_unlock(&agencia->lock);
}

/* Thread de uma agência */
static void* thread_agencia(void* arg) {
    Agencia* agencia = (Agencia*)arg;
    
    printf("[AGÊNCIA %d] %s iniciou operações\n", 
           agencia->id, agencia->nome);
    
    while (agencia->ativa && sistema_ativa) {
        processar_venda_agencia(agencia);
        sleep(TEMPO_VENDA_REAL);
    }
    
    printf("[AGÊNCIA %d] %s encerrou operações\n", 
           agencia->id, agencia->nome);
    
    return NULL;
}

/* Inicializar sistema de vendas */
void inicializar_sistema_vendas(FilaPrioridade* fila) {
    fila_global = fila;
    
    memset(&estatisticas, 0, sizeof(EstatisticasVendas));
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        agencias[i].id = i + 1;
        snprintf(agencias[i].nome, 50, "%s", nomes_agencias[i]);
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
    
    int vendas = processar_vendas_turno(fila_global, turno);
    
    pthread_mutex_lock(&stats_lock);
    estatisticas.vendas_por_turno[turno] += vendas;
    pthread_mutex_unlock(&stats_lock);
    
    printf("=== FIM DO TURNO ===\n");
}

/* Iniciar vendas concorrentes */
void iniciar_vendas_concorrentes(Turno turno) {
    (void)turno;
    
    if (!sistema_ativa) return;
    
    printf("\n=== VENDAS CONCORRENTES ===\n");
    printf("Iniciando %d agências simultaneamente...\n", NUM_AGENCIAS);
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        agencias[i].ativa = 1;
        pthread_create(&agencias[i].thread, NULL, thread_agencia, &agencias[i]);
    }
    
    printf("Agências iniciadas. Operando por 10 segundos...\n");
    sleep(10);
    
    parar_todas_agencias();
    printf("Vendas concorrentes concluídas.\n");
}

/* Parar todas as agências */
void parar_todas_agencias(void) {
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        agencias[i].ativa = 0;
    }
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        if (agencias[i].thread != 0) {
            pthread_join(agencias[i].thread, NULL);
            agencias[i].thread = 0;
        }
    }
    
    printf("[VENDAS] Todas as agências paradas\n");
}

/* Exibir relatório de vendas */
void exibir_relatorio_vendas(void) {
    pthread_mutex_lock(&stats_lock);
    
    printf("\n=== RELATÓRIO DE VENDAS ===\n");
    
    printf("\n📊 ESTATÍSTICAS GERAIS:\n");
    printf("   Total de vendas:      %d\n", estatisticas.total_vendas);
    printf("   Vendas para empresas: %d (%.1f%%)\n", 
           estatisticas.vendas_empresas,
           estatisticas.total_vendas > 0 ? 
           (estatisticas.vendas_empresas * 100.0) / estatisticas.total_vendas : 0);
    printf("   Vendas para público:  %d (%.1f%%)\n", 
           estatisticas.vendas_publico,
           estatisticas.total_vendas > 0 ? 
           (estatisticas.vendas_publico * 100.0) / estatisticas.total_vendas : 0);
    
    printf("\n🕒 VENDAS POR TURNO:\n");
    printf("   Manhã:  %d vendas\n", estatisticas.vendas_por_turno[MANHA]);
    printf("   Tarde:  %d vendas\n", estatisticas.vendas_por_turno[TARDE]);
    printf("   Noite:  %d vendas\n", estatisticas.vendas_por_turno[NOITE]);
    
    printf("\n📦 ESTOQUE ATUAL:\n");
    printf("   Disponíveis: %d/%d\n", 
           estoque_disponivel(), TOTAL_CARTOES);
    printf("   Vendidos:    %d/%d\n", 
           estoque_vendido(), TOTAL_CARTOES);
    
    pthread_mutex_unlock(&stats_lock);
}

/* Exibir relatório por agência */
void exibir_relatorio_agencias(void) {
    printf("\n=== RELATÓRIO POR AGÊNCIA ===\n");
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        pthread_mutex_lock(&agencias[i].lock);
        
        printf("\n🏢 %s (ID: %d)\n", agencias[i].nome, agencias[i].id);
        printf("   Vendas realizadas:    %d\n", agencias[i].vendas_realizadas);
        printf("   Clientes atendidos:   %d\n", agencias[i].clientes_atendidos);
        printf("   Status:               %s\n", 
               agencias[i].ativa ? "ATIVA" : "INATIVA");
        
        pthread_mutex_unlock(&agencias[i].lock);
    }
    
    printf("\n============================\n");
}

/* Exportar vendas para CSV */
void exportar_vendas_csv(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao abrir arquivo CSV");
        return;
    }
    
    fprintf(file, "agencia_id,agencia_nome,vendas_realizadas,clientes_atendidos,status\n");
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        pthread_mutex_lock(&agencias[i].lock);
        
        fprintf(file, "%d,%s,%d,%d,%s\n",
                agencias[i].id,
                agencias[i].nome,
                agencias[i].vendas_realizadas,
                agencias[i].clientes_atendidos,
                agencias[i].ativa ? "ATIVA" : "INATIVA");
        
        pthread_mutex_unlock(&agencias[i].lock);
    }
    
    fclose(file);
    printf("[VENDAS] Dados exportados para %s\n", filename);
}

/* Getters para testes */
int get_vendas_totais(void) {
    pthread_mutex_lock(&stats_lock);
    int total = estatisticas.total_vendas;
    pthread_mutex_unlock(&stats_lock);
    return total;
}

int get_vendas_empresas(void) {
    pthread_mutex_lock(&stats_lock);
    int empresas = estatisticas.vendas_empresas;
    pthread_mutex_unlock(&stats_lock);
    return empresas;
}

int get_vendas_publico(void) {
    pthread_mutex_lock(&stats_lock);
    int publico = estatisticas.vendas_publico;
    pthread_mutex_unlock(&stats_lock);
    return publico;
}

/* Reinicializar vendas */
void reinicializar_vendas(void) {
    parar_todas_agencias();
    
    pthread_mutex_lock(&stats_lock);
    memset(&estatisticas, 0, sizeof(EstatisticasVendas));
    pthread_mutex_unlock(&stats_lock);
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        pthread_mutex_lock(&agencias[i].lock);
        agencias[i].vendas_realizadas = 0;
        agencias[i].clientes_atendidos = 0;
        pthread_mutex_unlock(&agencias[i].lock);
    }
    
    printf("[VENDAS] Sistema reinicializado\n");
}
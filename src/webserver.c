#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "estoque.h"
#include "Fila_prioridade.h"
#include "vendas.h"
#include "contratacoes.h"
#include "webserver.h"

#define PORT_START 8080
#define PORT_END 8090
#define POST_BUFFER_SIZE 512
#define API_VERSION "1.0.0"

extern FilaPrioridade* fila_global;
extern Agencia agencias[NUM_AGENCIAS];
extern EstatisticasVendas estatisticas_vendas;
extern pthread_mutex_t stats_lock;

static struct MHD_Daemon* webserver_daemon = NULL;
static int webserver_port = 0;

// Função para testar integridade da imagem
void test_image_file(const char* filename) {
    printf("\n=== TESTANDO IMAGEM ===\n");
    printf("Arquivo: %s\n", filename);
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("❌ Arquivo não encontrado!\n");
        return;
    }
    
    // Verifica os primeiros bytes (assinatura PNG)
    unsigned char header[8];
    size_t read = fread(header, 1, 8, file);
    
    printf("Primeiros 8 bytes: ");
    for (int i = 0; i < read; i++) {
        printf("%02X ", header[i]);
    }
    printf("\n");
    
    // Assinatura PNG: 89 50 4E 47 0D 0A 1A 0A
    if (read == 8 && 
        header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G' &&
        header[4] == 0x0D && header[5] == 0x0A && header[6] == 0x1A && header[7] == 0x0A) {
        printf("✅ Assinatura PNG válida!\n");
    } else {
        printf("❌ Assinatura PNG inválida! O arquivo pode estar corrompido.\n");
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    printf("Tamanho total: %ld bytes\n", size);
    
    fclose(file);
}

char* read_file(const char* filename) {
    printf("📖 Tentando ler arquivo: %s\n", filename);
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("❌ Falha ao abrir arquivo: %s\n", filename);
        printf("   Erro: %s\n", strerror(errno));
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("   Tamanho do arquivo: %ld bytes\n", size);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        printf("❌ Falha ao alocar memória para: %s\n", filename);
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';
    fclose(file);
    
    printf("✅ Arquivo lido com sucesso: %s (%zu bytes)\n", filename, read);
    
    return buffer;
}

// Função específica para servir imagens binariamente
static enum MHD_Result handle_image_file(struct MHD_Connection* connection,
                                         const char* url,
                                         const char* content_type) {
    char filename[256];
    snprintf(filename, sizeof(filename), "web%s", url);
    
    printf("\n🖼️  SERVIDO IMAGEM\n");
    printf("   URL: %s\n", url);
    printf("   Arquivo: %s\n", filename);
    printf("   Content-Type: %s\n", content_type);
    
    // Abre arquivo em modo binário
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("❌ ERRO: Não foi possível abrir %s\n", filename);
        printf("   Erro: %s\n", strerror(errno));
        
        const char* not_found = "<html><body><h1>404 Imagem não encontrada</h1></body></html>";
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(not_found), (void*)not_found, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/html");
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    // Obtém tamanho do arquivo
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("   Tamanho da imagem: %ld bytes\n", size);
    
    // Aloca buffer para a imagem
    char* buffer = (char*)malloc(size);
    if (!buffer) {
        printf("❌ ERRO: Falha ao alocar memória\n");
        fclose(file);
        const char* error = "<html><body><h1>500 Erro interno</h1></body></html>";
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(error), (void*)error, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/html");
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    // Lê a imagem
    size_t read = fread(buffer, 1, size, file);
    fclose(file);
    
    printf("   Bytes lidos: %zu\n", read);
    
    if (read != size) {
        printf("❌ ERRO: Leitura incompleta\n");
        free(buffer);
        const char* error = "<html><body><h1>500 Erro na leitura</h1></body></html>";
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(error), (void*)error, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/html");
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    // Cria resposta com o buffer da imagem
    struct MHD_Response* response = MHD_create_response_from_buffer(
        size, (void*)buffer, MHD_RESPMEM_MUST_FREE);
    
    // Headers importantes para imagem
    MHD_add_response_header(response, "Content-Type", content_type);
    
    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%ld", size);
    MHD_add_response_header(response, "Content-Length", content_length);
    
    MHD_add_response_header(response, "Cache-Control", "no-cache");
    MHD_add_response_header(response, "Accept-Ranges", "bytes");
    
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    
    printf("✅ Imagem servida com sucesso: %s\n", filename);
    
    return ret;
}

static enum MHD_Result handle_static_file(struct MHD_Connection* connection,
                                          const char* url,
                                          const char* content_type) {
    char filename[256];
    snprintf(filename, sizeof(filename), "web%s", url);
    
    printf("\n📄 SERVIDO ARQUIVO\n");
    printf("   URL: %s\n", url);
    printf("   Content-Type: %s\n", content_type);
    printf("   Tentando abrir: %s\n", filename);
    
    char* content = read_file(filename);
    if (!content) {
        const char* not_found = "<html><body><h1>404 Not Found</h1></body></html>";
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(not_found), (void*)not_found, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/html");
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(content), (void*)content, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", content_type);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    
    return ret;
}

char* generate_estoque_json(void) {
    int disponiveis = estoque_disponivel();
    int vendidos = estoque_vendido();
    
    pthread_mutex_lock(&estoque_lock);
    
    char* json = (char*)malloc(4096);
    if (!json) {
        pthread_mutex_unlock(&estoque_lock);
        return NULL;
    }
    
    int offset = snprintf(json, 4096,
        "{"
        "\"total\": %d,"
        "\"disponiveis\": %d,"
        "\"vendidos\": %d,"
        "\"percentual\": %.1f,"
        "\"cartoes\": [",
        TOTAL_CARTOES, disponiveis, vendidos,
        vendidos > 0 ? (float)vendidos / TOTAL_CARTOES * 100 : 0);
    
    int count = 0;
    for (int i = 0; i < TOTAL_CARTOES; i++) {
        if (estoque[i].vendido == 1) {
            if (count > 0) offset += snprintf(json + offset, 4096 - offset, ",");
            
            char time_str[64] = {0};
            if (estoque[i].hora_venda > 0) {
                struct tm* tm_info = localtime(&estoque[i].hora_venda);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            }
            
            offset += snprintf(json + offset, 4096 - offset,
                "{\"id\":%d,\"hora_venda\":\"%s\"}", i, time_str);
            count++;
        }
    }
    
    offset += snprintf(json + offset, 4096 - offset, "]}");
    
    pthread_mutex_unlock(&estoque_lock);
    return json;
}

char* generate_fila_json(void* fila_ptr) {
    FilaPrioridade* fila = (FilaPrioridade*)fila_ptr;
    if (!fila) {
        return strdup("{\"tamanho\":0,\"max\":200,\"clientes\":[]}");
    }
    
    pthread_mutex_lock(&fila->lock);
    
    char* json = (char*)malloc(8192);
    if (!json) {
        pthread_mutex_unlock(&fila->lock);
        return NULL;
    }
    
    int offset = snprintf(json, 8192,
        "{"
        "\"tamanho\": %d,"
        "\"max\": %d,"
        "\"clientes\": [",
        fila->tamanho, get_max_fila());
    
    Node* atual = fila->frente;
    int pos = 1;
    
    while (atual && offset < 8000) {
        if (pos > 1) offset += snprintf(json + offset, 8192 - offset, ",");
        
        time_t agora = time(NULL);
        double espera = difftime(agora, atual->cliente.timestamp);
        int prioridade = calcular_prioridade_cliente(&atual->cliente);
        
        offset += snprintf(json + offset, 8192 - offset,
            "{"
            "\"posicao\": %d,"
            "\"id\": %d,"
            "\"tipo\": \"%s\","
            "\"prioridade\": %d,"
            "\"espera_segundos\": %.0f,"
            "\"espera_minutos\": %.1f"
            "}",
            pos,
            atual->cliente.id_cliente,
            atual->cliente.tipo == EMPRESA ? "EMPRESA" : "PUBLICO",
            prioridade,
            espera,
            espera / 60.0);
        
        atual = atual->next;
        pos++;
    }
    
    offset += snprintf(json + offset, 8192 - offset, "]}");
    
    pthread_mutex_unlock(&fila->lock);
    return json;
}

char* generate_rh_json(void) {
    int ativos = get_funcionarios_ativos();
    int vagas = get_vagas_disponiveis();
    int total_contratacoes = get_total_contratacoes();
    int total_demissoes = get_total_demissoes();
    
    char* json = (char*)malloc(4096);
    if (!json) return NULL;
    
    snprintf(json, 4096,
        "{"
        "\"limite\": %d,"
        "\"ativos\": %d,"
        "\"vagas\": %d,"
        "\"total_contratacoes\": %d,"
        "\"total_demissoes\": %d,"
        "\"percentual\": %.1f"
        "}",
        LIMITE_CONTRATACOES, ativos, vagas,
        total_contratacoes, total_demissoes,
        LIMITE_CONTRATACOES > 0 ? (float)ativos / LIMITE_CONTRATACOES * 100 : 0);
    
    return json;
}

char* generate_vendas_json(void) {
    int total = get_vendas_totais();
    int empresas = get_vendas_empresas();
    int publico = get_vendas_publico();
    
    int manha = 0, tarde = 0, noite = 0;
    
    pthread_mutex_lock(&stats_lock);
    manha = estatisticas_vendas.vendas_por_turno[MANHA];
    tarde = estatisticas_vendas.vendas_por_turno[TARDE];
    noite = estatisticas_vendas.vendas_por_turno[NOITE];
    pthread_mutex_unlock(&stats_lock);
    
    char* json = (char*)malloc(4096);
    if (!json) return NULL;
    
    snprintf(json, 4096,
        "{"
        "\"total\": %d,"
        "\"empresas\": %d,"
        "\"publico\": %d,"
        "\"turnos\": {"
        "\"manha\": %d,"
        "\"tarde\": %d,"
        "\"noite\": %d"
        "}"
        "}",
        total, empresas, publico, manha, tarde, noite);
    
    return json;
}

char* generate_agencias_json(void) {
    char* json = (char*)malloc(4096);
    if (!json) return NULL;
    
    int offset = snprintf(json, 4096,
        "{"
        "\"total_agencias\": %d,"
        "\"agencias\": [",
        NUM_AGENCIAS);
    
    for (int i = 0; i < NUM_AGENCIAS; i++) {
        if (i > 0) offset += snprintf(json + offset, 4096 - offset, ",");
        
        pthread_mutex_lock(&agencias[i].lock);
        offset += snprintf(json + offset, 4096 - offset,
            "{"
            "\"id\": %d,"
            "\"nome\": \"%s\","
            "\"vendas\": %d,"
            "\"clientes_atendidos\": %d,"
            "\"status\": \"%s\""
            "}",
            agencias[i].id,
            agencias[i].nome,
            agencias[i].vendas_realizadas,
            agencias[i].clientes_atendidos,
            agencias[i].ativa ? "ATIVA" : "INATIVA");
        pthread_mutex_unlock(&agencias[i].lock);
    }
    
    offset += snprintf(json + offset, 4096 - offset, "]}");
    
    return json;
}

char* generate_dashboard_json(void) {
    char* estoque_json = generate_estoque_json();
    char* fila_json = generate_fila_json(fila_global);
    char* rh_json = generate_rh_json();
    char* vendas_json = generate_vendas_json();
    char* agencias_json = generate_agencias_json();
    
    if (!estoque_json || !fila_json || !rh_json || !vendas_json || !agencias_json) {
        free(estoque_json);
        free(fila_json);
        free(rh_json);
        free(vendas_json);
        free(agencias_json);
        return NULL;
    }
    
    time_t now = time(NULL);
    char timestamp[64];
    struct tm* tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char* json = (char*)malloc(16384);
    if (!json) {
        free(estoque_json);
        free(fila_json);
        free(rh_json);
        free(vendas_json);
        free(agencias_json);
        return NULL;
    }
    
    snprintf(json, 16384,
        "{"
        "\"timestamp\": \"%s\","
        "\"api_version\": \"%s\","
        "\"estoque\": %s,"
        "\"fila\": %s,"
        "\"rh\": %s,"
        "\"vendas\": %s,"
        "\"agencias\": %s"
        "}",
        timestamp, API_VERSION,
        estoque_json, fila_json, rh_json, vendas_json, agencias_json);
    
    free(estoque_json);
    free(fila_json);
    free(rh_json);
    free(vendas_json);
    free(agencias_json);
    
    return json;
}

static enum MHD_Result handle_api_request(struct MHD_Connection* connection,
                                         const char* url) {
    char* json = NULL;
    
    printf("\n📡 API Request: %s\n", url);
    
    if (strcmp(url, "/api/estoque") == 0) {
        json = generate_estoque_json();
    } else if (strcmp(url, "/api/fila") == 0) {
        json = generate_fila_json(fila_global);
    } else if (strcmp(url, "/api/rh") == 0) {
        json = generate_rh_json();
    } else if (strcmp(url, "/api/vendas") == 0) {
        json = generate_vendas_json();
    } else if (strcmp(url, "/api/agencias") == 0) {
        json = generate_agencias_json();
    } else if (strcmp(url, "/api/dashboard") == 0) {
        json = generate_dashboard_json();
    } else {
        const char* error = "{\"error\": \"Endpoint not found\"}";
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(error), (void*)error, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    if (!json) {
        const char* error = "{\"error\": \"Internal server error\"}";
        struct MHD_Response* response = MHD_create_response_from_buffer(
            strlen(error), (void*)error, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(json), (void*)json, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Cache-Control", "no-cache");
    
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    
    return ret;
}

static void* web_contratar_thread(void* arg) {
    (void)arg;
    iniciar_processo_contratacao("Cliente Web", "Funcionário", 45000.0);
    return NULL;
}

static enum MHD_Result handle_post_operation(struct MHD_Connection* connection,
                                            const char* url,
                                            const char* data) {
    (void)url;
    
    printf("\n📨 POST Request: %s\n", data);
    
    if (strstr(data, "\"tipo\":\"contratar\"") != NULL) {
        pthread_t thread;
        pthread_create(&thread, NULL, web_contratar_thread, NULL);
        pthread_detach(thread);
        
        const char* response = "{\"status\":\"success\",\"message\":\"Processo de contratação iniciado\"}";
        struct MHD_Response* mhd_response = MHD_create_response_from_buffer(
            strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(mhd_response, "Content-Type", "application/json");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);
        MHD_destroy_response(mhd_response);
        return ret;
    }
    else if (strstr(data, "\"tipo\":\"demitir\"") != NULL) {
        const char* response = "{\"status\":\"success\",\"message\":\"Demissão processada\"}";
        struct MHD_Response* mhd_response = MHD_create_response_from_buffer(
            strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(mhd_response, "Content-Type", "application/json");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);
        MHD_destroy_response(mhd_response);
        return ret;
    }
    
    const char* response = "{\"status\":\"error\",\"message\":\"Invalid operation\"}";
    struct MHD_Response* mhd_response = MHD_create_response_from_buffer(
        strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(mhd_response, "Content-Type", "application/json");
    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, mhd_response);
    MHD_destroy_response(mhd_response);
    return ret;
}

static enum MHD_Result handle_connection(void* cls, struct MHD_Connection* connection, 
                                        const char* url, const char* method, 
                                        const char* version, const char* upload_data, 
                                        size_t* upload_data_size, void** con_cls) {
    (void)cls;
    (void)version;
    
    static int dummy;
    if (&dummy != *con_cls) {
        *con_cls = &dummy;
        return MHD_YES;
    }
    
    printf("\n🌐 REQUISIÇÃO: %s %s\n", method, url);
    
    if (strcmp(method, "GET") == 0) {
        if (strcmp(url, "/") == 0) {
            return handle_static_file(connection, "/index.html", "text/html");
        } else if (strstr(url, "/api/") == url) {
            return handle_api_request(connection, url);
        } else if (strstr(url, ".css") != NULL) {
            return handle_static_file(connection, url, "text/css");
        } else if (strstr(url, ".js") != NULL) {
            return handle_static_file(connection, url, "application/javascript");
        } else if (strstr(url, ".html") != NULL) {
            return handle_static_file(connection, url, "text/html");
        } else if (strstr(url, ".png") != NULL) {
            return handle_image_file(connection, url, "image/png");
        } else if (strstr(url, ".jpg") != NULL || strstr(url, ".jpeg") != NULL) {
            return handle_image_file(connection, url, "image/jpeg");
        } else if (strstr(url, ".gif") != NULL) {
            return handle_image_file(connection, url, "image/gif");
        } else if (strstr(url, ".svg") != NULL) {
            return handle_image_file(connection, url, "image/svg+xml");
        } else if (strstr(url, ".ico") != NULL) {
            return handle_image_file(connection, url, "image/x-icon");
        }
        return handle_static_file(connection, url, "text/plain");
    } else if (strcmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            char* data = strdup(upload_data);
            enum MHD_Result ret = handle_post_operation(connection, url, data);
            free(data);
            *upload_data_size = 0;
            return ret;
        }
        return MHD_YES;
    }
    
    const char* not_found = "<html><body><h1>404 Not Found</h1></body></html>";
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(not_found), (void*)not_found, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "text/html");
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    
    return ret;
}

int iniciar_webserver(void) {
    printf("\n=== INICIANDO SERVIDOR UNITEL ===\n");
    printf("Diretório atual: ");
    system("pwd 2>/dev/null || echo %cd%");
    
    printf("\n📁 Verificando estrutura de diretórios:\n");
    system("ls -la 2>/dev/null || dir");
    
    printf("\n📁 Verificando pasta web/:\n");
    system("ls -la web/ 2>/dev/null || dir web 2>nul");
    
    printf("\n📁 Verificando pasta web/img/:\n");
    system("ls -la web/img/ 2>/dev/null || dir web\\img 2>nul");
    
    // Testa a imagem
    test_image_file("web/img/unitel_logo_sem_fundo.png");
    
    for (int port = PORT_START; port <= PORT_END; port++) {
        webserver_daemon = MHD_start_daemon(
            MHD_USE_AUTO_INTERNAL_THREAD | MHD_USE_DEBUG,
            port,
            NULL, NULL,
            &handle_connection, NULL,
            MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int)30,
            MHD_OPTION_END
        );
        
        if (webserver_daemon != NULL) {
            webserver_port = port;
            printf("\n✅ Web server iniciado na porta %d\n", port);
            printf("📱 Acesse: http://localhost:%d\n", port);
            printf("📡 API: http://localhost:%d/api/dashboard\n", port);
            printf("🖼️  Imagem: http://localhost:%d/img/unitel_logo_sem_fundo.png\n", port);
            printf("\n=== SERVIDOR PRONTO ===\n");
            return 1;
        }
    }
    
    printf("\n❌ Não foi possível iniciar web server (portas %d-%d ocupadas)\n", 
           PORT_START, PORT_END);
    return 0;
}

void parar_webserver(void) {
    if (webserver_daemon) {
        MHD_stop_daemon(webserver_daemon);
        printf("\n🛑 Servidor web parado (porta %d)\n", webserver_port);
        webserver_daemon = NULL;
        webserver_port = 0;
    }
}

int webserver_esta_ativo(void) {
    return (webserver_daemon != NULL) ? 1 : 0;
}

int get_webserver_port(void) {
    return webserver_port;
}
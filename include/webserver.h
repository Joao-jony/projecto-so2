#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <microhttpd.h>
#include <pthread.h>
#include <time.h>

/* Porta padrão do web server */
#define WEBSERVER_PORT 8080

/* Estrutura para resposta HTTP */
typedef struct {
    char* data;
    size_t size;
    int status_code;
    char content_type[64];
} HttpResponse;

/* Estrutura para requisição POST */
typedef struct {
    char* data;
    size_t size;
} PostData;

/* Funções públicas do web server */
int iniciar_webserver(void);
void parar_webserver(void);
int webserver_esta_ativo(void);
int get_webserver_port(void);

/* Funções para gerar JSON */
char* generate_estoque_json(void);
char* generate_fila_json(void* fila);
char* generate_rh_json(void);
char* generate_vendas_json(void);
char* generate_agencias_json(void);
char* generate_dashboard_json(void);

#endif /* WEBSERVER_H */
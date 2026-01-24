#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Fila_prioridade.h"

// 1. Movemos a função de auxílio para o topo para evitar o warning de "implicit declaration"
void realizar_atendimento(Node** frente, Node** fim) {
    if (*frente == NULL) return;
    Node* temp = *frente;
    *frente = (*frente)->next;
    if (*frente == NULL) *fim = NULL;
    free(temp);
}

// 2. Funções principais (certifique-se de que NÃO têm a palavra 'static' antes)
FilaVendas* inicializar_fila() {
    FilaVendas* f = (FilaVendas*)malloc(sizeof(FilaVendas));
    f->frente_empresas = f->fim_empresas = NULL;
    f->frente_publico = f->fim_publico = NULL;
    return f;
}

void inserir_cliente(FilaVendas* fila, int id, TipoCliente tipo) {
    Node* novo = (Node*)malloc(sizeof(Node));
    if (!novo) return;
    
    novo->cliente.id = id;
    novo->cliente.tipo = tipo;
    novo->cliente.timestamp = time(NULL);
    novo->next = NULL;

    if (tipo == EMPRESA) {
        if (fila->fim_empresas == NULL) fila->frente_empresas = novo;
        else fila->fim_empresas->next = novo;
        fila->fim_empresas = novo;
    } else {
        if (fila->fim_publico == NULL) fila->frente_publico = novo;
        else fila->fim_publico->next = novo;
        fila->fim_publico = novo;
    }
}

void processar_vendas_turno(FilaVendas* fila, Turno turno_atual) {
    int limite, vendas_realizadas = 0;
    
    switch (turno_atual) {
        case MANHA: limite = 35; break;
        case TARDE: limite = 35; break;
        case NOITE: limite = 30; break;
        default: return;
    }

    while (vendas_realizadas < limite) {
        Node* alvo = NULL;
        char* tipo_str;

        if (fila->frente_empresas != NULL) {
            alvo = fila->frente_empresas;
            tipo_str = "EMPRESA";
        } else if (fila->frente_publico != NULL) {
            alvo = fila->frente_publico;
            tipo_str = "PUBLICO";
        }

        if (alvo != NULL) {
            char* hora_chegada = ctime(&alvo->cliente.timestamp);
            if (hora_chegada[24] == '\n' || hora_chegada[24] == '\r') hora_chegada[24] = '\0';

            printf("[VENDA %02d/%d] %-8s | ID: %d | Chegada: %s\n", 
                    vendas_realizadas + 1, limite, tipo_str, alvo->cliente.id, hora_chegada);

            if (fila->frente_empresas != NULL) {
                realizar_atendimento(&(fila->frente_empresas), &(fila->fim_empresas));
            } else {
                realizar_atendimento(&(fila->frente_publico), &(fila->fim_publico));
            }
            vendas_realizadas++;
        } else {
            break; 
        }
    }
}

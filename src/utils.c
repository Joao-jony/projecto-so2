#include "utils.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

/* Para sistemas Linux, definir SYS_gettid se não estiver definido */
#ifndef SYS_gettid
#ifdef __NR_gettid
#define SYS_gettid __NR_gettid
#else
#define SYS_gettid 186  /* Valor comum para x86_64 */
#endif
#endif

static FILE* log_file = NULL;
static LogLevel current_level = LOG_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Inicializar sistema de logs */
void init_logging(const char* filename) {
    pthread_mutex_lock(&log_mutex);
    
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            fprintf(stderr, "Erro ao abrir arquivo de log: %s\n", filename);
            log_file = stdout;
        }
    } else {
        log_file = stdout;
    }
    
    pthread_mutex_unlock(&log_mutex);
    
    log_message(LOG_INFO, "SYSTEM", "Sistema de logging inicializado");
}

/* Definir nível de log */
void set_log_level(LogLevel level) {
    current_level = level;
}

/* Mensagem de log */
void log_message(LogLevel level, const char* module, const char* format, ...) {
    if (level < current_level) return;
    
    pthread_mutex_lock(&log_mutex);
    
    const char* level_str[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
    };
    
    char* time_str = get_current_time_str();
    fprintf(log_file, "[%s] [%s] [%s] [TID:%d] ", 
            time_str, level_str[level], module, get_thread_id());
    
    free(time_str);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

/* Fechar logging */
void close_logging(void) {
    pthread_mutex_lock(&log_mutex);
    
    log_message(LOG_INFO, "SYSTEM", "Encerrando sistema de logging");
    
    if (log_file && log_file != stdout) {
        fclose(log_file);
    }
    
    log_file = NULL;
    pthread_mutex_unlock(&log_mutex);
}

/* Obter string do tempo atual */
char* get_current_time_str(void) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    char* buffer = (char*)malloc(20);
    if (buffer) {
        strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    }
    
    return buffer;
}

/* Print com header */
void print_header(const char* title) {
    printf("\n");
    print_separator();
    printf("  %s\n", title);
    print_separator();
}

/* Separador */
void print_separator(void) {
    printf("========================================\n");
}

/* Timestamp */
void print_timestamp(void) {
    char* time_str = get_current_time_str();
    printf("[%s] ", time_str);
    free(time_str);
}

/* Print thread-safe */
void thread_safe_printf(const char* format, ...) {
    pthread_mutex_lock(&log_mutex);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    pthread_mutex_unlock(&log_mutex);
}

/* Obter ID da thread - VERSÃO CORRIGIDA */
int get_thread_id(void) {
#ifdef __linux__
    /* Método 1: usar syscall */
    #ifdef SYS_gettid
        return (int)syscall(SYS_gettid);
    #else
        /* Método 2: usar gettid (glibc 2.30+) */
        #ifdef __GLIBC__
            #if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 30
                return (int)gettid();
            #else
                /* Método 3: usar pthread_self como fallback */
                return (int)(unsigned long)pthread_self();
            #endif
        #else
            /* Não é glibc, usar pthread_self */
            return (int)(unsigned long)pthread_self();
        #endif
    #endif
#else
    /* Não é Linux, usar pthread_self */
    return (int)(unsigned long)pthread_self();
#endif
}
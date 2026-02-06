#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>
#include <pthread.h>

/* Níveis de log */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
} LogLevel;

/* Configuração de logging */
void init_logging(const char* filename);
void set_log_level(LogLevel level);
void log_message(LogLevel level, const char* module, const char* format, ...);
void close_logging(void);

/* Utilitários */
void print_header(const char* title);
void print_separator(void);
void print_timestamp(void);
char* get_current_time_str(void);

/* Thread utilities */
void thread_safe_printf(const char* format, ...);
int get_thread_id(void);

#endif
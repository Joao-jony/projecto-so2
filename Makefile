# ================================================
# Makefile - Sistema Operacional UNITEL Simulator
# Versão CORRIGIDA - Sem dependência de webserver.h
# ================================================

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -pthread -I./include -I./src -g -D_GNU_SOURCE
LDFLAGS = -lpthread -lm
LIBS_NCURSES = -lncurses
LIBS_MICROHTTPD = -lmicrohttpd

# Executáveis
TARGET = unitel_os
TARGET_NO_NCURSES = unitel_os_web
TARGET_NO_WEB = unitel_os_terminal
TARGET_NO_BOTH = unitel_os_text
TEST_TARGET = teste_unitel

# Diretórios
SRC_DIR = src
INC_DIR = include
TEST_DIR = teste
BUILD_DIR = build
DOCS_DIR = docs
BACKUP_DIR = ../backups
WEB_DIR = web
LOGS_DIR = logs

# Cores para output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
BLUE = \033[0;34m
MAGENTA = \033[0;35m
CYAN = \033[0;36m
WHITE = \033[1;37m
NC = \033[0m

# Fontes principais - SEM referência a webserver.h
SRCS = $(SRC_DIR)/estoque.c \
       $(SRC_DIR)/Fila_prioridade.c \
       $(SRC_DIR)/vendas.c \
       $(SRC_DIR)/contratacoes.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/webserver.c \
       $(SRC_DIR)/main.c

# Headers principais - REMOVIDO webserver.h da lista
HEADERS = $(INC_DIR)/estoque.h \
          $(INC_DIR)/Fila_prioridade.h \
          $(INC_DIR)/vendas.h \
          $(INC_DIR)/contratacoes.h \
          $(INC_DIR)/utils.h

# ================================================
# VERIFICAÇÃO DE DEPENDÊNCIAS
# ================================================

.PHONY: all clean test run debug valgrind docs help \
        install-deps install-web-deps \
        web web-files web-clean \
        check-deps check-ncurses check-microhttpd check-dirs \
        run-web run-terminal run-text run-small run-fast run-slow \
        clean-all backup stats create-headers

# Alvo padrão
all: check-deps-force create-headers $(BUILD_DIR) web-files $(TARGET)
	@echo "$(GREEN)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(GREEN)✅ Sistema UNITEL compilado com sucesso!$(NC)"
	@echo "$(GREEN)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(CYAN)📱 Modo completo (ncurses + web):    make run$(NC)"
	@echo "$(CYAN)🌐 Apenas web server:               make run-web$(NC)"
	@echo "$(CYAN)📟 Apenas terminal:                 make run-terminal$(NC)"
	@echo "$(CYAN)📡 API JSON:                        http://localhost:8080/api/dashboard$(NC)"
	@echo "$(GREEN)═══════════════════════════════════════════════════════════$(NC)"

# Criar headers necessários
create-headers:
	@mkdir -p $(INC_DIR)
	@if [ ! -f $(INC_DIR)/webserver.h ]; then \
		echo "$(YELLOW)📝 Criando webserver.h...$(NC)"; \
		echo "#ifndef WEBSERVER_H" > $(INC_DIR)/webserver.h; \
		echo "#define WEBSERVER_H" >> $(INC_DIR)/webserver.h; \
		echo "" >> $(INC_DIR)/webserver.h; \
		echo "#include <microhttpd.h>" >> $(INC_DIR)/webserver.h; \
		echo "#include <pthread.h>" >> $(INC_DIR)/webserver.h; \
		echo "#include <time.h>" >> $(INC_DIR)/webserver.h; \
		echo "" >> $(INC_DIR)/webserver.h; \
		echo "#define WEBSERVER_PORT 8080" >> $(INC_DIR)/webserver.h; \
		echo "" >> $(INC_DIR)/webserver.h; \
		echo "typedef struct {" >> $(INC_DIR)/webserver.h; \
		echo "    char* data;" >> $(INC_DIR)/webserver.h; \
		echo "    size_t size;" >> $(INC_DIR)/webserver.h; \
		echo "    int status_code;" >> $(INC_DIR)/webserver.h; \
		echo "    char content_type[64];" >> $(INC_DIR)/webserver.h; \
		echo "} HttpResponse;" >> $(INC_DIR)/webserver.h; \
		echo "" >> $(INC_DIR)/webserver.h; \
		echo "typedef struct {" >> $(INC_DIR)/webserver.h; \
		echo "    char* data;" >> $(INC_DIR)/webserver.h; \
		echo "    size_t size;" >> $(INC_DIR)/webserver.h; \
		echo "} PostData;" >> $(INC_DIR)/webserver.h; \
		echo "" >> $(INC_DIR)/webserver.h; \
		echo "int iniciar_webserver(void);" >> $(INC_DIR)/webserver.h; \
		echo "void parar_webserver(void);" >> $(INC_DIR)/webserver.h; \
		echo "int webserver_esta_ativo(void);" >> $(INC_DIR)/webserver.h; \
		echo "" >> $(INC_DIR)/webserver.h; \
		echo "char* generate_estoque_json(void);" >> $(INC_DIR)/webserver.h; \
		echo "char* generate_fila_json(void* fila);" >> $(INC_DIR)/webserver.h; \
		echo "char* generate_rh_json(void);" >> $(INC_DIR)/webserver.h; \
		echo "char* generate_vendas_json(void);" >> $(INC_DIR)/webserver.h; \
		echo "char* generate_agencias_json(void);" >> $(INC_DIR)/webserver.h; \
		echo "char* generate_dashboard_json(void);" >> $(INC_DIR)/webserver.h; \
		echo "" >> $(INC_DIR)/webserver.h; \
		echo "#endif /* WEBSERVER_H */" >> $(INC_DIR)/webserver.h; \
		echo "$(GREEN)✅ webserver.h criado$(NC)"; \
	fi

# Verificar dependências - VERSÃO FORÇADA
check-deps-force:
	@echo "$(YELLOW)🔍 Verificando dependências...$(NC)"
	@# Verificar microhttpd
	@if [ -f /usr/include/microhttpd.h ]; then \
		echo "$(GREEN)  ✅ libmicrohttpd encontrado$(NC)"; \
	else \
		echo "$(YELLOW)  ⚠️  libmicrohttpd não encontrado - compilando sem web server$(NC)"; \
		$(eval TARGET := $(TARGET_NO_WEB)) \
	fi
	@# Verificar ncurses
	@if [ -f /usr/include/ncurses.h ]; then \
		echo "$(GREEN)  ✅ ncurses encontrado$(NC)"; \
	else \
		echo "$(YELLOW)  ⚠️  ncurses não encontrado - compilando em modo texto$(NC)"; \
		$(eval TARGET := $(TARGET_NO_NCURSES)) \
	fi
	@echo "$(GREEN)✅ Verificação concluída$(NC)"

# Criar diretórios
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(LOGS_DIR)

# ================================================
# FRONTEND - ARQUIVOS WEB
# ================================================

$(WEB_DIR):
	@mkdir -p $(WEB_DIR)

web-files: $(WEB_DIR)
	@if [ ! -f $(WEB_DIR)/index.html ]; then \
		echo "$(YELLOW)📝 Criando index.html...$(NC)"; \
		echo '<!DOCTYPE html><html><head><meta charset="UTF-8"><title>UNITEL OS</title><style>body{font-family:Arial;margin:40px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;text-align:center;}</style></head><body><h1>🌐 UNITEL Operating System</h1><p>Servidor web em execução!</p><p><small>Acesse /api/dashboard para JSON</small></p></body></html>' > $(WEB_DIR)/index.html; \
	fi
	@if [ ! -f $(WEB_DIR)/style.css ]; then \
		echo "$(YELLOW)📝 Criando style.css...$(NC)"; \
		echo 'body{font-family:"Inter",sans-serif;margin:0;padding:20px;background:#f3f4f6;}' > $(WEB_DIR)/style.css; \
	fi
	@if [ ! -f $(WEB_DIR)/script.js ]; then \
		echo "$(YELLOW)📝 Criando script.js...$(NC)"; \
		echo 'console.log("UNITEL OS Dashboard");setInterval(()=>{fetch("/api/dashboard").then(r=>r.json()).then(d=>console.log(d))},5000);' > $(WEB_DIR)/script.js; \
	fi
	@echo "$(GREEN)✅ Arquivos web verificados$(NC)"

web-clean:
	@echo "$(YELLOW)🧹 Limpando arquivos web...$(NC)"
	@rm -rf $(WEB_DIR)
	@echo "$(GREEN)✅ Arquivos web removidos$(NC)"

# ================================================
# COMPILAÇÃO - VERSÕES CORRIGIDAS
# ================================================

# Versão completa (ncurses + web server)
$(TARGET): $(SRCS) $(HEADERS) web-files create-headers
	@echo "$(YELLOW)🔨 Compilando UNITEL OS (ncurses + web server)...$(NC)"
	$(CC) $(CFLAGS) -DENABLE_WEBSERVER=1 $(SRCS) -o $@ $(LDFLAGS) $(LIBS_NCURSES) $(LIBS_MICROHTTPD) 2>/dev/null || \
	$(CC) $(CFLAGS) -DENABLE_WEBSERVER=1 $(SRCS) -o $@ $(LDFLAGS) $(LIBS_NCURSES) -lmicrohttpd
	@echo "$(GREEN)✅ Executável criado: $@$(NC)"

# Versão sem ncurses (modo texto + web server)
$(TARGET_NO_NCURSES): $(SRCS) $(HEADERS) web-files create-headers
	@echo "$(YELLOW)🔨 Compilando UNITEL OS (web server only)...$(NC)"
	$(CC) $(CFLAGS) -DENABLE_WEBSERVER=1 -DTEXT_MODE $(SRCS) -o $@ $(LDFLAGS) $(LIBS_MICROHTTPD) 2>/dev/null || \
	$(CC) $(CFLAGS) -DENABLE_WEBSERVER=1 -DTEXT_MODE $(SRCS) -o $@ $(LDFLAGS) -lmicrohttpd
	@echo "$(GREEN)✅ Executável criado: $@$(NC)"

# Versão sem web server (ncurses apenas)
$(TARGET_NO_WEB): $(filter-out $(SRC_DIR)/webserver.c, $(SRCS)) $(HEADERS)
	@echo "$(YELLOW)🔨 Compilando UNITEL OS (terminal only)...$(NC)"
	$(CC) $(CFLAGS) -UENABLE_WEBSERVER $(filter-out $(SRC_DIR)/webserver.c, $(SRCS)) -o $@ $(LDFLAGS) $(LIBS_NCURSES)
	@echo "$(GREEN)✅ Executável criado: $@$(NC)"

# Versão mínima (modo texto apenas)
$(TARGET_NO_BOTH): $(filter-out $(SRC_DIR)/webserver.c, $(SRCS)) $(HEADERS)
	@echo "$(YELLOW)🔨 Compilando UNITEL OS (text mode only)...$(NC)"
	$(CC) $(CFLAGS) -UENABLE_WEBSERVER -DTEXT_MODE $(filter-out $(SRC_DIR)/webserver.c, $(SRCS)) -o $@ $(LDFLAGS)
	@echo "$(GREEN)✅ Executável criado: $@$(NC)"

# ================================================
# EXECUÇÃO
# ================================================

# Executar sistema completo
run: web-files create-headers $(TARGET)
	@echo "$(CYAN)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(CYAN)🚀 Iniciando UNITEL OS - Modo Completo$(NC)"
	@echo "$(CYAN)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(YELLOW)📱 Dashboard ncurses: Terminal local$(NC)"
	@echo "$(YELLOW)🌐 Web dashboard:     http://localhost:8080$(NC)"
	@echo "$(YELLOW)📡 API JSON:          http://localhost:8080/api/dashboard$(NC)"
	@echo "$(MAGENTA)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@./$(TARGET)

# Executar apenas web server
run-web: $(TARGET_NO_NCURSES)
	@echo "$(CYAN)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(CYAN)🌐 Iniciando UNITEL OS - Modo Web Server$(NC)"
	@echo "$(CYAN)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(YELLOW)🌐 Web dashboard:     http://localhost:8080$(NC)"
	@echo "$(YELLOW)📡 API JSON:          http://localhost:8080/api/dashboard$(NC)"
	@echo "$(MAGENTA)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@./$(TARGET_NO_NCURSES)

# Executar apenas terminal
run-terminal: $(TARGET_NO_WEB)
	@echo "$(CYAN)📱 Iniciando UNITEL OS - Modo Terminal$(NC)"
	@./$(TARGET_NO_WEB)

# Executar modo texto puro
run-text: $(TARGET_NO_BOTH)
	@echo "$(CYAN)📟 Iniciando UNITEL OS - Modo Texto Puro$(NC)"
	@./$(TARGET_NO_BOTH)

# ================================================
# INSTALAÇÃO DE DEPENDÊNCIAS
# ================================================

install-deps:
	@echo "$(YELLOW)📦 Instalando todas as dependências...$(NC)"
	@sudo apt update
	@sudo apt install -y \
		libncurses-dev \
		libmicrohttpd-dev \
		build-essential \
		gdb \
		valgrind \
		tree \
		curl
	@echo "$(GREEN)✅ Dependências instaladas$(NC)"

install-web-deps:
	@echo "$(YELLOW)📦 Instalando dependências do web server...$(NC)"
	@sudo apt update
	@sudo apt install -y libmicrohttpd-dev
	@echo "$(GREEN)✅ Dependências web instaladas$(NC)"

# ================================================
# LIMPEZA
# ================================================

clean:
	@echo "$(YELLOW)🧹 Limpando arquivos...$(NC)"
	@rm -f $(TARGET) $(TARGET_NO_NCURSES) $(TARGET_NO_WEB) $(TARGET_NO_BOTH) $(TEST_TARGET)
	@rm -rf $(BUILD_DIR) *.dSYM
	@find . -name "*.o" -delete
	@find . -name "*.so" -delete
	@find . -name "*.a" -delete
	@find . -name "*~" -delete
	@find . -name "*.out" -delete
	@find . -name "*.log" -delete
	@find . -name "*.gch" -delete
	@echo "$(GREEN)✅ Limpeza concluída$(NC)"

clean-all: clean web-clean
	@echo "$(YELLOW)🧹 Limpeza total...$(NC)"
	@rm -f $(INC_DIR)/webserver.h
	@rm -rf $(DOCS_DIR)
	@rm -rf $(LOGS_DIR)
	@rm -f valgrind.supp
	@rm -f *.tar.gz
	@echo "$(GREEN)✅ Limpeza total concluída$(NC)"

# ================================================
# DOCUMENTAÇÃO E ESTATÍSTICAS
# ================================================

docs:
	@mkdir -p $(DOCS_DIR)
	@echo "$(MAGENTA)📚 Gerando documentação...$(NC)"
	@echo "# UNITEL OS - Documentação" > $(DOCS_DIR)/README.md
	@echo "" >> $(DOCS_DIR)/README.md
	@date >> $(DOCS_DIR)/README.md
	@echo "$(GREEN)✅ Documentação gerada$(NC)"

stats:
	@echo "$(CYAN)📊 Estatísticas do código:$(NC)"
	@echo "$(MAGENTA)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "📁 Arquivos .c:     $$(find $(SRC_DIR) -name "*.c" 2>/dev/null | wc -l)"
	@echo "📁 Arquivos .h:     $$(find $(INC_DIR) -name "*.h" 2>/dev/null | wc -l)"
	@echo "📁 Arquivos web:    $$(find $(WEB_DIR) -name "*.*" 2>/dev/null | wc -l)"
	@echo "📊 Linhas de código: $$(find $(SRC_DIR) $(INC_DIR) -name "*.[ch]" -exec cat {} \; 2>/dev/null | wc -l)"
	@echo "$(MAGENTA)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"

# ================================================
# BACKUP
# ================================================

backup:
	@echo "$(YELLOW)📦 Criando backup do projeto...$(NC)"
	@mkdir -p $(BACKUP_DIR)
	@tar -czf $(BACKUP_DIR)/unitel_os_$$(date +%Y%m%d_%H%M%S).tar.gz \
		--exclude='./$(BUILD_DIR)' \
		--exclude='./*.o' \
		--exclude='./$(TARGET)*' \
		--exclude='./$(LOGS_DIR)' \
		.
	@echo "$(GREEN)✅ Backup criado em $(BACKUP_DIR)/$(NC)"

# ================================================
# AJUDA
# ================================================

help:
	@echo "$(MAGENTA)════════════════════════════════════════════════════════════════$(NC)"
	@echo "$(MAGENTA)   SISTEMA UNITEL - COMANDOS MAKE (CORRIGIDO)   $(NC)"
	@echo "$(MAGENTA)════════════════════════════════════════════════════════════════$(NC)"
	@echo ""
	@echo "$(WHITE)📦 COMPILAÇÃO:$(NC)"
	@echo "  make              - Compilar versão completa"
	@echo "  make clean        - Limpeza básica"
	@echo "  make clean-all    - Limpeza total"
	@echo ""
	@echo "$(WHITE)🚀 EXECUÇÃO:$(NC)"
	@echo "  make run          - Modo completo (ncurses + web) 🌐+📱"
	@echo "  make run-web      - Apenas web server 🌐"
	@echo "  make run-terminal - Apenas terminal 📱"
	@echo "  make run-text     - Modo texto puro 📟"
	@echo ""
	@echo "$(WHITE)🌐 WEB SERVER:$(NC)"
	@echo "  make web-files    - Criar/recriar arquivos web"
	@echo "  make web-clean    - Limpar arquivos web"
	@echo ""
	@echo "$(WHITE)📦 INSTALAÇÃO:$(NC)"
	@echo "  make install-deps - Instalar todas dependências"
	@echo "  make install-web-deps - Apenas libmicrohttpd"
	@echo ""
	@echo "$(WHITE)📚 DOCUMENTAÇÃO:$(NC)"
	@echo "  make docs         - Gerar documentação"
	@echo "  make stats        - Estatísticas de código"
	@echo "  make help         - Mostrar esta ajuda"
	@echo ""
	@echo "$(MAGENTA)════════════════════════════════════════════════════════════════$(NC)"
	@echo "$(GREEN)📌 EXEMPLO RÁPIDO:$(NC)"
	@echo "  sudo make install-web-deps  # Instalar microhttpd"
	@echo "  make clean                  # Limpar"
	@echo "  make                       # Compilar"
	@echo "  make run                   # Executar"
	@echo "  # Acesse: http://localhost:8080"
	@echo "$(MAGENTA)════════════════════════════════════════════════════════════════$(NC)"

# ================================================
# ALVO PADRÃO
# ================================================

.DEFAULT_GOAL := all
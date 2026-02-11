# ================================================
# Makefile - Sistema Operacional UNITEL Simulator
# ================================================

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -pthread -I./include -g
LDFLAGS = -pthread -lm

# Suporte a ncurses
NCURSES_CFLAGS = -DUSE_NCURSES
NCURSES_LIBS = -lncurses

# Executáveis
TARGET = unitel
TEST_TARGET = teste_unitel
NCURSES_TARGET = unitel_ncurses

# Diretórios
SRC_DIR = src
INC_DIR = include
TEST_DIR = teste

# Fontes principais
SRCS = $(SRC_DIR)/estoque.c \
       $(SRC_DIR)/Fila_prioridade.c \
       $(SRC_DIR)/vendas.c \
       $(SRC_DIR)/contratacoes.c \
       $(SRC_DIR)/main.c

# Fontes de teste
TEST_SRCS = $(SRC_DIR)/estoque.c \
            $(SRC_DIR)/Fila_prioridade.c \
            $(TEST_DIR)/teste_integracao.c

# ================================================
# REGRAS PRINCIPAIS
# ================================================

.PHONY: all clean test run debug valgrind docs help ncurses run-ncurses check-ncurses

# Alvo padrão - compila diretamente sem arquivos .o
all: $(TARGET)

# Compilar executável principal em um único passo
$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)
	@echo "✅ Executável principal criado: $@"

# Compilar versão com ncurses
$(NCURSES_TARGET):
	@echo "📟 Compilando com suporte a ncurses..."
	$(CC) $(CFLAGS) $(NCURSES_CFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(NCURSES_LIBS)
	@echo "✅ Executável ncurses criado: $@"

# Compilar teste em um único passo
$(TEST_TARGET):
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $@ $(LDFLAGS)
	@echo "✅ Teste criado: $@"

# ================================================
# REGRAS AUXILIARES
# ================================================

# Executar sistema
run: all
	@./$(TARGET)

# Executar com ncurses
run-ncurses: $(NCURSES_TARGET)
	@echo "🎮 Executando com interface ncurses..."
	@./$(NCURSES_TARGET)

# Compilar versão ncurses
ncurses: $(NCURSES_TARGET)
	@echo "📟 Versão ncurses compilada com sucesso!"

# Executar testes
test: $(TEST_TARGET)
	@./$(TEST_TARGET)

# Debug com gdb (versão normal)
debug: CFLAGS += -DDEBUG -O0
debug: clean all
	@echo "🐛 Compilado para debug (modo texto). Use: gdb $(TARGET)"

# Debug com gdb (versão ncurses)
debug-ncurses: CFLAGS += -DDEBUG -DUSE_NCURSES -O0
debug-ncurses: clean $(NCURSES_TARGET)
	@echo "🐛 Compilado para debug (modo ncurses). Use: gdb $(NCURSES_TARGET)"

# Valgrind para memory leaks (versão normal)
valgrind: all
	@echo "🔍 Executando valgrind (modo texto)..."
	@valgrind --leak-check=full \
	          --show-leak-kinds=all \
	          --track-origins=yes \
	          --verbose \
	          ./$(TARGET)

# Valgrind para memory leaks (versão ncurses)
valgrind-ncurses: $(NCURSES_TARGET)
	@echo "🔍 Executando valgrind (modo ncurses)..."
	@valgrind --leak-check=full \
	          --show-leak-kinds=all \
	          --track-origins=yes \
	          --verbose \
	          ./$(NCURSES_TARGET)

# Verificar se ncurses está instalado
check-ncurses:
	@echo "🔍 Verificando instalação do ncurses..."
	@which ncurses5-config > /dev/null 2>&1 || which ncurses6-config > /dev/null 2>&1 || which ncursesw5-config > /dev/null 2>&1 && \
		echo "✅ ncurses já está instalado!" || \
		(echo "❌ ncurses não encontrado!" && \
		 echo "📦 Para instalar no Ubuntu/Debian:" && \
		 echo "   sudo apt update" && \
		 echo "   sudo apt install libncurses5-dev" && \
		 echo "" && \
		 echo "📦 Para instalar no Fedora/RHEL:" && \
		 echo "   sudo dnf install ncurses-devel" && \
		 echo "" && \
		 echo "📦 Para instalar no Arch Linux:" && \
		 echo "   sudo pacman -S ncurses")

# Limpeza
clean:
	@echo "🧹 Limpando arquivos..."
	@rm -f $(TARGET) $(TEST_TARGET) $(NCURSES_TARGET) *~ core
	@find . -name "*.so" -delete
	@find . -name "*.a" -delete
	@find . -name "*~" -delete
	@find . -name "*.out" -delete
	@echo "✅ Limpeza concluída."

# Documentação
docs:
	@echo "📚 Gerando documentação..."
	@mkdir -p docs
	@echo "# Documentação do Sistema UNITEL" > docs/README.md
	@echo "Gerado em: $$(date)" >> docs/README.md
	@echo "" >> docs/README.md
	@echo "## Modos de Compilação" >> docs/README.md
	@echo "" >> docs/README.md
	@echo "- **Modo Texto**: make all ou make run" >> docs/README.md
	@echo "- **Modo Ncurses**: make ncurses ou make run-ncurses" >> docs/README.md
	@echo "" >> docs/README.md
	@echo "## Estrutura de Arquivos:" >> docs/README.md
	@echo "\`\`\`" >> docs/README.md
	@tree -I 'docs' >> docs/README.md 2>/dev/null || find . -type f | sort >> docs/README.md
	@echo "\`\`\`" >> docs/README.md
	@echo "✅ Documentação gerada em docs/README.md"

# Instalar dependências ncurses
install-ncurses:
	@echo "📦 Instalando dependências ncurses..."
	@if command -v apt-get > /dev/null; then \
		sudo apt-get update && sudo apt-get install -y libncurses5-dev; \
	elif command -v dnf > /dev/null; then \
		sudo dnf install -y ncurses-devel; \
	elif command -v pacman > /dev/null; then \
		sudo pacman -S --noconfirm ncurses; \
	else \
		echo "❌ Gerenciador de pacotes não identificado."; \
		echo "Por favor, instale libncurses5-dev manualmente."; \
	fi

# Ajuda
help:
	@echo "================================================"
	@echo "📌 SISTEMA OPERACIONAL UNITEL - COMANDOS MAKE"
	@echo "================================================"
	@echo ""
	@echo "📦 COMPILAÇÃO:"
	@echo "  make all          - Compilar versão texto (padrão)"
	@echo "  make ncurses      - Compilar versão com interface gráfica ncurses"
	@echo "  make test        - Compilar testes"
	@echo ""
	@echo "🎮 EXECUÇÃO:"
	@echo "  make run          - Executar versão texto"
	@echo "  make run-ncurses  - Executar versão com ncurses"
	@echo "  make test        - Executar testes"
	@echo ""
	@echo "🐛 DEBUG:"
	@echo "  make debug       - Compilar versão texto com flags de debug"
	@echo "  make debug-ncurses - Compilar versão ncurses com flags de debug"
	@echo "  make valgrind     - Executar valgrind na versão texto"
	@echo "  make valgrind-ncurses - Executar valgrind na versão ncurses"
	@echo ""
	@echo "🔧 UTILITÁRIOS:"
	@echo "  make clean       - Limpar arquivos compilados"
	@echo "  make docs        - Gerar documentação"
	@echo "  make check-ncurses - Verificar instalação do ncurses"
	@echo "  make install-ncurses - Instalar dependências ncurses (Ubuntu/Debian)"
	@echo "  make help        - Mostrar esta ajuda"
	@echo ""
	@echo "================================================"
	@echo "📋 PRIMEIROS PASSOS:"
	@echo "  1. make check-ncurses  # Verificar dependências"
	@echo "  2. make install-ncurses # Instalar ncurses (se necessário)"
	@echo "  3. make ncurses        # Compilar com ncurses"
	@echo "  4. make run-ncurses    # Executar"
	@echo "================================================"
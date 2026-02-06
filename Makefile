# ================================================
# Makefile - Sistema Operacional UNITEL Simulator
# ================================================

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -pthread -I./include -g
LDFLAGS = -pthread -lm

# Executáveis
TARGET = unitel
TEST_TARGET = teste_unitel

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

.PHONY: all clean test run debug valgrind docs help

# Alvo padrão - compila diretamente sem arquivos .o
all: $(TARGET)

# Compilar executável principal em um único passo
$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)
	@echo "Executável principal criado: $@"

# Compilar teste em um único passo
$(TEST_TARGET):
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $@ $(LDFLAGS)
	@echo "Teste criado: $@"

# ================================================
# REGRAS AUXILIARES
# ================================================

# Executar sistema
run: all
	@./$(TARGET)

# Executar teste
test: $(TEST_TARGET)
	@./$(TEST_TARGET)

# Debug com gdb
debug: CFLAGS += -DDEBUG -O0
debug: clean all
	@echo "🐛 Compilado para debug. Use: gdb $(TARGET)"

# Valgrind para memory leaks
valgrind: all
	@echo "Executando valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Limpeza
clean:
	@echo " Limpando arquivos..."
	@rm -f $(TARGET) $(TEST_TARGET) *~ core
	@find . -name "*.so" -delete
	@find . -name "*.a" -delete
	@find . -name "*~" -delete
	@find . -name "*.out" -delete
	@echo "Limpeza concluída."

# Documentação
docs:
	@echo "📚 Gerando documentação..."
	@mkdir -p docs
	@echo "# Documentação do Sistema UNITEL" > docs/README.md
	@echo "Gerado em: $$(date)" >> docs/README.md
	@echo "\n## Estrutura de Arquivos:" >> docs/README.md
	@echo "\`\`\`" >> docs/README.md
	@tree -I 'docs' >> docs/README.md
	@echo "\`\`\`" >> docs/README.md
	@echo "Documentação gerada em docs/README.md"

# Ajuda
help:
	@echo "================================================"
	@echo "SISTEMA OPERACIONAL UNITEL - COMANDOS MAKE"
	@echo "================================================"
	@echo ""
	@echo "  make all     - Compilar tudo (padrão)"
	@echo "  make run     - Compilar e executar"
	@echo "  make test    - Compilar e executar testes"
	@echo "  make debug   - Compilar com flags de debug"
	@echo "  make valgrind- Executar com valgrind (memory leaks)"
	@echo "  make clean   - Limpar arquivos compilados"
	@echo "  make docs    - Gerar documentação"
	@echo "  make help    - Mostrar esta ajuda"
	@echo ""
	@echo "================================================"

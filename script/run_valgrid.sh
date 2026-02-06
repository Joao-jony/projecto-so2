#!/bin/bash
# Script para executar valgrind em todos os testes

echo "Executando valgrind no sistema..."

# Compilar
make clean
make all

# Testar módulos individuais
echo "1. Testando módulo Estoque..."
gcc -I./include -g -pthread src/estoque.c teste/teste_estoque.c -o bin/teste_estoque
valgrind --leak-check=full --show-leak-kinds=all ./bin/teste_estoque 2>&1 | grep -A5 "ERROR SUMMARY"

echo "2. Testando módulo Fila..."
gcc -I./include -g -pthread src/estoque.c src/Fila_prioridade.c teste/teste_fila.c -o bin/teste_fila
valgrind --leak-check=full --show-leak-kinds=all ./bin/teste_fila 2>&1 | grep -A5 "ERROR SUMMARY"

echo "3. Testando sistema completo..."
cd bin && valgrind --leak-check=full --show-leak-kinds=all ./simulador_unitel 2>&1 | tail -20

echo "✅ Valgrind concluído!"
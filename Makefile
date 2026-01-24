CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include
LDFLAGS = -pthread
TARGET = teste_unitel  

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)

all: $(TARGET)
	./$(TARGET)

obj:
	mkdir -p obj

$(TARGET): teste/teste_integracao.c $(OBJS)
	$(CC) $(CFLAGS) teste/teste_integracao.c $(OBJS) -o $(TARGET) $(LDFLAGS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj $(TARGET)

run: all

.PHONY: all clean run
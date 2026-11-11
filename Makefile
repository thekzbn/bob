CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
SRCS = src/lexer.c src/ast.c src/parser.c src/meta.c src/id_gen.c src/codegen.c
TARGET = bobc.exe

all: $(TARGET)

$(TARGET): $(SRCS) src/main.c
	$(CC) $(CFLAGS) $(SRCS) src/main.c -o $(TARGET)

test: $(SRCS) tests/test_suite.c
	$(CC) $(CFLAGS) $(SRCS) tests/test_suite.c -o tests/test_suite.exe
	./tests/test_suite.exe

clean:
	rm -f $(TARGET) tests/test_suite.exe

.PHONY: all test clean

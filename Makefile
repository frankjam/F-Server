# Makefile for HTTP server

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -fdiagnostics-color=always -g

# Source files
SRCS = httpd.c

# Output executable
TARGET = httpd.exe

# Libraries
LIBS = -lws2_32

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# run the target 
run: $(TARGET)
	./$(TARGET) 12
	
# Clean target
clean:
	rm -rf $(TARGET)

.PHONY: all clean run

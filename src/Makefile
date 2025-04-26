# Makefile for the kernel simulator project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -I.

# Directories
SRC_DIR    = .
OBJ_DIR    = obj
BIN_DIR    = bin
MODELS_DIR = models

# Target executables
PROCESS_GENERATOR = $(BIN_DIR)/process_generator
SCHEDULER        = $(BIN_DIR)/scheduler
PROCESS          = $(BIN_DIR)/process

# Source files
CLK_SRC         = $(SRC_DIR)/clk.c
SCHEDULER_SRC   = $(SRC_DIR)/scheduler.c
PROCESS_GEN_SRC = $(SRC_DIR)/process_generator.c
PROCESS_SRC     = $(SRC_DIR)/process.c
QUEUE_SRC       = $(MODELS_DIR)/Queue/queue.c
MINHEAP_SRC     = $(MODELS_DIR)/minHeap1/minHeap.c

# Object files
CLK_OBJ         = $(OBJ_DIR)/clk.o
SCHEDULER_OBJ   = $(OBJ_DIR)/scheduler.o
PROCESS_GEN_OBJ = $(OBJ_DIR)/process_generator.o
PROCESS_OBJ     = $(OBJ_DIR)/process.o
QUEUE_OBJ       = $(OBJ_DIR)/$(MODELS_DIR)/Queue/queue.o
MINHEAP_OBJ     = $(OBJ_DIR)/$(MODELS_DIR)/minHeap1/minHeap.o

.PHONY: all clean keyfile setup

# Default target - build all executables
all: setup $(PROCESS_GENERATOR) $(SCHEDULER) $(PROCESS) keyfile

# Create necessary directories
setup:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(OBJ_DIR)/$(MODELS_DIR)/Queue $(OBJ_DIR)/$(MODELS_DIR)/minHeap1

# Create keyfile for IPC
keyfile:
	@touch keyfile

# Build process_generator
$(PROCESS_GENERATOR): $(PROCESS_GEN_OBJ) $(CLK_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Build scheduler
$(SCHEDULER): $(SCHEDULER_OBJ) $(CLK_OBJ) $(QUEUE_OBJ) $(MINHEAP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Build process
$(PROCESS): $(PROCESS_OBJ) $(CLK_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile clk.c
$(CLK_OBJ): $(CLK_SRC) $(SRC_DIR)/clk.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile scheduler.c
$(SCHEDULER_OBJ): $(SCHEDULER_SRC) $(SRC_DIR)/scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile process_generator.c
$(PROCESS_GEN_OBJ): $(PROCESS_GEN_SRC) $(SRC_DIR)/clk.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile process.c
$(PROCESS_OBJ): $(PROCESS_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile queue.c
$(QUEUE_OBJ): $(QUEUE_SRC) $(MODELS_DIR)/Queue/queue.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile minHeap.c
$(MINHEAP_OBJ): $(MINHEAP_SRC) $(MODELS_DIR)/minHeap1/minHeap.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR) keyfile
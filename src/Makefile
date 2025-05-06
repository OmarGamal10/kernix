# Makefile for the kernel simulator project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -I.
LDFLAGS = -lm

# Directories
SRC_DIR    = .
MODELS_DIR = models

# Target executables
PROCESS_GENERATOR = os-sim
SCHEDULER        = scheduler
PROCESS          = process

# Source files
CLK_SRC         = $(SRC_DIR)/clk.c
SCHEDULER_SRC   = $(SRC_DIR)/scheduler.c
PROCESS_GEN_SRC = $(SRC_DIR)/process_generator.c
PROCESS_SRC     = $(SRC_DIR)/process.c
QUEUE_SRC       = $(MODELS_DIR)/Queue/queue.c
MINHEAP_SRC     = $(MODELS_DIR)/minHeap1/minHeap.c
BUDDY_SRC	  = $(SRC_DIR)/memory.c

# Object files
CLK_OBJ         = clk.o
SCHEDULER_OBJ   = scheduler.o
PROCESS_GEN_OBJ = process_generator.o
PROCESS_OBJ     = process.o
QUEUE_OBJ       = queue.o
MINHEAP_OBJ     = minheap.o
BUDDY_OBJ      = buddy.o

.PHONY: all clean keyfile setup

# Default target - build all executables
all: $(PROCESS_GENERATOR) $(PROCESS) keyfile

# Create necessary directories
setup:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Create keyfile for IPC
keyfile:
	@touch keyfile

# Build process_generator - must include scheduler objects
$(PROCESS_GENERATOR): $(PROCESS_GEN_OBJ) $(CLK_OBJ) $(SCHEDULER_OBJ) $(QUEUE_OBJ) $(MINHEAP_OBJ) $(BUDDY_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build scheduler
$(SCHEDULER): $(SCHEDULER_OBJ) $(CLK_OBJ) $(QUEUE_OBJ) $(MINHEAP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build process
$(PROCESS): $(PROCESS_OBJ) $(CLK_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build buddy allocator
$(BUDDY_OBJ): $(BUDDY_SRC) $(SRC_DIR)/memory.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile clk.c
$(CLK_OBJ): $(CLK_SRC) $(SRC_DIR)/clk.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile scheduler.c
$(SCHEDULER_OBJ): $(SCHEDULER_SRC) $(SRC_DIR)/scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile process_generator.c
$(PROCESS_GEN_OBJ): $(PROCESS_GEN_SRC) $(SRC_DIR)/clk.h $(SRC_DIR)/scheduler.h $(SRC_DIR)/process_generator.h
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
	@rm -rf	*.o $(PROCESS_GENERATOR) $(SCHEDULER) $(PROCESS) keyfile
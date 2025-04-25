# MyShell Project Makefile
# Builds the myshell executable with all components
# Handles compilation of both core shell functions and SDR modules
# Provides targets: all, clean, run

# Compiler and flags
CC = gcc
CFLAGS = -I./include
LIBS = -lreadline -lrtlsdr -lfftw3f -lm

# Directories
SRC_DIR = src
SDR_DIR = sdr
OBJ_DIR = obj
SDR_OBJ_DIR = $(OBJ_DIR)/sdr
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
SDR_SRCS = $(wildcard $(SDR_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SDR_OBJS = $(SDR_SRCS:$(SDR_DIR)/%.c=$(SDR_OBJ_DIR)/%.o)
ALL_OBJS = $(OBJS) $(SDR_OBJS)
EXEC = $(BIN_DIR)/myshell

# Create directories
$(shell mkdir -p $(OBJ_DIR) $(SDR_OBJ_DIR) $(BIN_DIR))

# Main target
all: $(EXEC)

# Link
$(EXEC): $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile main source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile SDR source files
$(SDR_OBJ_DIR)/%.o: $(SDR_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run
run: $(EXEC)
	$(EXEC)

.PHONY: all clean run
# Variables
SRC := src
OBJ := obj
BIN := bin
EXECUTABLE := filesys
IMG_FILE := fat32.img  # Name of the .img file to be used as an argument

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
INCS := -Iinclude/
DIRS := $(OBJ) $(BIN)
EXEC := $(BIN)/$(EXECUTABLE)
IMG_PATH := ./$(IMG_FILE)  # Path to the .img file in the main directory

CC := gcc
CFLAGS := -g -Wall -std=c99 $(INCS)
LDFLAGS :=

# Default target
all: $(DIRS) $(EXEC)

# Link the program
$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(EXEC)

# Compile the source files into object files
$(OBJ)/%.o: $(SRC)/%.c | $(DIRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Create necessary directories
$(DIRS):
	@mkdir -p $(DIRS)

# Run the program
run: $(EXEC)
	@echo "Running $(EXEC) with $(IMG_PATH)"
	@cp $(IMG_PATH) $(BIN)/   # Copy image file to bin directory
	@cd $(BIN) && ./$(EXECUTABLE) $(IMG_FILE)  # Run from bin directory

# Clean the build and output
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ) $(BIN)

.PHONY: all run clean

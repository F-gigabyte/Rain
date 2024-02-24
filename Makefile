SRC_DIR=src
INCLUDE_DIR=include
BIN_DIR=bin
OBJ_DIR=obj
OBJ_FLAGS=-g -O2 -Iinclude
EXE_FLAGS=
EXE_NAME=rain
CC=gcc

C_SRC = $(shell find $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SRC))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(OBJ_FLAGS) -c $< -o $@

$(BIN_DIR)/$(EXE_NAME): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(EXE_FLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -Rf $(OBJ_DIR)
	rm -Rf $(BIN_DIR)

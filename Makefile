SRC_DIR=src
INCLUDE_DIR=include
VENDOR_DIR=vendor
BIN_DIR=bin
OBJ_DIR=obj
OBJ_FLAGS=-g -I$(INCLUDE_DIR) -I$(VENDOR_DIR)
EXE_FLAGS=
EXE_NAME=rain
CC=gcc

VENDOR_SRC = $(shell find $(VENDOR_DIR) -type f -name "*.c")
C_SRC = $(shell find $(SRC_DIR) -type f -name "*.c")
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SRC)) $(patsubst $(VENDOR_DIR)/%.c, $(OBJ_DIR)/%.o, $(VENDOR_SRC))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(@D)
	$(CC) $(OBJ_FLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(VENDOR_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(@D)
	$(CC) $(OBJ_FLAGS) -c $< -o $@

$(BIN_DIR)/$(EXE_NAME): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(EXE_FLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -Rf $(OBJ_DIR)
	rm -Rf $(BIN_DIR)

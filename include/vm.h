#ifndef RAIN_VM_H
#define RAIN_VM_H

#include <chunk.h>

typedef struct {
    Chunk* chunk;
    inst_type* ip;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

// initialises virtual machine
void init_vm();
// interprets a chunk
InterpretResult interpret(Chunk* chunk);
// frees virtual machine
void free_vm();

#endif

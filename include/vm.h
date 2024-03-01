#ifndef RAIN_VM_H
#define RAIN_VM_H

#include <chunk.h>
#include <value.h>

#define STACK_MAX (4096 / sizeof(Value))

typedef struct {
    Chunk* chunk;
    inst_type* ip;
    Value stack[STACK_MAX];
    Value* stack_top;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

// initialises virtual machine
void init_vm();
// interprets a chunk
InterpretResult interpret(const char* src);
// push value onto stack
void push(Value value);
// pop value from stack
Value pop();
// frees virtual machine
void free_vm();

#endif

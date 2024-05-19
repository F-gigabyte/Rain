#ifndef RAIN_VM_H
#define RAIN_VM_H

#include <chunk.h>
#include <value.h>
#include <object.h>
#include <hash_table.h>

#define STACK_MAX (32768 / sizeof(Value))

typedef struct {
    Chunk* chunk;
    inst_type* ip;
    Value stack[STACK_MAX];
    Value* stack_base;
    Value* stack_top;
    Value* call_base;
    Value* searched;
    HashTable strings;
    Obj* objects;
    ObjUpvalue* open_upvalues;
    bool running;
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
InterpretResult interpret(const char* src, HashTable* global_names, Chunk* main_chunk);
// push value onto stack
void push(Value value);
// pop value from stack
Value pop();
// frees virtual machine
void free_vm();

#endif

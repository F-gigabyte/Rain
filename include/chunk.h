#ifndef RAIN_CHUNK_H
#define RAIN_CHUNK_H

#include <common.h>
#include <value.h>
#include <lines.h>

typedef uint8_t inst_type;

typedef enum
{
    OP_RETURN,
    OP_CONST_BYTE,
    OP_CONST_SHORT,
    OP_CONST_WORD,
    OP_CONST_LONG,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_NEGATE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NOT,
    OP_BIT_NOT,
    OP_REM,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_SHIFT_LEFT,
    OP_SHIFT_ARITH_RIGHT,
    OP_SHIFT_LOGIC_RIGHT,
    OP_EQL,
    OP_GREATER,
    OP_LESS,
    OP_CAST_INT,
    OP_CAST_FLOAT,
    OP_CAST_STR,
    OP_CAST_BOOL,
    OP_PRINT,
    OP_POP,
    OP_DEFINE_GLOBAL_BYTE,
    OP_DEFINE_GLOBAL_SHORT,
    OP_DEFINE_GLOBAL_WORD,
    OP_DEFINE_GLOBAL_LONG,
    OP_GET_GLOBAL_BYTE,
    OP_GET_GLOBAL_SHORT,
    OP_GET_GLOBAL_WORD,
    OP_GET_GLOBAL_LONG,
    OP_SET_GLOBAL_BYTE,
    OP_SET_GLOBAL_SHORT,
    OP_SET_GLOBAL_WORD,
    OP_SET_GLOBAL_LONG,
    OP_GET_LOCAL_BYTE,
    OP_GET_LOCAL_SHORT,
    OP_GET_LOCAL_WORD,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL_BYTE,
    OP_SET_LOCAL_SHORT,
    OP_SET_LOCAL_WORD,
    OP_SET_LOCAL_LONG,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
} Opcode;

typedef struct
{
    size_t size;
    size_t capacity;
    inst_type* code;
    LineArray line_encoding;
    ValueArray consts;
} Chunk;

// initialises chunk of bytecode
void init_chunk(Chunk* chunk);
// writes to a chunk of bytecode
void write_chunk(Chunk* chunk, inst_type inst, size_t line);
// adds a constant to the value array
size_t add_const(Chunk* chunk, Value value);
// writes a constant instruction to the bytecode
void write_chunk_const(Chunk* chunk, size_t const_index, size_t line);
// writes a define global instruction to the bytecode
void write_chunk_var(Chunk* chunk, size_t const_index, size_t line);
// writes a get global instruction to the bytecode
void write_chunk_get_global_var(Chunk* chunk, size_t const_index, size_t line);
// writes a set global instruction to the bytecode
void write_chunk_set_global_var(Chunk* chunk, size_t const_index, size_t line);
// writes a get local instruction to the bytecode
void write_chunk_get_local_var(Chunk* chunk, size_t const_index, size_t line);
// writes a set local instruction to the bytecode
void write_chunk_set_local_var(Chunk* chunk, size_t const_index, size_t line);
// reads a constant index from the bytecode
size_t read_chunk_const(inst_type* inst, size_t* offset, size_t off_size);
// frees a chunk of bytecode
void free_chunk(Chunk* chunk);

#endif

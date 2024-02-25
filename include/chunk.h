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
    OP_NEGATE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
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
// reads a constant index from the bytecode
size_t read_chunk_const(inst_type* inst, size_t* offset, size_t off_size);
// frees a chunk of bytecode
void free_chunk(Chunk* chunk);

#endif

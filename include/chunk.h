#ifndef RAIN_CHUNK_H
#define RAIN_CHUNK_H

#include <common.h>

typedef uint8_t inst_type;

typedef enum
{
    OP_RETURN,

} Opcode;

typedef struct
{
    size_t size;
    size_t capacity;
    inst_type* code;
} Chunk;

// initialises chunk of bytecode
void init_chunk(Chunk* chunk);
// writes to a chunk of bytecode -> true on success or false on fail
bool write_chunk(Chunk* chunk, inst_type inst);
// frees a chunk of bytecode
void free_chunk(Chunk* chunk);

#endif

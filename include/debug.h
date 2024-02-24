#ifndef RAIN_DEBUG_H
#define RAIN_DEBUG_H

#include <chunk.h>

void disassemble_chunk(Chunk* chunk, const char* name);
size_t disassemble_inst(Chunk* chunk, size_t offset);

#endif

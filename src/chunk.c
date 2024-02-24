#include <chunk.h>
#include <rain_memory.h>

void init_chunk(Chunk* chunk)
{
    chunk->capacity = 0;
    chunk->size = 0;
    chunk->code = NULL;
}

bool write_chunk(Chunk* chunk, inst_type inst)
{
    if(chunk->capacity < chunk->size + 1)
    {
        size_t next_cap = GROW_CAPACITY(chunk->capacity);
        inst_type* new_array = GROW_ARRAY(inst_type, chunk->code, chunk->capacity, next_cap);
        if(new_array == NULL)
        {
            return false;
        }
        chunk->code = new_array;
        chunk->capacity = next_cap;
    }
    chunk->code[chunk->size] = inst;
    chunk->size++;
    return true;
}

void free_chunk(Chunk* chunk)
{
    FREE_ARRAY(inst_type, chunk->code, chunk->capacity);
    init_chunk(chunk);
}

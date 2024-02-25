#include <chunk.h>
#include <rain_memory.h>

void init_chunk(Chunk* chunk)
{
    chunk->capacity = 0;
    chunk->size = 0;
    chunk->code = NULL;
    init_line_array(&chunk->line_encoding);
    init_value_array(&chunk->consts);
}

void write_chunk(Chunk* chunk, inst_type inst, size_t line)
{
    if(chunk->capacity < chunk->size + 1)
    {
        size_t next_cap = GROW_CAPACITY(chunk->capacity);
        inst_type* new_array = GROW_ARRAY(inst_type, chunk->code, chunk->capacity, next_cap);
        chunk->code = new_array;
        chunk->capacity = next_cap;
    }
    chunk->code[chunk->size] = inst;
    write_line_array(&chunk->line_encoding, line, chunk->size);
    chunk->size++;
}

size_t add_const(Chunk* chunk, Value value)
{
    write_value_array(&chunk->consts, value);
    return chunk->consts.size - 1;
}

void write_chunk_const(Chunk* chunk, size_t const_index, size_t line)
{
    size_t inst_limit = 1 << (sizeof(inst_type) * 8);
    size_t inst_shift = sizeof(inst_limit) * 8;
    size_t inst_mask = inst_limit - 1;
    if(const_index < 0x100)
    {
        write_chunk(chunk, OP_CONST_BYTE, line);
        write_chunk(chunk, const_index, line);
    }
    else if(const_index < 0x10000)
    {
        write_chunk(chunk, OP_CONST_SHORT, line);
        size_t shift = 0;
        while(shift < 16)
        {
            write_chunk(chunk, (const_index >> shift) & inst_mask, line);
            shift += inst_shift;
        }
    }
    else if(const_index < 0x100000000)
    {
        write_chunk(chunk, OP_CONST_WORD, line);
        size_t shift = 0;
        while(shift < 32)
        {
            write_chunk(chunk, (const_index >> shift) & inst_mask, line);
            shift += inst_shift;
        }
    }
    else
    {
        write_chunk(chunk, OP_CONST_LONG, line);
        size_t shift = 0;
        while(shift < 64)
        {
            write_chunk(chunk, (const_index >> shift) & inst_mask, line);
            shift += inst_shift;
        }
    }
}

size_t read_chunk_const(Chunk* chunk, size_t* offset, size_t off_size)
{
    size_t inst_size = sizeof(inst_type);
    size_t inst_shift = sizeof(inst_type) * 8;
    size_t index_size = (off_size + inst_size - 1) / inst_size;
    size_t constant = 0;
    for(size_t i = 0; i < index_size; i++)
    {
        constant |= chunk->code[*offset + 1 + i] << (inst_shift * i);
    }
    *offset += index_size;
    return constant;
}

void free_chunk(Chunk* chunk)
{
    FREE_ARRAY(inst_type, chunk->code, chunk->capacity);
    free_line_array(&chunk->line_encoding);
    free_value_array(&chunk->consts);
    init_chunk(chunk);
}

#include <chunk.h>
#include <rain_memory.h>

void init_chunk(Chunk* chunk)
{
    chunk->start_line = 0;
    chunk->capacity = 0;
    chunk->size = 0;
    chunk->entry = 0;
    chunk->code = NULL;
    init_line_array(&chunk->line_encoding);
    init_value_array(&chunk->consts);
    init_value_array(&chunk->globals);
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

static void write_chunk_const_impl(Chunk* chunk, size_t const_index, size_t line, inst_type byte_inst, inst_type short_inst, inst_type word_inst, inst_type long_inst)
{
    size_t inst_limit = 1 << (sizeof(inst_type) * 8);
    size_t inst_shift = sizeof(inst_limit) * 8;
    size_t inst_mask = inst_limit - 1;
    if(const_index < 0x100)
    {
        write_chunk(chunk, byte_inst, line);
        write_chunk(chunk, const_index, line);
    }
    else if(const_index < 0x10000)
    {
        write_chunk(chunk, short_inst, line);
        size_t shift = 0;
        while(shift < 16)
        {
            write_chunk(chunk, (const_index >> shift) & inst_mask, line);
            shift += inst_shift;
        }
    }
    else if(const_index < 0x100000000)
    {
        write_chunk(chunk, word_inst, line);
        size_t shift = 0;
        while(shift < 32)
        {
            write_chunk(chunk, (const_index >> shift) & inst_mask, line);
            shift += inst_shift;
        }
    }
    else
    {
        write_chunk(chunk, long_inst, line);
        size_t shift = 0;
        while(shift < 64)
        {
            write_chunk(chunk, (const_index >> shift) & inst_mask, line);
            shift += inst_shift;
        }
    }
    
}

void write_chunk_const(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_CONST_BYTE, OP_CONST_SHORT, OP_CONST_WORD, OP_CONST_LONG);
}

void write_chunk_get_global_var(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_GET_GLOBAL_BYTE, OP_GET_GLOBAL_SHORT, OP_GET_GLOBAL_WORD, OP_GET_GLOBAL_LONG);
}

void write_chunk_set_global_var(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_SET_GLOBAL_BYTE, OP_SET_GLOBAL_SHORT, OP_SET_GLOBAL_WORD, OP_SET_GLOBAL_LONG);
}
void write_chunk_get_upvalue(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_GET_UPVALUE_BYTE, OP_GET_UPVALUE_SHORT, OP_GET_UPVALUE_WORD, OP_GET_UPVALUE_LONG);
}

void write_chunk_set_upvalue(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_SET_UPVALUE_BYTE, OP_SET_UPVALUE_SHORT, OP_SET_UPVALUE_WORD, OP_SET_UPVALUE_LONG);
}

void write_chunk_get_local_var(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_GET_LOCAL_BYTE, OP_GET_LOCAL_SHORT, OP_GET_LOCAL_WORD, OP_GET_LOCAL_LONG);
}

void write_chunk_set_local_var(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_SET_LOCAL_BYTE, OP_SET_LOCAL_SHORT, OP_SET_LOCAL_WORD, OP_SET_LOCAL_LONG);
}

void write_chunk_closure(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_CLOSURE_BYTE, OP_CLOSURE_SHORT, OP_CLOSURE_WORD, OP_CLOSURE_LONG);
}

void write_chunk_attr(Chunk* chunk, size_t const_index, uint8_t visibility, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_ATTR_BYTE, OP_ATTR_SHORT, OP_ATTR_WORD, OP_ATTR_LONG);
    write_chunk(chunk, visibility, line);
}

void write_chunk_attr_get(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_ATTR_GET_BYTE, OP_ATTR_GET_SHORT, OP_ATTR_GET_WORD, OP_ATTR_GET_LONG);
}

void write_chunk_attr_peek(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_ATTR_PEEK_BYTE, OP_ATTR_PEEK_SHORT, OP_ATTR_PEEK_WORD, OP_ATTR_PEEK_LONG);
}

void write_chunk_attr_set(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_ATTR_SET_BYTE, OP_ATTR_SET_SHORT, OP_ATTR_SET_WORD, OP_ATTR_SET_LONG);
}

void write_chunk_attr_get_this(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_ATTR_GET_THIS_BYTE, OP_ATTR_GET_THIS_SHORT, OP_ATTR_GET_THIS_WORD, OP_ATTR_GET_THIS_LONG);
}

void write_chunk_attr_peek_this(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_ATTR_PEEK_THIS_BYTE, OP_ATTR_PEEK_THIS_SHORT, OP_ATTR_PEEK_THIS_WORD, OP_ATTR_PEEK_THIS_LONG);
}

void write_chunk_attr_set_this(Chunk* chunk, size_t const_index, size_t line)
{
    write_chunk_const_impl(chunk, const_index, line, OP_ATTR_SET_THIS_BYTE, OP_ATTR_SET_THIS_SHORT, OP_ATTR_SET_THIS_WORD, OP_ATTR_SET_THIS_LONG);
}

size_t read_chunk_const(inst_type* inst, size_t* offset, size_t off_size)
{
    size_t inst_size = sizeof(inst_type);
    size_t inst_shift = sizeof(inst_type) * 8;
    size_t index_size = (off_size + inst_size - 1) / inst_size;
    size_t constant = 0;
    for(size_t i = 0; i < index_size; i++)
    {
        constant |= inst[i] << (inst_shift * i);
    }
    *offset = index_size;
    return constant;
}

void pass_chunk_context(Chunk* from, Chunk* to)
{
    free_value_array(&to->consts);
    free_value_array(&to->globals);
    to->consts = from->consts;
    to->globals = from->globals;
    from->consts = (ValueArray){.values = NULL, .capacity = 0, .size = 0};
    from->globals = (ValueArray){.values = NULL, .capacity = 0, .size = 0};
}

void copy_chunk_context(Chunk* from, Chunk* to)
{
    free_value_array(&to->consts);
    free_value_array(&to->globals);
    init_value_array(&to->consts);
    init_value_array(&to->globals);
    for(size_t i = 0; i < from->globals.size; i++)
    {
       write_value_array(&to->globals, from->globals.values[i]); 
    }
    for(size_t i = 0; i < from->consts.size; i++)
    {
        write_value_array(&to->consts, from->consts.values[i]);
    }
}

void free_chunk(Chunk* chunk)
{
    FREE_ARRAY(inst_type, chunk->code, chunk->capacity);
    free_line_array(&chunk->line_encoding);
    free_value_array(&chunk->consts);
    free_value_array(&chunk->globals);
    init_chunk(chunk);
}

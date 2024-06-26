#include <debug.h>
#include <value.h>
#include <stdio.h>
#include <hash_table.h>

void disassemble_chunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    for(size_t off = 0; off < chunk->size;)
    {
        off = disassemble_inst(chunk, off);
    }
}

static size_t simple_inst(const char* name, size_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static size_t const_inst(const char* name, Chunk* chunk, size_t off_size, size_t offset)
{
    size_t inc_offset = 0;
    size_t constant = read_chunk_const(chunk->code + offset + 1, &inc_offset, off_size);
    printf("%-16s %4zu '", name, constant);
    print_value(chunk->consts.values[constant]);
    printf("'\n");
    return offset + inc_offset + 1;
}

static size_t attr_inst(const char* name, Chunk* chunk, size_t off_size, size_t offset)
{
    size_t inc_offset = 0;
    size_t constant = read_chunk_const(chunk->code + offset + 1, &inc_offset, off_size);
    uint8_t visibility = (uint8_t)chunk->code[offset + inc_offset + 1];
    printf("%-16s %4zu '", name, constant);
    print_value(chunk->consts.values[constant]);
    printf("' %c %c %c\n", IS_VAR_METHOD(visibility) ? 'M' : 'F', IS_VAR_PUB(visibility) ? '+' : (IS_VAR_PROT(visibility) ? '*' : (IS_VAR_PRIV(visibility) ? '-' : '?')), IS_VAR_CONST(visibility) ? 'C' : 'V');
    return offset + inc_offset + 2;
}

static size_t index_inst(const char* name, Chunk* chunk, size_t off_size, size_t offset)
{
    size_t inc_offset = 0;
    size_t constant = read_chunk_const(chunk->code + offset + 1, &inc_offset, off_size);
    printf("%-16s %4zu\n", name, constant);
    return offset + inc_offset + 1;
}

static size_t jump_inst(const char* name, int8_t sign, Chunk* chunk, size_t off_size, size_t offset)
{
    uint8_t* data = (uint8_t*)(chunk->code + offset + 1);
    size_t jump = 0;
    for(size_t j = off_size; j > 0; j--)
    {
        jump |= ((size_t)data[j - 1] << ((off_size - j) * 8));
    }
    size_t i = 0;
    for(;i < off_size; i += sizeof(inst_type)){}
    if(sign >= 0)
    {
        printf("%-16s %4zu -> %zu\n", name, offset, offset + 1 + i + jump);
    }
    else
    {
        printf("%-16s %4zu -> %zu\n", name, offset, offset + 1 + i - jump);
    }
    return offset + i + 1;
}

size_t disassemble_inst(Chunk* chunk, size_t offset)
{
    if(offset == chunk->entry)
    {
        printf("->");
    }
    else
    {
        printf("  ");
    }
    printf("%04zu ", offset);
    size_t current_line = get_line_number(&chunk->line_encoding, offset);
    if(offset > 0)
    {
        size_t last_line = get_line_number(&chunk->line_encoding, offset - 1);
        if(last_line == current_line)
        {
            printf("   | ");
        }
        else
        {
            printf("%4zu ", current_line);
        }
    }
    else
    {
        printf("%4zu ", current_line);
    }
    inst_type inst = chunk->code[offset];
    switch(inst)
    {
        case OP_RETURN:
        {
            return simple_inst("OP_RETURN", offset);
        }
        case OP_CONST_BYTE:
        {
            return const_inst("OP_CONST_BYTE", chunk, 1, offset);
        }
        case OP_CONST_SHORT:
        {
            return const_inst("OP_CONST_SHORT", chunk, 2, offset);
        }
        case OP_CONST_WORD:
        {
            return const_inst("OP_CONST_WORD", chunk, 4, offset);
        }
        case OP_CONST_LONG:
        {
            return const_inst("OP_CONST_LONG", chunk, 8, offset);
        }
        case OP_NULL:
        {
            return simple_inst("OP_NULL", offset);
        }
        case OP_TRUE:
        {
            return simple_inst("OP_TRUE", offset);
        }
        case OP_FALSE:
        {
            return simple_inst("OP_FALSE", offset);
        }
        case OP_NEGATE:
        {
            return simple_inst("OP_NEGATE", offset);
        }
        case OP_ADD:
        {
            return simple_inst("OP_ADD", offset);
        }
        case OP_SUB:
        {
            return simple_inst("OP_SUB", offset);
        }
        case OP_MUL:
        {
            return simple_inst("OP_MUL", offset);
        }
        case OP_DIV:
        {
            return simple_inst("OP_DIV", offset);
        }
        case OP_NOT:
        {
            return simple_inst("OP_NOT", offset);
        }
        case OP_REM:
        {
            return simple_inst("OP_REM", offset);
        }
        case OP_BIT_NOT:
        {
            return simple_inst("OP_BIT_NOT", offset);
        }
        case OP_BIT_AND:
        {
            return simple_inst("OP_BIT_AND", offset);
        }
        case OP_BIT_OR:
        {
            return simple_inst("OP_BIT_OR", offset);
        }
        case OP_BIT_XOR:
        {
            return simple_inst("OP_BIT_XOR", offset);
        }
        case OP_SHIFT_LEFT:
        {
            return simple_inst("OP_SHIFT_LEFT", offset);
        }
        case OP_SHIFT_ARITH_RIGHT:
        {
            return simple_inst("OP_SHIFT_ARITH_RIGHT", offset);
        }
        case OP_SHIFT_LOGIC_RIGHT:
        {
            return simple_inst("OP_SHIFT_LOGIC_RIGHT", offset);
        }
        case OP_EQL:
        {
            return simple_inst("OP_EQL", offset);
        }
        case OP_GREATER:
        {
            return simple_inst("OP_GREATER", offset);
        }
        case OP_LESS:
        {
            return simple_inst("OP_LESS", offset);
        }
        case OP_CAST_BOOL:
        {
            return simple_inst("OP_CAST_BOOL", offset);
        }
        case OP_CAST_INT:
        {
            return simple_inst("OP_CAST_INT", offset);
        }
        case OP_CAST_STR:
        {
            return simple_inst("OP_CAST_STR", offset);
        }
        case OP_CAST_FLOAT:
        {
            return simple_inst("OP_CAST_FLOAT", offset);
        }
        case OP_POP:
        {
            return simple_inst("OP_POP", offset);
        }
        case OP_GET_GLOBAL_BYTE:
        {
            return index_inst("OP_GET_GLOBAL_BYTE", chunk, 1, offset);
        }
        case OP_GET_GLOBAL_SHORT:
        {
            return index_inst("OP_GET_GLOBAL_SHORT", chunk, 2, offset);
        }
        case OP_GET_GLOBAL_WORD:
        {
            return index_inst("OP_GET_GLOBAL_WORD", chunk, 4, offset);
        }
        case OP_GET_GLOBAL_LONG:
        {
            return index_inst("OP_GET_GLOBAL_LONG", chunk, 8, offset);
        }
        case OP_SET_GLOBAL_BYTE:
        {
            return index_inst("OP_SET_GLOBAL_BYTE", chunk, 1, offset);
        }
        case OP_SET_GLOBAL_SHORT:
        {
            return index_inst("OP_SET_GLOBAL_SHORT", chunk, 2, offset);
        }
        case OP_SET_GLOBAL_WORD:
        {
            return index_inst("OP_SET_GLOBAL_WORD", chunk, 4, offset);
        }
        case OP_SET_GLOBAL_LONG:
        {
            return index_inst("OP_SET_GLOBAL_LONG", chunk, 8, offset);
        }
        case OP_GET_UPVALUE_BYTE:
        {
            return index_inst("OP_GET_UPVALUE_BYTE", chunk, 1, offset);
        }
        case OP_GET_UPVALUE_SHORT:
        {
            return index_inst("OP_GET_UPVALUE_SHORT", chunk, 2, offset);
        }
        case OP_GET_UPVALUE_WORD:
        {
            return index_inst("OP_GET_UPVALUE_WORD", chunk, 4, offset);
        }
        case OP_GET_UPVALUE_LONG:
        {
            return index_inst("OP_GET_UPVALUE_LONG", chunk, 8, offset);
        }
        case OP_SET_UPVALUE_BYTE:
        {
            return index_inst("OP_GET_UPVALUE_BYTE", chunk, 1, offset);
        }
        case OP_SET_UPVALUE_SHORT:
        {
            return index_inst("OP_GET_UPVALUE_SHORT", chunk, 2, offset);
        }
        case OP_SET_UPVALUE_WORD:
        {
            return index_inst("OP_GET_UPVALUE_WORD", chunk, 4, offset);
        }
        case OP_SET_UPVALUE_LONG:
        {
            return index_inst("OP_GET_UPVALUE_LONG", chunk, 8, offset);
        }
        case OP_GET_LOCAL_BYTE:
        {
            return index_inst("OP_GET_LOCAL_BYTE", chunk, 1, offset);
        }
        case OP_GET_LOCAL_SHORT:
        {
            return index_inst("OP_GET_LOCAL_SHORT", chunk, 2, offset);
        }
        case OP_GET_LOCAL_WORD:
        {
            return index_inst("OP_GET_LOCAL_WORD", chunk, 4, offset);
        }
        case OP_GET_LOCAL_LONG:
        {
            return index_inst("OP_GET_LOCAL_LONG", chunk, 8, offset);
        }
        case OP_SET_LOCAL_BYTE:
        {
            return index_inst("OP_SET_LOCAL_BYTE", chunk, 1, offset);
        }
        case OP_SET_LOCAL_SHORT:
        {
            return index_inst("OP_SET_LOCAL_SHORT", chunk, 2, offset);
        }
        case OP_SET_LOCAL_WORD:
        {
            return index_inst("OP_SET_LOCAL_WORD", chunk, 4, offset);
        }
        case OP_SET_LOCAL_LONG:
        {
            return index_inst("OP_SET_LOCAL_LONG", chunk, 8, offset);
        }
        case OP_JUMP_IF_FALSE_BYTE:
        {
            return jump_inst("OP_JUMP_IF_FALSE_BYTE", 1, chunk, 1, offset);
        }
        case OP_JUMP_IF_FALSE_SHORT:
        {
            return jump_inst("OP_JUMP_IF_FALSE_SHORT", 1, chunk, 2, offset);
        }
        case OP_JUMP_IF_FALSE_WORD:
        {
            return jump_inst("OP_JUMP_IF_FALSE_WORD", 1, chunk, 4, offset);
        }
        case OP_JUMP_IF_FALSE_LONG:
        {
            return jump_inst("OP_JUMP_IF_FALSE_LONG", 1, chunk, 8, offset);
        }
        case OP_JUMP_IF_TRUE_BYTE:
        {
            return jump_inst("OP_JUMP_IF_TRUE_BYTE", 1, chunk, 1, offset);
        }
        case OP_JUMP_IF_TRUE_SHORT:
        {
            return jump_inst("OP_JUMP_IF_TRUE_SHORT", 1, chunk, 2, offset);
        }
        case OP_JUMP_IF_TRUE_WORD:
        {
            return jump_inst("OP_JUMP_IF_TRUE_WORD", 1, chunk, 4, offset);
        }
        case OP_JUMP_IF_TRUE_LONG:
        {
            return jump_inst("OP_JUMP_IF_TRUE_LONG", 1, chunk, 8, offset);
        }
        case OP_JUMP_BYTE:
        {
            return jump_inst("OP_JUMP_BYTE", 1, chunk, 1, offset);
        }
        case OP_JUMP_SHORT:
        {
            return jump_inst("OP_JUMP_SHORT", 1, chunk, 2, offset);
        }
        case OP_JUMP_WORD:
        {
            return jump_inst("OP_JUMP_WORD", 1, chunk, 4, offset);
        }
        case OP_JUMP_LONG:
        {
            return jump_inst("OP_JUMP_LONG", 1, chunk, 8, offset);
        }
        case OP_JUMP_BACK_BYTE:
        {
            return jump_inst("OP_JUMP_BACK_BYTE", -1, chunk, 1, offset);
        }
        case OP_JUMP_BACK_SHORT:
        {
            return jump_inst("OP_JUMP_BACK_SHORT", -1, chunk, 2, offset);
        }
        case OP_JUMP_BACK_WORD:
        {
            return jump_inst("OP_JUMP_BACK_WORD", -1, chunk, 4, offset);
        }
        case OP_JUMP_BACK_LONG:
        {
            return jump_inst("OP_JUMP_BACK_LONG", -1, chunk, 8, offset);
        }
        case OP_INIT_ARRAY:
        {
            return simple_inst("OP_INIT_ARRAY", offset);
        }
        case OP_FILL_ARRAY:
        {
            return simple_inst("OP_FILL_ARRAY", offset);
        }
        case OP_INDEX_GET:
        {
            return simple_inst("OP_INDEX_GET", offset);
        }
        case OP_INDEX_SET:
        {
            return simple_inst("OP_INDEX_SET", offset);
        }
        case OP_INDEX_PEEK:
        {
            return simple_inst("OP_INDEX_PEEK", offset);
        }
        case OP_CALL:
        {
           return simple_inst("OP_CALL", offset); 
        }
        case OP_PUSH_CALL_BASE:
        {
            return simple_inst("OP_PUSH_CALL_BASE", offset);
        }
        case OP_CLOSURE_BYTE:
        {
            return const_inst("OP_CLOSURE_BYTE", chunk, 1, offset);
        }
        case OP_CLOSURE_SHORT:
        {
            return const_inst("OP_CLOSURE_SHORT", chunk, 2, offset);
        }
        case OP_CLOSURE_WORD:
        {
            return const_inst("OP_CLOSURE_WORD", chunk, 4, offset);
        }
        case OP_CLOSURE_LONG:
        {
            return const_inst("OP_CLOSURE_LONG", chunk, 8, offset);
        }
        case OP_CLOSE_UPVALUE:
        {
            return simple_inst("OP_CLOSE_UPVALUE", offset);
        }
        case OP_ATTR_BYTE:
        {
            return attr_inst("OP_ATTR_BYTE", chunk, 1, offset);
        }
        case OP_ATTR_SHORT:
        {
            return attr_inst("OP_ATTR_SHORT", chunk, 2, offset);
        }
        case OP_ATTR_WORD:
        {
            return attr_inst("OP_ATTR_WORD", chunk, 4, offset);
        }
        case OP_ATTR_LONG:
        {
            return attr_inst("OP_ATTR_LONG", chunk, 8, offset);
        }
        case OP_ATTR_GET_BYTE:
        {
           return const_inst("OP_ATTR_GET_BYTE", chunk, 1, offset); 
        }
        case OP_ATTR_GET_SHORT:
        {
           return const_inst("OP_ATTR_GET_SHORT", chunk, 2, offset); 
        }
        case OP_ATTR_GET_WORD:
        {
           return const_inst("OP_ATTR_GET_WORD", chunk, 4, offset); 
        }
        case OP_ATTR_GET_LONG:
        {
           return const_inst("OP_ATTR_GET_LONG", chunk, 8, offset); 
        }
        case OP_ATTR_PEEK_BYTE:
        {
           return const_inst("OP_ATTR_PEEK_BYTE", chunk, 1, offset); 
        }
        case OP_ATTR_PEEK_SHORT:
        {
           return const_inst("OP_ATTR_PEEK_SHORT", chunk, 2, offset); 
        }
        case OP_ATTR_PEEK_WORD:
        {
           return const_inst("OP_ATTR_PEEK_WORD", chunk, 4, offset); 
        }
        case OP_ATTR_PEEK_LONG:
        {
           return const_inst("OP_ATTR_PEEK_LONG", chunk, 8, offset); 
        }
        case OP_ATTR_SET_BYTE:
        {
           return const_inst("OP_ATTR_SET_BYTE", chunk, 1, offset); 
        }
        case OP_ATTR_SET_SHORT:
        {
           return const_inst("OP_ATTR_SET_SHORT", chunk, 2, offset); 
        }
        case OP_ATTR_SET_WORD:
        {
           return const_inst("OP_ATTR_SET_WORD", chunk, 4, offset); 
        }
        case OP_ATTR_SET_LONG:
        {
           return const_inst("OP_ATTR_SET_LONG", chunk, 8, offset); 
        }
        case OP_ATTR_GET_THIS_BYTE:
        {
           return const_inst("OP_ATTR_GET_THIS_BYTE", chunk, 1, offset); 
        }
        case OP_ATTR_GET_THIS_SHORT:
        {
           return const_inst("OP_ATTR_GET_THIS_SHORT", chunk, 2, offset); 
        }
        case OP_ATTR_GET_THIS_WORD:
        {
           return const_inst("OP_ATTR_GET_THIS_WORD", chunk, 4, offset); 
        }
        case OP_ATTR_GET_THIS_LONG:
        {
           return const_inst("OP_ATTR_GET_THIS_LONG", chunk, 8, offset); 
        }
        case OP_ATTR_PEEK_THIS_BYTE:
        {
           return const_inst("OP_ATTR_PEEK_THIS_BYTE", chunk, 1, offset); 
        }
        case OP_ATTR_PEEK_THIS_SHORT:
        {
           return const_inst("OP_ATTR_PEEK_THIS_SHORT", chunk, 2, offset); 
        }
        case OP_ATTR_PEEK_THIS_WORD:
        {
           return const_inst("OP_ATTR_PEEK_THIS_WORD", chunk, 4, offset); 
        }
        case OP_ATTR_PEEK_THIS_LONG:
        {
           return const_inst("OP_ATTR_PEEK_THIS_LONG", chunk, 8, offset); 
        }
        case OP_ATTR_SET_THIS_BYTE:
        {
           return const_inst("OP_ATTR_SET_THIS_BYTE", chunk, 1, offset); 
        }
        case OP_ATTR_SET_THIS_SHORT:
        {
           return const_inst("OP_ATTR_SET_THIS_SHORT", chunk, 2, offset); 
        }
        case OP_ATTR_SET_THIS_WORD:
        {
           return const_inst("OP_ATTR_SET_THIS_WORD", chunk, 4, offset); 
        }
        case OP_ATTR_SET_THIS_LONG:
        {
           return const_inst("OP_ATTR_SET_THIS_LONG", chunk, 8, offset); 
        }
        case OP_EXIT:
        {
            return simple_inst("OP_EXIT", offset);
        }
        default:
        {
            printf("Unknown opcode %d\n", inst);
            return offset + 1;
        }
    }
}

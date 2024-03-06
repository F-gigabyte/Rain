#include <debug.h>
#include <value.h>
#include <stdio.h>

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

size_t disassemble_inst(Chunk* chunk, size_t offset)
{
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
        case OP_PRINT:
        {
            return simple_inst("OP_PRINT", offset);
        }
        case OP_POP:
        {
            return simple_inst("OP_POP", offset);
        }
        default:
        {
            printf("Unknown opcode %d\n", inst);
            return offset + 1;
        }
    }
}

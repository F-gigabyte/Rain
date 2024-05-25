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
    OP_POP,
    OP_GET_GLOBAL_BYTE,
    OP_GET_GLOBAL_SHORT,
    OP_GET_GLOBAL_WORD,
    OP_GET_GLOBAL_LONG,
    OP_SET_GLOBAL_BYTE,
    OP_SET_GLOBAL_SHORT,
    OP_SET_GLOBAL_WORD,
    OP_SET_GLOBAL_LONG,
    OP_GET_UPVALUE_BYTE,
    OP_GET_UPVALUE_SHORT,
    OP_GET_UPVALUE_WORD,
    OP_GET_UPVALUE_LONG,
    OP_SET_UPVALUE_BYTE,
    OP_SET_UPVALUE_SHORT,
    OP_SET_UPVALUE_WORD,
    OP_SET_UPVALUE_LONG,
    OP_GET_LOCAL_BYTE,
    OP_GET_LOCAL_SHORT,
    OP_GET_LOCAL_WORD,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL_BYTE,
    OP_SET_LOCAL_SHORT,
    OP_SET_LOCAL_WORD,
    OP_SET_LOCAL_LONG,
    OP_JUMP_IF_FALSE_BYTE,
    OP_JUMP_IF_FALSE_SHORT,
    OP_JUMP_IF_FALSE_WORD,
    OP_JUMP_IF_FALSE_LONG,
    OP_JUMP_IF_TRUE_BYTE,
    OP_JUMP_IF_TRUE_SHORT,
    OP_JUMP_IF_TRUE_WORD,
    OP_JUMP_IF_TRUE_LONG,
    OP_JUMP_BYTE,
    OP_JUMP_SHORT,
    OP_JUMP_WORD,
    OP_JUMP_LONG,
    OP_JUMP_BACK_BYTE,
    OP_JUMP_BACK_SHORT,
    OP_JUMP_BACK_WORD,
    OP_JUMP_BACK_LONG,
    OP_INIT_ARRAY,
    OP_FILL_ARRAY,
    OP_INDEX_GET,
    OP_INDEX_PEEK,
    OP_INDEX_SET,
    OP_CALL,
    OP_PUSH_CALL_BASE,
    OP_CLOSURE_BYTE,
    OP_CLOSURE_SHORT,
    OP_CLOSURE_WORD,
    OP_CLOSURE_LONG,
    OP_CLOSE_UPVALUE,
    OP_ATTR_BYTE,
    OP_ATTR_SHORT,
    OP_ATTR_WORD,
    OP_ATTR_LONG,
    OP_ATTR_GET_BYTE,
    OP_ATTR_GET_SHORT,
    OP_ATTR_GET_WORD,
    OP_ATTR_GET_LONG,
    OP_ATTR_PEEK_BYTE,
    OP_ATTR_PEEK_SHORT,
    OP_ATTR_PEEK_WORD,
    OP_ATTR_PEEK_LONG,
    OP_ATTR_SET_BYTE,
    OP_ATTR_SET_SHORT,
    OP_ATTR_SET_WORD,
    OP_ATTR_SET_LONG,
    OP_ATTR_GET_THIS_BYTE,
    OP_ATTR_GET_THIS_SHORT,
    OP_ATTR_GET_THIS_WORD,
    OP_ATTR_GET_THIS_LONG,
    OP_ATTR_PEEK_THIS_BYTE,
    OP_ATTR_PEEK_THIS_SHORT,
    OP_ATTR_PEEK_THIS_WORD,
    OP_ATTR_PEEK_THIS_LONG,
    OP_ATTR_SET_THIS_BYTE,
    OP_ATTR_SET_THIS_SHORT,
    OP_ATTR_SET_THIS_WORD,
    OP_ATTR_SET_THIS_LONG,
    OP_EXIT,
} Opcode;

typedef struct
{
    size_t start_line;
    size_t size;
    size_t capacity;
    size_t entry;
    inst_type* code;
    LineArray line_encoding;
    ValueArray consts;
    ValueArray globals;
} Chunk;

// initialises chunk of bytecode
void init_chunk(Chunk* chunk);
// writes to a chunk of bytecode
void write_chunk(Chunk* chunk, inst_type inst, size_t line);
// adds a constant to the value array
size_t add_const(Chunk* chunk, Value value);
// writes a constant instruction to the bytecode
void write_chunk_const(Chunk* chunk, size_t const_index, size_t line);
// writes a get global instruction to the bytecode
void write_chunk_get_global_var(Chunk* chunk, size_t const_index, size_t line);
// writes a set global instruction to the bytecode
void write_chunk_set_global_var(Chunk* chunk, size_t const_index, size_t line);
// writes a get upvalue instruction to the bytecode
void write_chunk_get_upvalue(Chunk* chunk, size_t const_index, size_t line);
// writes a set upvalue instruction to the bytecode
void write_chunk_set_upvalue(Chunk* chunk, size_t const_index, size_t line);
// writes a get local instruction to the bytecode
void write_chunk_get_local_var(Chunk* chunk, size_t const_index, size_t line);
// writes a set local instruction to the bytecode
void write_chunk_set_local_var(Chunk* chunk, size_t const_index, size_t line);
// writes a closure instruction to the bytecode
void write_chunk_closure(Chunk* chunk, size_t const_index, size_t line);
// writes an attribute instruction to the bytecode
void write_chunk_attr(Chunk* chunk, size_t const_index, uint8_t visibility, size_t line);
// writes an attribute get instruction to the bytecode
void write_chunk_attr_get(Chunk* chunk, size_t const_index, size_t line);
// writes an attribute peek instruction to the bytecode
void write_chunk_attr_peek(Chunk* chunk, size_t const_index, size_t line);
// writes an attribute set instruction to the bytecode
void write_chunk_attr_set(Chunk* chunk, size_t const_index, size_t line);
// writes an attribute get this instruction to the bytecode
void write_chunk_attr_get_this(Chunk* chunk, size_t const_index, size_t line);
// writes an attribute peek this instruction to the bytecode
void write_chunk_attr_peek_this(Chunk* chunk, size_t const_index, size_t line);
// writes an attribute set this instruction to the bytecode
void write_chunk_attr_set_this(Chunk* chunk, size_t const_index, size_t line);
// reads a constant index from the bytecode
size_t read_chunk_const(inst_type* inst, size_t* offset, size_t off_size);
// frees a chunk of bytecode
void free_chunk(Chunk* chunk);
// passes global variables and constants between chunks
void pass_chunk_context(Chunk* from, Chunk* to);
// copies global variables and constants between chunks
void copy_chunk_context(Chunk* from, Chunk* to);
#endif

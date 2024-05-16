#include <compiler.h>
#include <common.h>
#include <scanner.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <object.h>
#include <convert.h>
#include <rain_memory.h>
#include <string.h>
#include <natives.h>

#ifdef DEBUG_PRINT_CODE
#include <debug.h>
#endif

#define MAXOFFSET 0xffffffff

#ifdef DEBUG_TOKEN_TYPES

static const char* token_type_str(TokenType type)
{
    switch(type)
    {
        case TOKEN_LEFT_PAREN:
        {
            return "LEFT_PAREN";
        }
        case TOKEN_RIGHT_PAREN:
        {
            return "RIGHT_PAREN";
        }
        case TOKEN_LEFT_BRACE:
        {
            return "LEFT_BRACE";
        }
        case TOKEN_RIGHT_BRACE:
        {
            return "RIGHT_BRACE";
        }
        case TOKEN_COMMA:
        {
            return "COMMA";
        }
        case TOKEN_DOT:
        {
            return "DOT";
        }
        case TOKEN_SEMICOLON:
        {
            return "SEMICOLON";
        }
        case TOKEN_LEFT_SQR:
        {
            return "LEFT_SQR";
        }
        case TOKEN_RIGHT_SQR:
        {
            return "RIGHT_SQR";
        }
        case TOKEN_MINUS:
        {
            return "MINUS";
        }
        case TOKEN_PLUS:
        {
            return "PLUS";
        }
        case TOKEN_STAR:
        {
            return "STAR";
        }
        case TOKEN_SLASH:
        {
            return "SLASH";
        }
        case TOKEN_PERC:
        {
            return "PERC";
        }
        case TOKEN_AMP:
        {
            return "AMP";
        }
        case TOKEN_LINE:
        {
            return "LINE";
        }
        case TOKEN_UP:
        {
            return "UP";
        }
        case TOKEN_BANG:
        {
            return "BANG";
        }
        case TOKEN_BANG_EQL:
        {
            return "BANG_EQL";
        }
        case TOKEN_EQL:
        {
            return "EQL";
        }
        case TOKEN_EQL_EQL:
        {
            return "EQL_EQL";
        }
        case TOKEN_GREATER:
        {
            return "GREATER";
        }
        case TOKEN_GREATER_EQL:
        {
            return "GREATER_EQL";
        }
        case TOKEN_LESS:
        {
            return "LESS";
        }
        case TOKEN_LESS_EQL:
        {
            return "LESS_EQL";
        }
        case TOKEN_LESS_LESS:
        {
            return "LESS_LESS";
        }
        case TOKEN_GREATER_GREATER:
        {
            return "GREATER_GREATER";
        }
        case TOKEN_MINUS_MINUS:
        {
            return "MINUS_MINUS";
        }
        case TOKEN_PLUS_PLUS:
        {
            return "PLUS_PLUS";
        }
        case TOKEN_PLUS_EQL:
        {
            return "PLUS_EQL";
        }
        case TOKEN_MINUS_EQL:
        {
            return "MINUS_EQL";
        }
        case TOKEN_STAR_EQL:
        {
            return "STAR_EQL";
        }
        case TOKEN_SLASH_EQL:
        {
            return "SLASH_EQL";
        }
        case TOKEN_AMP_EQL:
        {
            return "AMP_EQL";
        }
        case TOKEN_LINE_EQL:
        {
            return "LINE_EQL";
        }
        case TOKEN_UP_EQL:
        {
            return "UP_EQL";
        }
        case TOKEN_PERC_EQL:
        {
            return "PERC_EQL";
        }
        case TOKEN_GREATER_GREATER_EQL:
        {
            return "GREATER_GREATER_EQL";
        }
        case TOKEN_LESS_LESS_EQL:
        {
            return "LESS_LESS_EQL";
        }
        case TOKEN_IDENT:
        {
            return "IDENT";
        }
        case TOKEN_STR_START:
        {
            return "STR_START";
        }
        case TOKEN_STR_BODY:
        {
            return "STR_BODY";
        }
        case TOKEN_STR_END:
        {
            return "STR_END";
        }
        case TOKEN_INTERP_START:
        {
            return "INTERP_START";
        }
        case TOKEN_INTERP_END:
        {
            return "INTERP_END";
        }
        case TOKEN_INT:
        {
            return "INT";
        }
        case TOKEN_INT_HEX:
        {
            return "INT_HEX";
        }
        case TOKEN_INT_BIN:
        {
            return "INT_BIN";
        }
        case TOKEN_INT_OCT:
        {
            return "INT_OCT";
        }
        case TOKEN_FLOAT:
        {
            return "FLOAT";
        }
        case TOKEN_AND:
        {
            return "AND";
        }
        case TOKEN_ARRAY:
        {
            return "ARRAY";
        }
        case TOKEN_BOOL_CAST:
        {
            return "BOOL_CAST";
        }
        case TOKEN_CLASS:
        {
            return "CLASS";
        }
        case TOKEN_CONST:
        {
            return "CONST";
        }
        case TOKEN_ELSE:
        {
            return "ELSE";
        }
        case TOKEN_FALSE:
        {
            return "FALSE";
        }
        case TOKEN_FLOAT_CAST:
        {
            return "FLOAT_CAST";
        }
        case TOKEN_FOR:
        {
            return "FOR";
        }
        case TOKEN_FUNC:
        {
            return "FUNC";
        }
        case TOKEN_IF:
        {
            return "IF";
        }
        case TOKEN_IN:
        {
            return "IN";
        }
        case TOKEN_INT_CAST:
        {
            return "INT_CAST";
        }
        case TOKEN_IMPORT:
        {
            return "IMPORT";
        }
        case TOKEN_NOT:
        {
            return "NOT";
        }
        case TOKEN_NULL:
        {
            return "NULL";
        }
        case TOKEN_OR:
        {
            return "OR";
        }
        case TOKEN_OVERRIDE:
        {
            return "OVERRIDE";
        }
        case TOKEN_PRINT:
        {
            return "PRINT";
        }
        case TOKEN_PRIV:
        {
            return "PRIV";
        }
        case TOKEN_PROT:
        {
            return "PROT";
        }
        case TOKEN_PUB:
        {
            return "PUB";
        }
        case TOKEN_RET:
        {
            return "RET";
        }
        case TOKEN_SUPER:
        {
            return "SUPER";
        }
        case TOKEN_STR_CAST:
        {
            return "STR_CAST";
        }
        case TOKEN_THIS:
        {
            return "THIS";
        }
        case TOKEN_TRUE:
        {
            return "TRUE";
        }
        case TOKEN_VAR:
        {
            return "VAR";
        }
        case TOKEN_VIRTUAL:
        {
            return "VIRTUAL";
        }
        case TOKEN_WHILE:
        {
            return "WHILE";
        }
        case TOKEN_ERROR:
        {
            return "ERROR";
        }
        case TOKEN_EOF:
        {
            return "EOF";
        }
        default:
        {
            return "UNKNOWN";
        }
    }
}

#endif

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR, // or
    PREC_AND, // and
    PREC_EQL, // == !=
    PREC_COMP, // < > <= >=
    PREC_SHIFT, // << >>
    PREC_BIT_OR, // | ^
    PREC_BIT_AND, // &
    PREC_TERM, // + -
    PREC_FACTOR, // * / %
    PREC_UNARY, // - ! not
    PREC_CALL, // . ()
    PREC_PRIMARY,
} Precedence;

typedef void(*ParseFn)(bool assignable);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence prec;
} ParseRule;

typedef struct {
    Token name;
    bool constant;
    size_t depth;
} Local;

typedef enum
{
    JUMP_FORWARD,
    JUMP_BACKWARD,
} JumpType;

typedef struct {
    size_t from;
    size_t to;
    size_t bytes;
    JumpType type;
} JumpPair;

typedef struct {
    Local* locals;
    size_t local_size;
    size_t local_capacity;
    size_t scope_depth;
    JumpPair* jump_table;
    size_t jump_table_size;
    size_t jump_table_capacity;
    ObjFunc** func_table;
    size_t func_table_size;
    size_t func_table_capacity;
    HashTable globals;
} Compiler;

Parser parser;
Compiler* current = NULL;

Chunk* compiling_chunk;

static void var_declaration(bool constant);

ParseRule rules[];

static Chunk* current_chunk()
{
    return compiling_chunk;
}

static size_t get_next_line()
{
    return current_chunk()->size + current_chunk()->start_line;
}

static void add_jump(size_t from, size_t to, JumpType type)
{
    if(current->jump_table_size >= current->jump_table_capacity)
    {
        size_t new_cap = GROW_CAPACITY(current->jump_table_capacity);
        JumpPair* next_table = GROW_ARRAY(JumpPair, current->jump_table, current->jump_table_capacity, new_cap);
        current->jump_table_capacity = new_cap;
        current->jump_table = next_table;
    }
    current->jump_table[current->jump_table_size] = (JumpPair){.from = from, .to = to, .bytes = 0, .type = type};
    current->jump_table_size++;
}

static void add_func(ObjFunc* func)
{
    if(current->func_table_size >= current->func_table_capacity)
    {
        size_t new_cap = GROW_CAPACITY(current->func_table_capacity);
        ObjFunc** next_table = GROW_ARRAY(ObjFunc*, current->func_table, current->func_table_capacity, new_cap);
        current->func_table_capacity = new_cap;
        current->func_table = next_table;
    }
    current->func_table[current->func_table_size] = func;
    current->func_table_size++;
}

static void error_at(Token* token, const char* msg)
{
    if(parser.panic_mode)
    {
        return;
    }
    parser.panic_mode = true;
    fprintf(stderr, "[line %zu] Error", token->line);
    if(token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if(token->type == TOKEN_ERROR)
    {

    }
    else
    {
        fprintf(stderr, " at '%.*s'", (int)token->len, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser.had_error = true;
}

// just consumed
static void error(const char* msg)
{
    error_at(&parser.previous, msg);
}

// consuming (nom)
static void error_at_current(const char* msg)
{
    error_at(&parser.current, msg);
}

static void init_compiler(Compiler* compiler)
{
    compiler->local_size = 0;
    compiler->local_capacity = 0;
    compiler->locals = NULL;
    compiler->scope_depth = 0;
    compiler->jump_table = NULL;
    compiler->jump_table_capacity = 0;
    compiler->jump_table_size = 0;
    compiler->func_table = NULL;
    compiler->func_table_capacity = 0;
    compiler->func_table_size = 0;
    init_hash_table(&compiler->globals);
    current = compiler;
}

static void advance()
{
    parser.previous = parser.current;
    for(;;)
    {
        parser.current = scan_token();
        if(parser.current.type != TOKEN_ERROR)
        {
            break;
        }
        error_at_current(parser.current.start);
    }
}

// check token right type
static void consume(TokenType type, const char* msg)
{
    if(parser.current.type == type)
    {
        advance();
        return;
    }
    error_at_current(msg);
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if(!check(type))
    {
        return false;
    }
    advance();
    return true;
}

static void emit_inst(inst_type inst)
{
    write_chunk(current_chunk(), inst, parser.previous.line);
}

static void emit_inst_line(inst_type inst, size_t line)
{
    write_chunk(current_chunk(), inst, line);
}

static size_t make_const(Value value)
{
    size_t constant = add_const(current_chunk(), value);
    return constant;
}

static void emit_const(Value value)
{
    write_chunk_const(current_chunk(), make_const(value), parser.previous.line);
}

static size_t reserve_const()
{
    size_t index = make_const(NULL_VAL);
    write_chunk_const(current_chunk(), index, parser.previous.line);
    return index;
}

static void emit_insts(inst_type inst1, inst_type inst2)
{
    emit_inst(inst1);
    emit_inst(inst2);
}

static void emit_return()
{
    emit_const(NULL_VAL);
    emit_inst(OP_POP_BASE);
    emit_inst(OP_RETURN);
}

static void emit_exit()
{
    emit_inst(OP_EXIT);
}

static size_t emit_jump(inst_type type)
{
    emit_inst(type);
    size_t index = current->jump_table_size;
    add_jump(current_chunk()->size - 1, 0xffffffff, JUMP_FORWARD);
    return index;
}

static void patch_jump(size_t index)
{
    current->jump_table[index].to = current_chunk()->size;
}

static void emit_loop(size_t loop_start)
{
    emit_inst(OP_JUMP_BACK_BYTE);
    add_jump(current_chunk()->size - 1, loop_start, JUMP_BACKWARD);
}

static void end_compiler()
{
    emit_exit();
    FREE(JumpPair, current->jump_table);
    current->jump_table = NULL;
    current->jump_table_capacity = 0;
    current->jump_table_size = 0;
    FREE(ObjFunc*, current->func_table);
    current->func_table = NULL;
    current->func_table_capacity = 0;
    current->func_table_size = 0;
    free_hash_table(&current->globals);
#ifdef DEBUG_PRINT_CODE
    if(!parser.had_error)
    {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void emit_get_var(Value value, bool global)
{
    if(global)
    {
        write_chunk_get_global_var(current_chunk(), (size_t)AS_INT(value), parser.previous.line);
    }
    else
    {
        write_chunk_get_local_var(current_chunk(), (size_t)AS_INT(value), parser.previous.line);
    }
}

static void emit_set_var(Value value, bool global)
{
    if(global)
    {
        write_chunk_set_global_var(current_chunk(), (size_t)AS_INT(value), parser.previous.line);
    }
    else
    {
        write_chunk_set_local_var(current_chunk(), (size_t)AS_INT(value), parser.previous.line);
    }
}

static void expression();
static void statement(bool in_func);
static void declaration(bool in_func);

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

static void parse_precedence(Precedence precedence)
{
    advance();
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if(prefix_rule == NULL)
    {
        error("Expected expression");
        return;
    }
    bool assignable = precedence <= PREC_ASSIGNMENT;
    prefix_rule(assignable);
    while(precedence <= get_rule(parser.current.type)->prec)
    {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(assignable);
    }
    if(assignable && match(TOKEN_EQL))
    {
        error("Invalid assignment target");
    }
}

static void binary(bool assignable)
{
    TokenType oper_type = parser.previous.type;
    ParseRule* rule = get_rule(oper_type);
    parse_precedence((Precedence)(rule->prec + 1));
    switch(oper_type)
    {
        case TOKEN_PLUS:
        {
            emit_inst(OP_ADD);
            break;
        }
        case TOKEN_MINUS:
        {
            emit_inst(OP_SUB);
            break;
        }
        case TOKEN_STAR:
        {
            emit_inst(OP_MUL);
            break;
        }
        case TOKEN_SLASH:
        {
            emit_inst(OP_DIV);
            break;
        }
        case TOKEN_PERC:
        {
            emit_inst(OP_REM);
            break;
        }
        case TOKEN_AMP:
        {
            emit_inst(OP_BIT_AND);
            break;
        }
        case TOKEN_LINE:
        {
            emit_inst(OP_BIT_OR);
            break;
        }
        case TOKEN_UP:
        {
            emit_inst(OP_BIT_XOR);
            break;
        }
        case TOKEN_LESS_LESS_LESS:
        case TOKEN_LESS_LESS:
        {
            emit_inst(OP_SHIFT_LEFT);
            break;
        }
        case TOKEN_GREATER_GREATER:
        {
            emit_inst(OP_SHIFT_ARITH_RIGHT);
            break;
        }
        case TOKEN_GREATER_GREATER_GREATER:
        {
            emit_inst(OP_SHIFT_LOGIC_RIGHT);
            break;
        }
        case TOKEN_BANG_EQL:
        {
            emit_insts(OP_EQL, OP_NOT);
            break;
        }
        case TOKEN_EQL_EQL:
        {
            emit_inst(OP_EQL);
            break;
        }
        case TOKEN_GREATER:
        {
            emit_inst(OP_GREATER);
            break;
        }
        case TOKEN_GREATER_EQL:
        {
            emit_insts(OP_LESS, OP_NOT);
            break;
        }
        case TOKEN_LESS:
        {
            emit_inst(OP_LESS);
            break;
        }
        case TOKEN_LESS_EQL:
        {
            emit_insts(OP_GREATER, OP_NOT);
            break;
        }
        default:
        {
            return;
        }
    }
}

static size_t push_arguments()
{
    size_t args = 0;
    if(!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            expression();
            args++;
        } while(match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
    return args;
}

static void call(bool assignable)
{
    emit_inst(OP_PUSH_BASE);
    size_t inputs = push_arguments();
    emit_inst(OP_CALL);
}

static void unary(bool assignable)
{
    TokenType oper_type = parser.previous.type;
    parse_precedence(PREC_UNARY);
    switch(oper_type)
    {
        case TOKEN_MINUS:
        {
            emit_inst(OP_NEGATE);
            break;
        }
        case TOKEN_NOT:
        {
            emit_inst(OP_NOT);
            break;
        }
        case TOKEN_BANG:
        {
            emit_inst(OP_BIT_NOT);
            break;
        }
        default:
        {
            return;
        }
    }
}

static void grouping(bool assignable)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

static void reserve_array(bool assignable)
{
    consume(TOKEN_LEFT_SQR, "Expect '[' after 'array'");
    if(!(match(TOKEN_INT) || match(TOKEN_INT_HEX) || match(TOKEN_INT_BIN) || match(TOKEN_INT_OCT)))
    {
        error("Expect array size after '['");
        return;
    }
    int64_t len = 0;
    if(!str_to_int(&len, parser.previous.start, parser.previous.len))
    {
        error("Integer is too large");
        return;
    }
    if(len <= 0)
    {
        error("Array size must be larger than 0");
        return;
    }
    consume(TOKEN_RIGHT_SQR, "Expect ']' after array size");
    if(match(TOKEN_LEFT_PAREN))
    {
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after array initializer");
        emit_const(INT_VAL(len));
        emit_inst(OP_INIT_ARRAY);
    }
    else
    {
        emit_const(OBJ_VAL((Obj*)build_array(len, NULL_VAL)));
    }
}

static void array(bool assignable)
{
    int64_t len = 1;
    expression();
    while(!check(TOKEN_RIGHT_SQR) && !check(TOKEN_EOF))
    {
        len++;
        if(len <= 0)
        {
            error("Array is too large");
            return;
        }
        consume(TOKEN_COMMA, "Expect ',' to separate array values");
        expression();
    }
    consume(TOKEN_RIGHT_SQR, "Unterminated array");
    emit_const(INT_VAL(len));
    emit_inst(OP_FILL_ARRAY);
}

static void number(bool assignable)
{
    switch(parser.previous.type)
    {
        case TOKEN_INT:
        case TOKEN_INT_HEX:
        case TOKEN_INT_BIN:
        case TOKEN_INT_OCT:
        {
            int64_t value = 0;
            if(!str_to_int(&value, parser.previous.start, parser.previous.len))
            {
                error("Integer is too large");
                return;
            }
            emit_const(INT_VAL(value));
            break;
        }
        case TOKEN_FLOAT:
        {
            double value = 0;
            if(!str_to_float(&value, parser.previous.start, parser.previous.len))
            {
                error("Float is too large");
                return;
            }
            emit_const(FLOAT_VAL(value));
            break;
        }
        default:
        {
            error("Unknown number type");
            break;
        }
    }
}

static void literal(bool assignable)
{
    switch(parser.previous.type)
    {
        case TOKEN_FALSE:
        {
            emit_inst(OP_FALSE);
            break;
        }
        case TOKEN_NULL:
        {
            emit_inst(OP_NULL);
            break;
        }
        case TOKEN_TRUE:
        {
            emit_inst(OP_TRUE);
            break;
        }
        default:
        {
            error("Unknown literal type");
            break;
        }
    }
}

static void cast(bool assignable)
{
    TokenType type = parser.previous.type;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after cast");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
    switch(type)
    {
        case TOKEN_INT_CAST:
        {
            emit_inst(OP_CAST_INT);
            break;
        }
        case TOKEN_FLOAT_CAST:
        {
            emit_inst(OP_CAST_FLOAT);
            break;
        }
        case TOKEN_STR_CAST:
        {
            emit_inst(OP_CAST_STR);
            break;
        }
        case TOKEN_BOOL_CAST:
        {
            emit_inst(OP_CAST_BOOL);
            break;
        }
        default:
        {
            error("Unknown cast type");
            break;
        }
    }
}

// TOKEN_STR_START ((TOKEN_INTERP_START expr TOKEN_INTERP_END) | TOKEN_STR_BODY)* TOKEN_STR_END
static void string(bool assignable)
{
    advance();
    bool first = true;
    while(parser.previous.type != TOKEN_STR_END && parser.previous.type != TOKEN_EOF)
    {
        switch(parser.previous.type)
        {
            case TOKEN_STR_BODY:
            {
                emit_const(OBJ_VAL((Obj*)copy_str(parser.previous.start, parser.previous.len)));
                if(!first)
                {
                    emit_inst(OP_ADD);
                }
                first = false;
                break;
            }
            case TOKEN_INTERP_START:
            {
                expression();
                consume(TOKEN_INTERP_END, "Expect '}' after string interpolation");
                emit_inst(OP_CAST_STR);
                if(!first)
                {
                    emit_inst(OP_ADD);
                }
                first = false;
                break;
            }
            default:
            {
                error("Unknown token in string");
                break;
            }
        }
        advance();
    }
}

static Value ident_constant(Token* name);
static Value resolve_local(Compiler* compiler, Token* name);

#define EMIT_ASSIGNMENT(get_var) \
{ \
    if(match(TOKEN_EQL)) \
    { \
        expression(); \
    } \
    else if(match(TOKEN_PLUS_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_ADD); \
    } \
    else if(match(TOKEN_MINUS_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_SUB); \
    } \
    else if(match(TOKEN_STAR_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_MUL); \
    } \
    else if(match(TOKEN_SLASH_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_DIV); \
    } \
    else if(match(TOKEN_PERC_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_REM); \
    } \
    else if(match(TOKEN_UP_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_BIT_XOR); \
    } \
    else if(match(TOKEN_AMP_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_BIT_AND); \
    } \
    else if(match(TOKEN_LINE_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_BIT_OR); \
    } \
    else if(match(TOKEN_LESS_LESS_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_SHIFT_LEFT); \
    } \
    else if(match(TOKEN_GREATER_GREATER_EQL)) \
    { \
        get_var; \
        expression(); \
        emit_inst(OP_SHIFT_ARITH_RIGHT); \
        advance(); \
    } \
    else if(match(TOKEN_PLUS_PLUS)) \
    { \
        get_var; \
        emit_const(INT_VAL(1)); \
        emit_inst(OP_ADD); \
    } \
    else if(match(TOKEN_MINUS_MINUS)) \
    { \
        get_var; \
        emit_const(INT_VAL(1)); \
        emit_inst(OP_SUB); \
    } \
}

static void named_variable(Token name, bool assignable)
{
    bool global = false;
    bool constant = false;
    Value arg = resolve_local(current, &name);
    if(IS_NULL(arg))
    {
        global = true;
        Value global_name = ident_constant(&name);
        if(!hash_table_get(&current->globals, AS_STRING(global_name), &arg))
        {
            size_t len = snprintf(NULL, 0, "Undefined variable '%s'", AS_CSTRING(global_name));
            char* buffer = ALLOCATE(char, len + 1);
            snprintf(buffer, len + 1, "Undefined variable '%s'", AS_CSTRING(global_name));
            error(buffer);
            FREE(char, buffer);
            return;
        }
        constant = hash_table_is_const(&current->globals, AS_STRING(global_name)) == 2;
    }
    else
    {
        size_t index = (size_t)AS_INT(arg);
        if(current->locals[index].constant)
        {
            constant = true;
        }
    }
    if(assignable && (check(TOKEN_EQL) || check(TOKEN_PLUS_EQL) || check(TOKEN_MINUS_EQL) || check(TOKEN_STAR_EQL) || check(TOKEN_SLASH_EQL) || check(TOKEN_PERC_EQL) || check(TOKEN_UP_EQL) || check(TOKEN_AMP_EQL) || check(TOKEN_LINE_EQL) || check(TOKEN_LESS_LESS_EQL) || check(TOKEN_GREATER_GREATER_EQL) || check(TOKEN_PLUS_PLUS) || check(TOKEN_MINUS_MINUS)))
    {
        EMIT_ASSIGNMENT(emit_get_var(arg, global));
        emit_set_var(arg, global);
        if(constant)
        {
            size_t len = snprintf(NULL, 0, "Assigning to constant variable '%.*s'", (int)name.len, name.start);
            char* buffer = ALLOCATE(char, len + 1);
            snprintf(buffer, len + 1, "Assigning to constant variable '%.*s'", (int)name.len, name.start);
            error(buffer);
            FREE(char, buffer);
        }
    }
    else
    {
        emit_get_var(arg, global);
        while(match(TOKEN_LEFT_SQR))
        {
            expression();
            consume(TOKEN_RIGHT_SQR, "Expect ']' after index");
            if((check(TOKEN_EQL) || check(TOKEN_PLUS_EQL) || check(TOKEN_MINUS_EQL) || check(TOKEN_STAR_EQL) || check(TOKEN_SLASH_EQL) || check(TOKEN_PERC_EQL) || check(TOKEN_UP_EQL) || check(TOKEN_AMP_EQL) || check(TOKEN_LINE_EQL) || check(TOKEN_LESS_LESS_EQL) || check(TOKEN_GREATER_GREATER_EQL) || check(TOKEN_PLUS_PLUS) || check(TOKEN_MINUS_MINUS)))
            {
                EMIT_ASSIGNMENT(emit_inst(OP_INDEX_PEEK));
                emit_inst(OP_INDEX_SET);
                break;
            }
            else
            {
                emit_inst(OP_INDEX_GET);    
            }
        }    
    }
}

static void variable(bool assignable)
{
    named_variable(parser.previous, assignable);
}

static void and_(bool assignable)
{
    size_t index = emit_jump(OP_JUMP_IF_FALSE_BYTE);
    emit_inst(OP_POP);
    parse_precedence(PREC_AND);
    patch_jump(index);
}

static void or_(bool assignable)
{
    size_t index = emit_jump(OP_JUMP_IF_TRUE_BYTE);
    parse_precedence(PREC_OR);
    patch_jump(index);
    emit_inst(OP_POP);
}

static void expression()
{
    parse_precedence(PREC_ASSIGNMENT);
}

static void block(bool in_func)
{
    while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration(in_func);
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block");
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope()
{
    current->scope_depth--;
    while(current->local_size > 0 && current->locals[current->local_size - 1].depth > current->scope_depth + 1)
    {
        emit_inst(OP_POP);
        current->local_size--;
    }
}

static void end_func_scope()
{
    current->scope_depth--;
    while(current->local_size > 0 && current->locals[current->local_size - 1].depth > current->scope_depth + 1)
    {
        current->local_size--;
    }
}

static void synchronise()
{
    parser.panic_mode = false;
    while(parser.current.type != TOKEN_EOF)
    {
        if(parser.previous.type == TOKEN_SEMICOLON)
        {
            return;
        }
        switch (parser.current.type)
        {
            case TOKEN_CLASS:
            case TOKEN_FUNC:
            case TOKEN_VAR:
            case TOKEN_CONST:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_PRIV:
            case TOKEN_PROT:
            case TOKEN_PUB:
            case TOKEN_RET:
            case TOKEN_VIRTUAL:
            {
                return;
            }
            default:
            {
                break;
            }
        }
        advance();
    }
}

static void print_statement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after print");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after print expression");
    consume(TOKEN_SEMICOLON, "Expect ';' after statement");
    emit_inst(OP_PRINT);
}

static void if_statement(bool in_func)
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");
    consume(TOKEN_LEFT_BRACE, "Expect '{' after condition expression");
    size_t then_jump = emit_jump(OP_JUMP_IF_FALSE_BYTE);
    emit_inst(OP_POP);
    begin_scope();
    block(in_func);
    end_scope();
    if(match(TOKEN_ELSE))
    {
        size_t else_jump = emit_jump(OP_JUMP_BYTE);
        patch_jump(then_jump);
        emit_inst(OP_POP);
        if(match(TOKEN_IF))
        {
            if_statement(in_func);
        }
        else
        {
            consume(TOKEN_LEFT_BRACE, "Expect '{' after else");
            begin_scope();
            block(in_func);
            end_scope();
        }
        patch_jump(else_jump);
    }
    else
    {
        patch_jump(then_jump);
        emit_inst(OP_POP);
    }
}

static void while_statement(bool in_func)
{
    size_t loop_start = current_chunk()->size;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");
    consume(TOKEN_LEFT_BRACE, "Expect '{' after loop expression");
    size_t while_jump = emit_jump(OP_JUMP_IF_FALSE_BYTE);
    emit_inst(OP_POP);
    begin_scope();
    block(in_func);
    emit_loop(loop_start);
    end_scope();
    patch_jump(while_jump);
    emit_inst(OP_POP);
}

static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression");
    emit_inst(OP_POP);
}

static void for_statement(bool in_func)
{
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'");
    if(match(TOKEN_SEMICOLON))
    {

    }
    else if(match(TOKEN_VAR))
    {
        var_declaration(false);
    }
    else if(match(TOKEN_CONST))
    {
        var_declaration(true);
    }
    else
    {
        expression_statement();
    }
    size_t loop_start = current_chunk()->size;
    size_t exit_jump = 0;
    bool terminates = false;
    if(!match(TOKEN_SEMICOLON))
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");
        exit_jump = emit_jump(OP_JUMP_IF_FALSE_BYTE);
        terminates = true;
        emit_inst(OP_POP);
    }
    bool increments = false;
    Chunk inc_chunk;
    if(!match(TOKEN_RIGHT_PAREN))
    {
        increments = true;
        init_chunk(&inc_chunk);
        inc_chunk.consts = current_chunk()->consts;
        inc_chunk.globals = current_chunk()->globals;
        Chunk* temp = current_chunk();
        compiling_chunk = &inc_chunk; 
        expression();
        emit_inst(OP_POP);
        compiling_chunk = temp;
        compiling_chunk->consts = inc_chunk.consts;
        compiling_chunk->globals = inc_chunk.globals;
        inc_chunk.consts.values = NULL;
        inc_chunk.consts.capacity = 0;
        inc_chunk.consts.size = 0;
        inc_chunk.globals.values = NULL;
        inc_chunk.globals.capacity = 0;
        inc_chunk.globals.size = 0;
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses");
    }
    consume(TOKEN_LEFT_BRACE, "Expect '{' after loop expression");
    begin_scope();
    block(in_func);
    end_scope();
    if(increments)
    {
        for(size_t i = 0; i < inc_chunk.size; i++)
        {
            emit_inst(inc_chunk.code[i]);
        }
        free_chunk(&inc_chunk);
    }
    emit_loop(loop_start);
    if(terminates)
    {
        patch_jump(exit_jump);
        emit_inst(OP_POP);
    }
    end_scope();
}

static void return_statement(bool in_func)
{
    if(!in_func)
    {
        error("Can't return from top level code");
    }
    if(match(TOKEN_SEMICOLON))
    {
        emit_return();
    }
    else
    {
        expression();
        emit_inst(OP_POP_BASE);
        emit_inst(OP_RETURN);
        consume(TOKEN_SEMICOLON, "Expect ';' after return value");
    }
}

static void statement(bool in_func)
{
    if(match(TOKEN_PRINT))
    {
        print_statement();
    }
    else if(match(TOKEN_LEFT_BRACE))
    {
        begin_scope();
        block(in_func);
        end_scope();
    }
    else if(match(TOKEN_IF))
    {
        if_statement(in_func);
    }
    else if(match(TOKEN_WHILE))
    {
        while_statement(in_func);
    }
    else if(match(TOKEN_FOR))
    {
        for_statement(in_func);
    }
    else if(match(TOKEN_RET))
    {
        return_statement(in_func);
    }
    else
    {
        expression_statement();
    }
}

static Value ident_constant(Token* name)
{
    return OBJ_VAL((Obj*)copy_str(name->start, name->len));
}

static bool ident_equal(Token* a, Token* b)
{
    if(a->len != b->len)
    {
        return false;
    }
    return memcmp(a->start, b->start, a->len) == 0;
}

static Value resolve_local(Compiler* compiler, Token* name)
{
    for(size_t i = compiler->local_size; i > 0; i--)
    {
        Local* local = &compiler->locals[i - 1];
        if(ident_equal(name, &local->name))
        {
            if(local->depth == 0)
            {
                error("Can't read local variable in it's own initialiser");
            }
            return INT_VAL((int64_t)(i - 1));
        }
    }
    return NULL_VAL;
}

static void add_local(Token name, bool constant)
{
    if(current->local_size >= current->local_capacity)
    {
        size_t next_cap = GROW_CAPACITY(current->local_capacity);
        Local* new_locals = GROW_ARRAY(Local, current->locals, current->local_capacity, next_cap);
        current->local_capacity = next_cap;
        current->locals = new_locals;
    }
    current->locals[current->local_size] = (Local){.constant = constant, .name = name, .depth = 0}; 
    current->local_size++;
}

static Value add_global(Value name, bool constant)
{
    Value index = INT_VAL((int64_t)current_chunk()->globals.size);
    hash_table_insert(&current->globals, AS_STRING(name), constant, index);
    write_value_array(&current_chunk()->globals, NULL_VAL);
    return index;
}

static Value parse_variable(const char* error_msg, bool constant)
{
    consume(TOKEN_IDENT, error_msg);
    if(current->scope_depth > 0)
    {
        Token* name = &parser.previous;
        for(size_t i = current->local_size; i > 0; i--)
        {
            Local* local = &current->locals[i - 1];
            if(local->depth != 0 && local->depth < current->scope_depth + 1)
            {
                break;
            }
            if(ident_equal(name, &local->name))
            {
                size_t len = snprintf(NULL, 0, "Already defined '%.*s' in this scope", (int)local->name.len, local->name.start);
                char* buffer = ALLOCATE(char, len + 1);
                snprintf(buffer, len + 1, "Already defined '%.*s' in this scope", (int)local->name.len, local->name.start);
                error(buffer);
                FREE(char, buffer);
            }
        }
        add_local(*name, constant);
        return NULL_VAL;
    }
    Value name = ident_constant(&parser.previous);
    Value test;
    if(hash_table_get(&current->globals, AS_STRING(name), &test))
    {
        size_t len = snprintf(NULL, 0, "Already defined global '%s'", AS_CSTRING(name));
        char* buffer = ALLOCATE(char, len + 1);
        snprintf(buffer, len + 1, "Already defined global '%s'", AS_CSTRING(name));
        error(buffer);
        FREE(char, buffer);
    }
    return add_global(name, constant);
}

static void mark_inititialised()
{
    if(current->scope_depth == 0)
    {
        return;
    }
    current->locals[current->local_size - 1].depth = current->scope_depth + 1;
}

static void var_declaration(bool constant)
{
    Value name = parse_variable("Expect variable name", constant);
    if(match(TOKEN_EQL))
    {
        expression();
    }
    else
    {
        emit_inst(OP_NULL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    if(current->scope_depth == 0)
    {
        emit_set_var(name, true);
        emit_inst(OP_POP);
    }
    if(current->scope_depth > 0)
    {
        mark_inititialised();
    }
}

static void function(Value val)
{
    ObjString* func_name = copy_str(parser.previous.start, parser.previous.len);
    size_t from = emit_jump(OP_JUMP_BYTE);
    size_t offset = current_chunk()->size;
    ObjFunc* func = new_func();
    func->name = func_name;
    func->defined = true;
    func->num_inputs = 0;
    func->offset = offset;
    Local* prev_locals = current->locals;
    size_t prev_local_size = current->local_size;
    size_t prev_local_cap = current->local_capacity;
    current->locals = NULL;
    current->local_capacity = 0;
    current->local_size = 0;
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name");
    if(!check(TOKEN_RIGHT_PAREN))
    {
        parse_variable("Expect parameter name", false);
        mark_inititialised(); // initialise parameter
        func->num_inputs++;
        while(!check(TOKEN_RIGHT_PAREN) && !check(TOKEN_EOF))
        {
            consume(TOKEN_COMMA, "Expect ',' to separate function parameters");
            parse_variable("Expect parameter name", false);
            mark_inititialised(); // initialise parameter
            func->num_inputs++;
        }
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body");
    block(true);
    emit_return();
    end_func_scope();
    FREE(Local, current->locals);
    current->locals = prev_locals;
    current->local_size = prev_local_size;
    current->local_capacity = prev_local_cap;
    patch_jump(from);
    emit_const(OBJ_VAL((Obj*)func));
    add_func(func);
    if(!IS_NULL(val))
    {
        hash_table_get(&current->globals, func_name, &val);
        emit_set_var(val, true);
        emit_inst(OP_POP);
    }
}

static void func_declaration()
{
    Value val = parse_variable("Expect function name", true);
    mark_inititialised();
    function(val);
}

static void declaration(bool in_func)
{
    if(match(TOKEN_VAR))
    {
        var_declaration(false);
    }
    else if(match(TOKEN_CONST))
    {
        var_declaration(true);
    }
    else if(match(TOKEN_FUNC))
    {
        func_declaration();
    }
    else
    {
        statement(in_func);
    }

    if(parser.panic_mode)
    {
        synchronise();
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]              = {grouping, call,   PREC_CALL},
    [TOKEN_RIGHT_PAREN]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]              = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]                   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]                     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SEMICOLON]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_SQR]                = {array,    NULL,   PREC_NONE},
    [TOKEN_RIGHT_SQR]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]                   = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]                    = {NULL,     binary, PREC_TERM},
    [TOKEN_STAR]                    = {NULL,     binary, PREC_FACTOR},
    [TOKEN_SLASH]                   = {NULL,     binary, PREC_TERM},
    [TOKEN_PERC]                    = {NULL,     binary, PREC_TERM},
    [TOKEN_AMP]                     = {NULL,     binary, PREC_BIT_AND},
    [TOKEN_LINE]                    = {NULL,     binary, PREC_BIT_OR},
    [TOKEN_UP]                      = {NULL,     binary, PREC_BIT_OR},
    [TOKEN_BANG]                    = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQL]                = {NULL,     binary, PREC_EQL},
    [TOKEN_EQL]                     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQL_EQL]                 = {NULL,     binary, PREC_EQL},
    [TOKEN_GREATER]                 = {NULL,     binary, PREC_COMP},
    [TOKEN_GREATER_EQL]             = {NULL,     binary, PREC_COMP},
    [TOKEN_LESS]                    = {NULL,     binary, PREC_COMP},
    [TOKEN_LESS_EQL]                = {NULL,     binary, PREC_COMP},
    [TOKEN_LESS_LESS]               = {NULL,     binary, PREC_SHIFT},
    [TOKEN_GREATER_GREATER]         = {NULL,     binary, PREC_SHIFT},
    [TOKEN_LESS_LESS_LESS]          = {NULL,     binary, PREC_SHIFT},
    [TOKEN_GREATER_GREATER_GREATER] = {NULL,     binary, PREC_SHIFT},
    [TOKEN_MINUS_MINUS]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PLUS_PLUS]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PLUS_EQL]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS_EQL]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STAR_EQL]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH_EQL]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_AMP_EQL]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LINE_EQL]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_UP_EQL]                  = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PERC_EQL]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER_GREATER_EQL]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS_LESS_EQL]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IDENT]                   = {variable, NULL,   PREC_NONE},
    [TOKEN_STR_START]               = {string,   NULL,   PREC_NONE},
    [TOKEN_STR_BODY]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STR_END]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INTERP_START]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INTERP_END]              = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INT]                     = {number,   NULL,   PREC_NONE},
    [TOKEN_INT_HEX]                 = {number,   NULL,   PREC_NONE},
    [TOKEN_INT_BIN]                 = {number,   NULL,   PREC_NONE},
    [TOKEN_INT_OCT]                 = {number,   NULL,   PREC_NONE},
    [TOKEN_FLOAT]                   = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]                     = {NULL,     and_,    PREC_AND},
    [TOKEN_ARRAY]                   = {reserve_array,    NULL,   PREC_NONE},
    [TOKEN_BOOL_CAST]               = {cast,     NULL,   PREC_NONE},
    [TOKEN_CLASS]                   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CONST]                   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]                    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]                   = {literal,  NULL,   PREC_NONE},
    [TOKEN_FLOAT_CAST]              = {cast,     NULL,   PREC_NONE},
    [TOKEN_FOR]                     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUNC]                    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]                      = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IN]                      = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INT_CAST]                = {cast,     NULL,   PREC_NONE},
    [TOKEN_IMPORT]                  = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NOT]                     = {unary,    NULL,   PREC_NONE},
    [TOKEN_NULL]                    = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]                      = {NULL,     or_,      PREC_OR},
    [TOKEN_OVERRIDE]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]                   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRIV]                    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PROT]                    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PUB]                     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RET]                     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]                   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STR_CAST]                = {cast,     NULL,   PREC_NONE},
    [TOKEN_THIS]                    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]                    = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]                     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_VIRTUAL]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]                   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]                   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]                     = {NULL,     NULL,   PREC_NONE},
};

static void resolve_jump_table(Chunk* obj_chunk, Chunk* res)
{
    for(size_t i = 0; i < current->jump_table_size; i++)
    {
        // foward jump
        if(current->jump_table[i].type == JUMP_FORWARD)
        {
            size_t extra_bytes = 0;
            for(size_t j = i + 1; j < current->jump_table_size; j++)
            {
                if(current->jump_table[j].from >= current->jump_table[i].to)
                {
                    break;
                }
                else
                {
                    extra_bytes += 8 / sizeof(inst_type);
                }
            }
            size_t jump_len = extra_bytes + current->jump_table[i].to - current->jump_table[i].from;
            if(jump_len - (1 / sizeof(inst_type)) < 0x100 && sizeof(inst_type) == 1)
            {
                current->jump_table[i].bytes = 1;
            }
            else if(jump_len - (2 / sizeof(inst_type)) < 0x10000 && sizeof(inst_type) <= 2)
            {
                current->jump_table[i].bytes = 2;
            }
            else if(jump_len - (4 / sizeof(inst_type)) < 0x100000000 && sizeof(inst_type) <= 4)
            {
                current->jump_table[i].bytes = 4;
            }
            else
            {
                current->jump_table[i].bytes = 8;
            }
        }
        else
        {
            size_t extra_bytes = 0;
            for(size_t j = i; j > 0; j--)
            {
                size_t index = j - 1;
                if(current->jump_table[index].from <= current->jump_table[i].to)
                {
                    break;
                }
                else
                {
                    extra_bytes += 8 / sizeof(inst_type);
                }
            }
            size_t jump_len = extra_bytes + current->jump_table[i].from - current->jump_table[i].to + 1;
            if(jump_len + (1 / sizeof(inst_type)) < 0x100 && sizeof(inst_type) == 1)
            {
                current->jump_table[i].bytes = 1;
            }
            else if(jump_len + (2 / sizeof(inst_type)) < 0x10000 && sizeof(inst_type) <= 2)
            {
                current->jump_table[i].bytes = 2;
            }
            else if(jump_len + (4 / sizeof(inst_type)) < 0x100000000 && sizeof(inst_type) <= 4)
            {
                current->jump_table[i].bytes = 4;
            }
            else
            {
                current->jump_table[i].bytes = 8;
            }
        }
    }
    // sort out functions
    for(size_t i = 0; i < current->func_table_size; i++)
    {
        ObjFunc* func = current->func_table[i];
        size_t extra_bytes = 0;
        for(size_t j = 0; j < current->jump_table_size; j++)
        {
            if(current->jump_table[j].from >= func->offset)
            {
                break;
            }
            else
            {
                extra_bytes += current->jump_table[j].bytes;
            }
        }
        func->offset += (extra_bytes / sizeof(inst_type));
    }
    compiling_chunk = res;
    pass_chunk_context(obj_chunk, res);
    size_t jump_index = 0;
    for(size_t i = 0; i < obj_chunk->size; i++)
    {
        if(jump_index < current->jump_table_size && i == current->jump_table[jump_index].from)
        {
            size_t inst_inc = 0;
            switch(current->jump_table[jump_index].bytes)
            {
                case (2):
                {
                    inst_inc = 1;
                    break;
                }
                case (4):
                {
                    inst_inc = 2;
                    break;
                }
                case (8):
                {
                    inst_inc = 3;
                    break;
                }
                default:
                {
                    break;
                }
            }
            obj_chunk->code[i] += inst_inc;
            size_t jump_len = 0;
            if(current->jump_table[jump_index].type == JUMP_FORWARD)
            {
                size_t extra_bytes = 0;
                for(size_t j = jump_index + 1; j < current->jump_table_size; j++)
                {
                    if(current->jump_table[j].from >= current->jump_table[jump_index].to)
                    {
                        break;
                    }
                    else
                    {
                        extra_bytes += current->jump_table[j].bytes;
                    }
                }
                jump_len = current->jump_table[jump_index].to - current->jump_table[jump_index].from - (current->jump_table[jump_index].bytes / sizeof(inst_type)) + extra_bytes;
            }
            else
            {
                size_t extra_bytes = 0;
                for(size_t j = jump_index; j > 0; j--)
                {
                    size_t index = j - 1;
                    if(current->jump_table[index].from <= current->jump_table[jump_index].to)
                    {
                        break;
                    }
                    else
                    {
                        extra_bytes += current->jump_table[index].bytes;
                    }
                }
                jump_len = current->jump_table[jump_index].from - current->jump_table[jump_index].to + (current->jump_table[jump_index].bytes / sizeof(inst_type)) + extra_bytes + 1;
            }
            size_t line = get_line_number(&obj_chunk->line_encoding, i);
            emit_inst_line(obj_chunk->code[i], line);
            size_t offset = current_chunk()->size;
            for(size_t j = 0; j < current->jump_table[jump_index].bytes; j += sizeof(inst_type))
            {
                emit_inst_line(0x0, line);
            }
            uint8_t* data = (uint8_t*)(&current_chunk()->code[offset]);
            for(size_t j = current->jump_table[jump_index].bytes; j > 0; j--)
            {
                data[j - 1] = (jump_len >> (8 * (current->jump_table[jump_index].bytes - j))) & 0xff;
            }
            jump_index++;
        }
        else
        {
            emit_inst_line(obj_chunk->code[i], get_line_number(&obj_chunk->line_encoding, i));
        }
    }
}

static void define_native(const char* name, NativeFn func, size_t args)
{
    ObjString* func_name = copy_str(name, strlen(name));
    Value pos = add_global(OBJ_VAL((Obj*)func_name), true);
    current_chunk()->globals.values[(size_t)AS_INT(pos)] = OBJ_VAL((Obj*)new_native(func, func_name, args));
}


static void define_natives()
{
    define_native("time", time_native, 0);
}

bool compile(const char* src, Chunk* chunk, HashTable* global_names)
{
    init_scanner(src);
    
    Compiler compiler;
    init_compiler(&compiler);
    Chunk obj_chunk;
    init_chunk(&obj_chunk);
    if(global_names)
    {
        compiler.globals = *global_names;
        pass_chunk_context(chunk, &obj_chunk);
        compiling_chunk = &obj_chunk;
    }
    else
    {
        compiling_chunk = &obj_chunk;
        define_natives();
    }

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    
    while(!match(TOKEN_EOF))
    {
        declaration(false);
    }
    if(get_scanner_mode() != SCANNER_NORMAL)
    {
        switch(get_scanner_mode())
        {
            case SCANNER_INTERP:
            {
                error("Unescaped Interpolation");    
                break;
            }
            case SCANNER_STR:
            {
                error("Unescaped String");
                break;
            }
            default:
            {
                break;
            }
        }
    }

    resolve_jump_table(&obj_chunk, chunk);

    if(global_names)
    {
        *global_names = compiler.globals;
        compiler.globals.entries = NULL;
        compiler.globals.capacity = 0;
        compiler.globals.count = 0;
    }
    end_compiler();
    return !parser.had_error;
}

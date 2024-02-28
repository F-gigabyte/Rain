#include <compiler.h>
#include <common.h>
#include <scanner.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include <debug.h>
#endif

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
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // - ! not
    PREC_CALL, // . ()
    PREC_PRIMARY,
} Precedence;

typedef void(*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence prec;
} ParseRule;

Parser parser;
Chunk* compiling_chunk;

ParseRule rules[];

static Chunk* current_chunk()
{
    return compiling_chunk;
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

static void emit_inst(inst_type inst)
{
    write_chunk(current_chunk(), inst, parser.previous.line);
}

static void emit_insts(inst_type inst1, inst_type inst2)
{
    emit_inst(inst1);
    emit_inst(inst2);
}

static void emit_return()
{
    emit_inst(OP_RETURN);
}

static void end_compiler()
{
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if(!parser.had_error)
    {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
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

static void expression();

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
        error("Epected expression");
        return;
    }
    prefix_rule();
    while(precedence <= get_rule(parser.current.type)->prec)
    {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
    }
}

static void binary()
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
        default:
        {
            return;
        }
    }
}

static void unary()
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
        default:
        {
            return;
        }
    }
}

static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emit_const(value);
}

static void expression()
{
    parse_precedence(PREC_ASSIGNMENT);
}


ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]          = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]               = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]                = {NULL,     binary, PREC_TERM},
    [TOKEN_STAR]                = {NULL,     binary, PREC_FACTOR},
    [TOKEN_SLASH]               = {NULL,     binary, PREC_TERM},
    [TOKEN_PERC]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_AMP]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LINE]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_UP]                  = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BANG]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BANG_EQL]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQL]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQL_EQL]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER_EQL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS_EQL]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS_LESS]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER_GREATER]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS_MINUS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PLUS_PLUS]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PLUS_EQL]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS_EQL]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STAR_EQL]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH_EQL]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_AMP_EQL]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LINE_EQL]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_UP_EQL]              = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PERC_EQL]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER_GREATER_EQL] = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS_LESS_EQL]       = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IDENT]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STR_START]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STR_BODY]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STR_END]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INTERP_START]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INTERP_END]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INT]                 = {number,   NULL,   PREC_NONE},
    [TOKEN_INT_HEX]             = {number,   NULL,   PREC_NONE},
    [TOKEN_INT_BIN]             = {number,   NULL,   PREC_NONE},
    [TOKEN_INT_OCT]             = {number,   NULL,   PREC_NONE},
    [TOKEN_FLOAT]               = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ARRAY]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BOOL_CAST]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CONST]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FLOAT_CAST]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FOR]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUNC]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]                  = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IN]                  = {NULL,     NULL,   PREC_NONE},
    [TOKEN_INT_CAST]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IMPORT]              = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NOT]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NULL]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_OR]                  = {NULL,     NULL,   PREC_NONE},
    [TOKEN_OVERRIDE]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRIV]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PROT]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PUB]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RET]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STR_CAST]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]                = {NULL,     NULL,   PREC_NONE},
    [TOKEN_VAR]                 = {NULL,     NULL,   PREC_NONE},
    [TOKEN_VIRTUAL]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]               = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]                 = {NULL,     NULL,   PREC_NONE},
};

bool compile(const char* src, Chunk* chunk)
{
    init_scanner(src);

    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expected end of expression");
    end_compiler();
    return !parser.had_error;
}

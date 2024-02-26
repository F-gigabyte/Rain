#ifndef RAIN_SCANNER_H
#define RAIN_SCANNER_H

#include <common.h>

typedef enum {
    TOKEN_LEFT_PAREN, // (
    TOKEN_RIGHT_PAREN, // )
    TOKEN_LEFT_BRACE, // {
    TOKEN_RIGHT_BRACE, // }
    TOKEN_COMMA, // ,
    TOKEN_DOT, // .
    TOKEN_SEMICOLON, // ;
    TOKEN_LEFT_SQR, // [
    TOKEN_RIGHT_SQR, // ]

    TOKEN_MINUS, // -
    TOKEN_PLUS, // +
    TOKEN_STAR, // *
    TOKEN_SLASH, // /
    TOKEN_PERC, // %
    TOKEN_AMP, // &
    TOKEN_LINE, // |
    TOKEN_UP, // ^
    TOKEN_BANG, // !
    TOKEN_BANG_EQL, // !=
    TOKEN_EQL, // =
    TOKEN_EQL_EQL, // ==
    TOKEN_GREATER, // >
    TOKEN_GREATER_EQL, // >=
    TOKEN_LESS, // <
    TOKEN_LESS_EQL, // <=
    TOKEN_LESS_LESS, // <<
    TOKEN_GREATER_GREATER, // >>
    TOKEN_MINUS_MINUS, // --
    TOKEN_PLUS_PLUS, // ++
    TOKEN_PLUS_EQL, // +=
    TOKEN_MINUS_EQL, // -=
    TOKEN_STAR_EQL, // *=
    TOKEN_SLASH_EQL, // /=
    TOKEN_AMP_EQL, // &=
    TOKEN_LINE_EQL, // |=
    TOKEN_UP_EQL, // ^=
    TOKEN_PERC_EQL, // %=

    TOKEN_GREATER_GREATER_EQL, // >>=
    TOKEN_LESS_LESS_EQL, // <<=

    TOKEN_IDENT, // identifier
    TOKEN_STR, // string
    TOKEN_EXPR_STR_START, // expr string start
    TOKEN_EXPR_STR, // expr string middle
    TOKEN_EXPR_STR_END, // end of expr string
    TOKEN_INT, // integer
    TOKEN_INT_HEX, // hexadecimal integer
    TOKEN_INT_BIN, // binary integer
    TOKEN_INT_OCT, // octal integer
    TOKEN_FLOAT, // float

    TOKEN_AND, // and
    TOKEN_ARRAY, // array
    TOKEN_BOOL_CAST, // bool
    TOKEN_CLASS, // class
    TOKEN_CONST, // const
    TOKEN_ELSE, // else
    TOKEN_FALSE, // false
    TOKEN_FLOAT_CAST, // float
    TOKEN_FOR, // for
    TOKEN_FUNC, // func
    TOKEN_IF, // if
    TOKEN_IN, // in
    TOKEN_INT_CAST, // int
    TOKEN_IMPORT, // import
    TOKEN_NOT, // not
    TOKEN_NULL, // null
    TOKEN_OR, // or
    TOKEN_OVERRIDE, // override
    TOKEN_PRINT, // print
    TOKEN_PRIV, // priv
    TOKEN_PROT, // prot
    TOKEN_PUB, // pub
    TOKEN_RET, // ret
    TOKEN_SUPER, // super
    TOKEN_STR_CAST, // str
    TOKEN_THIS, // this
    TOKEN_TRUE, // true
    TOKEN_VAR, // var
    TOKEN_VIRTUAL, // virtual
    TOKEN_WHILE, // while

    TOKEN_ERROR, // error happened
    TOKEN_EOF, // end of file

} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t len;
    size_t line;
} Token;

// initialises scanner
void init_scanner(const char* src);
// scans a token
Token scan_token();

#endif

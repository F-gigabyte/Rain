#include <stdio.h>
#include <string.h>
#include <scanner.h>
#include <stdlib.h>
#include <utf8.h>

struct ModeNode {
    ScannerMode mode;
    ScannerChar str_quote;
    struct ModeNode* prev;
};

typedef struct {
    const char* start;
    const char* current;
    struct ModeNode* mode_node;
    size_t line;
    ScannerChar current_char;
    ScannerChar next_char;
} Scanner;

Scanner scanner;

void init_scanner(const char* src)
{
    scanner.start = src;
    scanner.current = src;
    scanner.line = 1;
    scanner.mode_node = (struct ModeNode*)malloc(sizeof(struct ModeNode));
    scanner.mode_node->mode = SCANNER_NORMAL;
    scanner.mode_node->prev = NULL;
    scanner.mode_node->str_quote = 0;
}

void free_scanner()
{
    struct ModeNode* current = scanner.mode_node;
    while(current != NULL)
    {
        free(current);
        current = scanner.mode_node->prev;
        scanner.mode_node = current;
    }
}

ScannerMode get_scanner_mode()
{
    return scanner.mode_node->mode;
}

static bool is_at_end()
{
    return *scanner.current == 0;
}

static Token make_token(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.len = (size_t)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token error_token(const char* msg)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = msg;
    token.len = (size_t)strlen(msg);
    token.line = scanner.line;
    return token;
}

static ScannerChar advance()
{
    uint8_t consumed = 0;
    ScannerChar next_char = decode_utf8_char(scanner.current, &consumed, 0);
    scanner.current += consumed;
    scanner.current_char = decode_utf8_char(scanner.current, &consumed, 0);
    if(is_at_end())
    {
        scanner.next_char = 0;
    }
    else
    {
        scanner.next_char = decode_utf8_char(scanner.current + consumed, NULL, 0);
    }
    return next_char;
}

static ScannerChar peek()
{
    return scanner.current_char;
}

static ScannerChar peek_next()
{
    return scanner.next_char;
}

static bool match(char expected)
{
    if(is_at_end())
    {
        return false;
    }
    if(*scanner.current != expected)
    {
        return false;
    }
    advance();
    return true;
}

static bool is_digit(ScannerChar c)
{
    return c >= '0' && c <= '9';
}

static bool is_hex_digit(ScannerChar c)
{
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || (c >= 0x430 && c <= 0x434) || c == 0x491;
}

static bool is_bin_digit(ScannerChar c)
{
    return c == '0' || c == '1';
}

static bool is_oct_digit(ScannerChar c)
{
    return c >= '0' && c <= '7';
}

static bool is_letter(ScannerChar c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == 0x404) || (c >= 0x406 && c <= 0x407) || (c >= 0x410 && c <= 0x429) || (c == 0x42c) || (c >= 0x42e && c <= 0x449)
        || (c == 0x44c) || (c >= 0x44e && c <= 0x44f) || (c == 0x454) || (c >= 0x456 && c <= 0x457) || (c >= 0x490 && c <= 0x491);
}

static bool is_alpha(ScannerChar c)
{
    return is_letter(c) || c == '_';
}

static Token number(ScannerChar start)
{
    // 0x or 0ш
    if(start == '0' && (peek() == 'x' || peek() == ord("ш")) && is_hex_digit(peek_next()))
    {
        advance();
        advance();
        while(is_hex_digit(peek()))
        {
            advance();
        }
        return make_token(TOKEN_INT_HEX);
    }
    // 0b or 0д
    else if(start == '0' && (peek() == 'b' || peek() == ord("д")) && is_bin_digit(peek_next()))
    {
        advance();
        advance();
        while(is_bin_digit(peek()))
        {
            advance();
        }
        return make_token(TOKEN_INT_BIN);
    }
    // 0o or 0в
    else if(start == '0' && (peek() == 'o' || peek() == ord("в")) && is_oct_digit(peek_next()))
    {
        advance();
        advance();
        while(is_oct_digit(peek()))
        {
            advance();
        }
        return make_token(TOKEN_INT_OCT);
    }
    else
    {
        while(is_digit(peek()))
        {
            advance();
        }
        if(peek() == '.' && is_digit(peek_next()))
        {
            advance();
            while(is_digit(peek()))
            {
                advance();
            }
            return make_token(TOKEN_FLOAT);
        }
        else
        {
            return make_token(TOKEN_INT);
        }
    }
}

static Token string(ScannerChar quote)
{
    while(peek() != quote && (peek() != '{' || peek_next() == '{') && !is_at_end())
    {
        if(peek() == '\n')
        {
            scanner.line++;
        }
        else if(peek() == '{' && peek_next() == '{')
        {
            advance(); // jump past escape sequence for {
        }
        else if(peek() == '\\' && peek_next() == quote)
        {
            advance(); // jump past escape sequence for quote
        }
        advance();
    }
    if(is_at_end())
    {
        return error_token("Unterminated string");
    }
    return make_token(TOKEN_STR_BODY);
}

static TokenType check_keyword(size_t start, size_t len, const char* rest, TokenType type)
{
    if(scanner.current - scanner.start == start + len && memcmp(scanner.start + start, rest, len) == 0)
    {
        return type;
    }
    return TOKEN_IDENT;
}

static TokenType identifier_type()
{
    if(scanner.current - scanner.start > 8)
    {
        return TOKEN_IDENT;
    }
    switch(scanner.start[0])
    {
        case ('a'):
        {
            if(scanner.current - scanner.start > 2)
            {
                switch(scanner.start[1])
                {
                    case ('n'):
                    {
                        return check_keyword(2, 1, "d", TOKEN_AND);
                    }
                    case ('r'):
                    {
                        return check_keyword(2, 3, "ray", TOKEN_ARRAY);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('b'):
        {
            return check_keyword(1, 3, "ool", TOKEN_BOOL_CAST);
            break;
        }
        case ('c'):
        {
            if(scanner.current - scanner.start == 5)
            {
                switch(scanner.start[1])
                {
                    case ('l'):
                    {
                        return check_keyword(2, 3, "ass", TOKEN_CLASS);
                    }
                    case ('o'):
                    {
                        return check_keyword(2, 3, "nst", TOKEN_CONST);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('e'):
        {
            return check_keyword(1, 3, "lse", TOKEN_ELSE);
        }
        case ('f'):
        {
            if(scanner.current - scanner.start > 2)
            {
                switch(scanner.start[1])
                {
                    case ('a'):
                    {
                        return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    }
                    case ('l'):
                    {
                        return check_keyword(2, 3, "oat", TOKEN_FLOAT_CAST);
                    }
                    case ('o'):
                    {
                        return check_keyword(2, 1, "r", TOKEN_FOR);
                    }
                    case ('u'):
                    {
                        return check_keyword(2, 2, "nc", TOKEN_FUNC);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('i'):
        {
            if(scanner.current - scanner.start > 1)
            {
                switch(scanner.start[1])
                {
                    case ('f'):
                    {
                        if(scanner.current - scanner.start == 2)
                        {
                            return TOKEN_IF;
                        }
                        break;
                    }
                    case ('n'):
                    {
                        if(scanner.current - scanner.start == 2)
                        {
                            return TOKEN_IN;
                        }
                        else
                        {
                            return check_keyword(2, 1, "t", TOKEN_INT_CAST);
                        }
                    }
                    case ('m'):
                    {
                        return check_keyword(2, 4, "port", TOKEN_IMPORT);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('n'):
        {
            if(scanner.current - scanner.start > 2)
            {
                switch(scanner.start[1])
                {
                    case ('o'):
                    {
                        return check_keyword(2, 1, "t", TOKEN_NOT);
                    }
                    case('u'):
                    {
                        return check_keyword(2, 2, "ll", TOKEN_NULL);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('o'):
        {
            if(scanner.current - scanner.start > 1)
            {
                switch(scanner.start[1])
                {
                    case ('r'):
                    {
                        if(scanner.current - scanner.start == 0)
                        {
                            return TOKEN_OR;
                        }
                        break;
                    }
                    case ('v'):
                    {
                        return check_keyword(2, 6, "erride", TOKEN_OVERRIDE);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('p'):
        {
            if(scanner.current - scanner.start > 3)
            {
                switch(scanner.start[1])
                {
                    case ('r'):
                    {
                        switch(scanner.start[2])
                        {
                            case('i'):
                            {
                                switch(scanner.start[3])
                                {
                                    case('n'):
                                    {
                                        return check_keyword(4, 1, "t", TOKEN_PRINT);
                                    }
                                    case('v'):
                                    {
                                        if(scanner.current - scanner.start == 0)
                                        {
                                            return TOKEN_PRIV;
                                        }
                                        break;
                                    }
                                    default:
                                    {
                                        break;
                                    }
                                }
                            }
                            case('o'):
                            {
                                return check_keyword(3, 1, "t", TOKEN_PROT);
                            }
                            default:
                            {
                                break;
                            }
                        }
                        break;
                    }
                    case ('u'):
                    {
                        return check_keyword(2, 1, "b", TOKEN_PUB);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('r'):
        {
            return check_keyword(1, 2, "et", TOKEN_RET);
        }
        case ('s'):
        {
            if(scanner.current - scanner.start > 2)
            {
                switch(scanner.start[1])
                {
                    case ('u'):
                    {
                        return check_keyword(2, 3, "per", TOKEN_SUPER);
                    }
                    case ('t'):
                    {
                        return check_keyword(2, 1, "r", TOKEN_STR_CAST);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('t'):
        {
            if(scanner.current - scanner.start == 4)
            {
                switch(scanner.start[1])
                {
                    case ('h'):
                    {
                        return check_keyword(2, 2, "is", TOKEN_THIS);
                    }
                    case ('r'):
                    {
                        return check_keyword(2, 2, "ue", TOKEN_TRUE);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('v'):
        {
            if(scanner.current - scanner.start > 2)
            {
                switch(scanner.start[1])
                {
                    case ('a'):
                    {
                        return check_keyword(2, 1, "r", TOKEN_VAR);
                    }
                    case ('i'):
                    {
                        return check_keyword(2, 5, "rtual", TOKEN_VIRTUAL);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case ('w'):
        {
            return check_keyword(1, 4, "hile", TOKEN_WHILE);
        }
    }
    return TOKEN_IDENT;
}

static Token identifier()
{
    while(is_alpha(peek()) || is_digit(peek()))
    {
        advance();
    }
    return make_token(identifier_type());
}

static void skip_whitespace()
{
    for(;;)
    {
        ScannerChar c = peek();
        switch(c)
        {
            case (' '):
            case ('\r'):
            case ('\t'):
            {
                advance();
                break;
            }
            case ('\n'):
            {
                scanner.line++;
                advance();
                break;
            }
            case ('#'):
            {
                advance();
                if(match('#') && match('#'))
                {
                    size_t num_hashes = 0;
                    while(!is_at_end() && num_hashes < 3)
                    {
                        if(peek() == '#')
                        {
                            num_hashes++;
                        }
                        else
                        {
                            num_hashes = 0;
                        }
                        advance();
                    }
                    break;
                }
                while(peek() != '\n' && !is_at_end())
                {
                    advance();
                }
                break;
            }
            default:
            {
                return;
            }
        }
    }
}

static void push_mode(ScannerMode mode, ScannerChar quote)
{
    struct ModeNode* next_mode = malloc(sizeof(struct ModeNode));
    if(next_mode == NULL)
    {
        exit(74);
    }
    next_mode->mode = mode;
    next_mode->prev = scanner.mode_node;
    next_mode->str_quote = quote;
    scanner.mode_node = next_mode;
}

static void pop_mode()
{
    struct ModeNode* next_mode = scanner.mode_node->prev;
    free(scanner.mode_node);
    scanner.mode_node = next_mode;
}

Token scan_token()
{
    if(scanner.mode_node->mode != SCANNER_STR)
    {
        skip_whitespace();
    }
    scanner.start = scanner.current;
    if(is_at_end())
    {
        return make_token(TOKEN_EOF);
    }
    ScannerChar c = advance();

    if(scanner.mode_node->mode == SCANNER_NORMAL || scanner.mode_node->mode == SCANNER_INTERP)
    {
        if(is_alpha(c))
        {
            return identifier();
        }
        if(is_digit(c))
        {
            return number(c);
        }
        switch(c)
        {
            case ('('):
            {
                return make_token(TOKEN_LEFT_PAREN); 
            }
            case (')'):
            {
                return make_token(TOKEN_RIGHT_PAREN);
            }
            case ('{'):
            {
                if(scanner.mode_node->mode == SCANNER_NORMAL)
                {
                    return make_token(TOKEN_LEFT_BRACE);
                }
                else
                {
                    return error_token("Sub-blocks not allowed in string interpolation");
                }
            }
            case ('}'):
            {
                if(scanner.mode_node->mode == SCANNER_NORMAL)
                {
                    return make_token(TOKEN_RIGHT_BRACE);    
                }
                else
                {
                    pop_mode();
                    return make_token(TOKEN_INTERP_END);
                }
            }
            case ('['):
            {
                return make_token(TOKEN_LEFT_SQR);
            }
            case (']'):
            {
                return make_token(TOKEN_RIGHT_SQR);
            }
            case (';'):
            {
                return make_token(TOKEN_SEMICOLON);
            }
            case (','):
            {
                return make_token(TOKEN_COMMA);
            }
            case ('.'):
            {
                return make_token(TOKEN_DOT);
            }
            case ('-'):
            {
                if(match('-'))
                {
                    return make_token(TOKEN_MINUS_MINUS);
                }
                else if(match('='))
                {
                    return make_token(TOKEN_MINUS_EQL);
                }
                else if(is_digit(peek()))
                {
                    return number(c);
                }
                else
                {
                    return make_token(TOKEN_MINUS);
                }
                break;
            }
            case ('+'):
            {
                if(match('+'))
                {
                    return make_token(TOKEN_PLUS_PLUS);
                }
                else if(match('='))
                {
                    return make_token(TOKEN_PLUS_EQL);
                }
                else
                {
                    return make_token(TOKEN_PLUS);
                }
                break;
            }
            case ('*'):
            {
                return make_token(match('=') ? TOKEN_STAR_EQL : TOKEN_STAR);
            }
            case ('/'):
            {
                return make_token(match('=') ? TOKEN_SLASH_EQL : TOKEN_SLASH);
            }
            case ('%'):
            {
                return make_token(match('=') ? TOKEN_PERC_EQL : TOKEN_PERC);
            }
            case ('&'):
            {
                return make_token(match('=') ? TOKEN_AMP_EQL : TOKEN_AMP);
            }
            case ('|'):
            {
                return make_token(match('=') ? TOKEN_LINE_EQL : TOKEN_LINE);
            }
            case ('^'):
            {
                return make_token(match('=') ? TOKEN_UP_EQL : TOKEN_UP);
            }
            case ('!'):
            {
                return make_token(match('=') ? TOKEN_BANG_EQL : TOKEN_BANG);
            }
            case ('='):
            {
                return make_token(match('=') ? TOKEN_EQL_EQL : TOKEN_EQL);
            }
            case ('>'):
            {
                if(match('='))
                {
                    return make_token(TOKEN_GREATER_EQL);
                }
                else if(match('>'))
                {
                    if(match('='))
                    {
                        return make_token(TOKEN_GREATER_GREATER_EQL);
                    }
                    else if(match('>'))
                    {
                        return make_token(TOKEN_GREATER_GREATER_GREATER);
                    }
                    else
                    {
                        return make_token(TOKEN_GREATER_GREATER);
                    }
                }
                else
                {
                    return make_token(TOKEN_GREATER);
                }
                break;
            }
            case ('<'):
            {
                if(match('='))
                {
                    return make_token(TOKEN_LESS_EQL);
                }
                else if(match('<'))
                {
                    if(match('='))
                    {
                        return make_token(TOKEN_LESS_LESS_EQL);
                    }
                    else if(match('<'))
                    {
                        return make_token(TOKEN_LESS_LESS_LESS);
                    }
                    else
                    {
                        return make_token(TOKEN_LESS_LESS);
                    }
                }
                else
                {
                    return make_token(TOKEN_LESS);
                }
                break;
            }
            case ('\''):
            case ('\"'):
            {
                push_mode(SCANNER_STR, c);
                return make_token(TOKEN_STR_START);
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        if(c == scanner.mode_node->str_quote)
        {
            pop_mode();
            return make_token(TOKEN_STR_END);
        }
        else if(c == '{')
        {
            if(peek() == '{')
            {
                advance();
            }
            else
            {
                push_mode(SCANNER_INTERP, 0);
                return make_token(TOKEN_INTERP_START);
            }
        }
        else if(c == '\\')
        {
            if(peek() == scanner.mode_node->str_quote)
            {
                advance();
            }
        }
        return string(scanner.mode_node->str_quote);
    }
    return error_token("Unexpected character");
}

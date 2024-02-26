#include <stdio.h>
#include <string.h>
#include <scanner.h>

typedef struct {
    const char* start;
    const char* current;
    size_t line;
} Scanner;

Scanner scanner;

void init_scanner(const char* src)
{
    scanner.start = src;
    scanner.current = src;
    scanner.line = 1;
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

static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static char peek()
{
    return *scanner.current;
}

static char peek_next()
{
    if(is_at_end())
    {
        return 0;
    }
    return scanner.current[1];
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
    scanner.current++;
    return true;
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_hex_digit(char c)
{
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool is_bin_digit(char c)
{
    return c == '0' || c == '1';
}

static bool is_oct_digit(char c)
{
    return c >= '0' && c <= '7';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static Token number(char start)
{
    if(start == '0' && peek() == 'x' && is_hex_digit(peek_next()))
    {
        advance();
        advance();
        while(is_hex_digit(peek()))
        {
            advance();
        }
        return make_token(TOKEN_INT_HEX);
    }
    else if(start == '0' && peek() == 'b' && is_bin_digit(peek_next()))
    {
        advance();
        advance();
        while(is_bin_digit(peek()))
        {
            advance();
        }
        return make_token(TOKEN_INT_BIN);
    }
    else if(start == '0' && peek() == 'o' && is_oct_digit(peek_next()))
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

static Token string(char quote)
{
    while(peek() != quote && !is_at_end())
    {
        if(peek() == '\n')
        {
            scanner.line++;
        }
        advance();
    }
    if(is_at_end())
    {
        return error_token("Unterminated string");
    }
    advance();
    return make_token(TOKEN_STR);
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
            check_keyword(1, 3, "ool", TOKEN_BOOL_CAST);
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

        }
        case ('n'):
        {

        }
        case ('o'):
        {

        }
        case ('p'):
        {

        }
        case ('r'):
        {

        }
        case ('s'):
        {

        }
        case ('t'):
        {

        }
        case ('v'):
        {

        }
        case ('w'):
        {

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
        char c = peek();
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

Token scan_token()
{
    skip_whitespace();
    scanner.start = scanner.current;
    if(is_at_end())
    {
        return make_token(TOKEN_EOF);
    }
    char c = advance();

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
            return make_token(TOKEN_LEFT_BRACE);
        }
        case ('}'):
        {
            return make_token(TOKEN_RIGHT_BRACE);
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
            return make_token(match('=') ? TOKEN_UP : TOKEN_UP_EQL);
        }
        case ('!'):
        {
            return make_token(match('=') ? TOKEN_BANG : TOKEN_BANG_EQL);
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
            return string(c);
        }
    }

    return error_token("Unexpected character");
}

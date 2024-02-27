#include <compiler.h>
#include <common.h>
#include <scanner.h>
#include <stdio.h>

void compile(const char* src)
{
    init_scanner(src);
    size_t line = -1;
    for(;;)
    {
        Token token = scan_token();
        if(token.line != line)
        {
            printf("%4zu ", token.line);
            line = token.line;
        }
        else
        {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", token.type, (int)token.len, token.start);
        if(token.type == TOKEN_EOF)
        {
            break;
        }
    }
}

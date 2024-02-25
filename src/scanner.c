#include <stdio.h>
#include <string.h>
#include <common.h>
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

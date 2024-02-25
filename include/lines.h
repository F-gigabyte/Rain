#ifndef RAIN_LINES_H
#define RAIN_LINES_H

#include <common.h>

typedef struct
{
    size_t capacity;
    size_t size;
    size_t* lines;
} LineArray;

// initialises line array
void init_line_array(LineArray* array);
// write a chunk offset to the line array where line is in 1 to infinity
void write_line_array(LineArray* array, size_t line, size_t chunk_off);
// decodes line number from chunk offset
size_t get_line_number(LineArray* array, size_t chunk_off);
// frees the line array
void free_line_array(LineArray* array);
#endif

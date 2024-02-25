#include <lines.h>
#include <rain_memory.h>
#include <string.h>

void init_line_array(LineArray* array)
{
    array->capacity = 0;
    array->lines = NULL;
    array->size = 0;
}

void write_line_array(LineArray* array, size_t line, size_t chunk_off)
{
    if(array->capacity < line)
    {
        size_t next_cap = GROW_CAPACITY(array->capacity);
        next_cap = next_cap < line ? line : next_cap;
        size_t* next_array = GROW_ARRAY(size_t, array->lines, array->capacity, next_cap);
        memset(next_array + array->capacity, 0, sizeof(size_t) * (next_cap - array->capacity));
        array->capacity = next_cap;
        array->lines = next_array;
    }
    if(array->size < line)
    {
        array->size = line;
    }
    if(array->lines[line - 1] == 0 || chunk_off + 1 > array->lines[line - 1])
    {
        array->lines[line - 1] = chunk_off + 1;
    }
}

void free_line_array(LineArray* array)
{
    FREE_ARRAY(size_t, array->lines, array->capacity);
    init_line_array(array);
}

size_t get_line_number(LineArray* array, size_t chunk_off)
{
    for(size_t i = 0; i < array->size; i++)
    {
        if(chunk_off < array->lines[i])
        {
            return i + 1;
        }
    }
    return 0; // unknown
}

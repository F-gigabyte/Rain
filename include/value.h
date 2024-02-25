#ifndef RAIN_VALUE_H
#define RAIN_VALUE_H

#include <common.h>

typedef double Value;

typedef struct
{
    size_t capacity;
    size_t size;
    Value* values;
} ValueArray;

// initialises value array
void init_value_array(ValueArray* array);
// write a value to the value array
void write_value_array(ValueArray* array, Value value);
// frees the value array
void free_value_array(ValueArray* array);
// prints a value
void print_value(Value value);

#endif

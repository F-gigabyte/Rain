#include <stdio.h>

#include <value.h>
#include <rain_memory.h>

void init_value_array(ValueArray* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->size = 0;
}

void write_value_array(ValueArray* array, Value value)
{
    if(array->capacity < array->size + 1)
    {
        size_t next_cap = GROW_CAPACITY(array->capacity);
        Value* new_array = GROW_ARRAY(Value, array->values, array->capacity, next_cap);
        array->capacity = next_cap;
        array->values = new_array;
    }
    array->values[array->size] = value;
    array->size++;
}

void free_value_array(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

void print_value(Value value)
{
    printf("%f", value);
}

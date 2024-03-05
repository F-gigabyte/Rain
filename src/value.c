#include <stdio.h>
#include <value.h>
#include <rain_memory.h>
#include <object.h>
#include <string.h>

void init_value_array(ValueArray* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->size = 0;
}

bool values_eql(Value a, Value b)
{
    if(a.type != b.type)
    {
        return false;
    }
    switch(a.type)
    {
        case VAL_BOOL:
        {
            return AS_BOOL(a) == AS_BOOL(b);
        }
        case VAL_INT:
        {
            return AS_INT(a) == AS_INT(b);
        }
        case VAL_FLOAT:
        {
            return AS_FLOAT(a) == AS_FLOAT(b);
        }
        case VAL_NULL:
        {
            return true;
        }
        case VAL_OBJ:
        {
            return AS_OBJ(a) == AS_OBJ(b);
        }
        default:
        {
            return false;
        }
    }
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
    switch(value.type)
    {
        case VAL_FLOAT:
        {
            printf("%f", AS_FLOAT(value));
            break;
        }
        case VAL_INT:
        {
#ifdef LONG64
            printf("%ld", AS_INT(value));
#else
            printf("%lld", AS_INT(value));
#endif
            break;
        }
        case VAL_BOOL:
        {
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        }
        case VAL_NULL:
        {
            printf("null");
            break;
        }
        case VAL_OBJ:
        {
            print_obj(value);
            break;
        }
        default:
        {
            printf("unknown");
            break;
        }
    }
}

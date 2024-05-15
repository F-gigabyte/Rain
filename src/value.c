#include <stdio.h>
#include <value.h>
#include <rain_memory.h>
#include <object.h>
#include <string.h>
#include <convert.h>

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
    ObjString* text = value_to_str(value);
    printf("%s", text->chars);
}

ObjString* value_to_str(Value value)
{
    switch(value.type)
    {
        case VAL_FLOAT:
        {
            double num = AS_FLOAT(value);
            char* res_chars = float_to_str(num);
            return take_str(res_chars, strlen(res_chars));
        }
        case VAL_INT:
        {
            int64_t num = AS_INT(value);
            char* res_chars = int_to_dec_str(num);
            return take_str(res_chars, strlen(res_chars));
        }
        case VAL_BOOL:
        {
            bool res = AS_BOOL(value);
            return copy_str(res ? "true" : "false", res ? 4 : 5);
        }
        case VAL_NULL:
        {
            return copy_str("null", 4);
        }
        case VAL_OBJ:
        {
            return obj_to_str(value);
        }
        default:
        {
            return copy_str("Unknown", 7);
        }
    }
}

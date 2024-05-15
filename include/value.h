#ifndef RAIN_VALUE_H
#define RAIN_VALUE_H

#include <common.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjArray ObjArray;

typedef enum {
    VAL_BOOL,
    VAL_NULL,
    VAL_INT,
    VAL_FLOAT,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool bool_data;
        uint64_t int_data;
        double float_data;
        Obj* obj_data;
    } as;
} Value;

typedef struct
{
    size_t capacity;
    size_t size;
    Value* values;
} ValueArray;

bool values_eql(Value a, Value b);

#define BOOL_VAL(value)  ((Value){VAL_BOOL, {.bool_data = value}})
#define INT_VAL(value)   ((Value){VAL_INT,  {.int_data = value}})
#define FLOAT_VAL(value) ((Value){VAL_FLOAT, {.float_data = value}})
#define OBJ_VAL(value)   ((Value){VAL_OBJ, {.obj_data = value}})
#define NULL_VAL         ((Value){VAL_NULL, {.int_data = 0}}) 

#define AS_BOOL(value)   ((value).as.bool_data)
#define AS_INT(value)    ((value).as.int_data)
#define AS_FLOAT(value)  ((value).as.float_data)
#define AS_OBJ(value)    ((value).as.obj_data)

#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_INT(value)    ((value).type == VAL_INT)
#define IS_FLOAT(value)  ((value).type == VAL_FLOAT)
inline bool IS_NUMBER(Value value)
{
    return IS_INT(value) || IS_FLOAT(value);
}

#define IS_OBJ(value)    ((value).type == VAL_OBJ)

#define IS_NULL(value)   ((value).type == VAL_NULL)

// initialises value array
void init_value_array(ValueArray* array);
// write a value to the value array
void write_value_array(ValueArray* array, Value value);
// frees the value array
void free_value_array(ValueArray* array);
// prints a value
void print_value(Value value);
ObjString* value_to_str(Value value);

#endif

#ifndef RAIN_OBJECT_H
#define RAIN_OBJECT_H

#include <common.h>
#include <value.h>

#define OBJ_TYPE(value)   (AS_OBJ(value)->type)
#define IS_STRING(value)  is_obj_type(value, OBJ_STRING)
#define IS_ARRAY(value) is_obj_type(value, OBJ_ARRAY)
#define IS_FUNC(value) is_obj_type(value, OBJ_FUNC)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)

#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define AS_ARRAY(value) ((ObjArray*)AS_OBJ(value))
#define AS_CARRAY(value) (((ObjArray*)AS_OBJ(value))->data)

#define AS_FUNC(value) ((ObjFunc*)AS_OBJ(value))
#define AS_NATIVE(value) ((ObjNative*)AS_OBJ(value))

typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_FUNC,
    OBJ_NATIVE
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    size_t len;
    uint32_t hash;
    char chars[];
};

struct ObjArray {
    Obj obj;
    int64_t len;
    Value data[];
};

typedef struct
{
    Obj obj;
    ObjString* name;
    size_t offset;
    size_t num_inputs;
    bool defined;
} ObjFunc;

typedef Value (*NativeFn)(Value* args);

typedef struct
{
    Obj obj;
    ObjString* name;
    size_t num_inputs;
    NativeFn func;
} ObjNative;

static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjString* take_str(char* chars, size_t len);
ObjString* copy_str(const char* chars, size_t len);
ObjString* concat_str(ObjString* a, ObjString* b);
ObjArray* build_array(int64_t len, Value val);
ObjArray* fill_array(int64_t len, Value* values);
ObjFunc* new_func();
ObjNative* new_native(NativeFn func, ObjString* name, size_t args);
ObjString* obj_to_str(Value value);

#endif

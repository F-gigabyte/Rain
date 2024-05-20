#ifndef RAIN_OBJECT_H
#define RAIN_OBJECT_H

#include <common.h>
#include <value.h>

#define OBJ_TYPE(value)   (AS_OBJ(value)->type_fields.type)
#define IS_STRING(value)  is_obj_type(value, OBJ_STRING)
#define IS_ARRAY(value) is_obj_type(value, OBJ_ARRAY)
#define IS_FUNC(value) is_obj_type(value, OBJ_FUNC)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_UPVALUE(value) is_obj_type(value, OBJ_UPVALUE)

#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define AS_ARRAY(value) ((ObjArray*)AS_OBJ(value))
#define AS_CARRAY(value) (((ObjArray*)AS_OBJ(value))->data)

#define AS_FUNC(value) ((ObjFunc*)AS_OBJ(value))
#define AS_NATIVE(value) ((ObjNative*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_UPVALUE(value) ((ObjUpvalue*)AS_OBJ(value))

typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_FUNC,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    struct Obj* next;
    struct
    {
        ObjType type;
        bool marked;
        bool immortal;
        bool defined;
    } type_fields;
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
} ObjFunc;

typedef struct
{
    Obj obj;
    Value* value;
    struct ObjUpvalue* next;
    Value closed;
} ObjUpvalue;

typedef struct
{
    size_t index;
    bool local;
} UpvalueIndex;

typedef struct
{
    Obj obj;
    ObjFunc* func;
    size_t num_upvalues;
    union
    {
        ObjUpvalue* upvalue;
        UpvalueIndex indexes;
    } upvalues[];
} ObjClosure;

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
    return IS_OBJ(value) && AS_OBJ(value)->type_fields.type == type;
}

ObjString* take_str(char* chars, size_t len);
ObjString* copy_str(const char* chars, size_t len);
ObjString* concat_str(ObjString* a, ObjString* b);
ObjArray* build_array(int64_t len, Value val);
ObjArray* fill_array(int64_t len, Value* values);
ObjFunc* new_func();
ObjClosure* new_closure(ObjFunc* func, size_t num_upvalues);
ObjUpvalue* new_upvalue(Value* loc);
ObjNative* new_native(NativeFn func, ObjString* name, size_t args);
ObjString* obj_to_str(Value value);

#ifdef DEBUG_LOG_GC
const char* get_obj_type_name(ObjType type);
#endif

#endif

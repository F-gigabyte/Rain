#include <stdio.h>
#include <string.h>
#include <rain_memory.h>
#include <object.h>
#include <value.h>
#include <vm.h>

#define ALLOCATE_OBJ(type, obj_type) \
    (type*)allocate_obj(sizeof(type), obj_type)
#define ALLOCATE_STR(len) \
    (ObjString*)allocate_obj(sizeof(ObjString) + (len), OBJ_STRING)
#define ALLOCATE_CLOSURE(len) \
    (ObjClosure*)allocate_obj(sizeof(ObjClosure) + (len) * sizeof(UpvalueIndex), OBJ_CLOSURE)
#define ALLOCATE_ARRAY(len) \
    (ObjArray*)allocate_obj(sizeof(ObjArray) + (len) * sizeof(Value), OBJ_ARRAY)

static uint32_t hash_str(const char* str, size_t len)
{
    uint32_t hash = 2166136261u;
    for(size_t i = 0; i < len; i++)
    {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }
    return hash;
}

static Obj* allocate_obj(size_t size, ObjType type)
{
    Obj* obj = (Obj*)reallocate(NULL, 0, size);
    obj->type_fields.type = type;
    obj->next = vm.objects;
    obj->type_fields.marked = false;
    obj->type_fields.defined = false;
    vm.objects = obj;
#ifdef DEBUG_LOG_GC
    printf("%p allocated %zu bytes for %s\n", (void*)obj, size, get_obj_type_name(type));
#endif
    return obj;
}

static ObjString* allocate_str(const char* chars, size_t len)
{
    ObjString* str = ALLOCATE_STR(len + 1);
    str->len = len;
    memcpy(str->chars, chars, len);
    str->chars[len] = 0;
    str->hash = hash_str(str->chars, str->len);
    hash_table_insert(&vm.strings, str, false, NULL_VAL);
    return str;
}

static ObjArray* allocate_array(int64_t len)
{
    ObjArray* array = ALLOCATE_ARRAY(len);
    array->len = len;
    return array;
}

ObjString* take_str(char* chars, size_t len)
{
    uint32_t hash = hash_str(chars, len);
    ObjString* interned = hash_table_find_str(&vm.strings, chars, len, hash);
    if(interned != NULL)
    {
        FREE_ARRAY(char, chars, len);
        return interned;
    }
    ObjString* res = allocate_str(chars, len);
    FREE_ARRAY(char, chars, len);
    return res;
}

ObjString* copy_str(const char* chars, size_t len)
{
    size_t res_len = 0;
    size_t i = 0;
    for(;(i < len - 1) && len; i++)
    {
        switch(chars[i])
        {
            case('\\'):
            {
                i++;
                switch(chars[i])
                {
                    case '\'':
                    case '\"':
                    case 't':
                    case 'n':
                    case 'r':
                    case '\\':
                    {
                        res_len++;
                        break;
                    }
                    default:
                    {
                        res_len += 2;
                        break;
                    }
                }
                break;
            }
            case('{'):
            {
                i++;
                if(chars[i] == '{')
                {
                    res_len++;
                }
                else
                {
                    res_len += 2;
                }
                break;
            }
            default:
            {
                res_len++;
                break;
            }
        }
    }
    if(i < len)
    {
        res_len++;
    }
    char* res_chars = ALLOCATE(char, res_len); 
    size_t pos = 0;
    for(i = 0; (i < len - 1) && len && pos < res_len; i++,pos++)
    {
        switch(chars[i])
        {
            case('\\'):
            {
                i++;
                switch(chars[i])
                {
                    case '\'':
                    {
                        res_chars[pos] = '\'';
                        break;
                    }
                    case '\"':
                    {
                        res_chars[pos] = '\"';
                        break;
                    }
                    case 't':
                    {
                        res_chars[pos] = '\t';
                        break;
                    }
                    case 'n':
                    {
                        res_chars[pos] = '\n';
                        break;
                    }
                    case 'r':
                    {
                        res_chars[pos] = '\r';
                        break;
                    }
                    case '\\':
                    {
                        res_chars[pos] = '\\';
                        break;
                    }
                    default:
                    {
                        res_chars[pos] = '\\';
                        pos++;
                        res_chars[pos] = chars[i];
                        break;
                    }
                }
                break;
            }
            case('{'):
            {
                i++;
                if(chars[i] == '{')
                {
                    res_chars[pos] = '{';
                }
                else
                {
                    res_chars[pos] = '{';
                    pos++;
                    res_chars[pos] = chars[i];
                }
                break;
            }
            default:
            {
                res_chars[pos] = chars[i];
                break;
            }
        }
    }
    if(i < len)
    {
        res_chars[pos] = chars[i];
    }
    uint32_t hash = hash_str(res_chars, res_len);
    ObjString* interned = hash_table_find_str(&vm.strings, res_chars, len, hash);
    if(interned != NULL)
    {
        FREE_ARRAY(char, res_chars, res_len);
        return interned;
    }
    ObjString* res = allocate_str(res_chars, res_len);
    FREE_ARRAY(char, res_chars, res_len);
    return res;
}

ObjString* concat_str(ObjString* a, ObjString* b)
{
    size_t res_len = a->len + b->len;
    char* res_chars = ALLOCATE(char, res_len);
    memcpy(res_chars, a->chars, a->len);
    memcpy(res_chars + a->len, b->chars, b->len);
    return take_str(res_chars, res_len);

}

ObjString* obj_to_str(Value value)
{
    switch(OBJ_TYPE(value))
    {
        case OBJ_STRING:
        {
            return AS_STRING(value);
        }
        case OBJ_ARRAY:
        {
            ObjString* res = copy_str("[", 1);
            for(size_t i = 0; i < AS_ARRAY(value)->len - 1; i++)
            {
                res = concat_str(res, value_to_str(AS_CARRAY(value)[i])); 
                res = concat_str(res, copy_str(", ", 2));
            }
            res = concat_str(res, value_to_str(AS_CARRAY(value)[AS_ARRAY(value)->len - 1]));
            res = concat_str(res, copy_str("]", 1));
            return res;
        }
        case OBJ_FUNC:
        {
            size_t len = snprintf(NULL, 0, "<func %s (args: %zu): %zu>", AS_FUNC(value)->name->chars, AS_FUNC(value)->num_inputs, AS_FUNC(value)->offset);
            char* res_chars = ALLOCATE(char, len + 1);
            snprintf(res_chars, len + 1, "<func %s (args: %zu): %zu>", AS_FUNC(value)->name->chars, AS_FUNC(value)->num_inputs, AS_FUNC(value)->offset);
            return take_str(res_chars, len);
        }
        case OBJ_NATIVE:
        {
            size_t len = snprintf(NULL, 0, "<native func %s (args: %zu): external>", AS_NATIVE(value)->name->chars, AS_NATIVE(value)->num_inputs);
            char* res_chars = ALLOCATE(char, len + 1);
            snprintf(res_chars, len + 1, "<native func %s (args: %zu): external>", AS_NATIVE(value)->name->chars, AS_NATIVE(value)->num_inputs);
            return take_str(res_chars, len);
        }
        case OBJ_CLOSURE:
        {
            return obj_to_str(OBJ_VAL((Obj*)AS_CLOSURE(value)->func));
        }
        case OBJ_UPVALUE:
        {
            return copy_str("Upvalue", 7);
        }
        case OBJ_CLASS:
        {
            size_t len = snprintf(NULL, 0, "<class %s>", AS_CLASS(value)->name->chars);
            char* res_chars = ALLOCATE(char, len + 1);
            snprintf(res_chars, len + 1, "<class %s>", AS_CLASS(value)->name->chars);
            return take_str(res_chars, len);
        }
        case OBJ_INSTANCE:
        {
            size_t len = snprintf(NULL, 0, "<instance of class %s>", AS_INSTANCE(value)->klass->name->chars);
            char* res_chars = ALLOCATE(char, len + 1);
            snprintf(res_chars, len + 1, "<instance of class %s>", AS_INSTANCE(value)->klass->name->chars);
            return take_str(res_chars, len);
        }
        case OBJ_BOUND_METHOD:
        {
            return obj_to_str(OBJ_VAL((Obj*)AS_BOUND_METHOD(value)->method));
        }
        default:
        {
            return copy_str("Unknown Object", 14);
        }
    }
}

ObjArray* build_array(int64_t len, Value val)
{
    ObjArray* array = allocate_array(len);
    for(size_t i = 0; i < len; i++)
    {
        array->data[i] = val;
    }
    return array;
}

ObjArray* fill_array(int64_t len, Value* values)
{
    ObjArray* array = allocate_array(len);
    for(size_t i = 0; i < len; i++)
    {
        array->data[i] = values[i];
    }
    FREE_ARRAY(Value, values, len);
    return array;
}

ObjFunc* new_func()
{
    ObjFunc* func = ALLOCATE_OBJ(ObjFunc, OBJ_FUNC);
    func->name = NULL;
    func->num_inputs = 0;
    func->offset = 0;
    return func;
}

ObjNative* new_native(NativeFn func, ObjString* name, size_t args)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->func = func;
    native->name = name;
    native->num_inputs = args;
    return native;
}

ObjClosure* new_closure(ObjFunc* func, size_t num_upvalues)
{
    ObjClosure* closure = ALLOCATE_CLOSURE(num_upvalues);
    closure->func = func;
    closure->num_upvalues = num_upvalues;
    memset(closure->upvalues, 0, sizeof(UpvalueIndex) * num_upvalues);
    return closure;
}

ObjUpvalue* new_upvalue(Value* loc)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NULL_VAL;
    upvalue->next = NULL;
    upvalue->value = loc;
    return upvalue;
}

ObjClass* new_class(ObjString* name)
{
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    init_hash_table(&klass->attributes);
    return klass;
}

ObjInstance* new_instance(ObjClass* klass)
{
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    copy_hash_table(&klass->attributes, &instance->attributes);
    return instance;
}

ObjBoundMethod* new_bound_method(Value reciever, Obj* method)
{
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->reciever = reciever;
    bound->method = method;
    return bound;
}

#ifdef DEBUG_LOG_GC

const char* get_obj_type_name(ObjType type)
{
    switch(type)
    {
        case OBJ_FUNC:
        {
            return "func";
        }
        case OBJ_ARRAY:
        {
            return "array";
        }
        case OBJ_NATIVE:
        {
            return "native func";
        }
        case OBJ_STRING:
        {
            return "string";
        }
        case OBJ_CLOSURE:
        {
            return "closure";
        }
        case OBJ_UPVALUE:
        {
            return "upvalue";
        }
        case OBJ_CLASS:
        {
            return "class";
        }
        case OBJ_INSTANCE:
        {
            return "class instance";
        }
        case OBJ_BOUND_METHOD:
        {
            return "bound method";
        }
        default:
        {
            return "unknown";
        }
    }
}

#endif

#include <stdio.h>
#include <string.h>
#include <rain_memory.h>
#include <object.h>
#include <value.h>
#include <vm.h>

#define ALLOCATE_OBJ(type, obj_type) \
    (type*)allocate_obj(sizeof(type), obj_type)
#define ALLOCATE_STR(len) \
    (ObjString*)allocate_obj(sizeof(ObjString) + len, OBJ_STRING)
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
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
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
    func->defined = false;
    func->name = NULL;
    func->num_inputs = 0;
    func->offset = 0;
    return func;
}

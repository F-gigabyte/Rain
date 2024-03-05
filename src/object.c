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
    str->chars[len + 1] = 0;
    str->hash = hash_str(str->chars, str->len);
    hash_table_set(&vm.strings, str, NULL_VAL);
    return str;
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

void print_obj(Value value)
{
    switch(OBJ_TYPE(value))
    {
        case OBJ_STRING:
        {
            printf("%s", AS_CSTRING(value));
            break;
        }
        default:
        {
            printf("unknown object");
            break;
        }
    }
}

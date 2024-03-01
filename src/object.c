#include <stdio.h>
#include <string.h>
#include <rain_memory.h>
#include <object.h>
#include <value.h>
#include <vm.h>

#define ALLOCATE_OBJ(type, obj_type) \
    (type*)allocate_obj(sizeof(type), obj_type)

static Obj* allocate_obj(size_t size, ObjType type)
{
    Obj* obj = (Obj*)reallocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

static ObjString* allocate_str(char* chars, size_t len)
{
    ObjString* str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    str->len = len;
    str->chars = chars;
    return str;
}

ObjString* take_str(char* chars, size_t len)
{
    return allocate_str(chars, len);
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
    char* heap_chars = ALLOCATE(char, res_len + 1);
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
                        heap_chars[pos] = '\'';
                        break;
                    }
                    case '\"':
                    {
                        heap_chars[pos] = '\"';
                        break;
                    }
                    case 't':
                    {
                        heap_chars[pos] = '\t';
                        break;
                    }
                    case 'n':
                    {
                        heap_chars[pos] = '\n';
                        break;
                    }
                    case 'r':
                    {
                        heap_chars[pos] = '\r';
                        break;
                    }
                    case '\\':
                    {
                        heap_chars[pos] = '\\';
                        break;
                    }
                    default:
                    {
                        heap_chars[pos] = '\\';
                        pos++;
                        heap_chars[pos] = chars[i];
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
                    heap_chars[pos] = '{';
                }
                else
                {
                    heap_chars[pos] = '{';
                    pos++;
                    heap_chars[pos] = chars[i];
                }
                break;
            }
            default:
            {
                heap_chars[pos] = chars[i];
                break;
            }
        }
    }
    if(i < len)
    {
        heap_chars[pos] = chars[i];
    }
    heap_chars[res_len] = 0;
    return allocate_str(heap_chars, res_len);
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

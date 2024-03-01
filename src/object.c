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

static Obj* allocate_obj(size_t size, ObjType type)
{
    Obj* obj = (Obj*)reallocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

ObjString* allocate_str(size_t len)
{
    ObjString* str = ALLOCATE_STR(len + 1);
    str->len = len;
    return str;
}

ObjString* make_str(const char* chars, size_t len)
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
    ObjString* res_str = allocate_str(res_len);
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
                        res_str->chars[pos] = '\'';
                        break;
                    }
                    case '\"':
                    {
                        res_str->chars[pos] = '\"';
                        break;
                    }
                    case 't':
                    {
                        res_str->chars[pos] = '\t';
                        break;
                    }
                    case 'n':
                    {
                        res_str->chars[pos] = '\n';
                        break;
                    }
                    case 'r':
                    {
                        res_str->chars[pos] = '\r';
                        break;
                    }
                    case '\\':
                    {
                        res_str->chars[pos] = '\\';
                        break;
                    }
                    default:
                    {
                        res_str->chars[pos] = '\\';
                        pos++;
                        res_str->chars[pos] = chars[i];
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
                    res_str->chars[pos] = '{';
                }
                else
                {
                    res_str->chars[pos] = '{';
                    pos++;
                    res_str->chars[pos] = chars[i];
                }
                break;
            }
            default:
            {
                res_str->chars[pos] = chars[i];
                break;
            }
        }
    }
    if(i < len)
    {
        res_str->chars[pos] = chars[i];
    }
    res_str->chars[res_len] = 0;
    return res_str;
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

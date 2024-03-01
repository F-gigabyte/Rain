#include <rain_memory.h>
#include <stdlib.h>
#include <vm.h>
#include <object.h>

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    if(new_size == 0)
    {
        free(ptr);
        return NULL;
    }
    void* result = realloc(ptr, new_size);
    if(result == NULL)
    {
        exit(1);
    }
    return result;
}

static void free_obj(Obj* obj)
{
    switch(obj->type)
    {
        case OBJ_STRING:
        {
            ObjString* str = (ObjString*)obj;
            FREE(ObjString, obj);
            break;
        }
        default:
        {
            break;
        }
    }
}

void free_objs()
{
    Obj* obj = vm.objects;
    while(obj != NULL)
    {
        Obj* next = obj->next;
        free_obj(obj);
        obj = next;
    }
}

#include <rain_memory.h>
#include <stdlib.h>
#include <vm.h>
#include <object.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include <debug.h>
#endif

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    if(vm.running && new_size > old_size)
    {
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#endif
    }
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
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, obj->type);
#endif
    switch(obj->type)
    {
        case OBJ_STRING:
        {
            ObjString* str = (ObjString*)obj;
            FREE(ObjString, obj);
            break;
        }
        case OBJ_ARRAY:
        {
            ObjArray* array = (ObjArray*)obj;
            FREE(ObjArray, array);
            break;
        }
        case OBJ_FUNC:
        {
            ObjFunc* func = (ObjFunc*)obj;
            FREE(ObjFunc, func);
            break;
        }
        case OBJ_NATIVE:
        {
            ObjNative* native = (ObjNative*)obj;
            FREE(ObjNative, native);
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            FREE(ObjClosure, closure);
            break;
        }
        case OBJ_UPVALUE:
        {
            ObjUpvalue* upvalue = (ObjUpvalue*)obj;
            FREE(ObjUpvalue, upvalue);
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

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif



#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

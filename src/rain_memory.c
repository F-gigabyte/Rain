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
    printf("%p free type %s\n", (void*)obj, get_obj_type_name(obj->type));
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

void mark_obj(Obj* obj)
{
    if(obj != NULL)
    {
#ifdef DEBUG_LOG_GC
        printf("%p marked\n", (void*)obj);
#endif
        obj->marked = true;
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

static void mark_roots()
{
    for(Value* slot = vm.stack; slot < vm.stack_top; slot++)
    {
        if(IS_OBJ(*slot))
        {
            mark_obj(AS_OBJ(*slot));    
        }
    }
    for(size_t i = 0; i < vm.chunk->globals.size; i++)
    {
        Value val = vm.chunk->globals.values[i];
        if(IS_OBJ(val))
        {
            mark_obj(AS_OBJ(val));
        }
    }
    for(size_t i = 0; i < vm.chunk->consts.size; i++)
    {
        Value val = vm.chunk->consts.values[i];
        if(IS_OBJ(val))
        {
            mark_obj(AS_OBJ(val));
        }
    }
    for(ObjUpvalue* upvalue = vm.open_upvalues; upvalue != NULL; upvalue = (ObjUpvalue*)upvalue->next)
    {
        mark_obj((Obj*)upvalue);
    }
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("\n-- gc begin\n");
#endif
    mark_roots();
#ifdef DEBUG_LOG_GC
    printf("\n-- gc end\n");
#endif
}

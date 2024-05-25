#include <rain_memory.h>
#include <stdlib.h>
#include <vm.h>
#include <object.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include <debug.h>
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    vm.bytes_allocated += new_size;
    vm.bytes_allocated -= old_size;
    if(vm.running && vm.gc && new_size > old_size)
    {
        vm.gc = false;
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#else
        if(vm.bytes_allocated > vm.next_gc)
        {
            collect_garbage();
        }
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
    printf("%p free type %s\n", (void*)obj, get_obj_type_name(obj->type_fields.type));
#endif
    switch(obj->type_fields.type)
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
        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)obj;
            free_hash_table(&klass->attributes);
            FREE(ObjClass, klass);
            break;
        }
        case OBJ_INSTANCE:
        {
            ObjInstance* instance = (ObjInstance*)obj;
            free_hash_table(&instance->attributes);
            FREE(ObjInstance, instance);
            break;
        }
        case OBJ_BOUND_METHOD:
        {
            ObjBoundMethod* bound = (ObjBoundMethod*)obj;
            FREE(ObjBoundMethod, bound);
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
    if(obj != NULL && obj->type_fields.marked != vm.mark_bit)
    {
#ifdef DEBUG_LOG_GC
        printf("%p marked\n", (void*)obj);
#endif
        obj->type_fields.marked = vm.mark_bit;
        if(vm.gray_size >= vm.gray_capacity)
        {
            vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
            vm.gray_stack = (Obj**)realloc(vm.gray_stack, sizeof(Obj*) * vm.gray_capacity);
            if(vm.gray_stack == NULL)
            {
                exit(1);
            }
        }
        vm.gray_stack[vm.gray_size] = obj;
        vm.gray_size++;
    }
}

static void process_obj(Obj* obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p processing\n", (void*)obj);
#endif
    switch(obj->type_fields.type)
    {
        case OBJ_STRING:
        {
            break;
        }
        case OBJ_UPVALUE:
        {
            ObjUpvalue* upvalue = (ObjUpvalue*)obj;
            if(IS_OBJ(upvalue->closed))
            {
                mark_obj(AS_OBJ(upvalue->closed));
            }
            break;
        }
        case OBJ_FUNC:
        {
            ObjFunc* func = (ObjFunc*)obj;
            mark_obj((Obj*)func->name);
            break;
        }
        case OBJ_ARRAY:
        {
            ObjArray* array = (ObjArray*)obj;
            for(size_t i = 0; i < array->len; i++)
            {
                if(IS_OBJ(array->data[i]))
                {
                    mark_obj(AS_OBJ(array->data[i]));
                }
            }
            break;
        }
        case OBJ_NATIVE:
        {
            ObjNative* native = (ObjNative*)obj;
            mark_obj((Obj*)native->name);
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            mark_obj((Obj*)closure->func);
            if(closure->obj.type_fields.defined)
            {
                for(size_t i = 0; i < closure->num_upvalues; i++)
                {
                    mark_obj((Obj*)closure->upvalues[i].upvalue);
                }
            }
            break;
        }
        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)obj;
            mark_obj((Obj*)klass->name);
            break;
        }
        case OBJ_INSTANCE:
        {
            ObjInstance* instance = (ObjInstance*)obj;
            mark_obj((Obj*)instance->klass);
            break;
        }
        case OBJ_BOUND_METHOD:
        {
            ObjBoundMethod* bound = (ObjBoundMethod*)obj;
            if(IS_OBJ(bound->reciever))
            {
                mark_obj(AS_OBJ(bound->reciever));
            }
            mark_obj((Obj*)bound->method);
            break;
        }
        default:
        {
            printf("GC unknown object\n");
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

static void trace_refs()
{
    while(vm.gray_size > 0)
    {
        vm.gray_size--;
        Obj* obj = vm.gray_stack[vm.gray_size];
        process_obj(obj);
    }
}

static void sweep()
{
    Obj* prev = NULL;
    Obj* obj = vm.objects;
    while(obj != NULL)
    {
        if(obj->type_fields.marked == vm.mark_bit || obj->type_fields.immortal)
        {
            prev = obj;
            obj = obj->next;
        }
        else
        {
            Obj* trash = obj;
            obj = obj->next;
            if(prev != NULL)
            {
                prev->next = obj;
            }
            else
            {
                vm.objects = obj;
            }
            free_obj(trash);
        }
    }
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("\n-- gc begin\n");
    size_t before = vm.bytes_allocated;
#endif
    mark_roots();
    trace_refs();
    hash_table_remove_clear(&vm.strings);
    sweep();
    vm.mark_bit = !vm.mark_bit;
    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
    printf("\n-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu", before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}

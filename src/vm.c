#include <vm.h>
#include <common.h>
#include <stdio.h>
#include <compiler.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <lines.h>
#include <rain_memory.h>
#include <string.h>
#include <convert.h>
#include <call_stack.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <debug.h>
#endif

VM vm;

static void close_upvalue()
{
    ObjUpvalue* last = vm.open_upvalues;
    last->closed = *last->value;
    last->value = &last->closed;
    vm.open_upvalues = (ObjUpvalue*)last->next;
}

static void close_func_upvalues()
{
    while(vm.open_upvalues != NULL && vm.open_upvalues->value >= vm.stack_base)
    {
        ObjUpvalue* last = vm.open_upvalues;
        last->closed = *last->value;
        last->value = &last->closed;
        vm.open_upvalues = (ObjUpvalue*)last->next;
    }
}

static void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.stack_base = vm.stack;
    vm.call_base = vm.stack;
}

static void runtime_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t inst = vm.ip - vm.chunk->code - 1;
    size_t line = get_line_number(&vm.chunk->line_encoding, inst);
    fprintf(stderr, "[line %zu] in script\n", line);
    reset_stack();
}

void init_vm()
{
    reset_stack();
    vm.objects = NULL;
    vm.open_upvalues = NULL;
    vm.running = false;
    vm.gc = true;
    vm.gray_size = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;
    vm.mark_bit = true;
    vm.bytes_allocated = 0;
    vm.next_gc =  0x1000;
    init_hash_table(&vm.strings);
}

static Value peek(int64_t distance)
{
    return vm.stack_top[-1 - distance];
}

static Value peek_back(size_t distance)
{
    size_t stack_size = vm.stack_top - vm.stack_base;
    size_t pos = stack_size - 1 - distance;
    return vm.stack_base[pos];
}

static void concatenate()
{
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());
    push(OBJ_VAL((Obj*)concat_str(a, b)));
}

#define READ_INST() (*(vm.ip++))

static size_t read_inst_index(size_t offset_size)
{
    size_t offset = 0;
    size_t index = read_chunk_const(vm.ip, &offset, offset_size);
    vm.ip += offset;
    return index;
}

static Value read_const(size_t offset_size)
{
    size_t index = read_inst_index(offset_size);
    Value val = vm.chunk->consts.values[index];
    return val;
}

static Value read_global(size_t offset_size)
{
    size_t index = read_inst_index(offset_size);
    return vm.chunk->globals.values[index];
}

static void write_global(size_t offset_size, Value value)
{
    size_t index = read_inst_index(offset_size);
    vm.chunk->globals.values[index] = value;
}

static size_t read_jump(size_t offset_size)
{
    uint8_t* data = (uint8_t*)vm.ip;
    size_t offset = 0;
    for(size_t i = offset_size; i > 0; i--)
    {
        offset |= ((size_t)data[(i - 1)] << ((offset_size - i) * 8));
    }
    for(size_t i = 0; i < offset_size; i += sizeof(inst_type))
    {
        vm.ip++;
    }
    return offset;
}

static bool setup_call(size_t expected_inputs)
{
    vm.stack_base = vm.call_base;
    size_t args = (size_t)(vm.stack_top - vm.stack_base);
    if(args != expected_inputs)
    {
        runtime_error("Expected %zu args but got %zu", expected_inputs, args);
        return false;
    }
    vm.stack_base[STACK_RET_ADDR] = INT_VAL((int64_t)(size_t)((vm.ip - vm.chunk->code)));
    return true;
}

static ObjUpvalue* capture_upvalue(Value* loc)
{
    ObjUpvalue* prev = NULL;
    ObjUpvalue* upvalue = vm.open_upvalues;
    while(upvalue != NULL && upvalue->value > loc)
    {
        prev = upvalue;
        upvalue = (ObjUpvalue*)upvalue->next;
    }
    if(upvalue != NULL && upvalue->value == loc)
    {
        return upvalue;
    }
    ObjUpvalue* created = new_upvalue(loc);
    created->next = (struct ObjUpvalue*)upvalue;
    if(prev == NULL)
    {
        vm.open_upvalues = created;
    }
    else
    {
        prev->next = (struct ObjUpvalue*)created;
    }
    return created;
}

static void init_closure(ObjClosure* closure)
{
    for(size_t i = 0; i < closure->num_upvalues; i++)
    {
        Value* loc;
        if(closure->upvalues[i].indexes.local)
        {
            loc = &vm.stack_base[closure->upvalues[i].indexes.index];
        }
        else
        {
            ObjClosure* prev = AS_CLOSURE(vm.stack_base[-4]);
            loc = prev->upvalues[closure->upvalues[i].indexes.index].upvalue->value;
        }
        closure->upvalues[i].upvalue = capture_upvalue(loc);
    }
    closure->obj.type_fields.defined = true;
}

#define READ_STRING(offset_size) AS_STRING(read_const(offset_size))

static bool get_attr(size_t bytes, bool get)
{
    if(!IS_INSTANCE(peek(0)))
    {
        runtime_error("Only instances have attributes");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(peek(0));
    ObjString* name = READ_STRING(bytes);
    Value val;
    if(hash_table_get(&instance->attributes, name, &val))
    {
        uint8_t scope = hash_table_get_scope(&instance->attributes, name) - 1;
        if(!IS_VAR_PUB(scope))
        {
            runtime_error("Trying to access non public attribute '%s'", name->chars);
            return false;
        }
        if(IS_VAR_METHOD(scope))
        {
            ObjBoundMethod* bound = new_bound_method(peek(0), AS_OBJ(val));
            val = OBJ_VAL((Obj*)bound);
        }
        if(get)
        {
            pop();
        }
        push(val);
    }
    else
    {
        runtime_error("Undefined attribute '%s'", name->chars);
        return false;
    }
    return true;
}

static bool get_this_attr(size_t bytes, bool get)
{
    if(!IS_INSTANCE(peek(0)))
    {
        runtime_error("Only instances have attributes");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(peek(0));
    ObjString* name = READ_STRING(bytes);
    Value val;
    if(hash_table_get(&instance->attributes, name, &val))
    {
        uint8_t scope = hash_table_get_scope(&instance->attributes, name) - 1;
        if(IS_VAR_METHOD(scope))
        {
            ObjBoundMethod* bound = new_bound_method(peek(0), AS_OBJ(val));
            val = OBJ_VAL((Obj*)bound);
        }
        if(get)
        {
            pop();
        }
        push(val);
    }
    else
    {
        runtime_error("Undefined attribute '%s'", name->chars);
        return false;
    }
    return true;
}

static bool set_attr(size_t bytes)
{
    if(!IS_INSTANCE(peek(1)))
    {
        runtime_error("Only instances have attributes");
        return INTERPRET_RUNTIME_ERROR;
    }
    ObjInstance* instance = AS_INSTANCE(peek(1));
    ObjString* name = READ_STRING(bytes);
    uint8_t scope = hash_table_get_scope(&instance->attributes, name);
    if(scope > 0)
    {
        scope -= 1;
        if(!IS_VAR_PUB(scope))
        {
            runtime_error("Trying to access non public attribute '%s'", name->chars);
            return false;
        }
        else if(IS_VAR_CONST(scope))
        {
            runtime_error("Assigning to constant attribute '%s'", name->chars);
            return false;
        }
        hash_table_set(&instance->attributes, name, peek(0));
        Value value = pop();
        pop();
        push(value);
    }
    else
    {
        runtime_error("Undefined attribute '%s'", name->chars);
        return false;
    }
    return true;
}

static bool set_this_attr(size_t bytes)
{
    if(!IS_INSTANCE(peek(1)))
    {
        runtime_error("Only instances have attributes");
        return INTERPRET_RUNTIME_ERROR;
    }
    ObjInstance* instance = AS_INSTANCE(peek(1));
    ObjString* name = READ_STRING(bytes);
    uint8_t scope = hash_table_get_scope(&instance->attributes, name);
    if(scope > 0)
    {
        scope -= 1;
        if(IS_VAR_CONST(scope))
        {
            runtime_error("Assigning to constant attribute '%s'", name->chars);
            return false;
        }
        hash_table_set(&instance->attributes, name, peek(0));
        Value value = pop();
        pop();
        push(value);
    }
    else
    {
        runtime_error("Undefined attribute '%s'", name->chars);
        return false;
    }
    return true;
}

#undef READ_STRING

static void call(ObjFunc* func)
{
    vm.ip = vm.chunk->code + func->offset;
}

static bool call_value(Value callee, size_t extra_inputs)
{
    if(IS_OBJ(callee))
    {
        switch(OBJ_TYPE(callee))
        {
            case OBJ_FUNC:
            {
                ObjFunc* func = AS_FUNC(callee);
                if(!setup_call(func->num_inputs + extra_inputs))
                {
                    return false;
                }
                call(func);
                return true;
            }
            case OBJ_CLOSURE:
            {
                ObjFunc* func = AS_CLOSURE(callee)->func;
                if(!setup_call(func->num_inputs + extra_inputs))
                {
                    return false;
                }
                call(func);
                return true;
            }
            case OBJ_BOUND_METHOD:
            {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                push(bound->reciever);
                return call_value(OBJ_VAL(bound->method), 1);
            }
            case OBJ_NATIVE:
            {
                ObjNative* native = AS_NATIVE(callee);
                if(!setup_call(native->num_inputs + extra_inputs))
                {
                    return false;
                }
                Value ret = native->func(vm.stack_base);
                close_func_upvalues();
                vm.stack_top = vm.stack_base;
                Value call_base_addr = pop();
                Value base_addr = pop();
                pop(); // remove return address (don't need)
                vm.call_base = (Value*)(size_t)AS_INT(call_base_addr);
                vm.stack_base = (Value*)(size_t)AS_INT(base_addr);
                pop(); // remove calling function
                push(ret);
                return true;
            }
            case OBJ_CLASS:
            {
                ObjClass* klass = AS_CLASS(callee);
                if(!setup_call(0))
                {
                    return false;
                }
                Value ret = OBJ_VAL((Obj*)new_instance(klass));
                close_func_upvalues();
                vm.stack_top = vm.stack_base;
                Value call_base_addr = pop();
                Value base_addr = pop();
                pop();
                vm.call_base = (Value*)(size_t)AS_INT(call_base_addr);
                vm.stack_base = (Value*)(size_t)AS_INT(base_addr);
                pop();
                push(ret);
                return true;
            }
            default:
            {
                break;
            }
        }
    }
    runtime_error("Calling a variable that isn't a function or class");
    return false;
}

static void define_attr(ObjString* name)
{
    Value attr = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    uint8_t scope = (uint8_t)*vm.ip;
    vm.ip++;
    hash_table_insert(&klass->attributes, name, scope, attr);
    pop();
}

#define READ_STRING(offset_size) AS_STRING(read_const(offset_size))

static InterpretResult run()
{
    vm.running = true;
    for(;;)
    {
        vm.gc = true;
#ifdef DEBUG_TRACE_EXECUTION
        printf("        ");
        for(Value* slot = vm.stack_base; slot < vm.stack_top; slot++)
        {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_inst(vm.chunk, (size_t)(vm.ip - vm.chunk->code));
#endif
        inst_type inst;
        switch(inst = READ_INST())
        {
            case OP_RETURN:
            {
                Value ret = pop();
                close_func_upvalues();
                vm.stack_top = vm.stack_base;
                Value call_base_addr = pop();
                Value base_addr = pop();
                Value ret_addr = pop();
                vm.call_base = (Value*)(size_t)AS_INT(call_base_addr);
                vm.stack_base = (Value*)(size_t)AS_INT(base_addr);
                vm.ip = vm.chunk->code + (size_t)AS_INT(ret_addr);
                pop(); // remove calling function
                push(ret);
                break;
            }
            case OP_EXIT:
            {
                return INTERPRET_OK;
            }
            case OP_CONST_BYTE:
            {
                Value constant = read_const(1);
                push(constant);
                break;
            }
            case OP_CONST_SHORT:
            {
                Value constant = read_const(2);
                push(constant);
                break;
            }
            case OP_CONST_WORD:
            {
                Value constant = read_const(4);
                push(constant);
                break;
            }
            case OP_CONST_LONG:
            {
                Value constant = read_const(8);
                push(constant);
                break;
            }
            case OP_NULL:
            {
                push(NULL_VAL);
                break;
            }
            case OP_TRUE:
            {
                push(BOOL_VAL(true));
                break;
            }
            case OP_FALSE:
            {
                push(BOOL_VAL(false));
                break;
            }
            case OP_NEGATE:
            {
                if(IS_INT(peek(0)))
                {
                    push(INT_VAL(-AS_INT(pop())));
                }
                else if(IS_FLOAT(peek(0)))
                {
                    push(FLOAT_VAL(-AS_FLOAT(pop())));
                }
                else
                {
                    runtime_error("Operand must be an integer or float");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_ADD:
            {
                if(IS_STRING(peek(0)) && IS_STRING(peek(1)))
                {
                    concatenate();
                }
                else if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a + b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a + (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a + b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a + b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUB:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a - b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a - (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a - b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a - b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_MUL:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a * b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a * (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a * b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a * b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DIV:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b == 0)
                    {
                        runtime_error("Divide by 0 error");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a / b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a / (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a / b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a / b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_REM:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b == 0)
                    {
                        runtime_error("Divide by 0 error");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a % b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_NOT:
            {
                if(IS_BOOL(peek(0)))
                {
                    push(BOOL_VAL(!AS_BOOL(pop())));
                }
                else
                {
                    runtime_error("Operand must be a boolean");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_NOT:
            {
                if(IS_INT(peek(0)))
                {
                    push(INT_VAL(~AS_INT(pop())));
                }
                else
                {
                    runtime_error("Operand must be an integer");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_AND:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a & b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_OR:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a | b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_XOR:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a ^ b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SHIFT_LEFT:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b < 0)
                    {
                        runtime_error("Shift value can't be negative");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a << b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SHIFT_ARITH_RIGHT:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b < 0)
                    {
                        runtime_error("Shift value can't be negative");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a >> b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SHIFT_LOGIC_RIGHT:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b < 0)
                    {
                        runtime_error("Shift value can't be negative");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    if(a < 0 && b > 0)
                    {
                        a &= 0x7fffffffffffffff;
                    }
                    push(INT_VAL(a >> b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQL:
            {
                Value b = pop();
                Value a = pop();
                if(a.type == VAL_NULL || b.type == VAL_NULL || a.type == b.type)
                {
                    push(BOOL_VAL(values_eql(a, b)));
                }
                else
                {
                    runtime_error("Operands must be the same type");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GREATER:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(BOOL_VAL(a > b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(BOOL_VAL(a > b));
                }
                else
                {
                    runtime_error("Operands must be the same type and a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_LESS:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(BOOL_VAL(a < b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(BOOL_VAL(a < b));
                }
                else
                {
                    runtime_error("Operands must be the same type and a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CAST_BOOL:
            {
                Value val = pop();
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(val);
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(BOOL_VAL(false));
                        break;
                    }
                    case VAL_INT:
                    {
                        push(BOOL_VAL(AS_INT(val) != 0));
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        push(BOOL_VAL(AS_FLOAT(val) != 0.0));
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                if(AS_STRING(val)->len == 4 && memcmp(AS_CSTRING(val), "true", 4) == 0)
                                {
                                    push(BOOL_VAL(true));
                                }
                                else if(AS_STRING(val)->len == 5 && memcmp(AS_CSTRING(val), "false", 5) == 0)
                                {
                                    push(BOOL_VAL(false));
                                }
                                else
                                {
                                    runtime_error("Cannot cast '%s' to bool", AS_CSTRING(val));
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                break;
                            }
                            default:
                            {
                                push(BOOL_VAL(true));
                                break;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_CAST_INT:
            {
                Value val = pop();
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(INT_VAL(AS_BOOL(val) ? 1 : 0));
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(INT_VAL(0));
                        break;
                    }
                    case VAL_INT:
                    {
                        push(val);
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        push(INT_VAL((int64_t)AS_FLOAT(val)));
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                int64_t int_val = 0;
                                if(!str_to_int(&int_val, AS_CSTRING(val), AS_STRING(val)->len))
                                {
                                    runtime_error("Cannot cast '%s' to int", AS_CSTRING(val));
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                push(INT_VAL(int_val));
                                break;
                            }
                            default:
                            {
                                runtime_error("Unsupported conversion of object to int");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_CAST_STR:
            {
                Value val = pop();
                push(OBJ_VAL((Obj*)value_to_str(val)));
                break;
            }
            case OP_CAST_FLOAT:
            {
                Value val = pop();
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(FLOAT_VAL(AS_BOOL(val) ? 1.0 : 0.0));
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(FLOAT_VAL(0.0));
                        break;
                    }
                    case VAL_INT:
                    {
                        push(FLOAT_VAL((double)AS_INT(val)));
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        push(val);
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                double float_val = 0;
                                if(!str_to_float(&float_val, AS_CSTRING(val), AS_STRING(val)->len))
                                {
                                    runtime_error("Cannot cast '%s' to float", AS_CSTRING(val));
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                push(FLOAT_VAL(float_val));
                                break;
                            }
                            default:
                            {
                                runtime_error("Unsupported conversion of object to float");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_POP:
            {
                pop();
                break;
            }
            case OP_CLOSE_UPVALUE:
            {
                close_upvalue();
                pop();
                break;
            }
            case OP_GET_GLOBAL_BYTE:
            {
                Value value = read_global(1);
                push(value);
                break;
            }
            case OP_GET_GLOBAL_SHORT:
            {
                Value value = read_global(2);
                push(value);
                break;
            }
            case OP_GET_GLOBAL_WORD:
            {
                Value value = read_global(4);
                push(value);
                break;
            }
            case OP_GET_GLOBAL_LONG:
            {
                Value value = read_global(8);
                push(value);
                break;
            }
            case OP_SET_GLOBAL_BYTE:
            {
                Value value = peek(0);
                write_global(1, value);
                break;
            }
            case OP_SET_GLOBAL_SHORT:
            {
                Value value = peek(0);
                write_global(2, value);
                break;
            }
            case OP_SET_GLOBAL_WORD:
            {
                Value value = peek(0);
                write_global(4, value);
                break;
            }
            case OP_SET_GLOBAL_LONG:
            {
                Value value = peek(0);
                write_global(8, value);
                break;
            }
            case OP_GET_UPVALUE_BYTE:
            {
                size_t slot = read_inst_index(1);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                push(*closure->upvalues[slot].upvalue->value);
                break;
            }
            case OP_GET_UPVALUE_SHORT:
            {
                size_t slot = read_inst_index(2);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                push(*closure->upvalues[slot].upvalue->value);
                break;
            }
            case OP_GET_UPVALUE_WORD:
            {
                size_t slot = read_inst_index(4);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                push(*closure->upvalues[slot].upvalue->value);
                break;
            }
            case OP_GET_UPVALUE_LONG:
            {
                size_t slot = read_inst_index(8);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                push(*closure->upvalues[slot].upvalue->value);
                break;
            }
            case OP_SET_UPVALUE_BYTE:
            {
                size_t slot = read_inst_index(1);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                Value value = peek(0);
                *closure->upvalues[slot].upvalue->value = value;
                break;
            }
            case OP_SET_UPVALUE_SHORT:
            {
                size_t slot = read_inst_index(2);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                Value value = peek(0);
                *closure->upvalues[slot].upvalue->value = value;
                break;
            }
            case OP_SET_UPVALUE_WORD:
            {
                size_t slot = read_inst_index(4);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                Value value = peek(0);
                *closure->upvalues[slot].upvalue->value = value;
                break;
            }
            case OP_SET_UPVALUE_LONG:
            {
                size_t slot = read_inst_index(8);
                ObjClosure* closure = AS_CLOSURE(vm.stack_base[-4]);
                Value value = peek(0);
                *closure->upvalues[slot].upvalue->value = value;
                break;
            }
            case OP_GET_LOCAL_BYTE:
            {
                size_t slot = read_inst_index(1);
                push(vm.stack_base[slot]);
                break;
            }
            case OP_GET_LOCAL_SHORT:
            {
                size_t slot = read_inst_index(2);
                push(vm.stack_base[slot]);
                break;
            }
            case OP_GET_LOCAL_WORD:
            {
                size_t slot = read_inst_index(4);
                push(vm.stack_base[slot]);
                break;
            }
            case OP_GET_LOCAL_LONG:
            {
                size_t slot = read_inst_index(8);
                push(vm.stack_base[slot]);
                break;
            }
            case OP_SET_LOCAL_BYTE:
            {
                size_t slot = read_inst_index(1);
                vm.stack_base[slot] = peek(0);
                break;
            }
            case OP_SET_LOCAL_SHORT:
            {
                size_t slot = read_inst_index(2);
                vm.stack_base[slot] = peek(0);
                break;
            }
            case OP_SET_LOCAL_WORD:
            {
                size_t slot = read_inst_index(4);
                vm.stack_base[slot] = peek(0);
                break;
            }
            case OP_SET_LOCAL_LONG:
            {
                size_t slot = read_inst_index(8);
                vm.stack_base[slot] = peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE_BYTE:
            {
                size_t offset = read_jump(1);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == false)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_FALSE_SHORT:
            {
                size_t offset = read_jump(2);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == false)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_FALSE_WORD:
            {
                size_t offset = read_jump(4);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == false)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_FALSE_LONG:
            {
                size_t offset = read_jump(8);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == false)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE_BYTE:
            {
                size_t offset = read_jump(1);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == true)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE_SHORT:
            {
                size_t offset = read_jump(2);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == true)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE_WORD:
            {
                size_t offset = read_jump(4);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == true)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE_LONG:
            {
                size_t offset = read_jump(8);
                if(!IS_BOOL(peek(0)))
                {
                    runtime_error("Condition expression is not boolean");
                }
                if(AS_BOOL(peek(0)) == true)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP_BYTE:
            {
                size_t offset = read_jump(1);
                vm.ip += offset;
                break;
            }
            case OP_JUMP_SHORT:
            {
                size_t offset = read_jump(2);
                vm.ip += offset;
                break;
            }
            case OP_JUMP_WORD:
            {
                size_t offset = read_jump(4);
                vm.ip += offset;
                break;
            }
            case OP_JUMP_LONG:
            {
                size_t offset = read_jump(8);
                vm.ip += offset;
                break;
            }
            case OP_JUMP_BACK_BYTE:
            {
                size_t offset = read_jump(1);
                vm.ip -= offset;
                break;
            }
            case OP_JUMP_BACK_SHORT:
            {
                size_t offset = read_jump(2);
                vm.ip -= offset;
                break;
            }
            case OP_JUMP_BACK_WORD:
            {
                size_t offset = read_jump(4);
                vm.ip -= offset;
                break;
            }
            case OP_JUMP_BACK_LONG:
            {
                size_t offset = read_jump(8);
                vm.ip -= offset;
                break;
            }
            case OP_INIT_ARRAY:
            {
                if(!IS_INT(peek(0)))
                {
                    runtime_error("Must have integer size for array");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(AS_INT(peek(0)) <= 0)
                {
                    runtime_error("Array size must be greater than 0");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value size = pop();
                Value val = pop();
                push(OBJ_VAL((Obj*)build_array(AS_INT(size), val)));
                break;
            }
            case OP_FILL_ARRAY:
            {
                if(!IS_INT(peek(0)))
                {
                    runtime_error("Must have integer size for array");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(AS_INT(peek(0)) <= 0)
                {
                    runtime_error("Array size must be greater than 0");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value size = pop();
                ObjArray* array = build_array(AS_INT(size), NULL_VAL);
                for(size_t i = AS_INT(size); i > 0; i--)
                {
                    array->data[i - 1] = pop();
                }
                push(OBJ_VAL((Obj*)array));
                break;
            }
            case OP_INDEX_GET:
            {
                if(!IS_ARRAY(peek(1)) && !IS_STRING(peek(1)))
                {
                    runtime_error("Cannot index value");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(!IS_INT(peek(0)))
                {
                    runtime_error("Must have integer index");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(AS_INT(peek(0)) < 0)
                {
                    runtime_error("Index can't be negative");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value index = pop();
                Value array = pop();
                if(IS_ARRAY(array))
                {
                    if(AS_INT(index) >= AS_ARRAY(array)->len)
                    {
                        runtime_error("Index is beyond array's bounds");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(AS_CARRAY(array)[AS_INT(index)]);
                }
                else
                {
                    if(AS_INT(index) >= AS_STRING(array)->len)
                    {
                        runtime_error("Index is beyond string's bounds");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    char c = AS_CSTRING(array)[AS_INT(index)];
                    char* letter = ALLOCATE(char, 1);
                    *letter = c;
                    push(OBJ_VAL((Obj*)take_str(letter, 1)));
                }
                break;
            }
            case OP_INDEX_PEEK:
            {
                if(!IS_ARRAY(peek(1))) 
                {
                    runtime_error("Cannot index value");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(!IS_INT(peek(0)))
                {
                    runtime_error("Must have integer index");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(AS_INT(peek(0)) < 0)
                {
                    runtime_error("Index can't be negative");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value index = peek(0);
                Value array = peek(1);
                push(AS_CARRAY(array)[AS_INT(index)]);
                break;
            }
            case OP_INDEX_SET:
            {
                if(!IS_ARRAY(peek(2)))
                {
                    runtime_error("Cannot index value");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(!IS_INT(peek(1)))
                {
                    runtime_error("Must have integer index");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if(AS_INT(peek(1)) < 0)
                {
                    runtime_error("Index can't be negative");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value val = pop();
                Value index = pop();
                Value array = pop();
                if(AS_INT(index) >= AS_ARRAY(array)->len)
                {
                    runtime_error("Index is beyond value's bounds");
                    return INTERPRET_RUNTIME_ERROR;
                }
                AS_CARRAY(array)[AS_INT(index)] = val;
                push(array);
                break;
            }
            case OP_CALL:
            {
                if(!call_value(vm.call_base[-4], 0))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_PUSH_CALL_BASE:
            {
                push(NULL_VAL);
                push(INT_VAL((int64_t)(size_t)(vm.stack_base)));
                push(INT_VAL((int64_t)(size_t)(vm.call_base)));
                vm.call_base = vm.stack_top;
                break;
            }
            case OP_CLOSURE_BYTE:
            {
                ObjClosure* closure = AS_CLOSURE(read_const(1));
                init_closure(closure);
                push(OBJ_VAL((Obj*)closure));
                break;
            }
            case OP_CLOSURE_SHORT:
            {
                ObjClosure* closure = AS_CLOSURE(read_const(2));
                init_closure(closure);
                push(OBJ_VAL((Obj*)closure));
                break;
            }
            case OP_CLOSURE_WORD:
            {
                ObjClosure* closure = AS_CLOSURE(read_const(4));
                init_closure(closure);
                push(OBJ_VAL((Obj*)closure));
                break;
            }
            case OP_CLOSURE_LONG:
            {
                ObjClosure* closure = AS_CLOSURE(read_const(8));
                init_closure(closure);
                push(OBJ_VAL((Obj*)closure));
                break;
            }
            case OP_ATTR_BYTE:
            {
                define_attr(READ_STRING(1));
                break;
            }
            case OP_ATTR_SHORT:
            {
                define_attr(READ_STRING(2));
                break;
            }
            case OP_ATTR_WORD:
            {
                define_attr(READ_STRING(4));
                break;
            }
            case OP_ATTR_LONG:
            {
                define_attr(READ_STRING(8));
                break;
            }
            case OP_ATTR_GET_BYTE:
            {
                if(get_attr(1, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_GET_SHORT:
            {
                if(get_attr(2, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_GET_WORD:
            {
                if(get_attr(4, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_GET_LONG:
            {
                if(get_attr(8, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_BYTE:
            {
                if(get_attr(1, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_SHORT:
            {
                if(get_attr(2, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_WORD:
            {
                if(get_attr(4, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_LONG:
            {
                if(get_attr(8, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_BYTE:
            {
                if(set_attr(1))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_SHORT:
            {
                if(set_attr(2))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_WORD:
            {
                if(set_attr(4))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_LONG:
            {
                if(set_attr(8))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_GET_THIS_BYTE:
            {
                if(get_this_attr(1, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_GET_THIS_SHORT:
            {
                if(get_this_attr(2, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_GET_THIS_WORD:
            {
                if(get_this_attr(4, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_GET_THIS_LONG:
            {
                if(get_this_attr(8, true))
                {
                    break;    
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_THIS_BYTE:
            {
                if(get_this_attr(1, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_THIS_SHORT:
            {
                if(get_this_attr(2, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_THIS_WORD:
            {
                if(get_this_attr(4, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_PEEK_THIS_LONG:
            {
                if(get_this_attr(8, false))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_THIS_BYTE:
            {
                if(set_this_attr(1))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_THIS_SHORT:
            {
                if(set_this_attr(2))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_THIS_WORD:
            {
                if(set_this_attr(4))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ATTR_SET_THIS_LONG:
            {
                if(set_this_attr(8))
                {
                    break;
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            default:
            {
                runtime_error("Unknown instruction %u", inst);
                return INTERPRET_RUNTIME_ERROR;
            }
        }
    }
    vm.running = false;
}
#undef READ_INST
#undef READ_STRING

InterpretResult interpret(const char* src, HashTable* global_names, Chunk* main_chunk)
{
    Chunk chunk;
    init_chunk(&chunk);
    if(main_chunk == NULL)
    {
        vm.chunk = &chunk;
    }
    else
    {
        vm.chunk = main_chunk;
        vm.chunk->entry = vm.chunk->size;
    }
    if(!compile(src, vm.chunk, global_names))
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.ip = vm.chunk->code + vm.chunk->entry;

    InterpretResult res = run();
    free_chunk(&chunk);
    return res;
}

void push(Value value)
{
    if(vm.stack_top >= vm.stack + STACK_MAX)
    {
        runtime_error("Stack Overflow - please report on stackoverflow.com");
        return;
    }
    *vm.stack_top = value;
    vm.stack_top++;
}

Value pop()
{
    if(vm.stack_top == vm.stack)
    {
        runtime_error("No more values on stack");
        return NULL_VAL;
    }
    vm.stack_top--;
    return *vm.stack_top;
}

void free_vm()
{
    free_hash_table(&vm.strings);
    free(vm.gray_stack);
    vm.gray_size = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;
    free_objs();
}

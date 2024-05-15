#include <vm.h>
#include <common.h>
#include <stdio.h>
#include <compiler.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <lines.h>
#include <object.h>
#include <rain_memory.h>
#include <string.h>
#include <convert.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <debug.h>
#endif

VM vm;

static void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.stack_base = vm.stack;
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
    return vm.chunk->consts.values[index];
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

static bool call(ObjFunc* func, size_t args)
{
    if(args != func->num_inputs)
    {
        runtime_error("Expected %zu args but got %zu", func->num_inputs, args);
        return false;
    }
    vm.stack_base = vm.stack_top - args;
    vm.stack_base[-2] = INT_VAL((int64_t)(size_t)((vm.ip - vm.chunk->code)));
    vm.ip = vm.chunk->code + func->offset;
    return true;
}

static bool call_value(Value callee, size_t args)
{
    if(IS_OBJ(callee))
    {
        switch(OBJ_TYPE(callee))
        {
            case OBJ_FUNC:
            {
                return call(AS_FUNC(callee), args);
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

#define READ_STRING(offset_size) AS_STRING(read_const(offset_size))

static InterpretResult run()
{
    for(;;)
    {
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
                vm.ip = vm.chunk->code + (size_t)AS_INT(peek(1));
                vm.stack_top[-3] = vm.stack_top[-1];
                pop();
                pop();
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
            case OP_PRINT:
            {
                print_value(pop());
                printf("\n");
                break;
            }
            case OP_POP:
            {
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
            case OP_CALL_BYTE:
            {
                size_t args = read_inst_index(1);
                if(!call_value(peek_back(args + 2), args))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CALL_SHORT:
            {
                size_t args = read_inst_index(2);
                if(!call_value(peek_back(args + 2), args))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CALL_WORD:
            {
                size_t args = read_inst_index(4);
                if(!call_value(peek_back(args + 2), args))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CALL_LONG:
            {
                size_t args = read_inst_index(8);
                if(!call_value(peek_back(args + 2), args))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_PUSH_BASE:
            {
                push(NULL_VAL);
                push(INT_VAL((int64_t)(size_t)(vm.stack_base)));
                break;
            }
            case OP_POP_BASE:
            {
                Value ret = pop();
                vm.stack_top = vm.stack_base;
                Value val = pop();
                vm.stack_base = (Value*)(size_t)AS_INT(val);
                push(ret);
                break;
            }
            default:
            {
                runtime_error("Unknown instruction %u", inst);
                return INTERPRET_RUNTIME_ERROR;
            }
        }
    }
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
    free_objs();
}

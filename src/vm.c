#include <vm.h>
#include <common.h>
#include <stdio.h>
#include <compiler.h>
#include <stdarg.h>
#include <lines.h>
#include <object.h>
#include <rain_memory.h>
#include <string.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <debug.h>
#endif

VM vm;

static void reset_stack()
{
    vm.stack_top = vm.stack;
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
}

static Value peek(int64_t distance)
{
    return vm.stack_top[-1 - distance];
}

static void concatenate()
{
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());
    size_t len = a->len + b->len;
    char* chars = ALLOCATE(char, len + 1);
    memcpy(chars, a->chars, a->len);
    memcpy(chars + a->len, b->chars, b->len);
    chars[len] = 0;
    ObjString* res = take_str(chars, len);
    push(OBJ_VAL((Obj*)res));
}

#define READ_INST() (*(vm.ip++))

static Value read_const(size_t offset_size)
{
    size_t offset = 0;
    size_t index = read_chunk_const(vm.ip, &offset, offset_size);
    vm.ip += offset;
    return vm.chunk->consts.values[index];
}

static InterpretResult run()
{
    for(;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("        ");
        for(Value* slot = vm.stack; slot < vm.stack_top; slot++)
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
                print_value(pop());
                printf("\n");
                vm.ip++;
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
                        push(BOOL_VAL(true));
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
                        runtime_error("Unsupported conversion of object to int");
                        return INTERPRET_RUNTIME_ERROR;
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
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(OBJ_VAL((Obj*)(AS_BOOL(val) ? copy_str("true", 4) : copy_str("false", 5))));
                        break;
                    }
                    case VAL_INT:
                    {
                        int64_t num = AS_INT(val);
#ifdef LONG64
                        size_t len = (size_t)snprintf(NULL, 0, "%li", num);
#else
                        size_t len = (size_t)snprintf(NULL, 0, "%lli", num);
#endif
                        char* chars = ALLOCATE(char, len + 1);
#ifdef LONG64
                        snprintf(chars, len + 1, "%li", num);
#else
                        size_t len = (size_t)snprintf(NULL, 0, "%lli", num);
#endif
                        push(OBJ_VAL((Obj*)take_str(chars, len)));
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(OBJ_VAL((Obj*)copy_str("null", 4)));
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        double num = AS_FLOAT(val);
                        size_t len = (size_t)snprintf(NULL, 0, "%f", num);
                        char* chars = ALLOCATE(char, len + 1);
                        snprintf(chars, len + 1, "%f", num);
                        push(OBJ_VAL((Obj*)take_str(chars, len)));
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                push(val);
                                break;
                            }
                            default:
                            {
                                runtime_error("Unsupported conversion of object to string");
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
                        runtime_error("Unsupported conversion of object to float");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
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
#undef BINARY_OP

InterpretResult interpret(const char* src)
{
    Chunk chunk;
    init_chunk(&chunk);
    if(!compile(src, &chunk))
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

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
    vm.stack_top--;
    return *vm.stack_top;
}

void free_vm()
{
    free_objs();
}

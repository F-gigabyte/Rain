#include <vm.h>
#include <common.h>
#include <stdio.h>
#include <compiler.h>
#include <stdarg.h>
#include <lines.h>

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
}

static Value peek(int64_t distance)
{
    return vm.stack_top[-1 - distance];
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
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
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

}

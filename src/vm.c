#include <vm.h>
#include <common.h>
#include <stdio.h>
#include <compiler.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <debug.h>
#endif

VM vm;

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

void init_vm()
{
    reset_stack();
}

#define READ_INST() (*(vm.ip++))

static Value read_const(size_t offset_size)
{
    size_t offset = 0;
    size_t index = read_chunk_const(vm.ip, &offset, offset_size);
    vm.ip += offset;
    return vm.chunk->consts.values[index];
}

#define BINARY_OP(op) \
    do \
    { \
        Value b = pop(); \
        Value a = pop(); \
        push (a op b); \
    } while(false) \

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
            case OP_NEGATE:
            {
                push(-pop());
                break;
            }
            case OP_ADD:
            {
                BINARY_OP(+);
                break;
            }
            case OP_SUB:
            {
                BINARY_OP(-);
                break;
            }
            case OP_MUL:
            {
                BINARY_OP(*);
                break;
            }
            case OP_DIV:
            {
                BINARY_OP(/);
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

#include <vm.h>
#include <common.h>
#include <stdio.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <debug.h>
#endif

VM vm;

void init_vm()
{

}

#define READ_INST() (*(vm.ip++))

static Value read_const(size_t offset_size)
{
    size_t offset = 0;
    size_t index = read_chunk_const(vm.chunk, &offset, offset_size);
    vm.ip += offset;
    return vm.chunk->consts.values[index];
}

static InterpretResult run()
{
    for(;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        disassemble_inst(vm.chunk, (size_t)(vm.ip - vm.chunk->code));
#endif
        inst_type inst;
        switch(inst = READ_INST())
        {
            case OP_RETURN:
            {
                return INTERPRET_OK;
            }
            case OP_CONST_BYTE:
            {
                Value constant = read_const(1);
                print_value(constant);
                printf("\n");
                break;
            }
            case OP_CONST_SHORT:
            {
                Value constant = read_const(2);
                print_value(constant);
                printf("\n");
                break;
            }
            case OP_CONST_WORD:
            {
                Value constant = read_const(4);
                print_value(constant);
                printf("\n");
                break;
            }
            case OP_CONST_LONG:
            {
                Value constant = read_const(8);
                print_value(constant);
                printf("\n");
                break;
            }
        }
    }
}

#undef READ_INST

InterpretResult interpret(Chunk* chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

void free_vm()
{

}

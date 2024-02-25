#include <common.h>
#include <vm.h>
#include <debug.h>

int main(int argc, const char* argv[])
{
    init_vm();
    Chunk chunk;
    init_chunk(&chunk);
    for(size_t i = 0; i < 20; i++)
    {
        size_t constant = add_const(&chunk, (double)(i * 10) + ((double)i / 10));
        write_chunk_const(&chunk, constant, i + 1);
    }
    write_chunk(&chunk, OP_RETURN, 21);
    interpret(&chunk);
    free_vm();
    free_chunk(&chunk);
    return 0;
}

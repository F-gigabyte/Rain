#include <common.h>
#include <vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void repl()
{
    Chunk main_chunk;
    HashTable global_names;
    init_chunk(&main_chunk);
    init_hash_table(&global_names);
    for(;;)
    {
        printf("~> ");
        char* line = NULL;
        size_t buffer_size = 0;
        ssize_t len = getline(&line, &buffer_size, stdin);
        if(len == -1)
        {
            fprintf(stderr, "Unable to read input line\n");
            exit(74);
        }
        if(len == 0 || (len == 1 && line[0] == '\n'))
        {
            free(line);
            break;
        }
        interpret(line, &global_names, &main_chunk);
        free(line);
    }
}

static char* read_file(const char* path)
{
    FILE* file = fopen(path, "r");
    if(file == NULL)
    {
        fprintf(stderr, "Unable to open '%s'\n", path);
        exit(74);
    }
    if(fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        fprintf(stderr, "Unable to seek to end of '%s'\n", path);
        exit(74);
    }
    size_t file_size = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(file_size + 1);
    if(buffer == NULL)
    {
        fprintf(stderr, "Unable to allocate enough memory to read '%s'\n", path);
        exit(74);
    }
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = 0;
    if(bytes_read < file_size)
    {
        free(buffer);
        fprintf(stderr, "Could not read file '%s'\n", path);
        exit(74);
    }
    fclose(file);
    return buffer;
}

static void run_file(const char* path)
{
    char* src = read_file(path);
    InterpretResult result = interpret(src, NULL, NULL);
    free(src);
    if(result == INTERPRET_COMPILE_ERROR)
    {
        exit(65);
    }
    if(result == INTERPRET_RUNTIME_ERROR)
    {
        exit(70);
    }
}

int main(int argc, const char* argv[])
{
    init_vm();
    
    if(argc == 1)
    {
        repl();
    }
    else if(argc == 2)
    {
        run_file(argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage: %s [path]\n", argv[0]);
        exit(64);
    }

    free_vm();
    return 0;
}

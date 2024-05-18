#include <common.h>
#include <vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* merge_text(const char* a, const char* b, size_t len_a, size_t len_b)
{
    size_t len = len_a + len_b + 1;
    char* res = malloc(len);
    memcpy(res, a, len_a);
    memcpy(res + len_a, b, len_b);
    res[len - 1] = 0;
    return res;
}

static void repl()
{
    Chunk main_chunk;
    HashTable global_names;
    init_chunk(&main_chunk);
    init_hash_table(&global_names);
    char* current_text = NULL;
    size_t len = 0;
    size_t paren = 0;
    for(;;)
    {
        if(current_text == NULL)
        {
            printf("~> ");
        }
        else
        {
            printf("   ");
        }
        char* line = NULL;
        size_t buffer_size = 0;
        ssize_t len_line = getline(&line, &buffer_size, stdin);
        size_t paren_closed = 0;
        bool term_statement = false;
        if(len_line == -1)
        {
            fprintf(stderr, "Unable to read input line\n");
            exit(74);
        }
        if((len_line == 0 || (len_line == 1 && line[0] == '\n')) && paren == 0)
        {
            free(line);
            break;
        }
        for(size_t i = 0; i < len_line; i++)
        {
            switch(line[i])
            {
                case '[':
                case '{':
                case '(':
                {
                    paren++;
                    term_statement = false;
                    break;
                }
                case ']':
                case '}':
                case ')':
                {
                    if(paren > 0)
                    {
                        paren--;
                        paren_closed++;
                    }
                    term_statement = false;
                    break;
                }
                case ';':
                {
                    term_statement = true;
                    break;
                }
                case '\t':
                case ' ':
                case '\n':
                case '\r':
                {
                    break;
                }
                default:
                {
                    term_statement = false;
                    break;
                }
            }
        }
        char* next_text = merge_text(current_text, line, len, len_line);
        free(current_text);
        current_text = next_text;
        len += len_line;
        if(paren == 0 && (paren_closed > 0 || term_statement))
        {
            interpret(current_text, &global_names, &main_chunk);
            free(current_text);
            current_text = NULL;
            len = 0;
        }
        free(line);
    }
    free(current_text);
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

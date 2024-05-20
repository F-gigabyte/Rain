#include <natives.h>
#include <time.h>
#include <stdio.h>
#include <object.h>

Value time_native(Value* args)
{
    struct timespec res;
    if(clock_gettime(CLOCK_REALTIME, &res) != 0)
    {
        return INT_VAL(0);
    }
    return INT_VAL((int64_t)(((size_t)res.tv_sec * 1000000000) + (size_t)res.tv_nsec));
}

Value print_native(Value* args)
{
    print_value(args[0]);
    return NULL_VAL;
}

Value println_native(Value* args)
{
    print_value(args[0]);
    printf("\n");
    return NULL_VAL;
}

Value input_native(Value* args)
{
    print_value(args[0]);
    char* input = NULL;
    size_t input_size = 0;
    ssize_t len = getline(&input, &input_size, stdin);
    if(len == -1)
    {
        return NULL_VAL;
    }
    return OBJ_VAL((Obj*)take_str(input, len));
}

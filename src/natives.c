#include <natives.h>
#include <time.h>
#include <stdio.h>

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
    printf("\n");
    return NULL_VAL;
}
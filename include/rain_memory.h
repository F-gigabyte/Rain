#ifndef RAIN_MEMORY_H
#define RAIN_MEMORY_H

#include <common.h>

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, ptr, old_size, new_size) \
    (type*)reallocate(ptr, sizeof(type) * (old_size), sizeof(type) * (new_size))

#define FREE_ARRAY(type, ptr, old_size) \
    (void)reallocate(ptr, sizeof(type) * (old_size), 0)

// resize a section of allocated memory
void* reallocate(void* ptr, size_t old_size, size_t new_size);

#endif

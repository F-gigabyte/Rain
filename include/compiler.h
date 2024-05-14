#ifndef RAIN_COMPILER_H
#define RAIN_COMPILER_H

#include <vm.h>

bool compile(const char* src, Chunk* chunk, HashTable* global_names);

#endif

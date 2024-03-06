#ifndef RAIN_HASH_TABLE_H
#define RAIN_HASH_TABLE_H

#include <common.h>
#include <value.h>

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    size_t count;
    size_t capacity;
    Entry* entries;
} HashTable;

void init_hash_table(HashTable* table);
void free_hash_table(HashTable* table);
bool hash_table_set(HashTable* table, ObjString* key, Value value);
bool hash_table_get(HashTable* table, ObjString* key, Value* value);
bool hash_table_delete(HashTable* table, ObjString* key);
void copy_hash_table(HashTable* from, HashTable* to);
ObjString* hash_table_find_str(HashTable* table, const char* chars, size_t len, uint32_t hash);

#endif
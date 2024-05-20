#ifndef RAIN_HASH_TABLE_H
#define RAIN_HASH_TABLE_H

#include <common.h>
#include <value.h>

typedef struct
{
    bool constant;
    Value value;
} VarValue;

typedef struct {
    ObjString* key;
    VarValue var;
} Entry;

typedef struct {
    size_t count;
    size_t capacity;
    Entry* entries;
} HashTable;

void init_hash_table(HashTable* table);
void free_hash_table(HashTable* table);
bool hash_table_insert(HashTable* table, ObjString* key, bool constant, Value value);
bool hash_table_set(HashTable* table, ObjString* key, Value value);
bool hash_table_get(HashTable* table, ObjString* key, Value* value);
// 0 -> not present, 1 -> variable, 2 -> constant
uint8_t hash_table_is_const(HashTable* table, ObjString* key);
bool hash_table_delete(HashTable* table, ObjString* key);
void copy_hash_table(HashTable* from, HashTable* to);
ObjString* hash_table_find_str(HashTable* table, const char* chars, size_t len, uint32_t hash);
void hash_table_remove_clear(HashTable* table);

#endif

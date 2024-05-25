#ifndef RAIN_HASH_TABLE_H
#define RAIN_HASH_TABLE_H

#include <common.h>
#include <value.h>

#define IS_VAR_CONST(scope) ((scope) & 1)
#define SCOPE_VISIBILITY(scope) (((scope) >> 1) & 3)
#define IS_VAR_PRIV(scope) (SCOPE_VISIBILITY(scope) == 0)
#define IS_VAR_PROT(scope) (SCOPE_VISIBILITY(scope) == 1)
#define IS_VAR_PUB(scope) (SCOPE_VISIBILITY(scope) == 3)
#define IS_VAR_METHOD(scope) ((scope >> 3) & 1)

#define VAR_METHOD 0x8
#define VAR_PUB 0x6
#define VAR_PROT 0x2
#define VAR_PRIV 0x0
#define VAR_CONST 0x1

/* Scope (4 bits)
 *  method - 1 bit scope - 2 bits  const - 1 bit
 *  const
 *  1 - constant, 0 - variable, non constant
 *  scope
 *  private   - 00
 *  protected - 01 (subclasses)
 *  public    - 11
 *  method
 *  1 - is a method, 0 - is a variable (used by classes and instances)
*/

typedef struct
{
    uint8_t scope;
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
bool hash_table_insert(HashTable* table, ObjString* key, uint8_t scope, Value value);
bool hash_table_set(HashTable* table, ObjString* key, Value value);
bool hash_table_get(HashTable* table, ObjString* key, Value* value);
// 0 -> not present, else scope + 1
uint8_t hash_table_get_scope(HashTable* table, ObjString* key);
bool hash_table_delete(HashTable* table, ObjString* key);
void copy_hash_table(HashTable* from, HashTable* to);
ObjString* hash_table_find_str(HashTable* table, const char* chars, size_t len, uint32_t hash);
void hash_table_remove_clear(HashTable* table);

#endif

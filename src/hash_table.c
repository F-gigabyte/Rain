#include <hash_table.h>
#include <rain_memory.h>
#include <object.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

void init_hash_table(HashTable* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_hash_table(HashTable* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    init_hash_table(table);
}

static Entry* find_entry(Entry* entries, size_t len, ObjString* key)
{
    size_t index = key->hash % len;
    Entry* tombstone = NULL;
    for(;;)
    {
        Entry* entry = entries + index;
        if(entry->key == NULL)
        {
            if(IS_NULL(entry->value))
            {
                return tombstone != NULL ? tombstone : entry;
            }
            else if(tombstone == NULL)
            {
                tombstone = entry;
            }
        }
        else if(entry->key == key)
        {
            return entry;
        }
        index = ((index + 1) * (index + 1)) % len;
    }
}

static void adjust_capacity(HashTable* table, size_t capacity)
{
    Entry* entries = ALLOCATE(Entry, capacity);
    for(size_t i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NULL_VAL;
    }
    table->count = 0;
    for(size_t i = 0; i < table->capacity; i++)
    {
        Entry* entry = table->entries + i;
        if(entry->key != NULL)
        {
            Entry* dest = find_entry(entries, capacity, entry->key);
            dest->key = entry->key;
            dest->value = entry->value;
            table->count++;
        }
    }
    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool hash_table_set(HashTable* table, ObjString* key, Value value)
{
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        size_t capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    Entry* entry = find_entry(table->entries, table->capacity, key);
    bool new_key = entry->key == NULL;
    if(new_key && IS_NULL(entry->value))
    {
        table->count++;
    }
    entry->key = key;
    entry->value = value;
    return new_key;
}

bool hash_table_get(HashTable* table, ObjString* key, Value* value)
{
    if(table->count == 0)
    {
        return false;
    }
    Entry* entry = find_entry(table->entries, table->capacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    *value = entry->value;
    return true;
}

bool hash_table_delete(HashTable* table, ObjString* key)
{
    if(table->count == 0)
    {
        return false;
    }
    Entry* entry = find_entry(table->entries, table->capacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

void copy_hash_table(HashTable* from, HashTable* to)
{
    for(size_t i = 0; i < from->capacity; i++)
    {
        Entry* entry = from->entries + i;
        if(entry->key != NULL)
        {
            hash_table_set(to, entry->key, entry->value);
        }
    }
}

ObjString* hash_table_find_str(HashTable* table, const char* chars, size_t len, uint32_t hash)
{
    if(table->count == 0)
    {
        return NULL;
    }
    uint32_t index = hash % table->capacity;
    for(;;)
    {
        Entry* entry = &table->entries[index];
        if(entry->key == NULL)
        {
            if(IS_NULL(entry->value))
            {
                return NULL;
            }
        }
        else if(entry->key->len == len && entry->key->hash == hash && memcmp(entry->key->chars, chars, len) == 0)
        {
            return entry->key;
        }
        index = ((index + 1) * (index + 1)) % table->capacity;
    }
}

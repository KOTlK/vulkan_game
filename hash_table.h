#pragma once

#include "basic.h"
#include "allocator.h"
#include "assert.h"
#include <memory.h>
#include "hash_functions.h"

#define HASH_TABLE_INITIAL_LENGTH  256
#define HASH_TABLE_REALLOC_STEP    128
#define HASH_TABLE_MAX_LOAD_FACTOR 70

#define Template template <typename Key, typename Value>

Template
struct HashTableSlot {
    Key   key;
    Value value;
    u64   hash;
    bool  tombstone;
};

Template
struct HashTable {
    HashTableSlot<Key, Value>* data;
    u32                   count;
    u32                   length;
    Allocator*            allocator;

    HashTable(u32 length = HASH_TABLE_INITIAL_LENGTH, Allocator* allocator = &Allocator_Persistent) :
                                                                   count(0),
                                                                   length(length),
                                                                   allocator(allocator) {
        data = (HashTableSlot<Key, Value>*)allocator_alloc(allocator, sizeof(HashTableSlot<Key, Value>) * length);
        Assert(data, "Cannot allocate memory for hash_table data.");

        memset(data, 0, sizeof(HashTableSlot<Key, Value>) * length);
    }

    ~HashTable() {
        if (allocator == &Allocator_Temp) return;

        allocator_free(allocator, data);
    }

    Value& operator[](Key key) {
        u64 hash      = get_hash(key);
        u32 iteration = 0;
        u32 index     = 0;

        while (true) {
            index = table_double_hash(hash, length, iteration++);

            if (data[index].tombstone)    continue;
            if (data[index].hash == hash) break;
            if (data[index].hash == 0)    break;
        }

        Assert(data[index].hash == hash, "The key was not presented in the hash table.");

        return data[index].value;
    }

    const Value& operator[](Key key) const {
        u64 hash      = get_hash(key);
        u32 iteration = 0;
        u32 index     = 0;

        while (true) {
            index = table_double_hash(hash, length, iteration++);

            if (data[index].tombstone)    continue;
            if (data[index].hash == hash) break;
            if (data[index].hash == 0)    break;
        }

        Assert(data[index].hash == hash, "The key was not presented in the hash table.");

        return data[index].value;
    }
};

Template
static inline
void
table_make(HashTable<Key, Value>* hash_table, u32 length = HASH_TABLE_INITIAL_LENGTH, Allocator* allocator = &Allocator_Persistent);

Template
static inline
void
table_realloc(HashTable<Key, Value>* hash_table, u32 length);

Template
static inline
void
table_free(HashTable<Key, Value>* hash_table);

Template
static inline
void
table_add(HashTable<Key, Value>* hash_table, Key key, Value value);

Template
static inline
void
table_set(HashTable<Key, Value>* hash_table, Key key, Value value);

Template
static inline
bool
table_add_or_set(HashTable<Key, Value>* hash_table, Key key, Value value); // Adds or sets element. If element with the same key already been added, returns true, otherwise return false.

Template
static inline
void
table_remove(HashTable<Key, Value>* hash_table, Key key);

Template
static inline
bool
table_remove_if_contains(HashTable<Key, Value>* hash_table, Key key); // Removes element from hash table if it exist. Returns true if element was removed, false if not.

Template
static inline
bool
table_contains(HashTable<Key, Value>* hash_table, Key key);

Template
static inline
Value
table_get(HashTable<Key, Value>* hash_table, Key key);

template <typename Key, typename Value, typename Iterator>
static inline
void
table_iterate(HashTable<Key, Value>* hash_table, Iterator iterator_func);

static inline
u64
table_double_hash(u64 hash, u32 length, u32 iteration = 0);


// Implementation
Template
static inline
void
table_make(HashTable<Key, Value>* hash_table, u32 length, Allocator* allocator) {
    auto data = (HashTableSlot<Key, Value>*)allocator_alloc(allocator, sizeof(HashTableSlot<Key, Value>) * length);
    Assert(data, "Cannot allocate memory for hash_table data.");

    memset(data, 0, sizeof(HashTableSlot<Key, Value>) * length);

    hash_table->data      = data;
    hash_table->count     = 0;
    hash_table->length    = length;
    hash_table->allocator = allocator;
}

Template
static inline
void
table_realloc(HashTable<Key, Value>* hash_table, u32 length) {
    Assert(length > hash_table->length, "Cannot resize hash table with less size.");

    auto new_data = (HashTableSlot<Key, Value>*)allocator_alloc(hash_table->allocator, sizeof(HashTableSlot<Key, Value>) * length);
    Assert(new_data, "Cannot allocate enough memory for new hash table data");

    memset(new_data, 0, sizeof(HashTableSlot<Key, Value>) * length);

    for (u32 i = 0; i < hash_table->length; i++) {
        if (hash_table->data[i].hash != 0) {
            u32 iteration = 0;
            u32 index     = 0;

            while (true) {
                index = table_double_hash(hash_table->data[i].hash, length, iteration++);

                if (new_data[index].hash == 0) break;
            }

            new_data[index].hash      = hash_table->data[i].hash;
            new_data[index].value     = hash_table->data[i].value;
            new_data[index].tombstone = false;
        }
    }

    if (hash_table->allocator != &Allocator_Temp) {
        allocator_free(hash_table->allocator, hash_table->data);
    }

    hash_table->data   = new_data;
    hash_table->length = length;
}

Template
static inline
void
table_free(HashTable<Key, Value>* hash_table) {
    // nothing to free if using Allocator_Temp
    if (hash_table->allocator == &Allocator_Temp) return;

    allocator_free(hash_table->allocator, hash_table->data);
    allocator_free(hash_table->allocator, hash_table);
}

Template
static inline
void
table_add(HashTable<Key, Value>* hash_table, Key key, Value value) {
    u64 hash      = get_hash(key);
    Assert(hash != 0, "Hash cannot be 0, fix your hash function.");
    u32 iteration = 0;
    u32 index     = 0;

    while (true) {
        index = table_double_hash(hash, hash_table->length, iteration++);

        if (hash_table->data[index].tombstone)    continue;
        if (hash_table->data[index].hash == hash) break;
        if (hash_table->data[index].hash == 0)    break;
    }

    Assert(hash_table->data[index].hash != hash, "An item with the same key has already been added.");

    auto slot = HashTableSlot<Key, Value> {};
    slot.hash  = hash;
    slot.value = value;

    hash_table->data[index] = slot;
    hash_table->count++;

    u32 load_factor = hash_table->count * 100 / hash_table->length;

    if (load_factor >= HASH_TABLE_MAX_LOAD_FACTOR) {
        table_realloc(hash_table, hash_table->length + HASH_TABLE_REALLOC_STEP);
    }
}

Template
static inline
void
table_set(HashTable<Key, Value>* hash_table, Key key, Value value) {
    u64 hash      = get_hash(key);
    u32 iteration = 0;
    u32 index     = 0;

    while (true) {
        index = table_double_hash(hash, hash_table->length, iteration++);

        if (hash_table->data[index].tombstone)    continue;
        if (hash_table->data[index].hash == hash) break;
        if (hash_table->data[index].hash == 0)    break;
    }

    Assert(hash_table->data[index].hash == hash, "The key is not presented in the hash table.");

    auto slot  = HashTableSlot<Key, Value> {0};
    slot.hash  = hash;
    slot.value = value;

    hash_table->data[index] = slot;
}

Template
static inline
bool
table_add_or_set(HashTable<Key, Value>* hash_table, Key key, Value value) {
    u64 hash      = get_hash(key);
    u32 iteration = 0;
    u32 index     = 0;

    while (true) {
        index = table_double_hash(hash, hash_table->length, iteration++);

        if (hash_table->data[index].tombstone)    continue;
        if (hash_table->data[index].hash == hash) break;
        if (hash_table->data[index].hash == 0)    break;
    }

    bool has = false;

    if (hash_table->data[index].hash == hash) {
        has = true;
    }

    auto slot = HashTableSlot<Key, Value> {0};
    slot.hash  = hash;
    slot.value = value;
    hash_table->data[index] = slot;

    if (!has) {
        hash_table->count++;
        u32 load_factor = hash_table->count * 100 / hash_table->length;

        if (load_factor >= HASH_TABLE_MAX_LOAD_FACTOR) {
            table_realloc(hash_table, hash_table->length + HASH_TABLE_REALLOC_STEP);
        }
    }

    return has;
}

Template
static inline
void
table_remove(HashTable<Key, Value>* hash_table, Key key) {
    u64 hash      = get_hash(key);
    u32 iteration = 0;
    u32 index     = 0;

    while (true) {
        index = table_double_hash(hash, hash_table->length, iteration++);

        if (hash_table->data[index].tombstone)    continue;
        if (hash_table->data[index].hash == hash) break;
        if (hash_table->data[index].hash == 0)    break;
    }

    hash_table->data[index].tombstone = true;
    hash_table->data[index].hash      = 0;
    hash_table->data[index].value     = {0};
    hash_table->count--;
}

Template
static inline
bool
table_remove_if_contains(HashTable<Key, Value>* hash_table, Key key) {
    u64 hash      = get_hash(key);
    u32 iteration = 0;
    u32 index     = 0;

    while (true) {
        index = table_double_hash(hash, hash_table->length, iteration++);

        if (hash_table->data[index].tombstone)    continue;
        if (hash_table->data[index].hash == hash) break;
        if (hash_table->data[index].hash == 0)    break;
    }

    if (hash_table->data[index].hash != hash) {
        return false;
    }

    hash_table->data[index].tombstone = true;
    hash_table->data[index].hash      = 0;
    hash_table->data[index].value     = {0};
    hash_table->count--;

    return true;
}

Template
static inline
bool
table_contains(HashTable<Key, Value>* hash_table, Key key) {
    u64 hash      = get_hash(key);
    u32 iteration = 0;
    u32 index     = 0;

    while (true) {
        index = table_double_hash(hash, hash_table->length, iteration++);

        if (hash_table->data[index].tombstone)    continue;
        if (hash_table->data[index].hash == hash) return true;
        if (hash_table->data[index].hash == 0)    return false;
    }
}

Template
static inline
Value
table_get(HashTable<Key, Value>* hash_table, Key key) {
    u64 hash      = get_hash(key);
    u32 iteration = 0;
    u32 index     = 0;

    while (true) {
        index = table_double_hash(hash, hash_table->length, iteration++);

        if (hash_table->data[index].tombstone)    continue;

        if (hash_table->data[index].hash == hash) return hash_table->data[index].value;
        if (hash_table->data[index].hash == 0)    return NULL;
    }
}

template <typename Key, typename Value, typename Iterator>
static inline
void
table_iterate(HashTable<Key, Value>* hash_table, Iterator iterator_func) {
    for (u32 i = 0; i < hash_table->length; i++) {
        if (hash_table->data[i].hash != 0) {
            iterator_func(hash_table->data[i].key, hash_table->data[i].value);
        }
    }
}

static inline
u64
table_double_hash(u64 hash, u32 length, u32 iteration) {
    u64 secondary = 1 + (hash % (length - 1));
    return (hash + iteration * secondary) % length;
}
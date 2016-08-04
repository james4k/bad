// sng_htable.h v0.0.0
// James Gray, July 2016
//
// OVERVIEW
//
// Experimenting with templated data structures without C++ templates.
//
// sng_htable implements a simple hash table with a fixed number of
// buckets.
//
// Each entry requires an allocation, but malloc/free can be user-defined.
//
// USAGE
//
// This is more of a template, rather than your standard header. You
// must define:
//
//  - SNG_HTABLE_HASH_FUNC
//  - SNG_HTABLE_KEY
//  - SNG_HTABLE_VALUE
//
// Optionally:
//
//  - SNG_HTABLE_NAME
//  - SNG_HTABLE_BUCKET_BITS
//
// And in a .c or .cpp file, define SNG_HTABLE_IMPLEMENTATION. All of
// these should be defined before the include.
//
// TODO
//
//  - iteration
//  - hash field in entry struct optional, for small keys
//  - tests
//  - rewrite USAGE section for clarity
//
// LICENSE
//
// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy,
// distribute, and modify this file as you see fit.
//
// No warranty. Use at your own risk.

#ifndef SNG_HTABLE_H
#define SNG_HTABLE_H

#include <stdint.h>

typedef uint32_t  b32;
typedef uintptr_t uptr;

#ifndef SNG_HTABLE_API
#define SNG_HTABLE_API
#endif

#ifndef SNG_HTABLE_MALLOC
#define SNG_HTABLE_MALLOC(x) malloc(x)
#endif

#ifndef SNG_HTABLE_FREE
#define SNG_HTABLE_FREE(x) free(x)
#endif

#ifndef SNG_HTABLE_HASH_TYPE
#define SNG_HTABLE_HASH_TYPE uint32_t
#endif

#ifndef SNG_HTABLE_HASH_FUNC
#error SNG_HTABLE_HASH_FUNC must be defined
#endif

#ifndef SNG_HTABLE_KEY
#error SNG_HTABLE_KEY must be defined
#endif

#ifndef SNG_HTABLE_VALUE
#error SNG_HTABLE_KEY must be defined
#endif

#ifndef SNG_HTABLE_NAME
#define SNG_HTABLE_NAME SngHTable
#endif

#ifndef SNG_HTABLE_FUNC_PREFIX
#define SNG_HTABLE_FUNC_PREFIX sngHTable
#endif

#ifndef SNG_HTABLE_BUCKET_BITS
#define SNG_HTABLE_BUCKET_BITS 8
#endif

#define SNG_HTABLE_BUCKET_COUNT (1 << SNG_HTABLE_BUCKET_BITS)
#define SNG_HTABLE_BUCKET_MASK (SNG_HTABLE_BUCKET_COUNT - 1)

// TODO: ugh. pragma push or whatever
#define JOIN_(a, b) a##b
#define JOIN2(a, b) JOIN_(a, b)
// eventually may generate this kind of thing based on a tool.
// otherwise, this is a little crazy. these must be undefed at end of
// header
#define SngHTable SNG_HTABLE_NAME
#define SngHTableEntry JOIN2(SNG_HTABLE_NAME, Entry)
#define SngHTableHash JOIN2(SNG_HTABLE_NAME, Hash)
#define SngHTableKey JOIN2(SNG_HTABLE_NAME, Key)
#define SngHTableValue JOIN2(SNG_HTABLE_NAME, Value)
#define sngHTableInit JOIN2(SNG_HTABLE_FUNC_PREFIX, Init)
#define sngHTableClear JOIN2(SNG_HTABLE_FUNC_NAME, Clear)
#define sngHTableGet JOIN2(SNG_HTABLE_FUNC_PREFIX, Get)
#define sngHTablePut JOIN2(SNG_HTABLE_FUNC_PREFIX, Put)
#define sngHTableDelete JOIN2(SNG_HTABLE_FUNC_PREFIX, Delete)

typedef SNG_HTABLE_HASH_TYPE SngHTableHash;
typedef SNG_HTABLE_KEY SngHTableKey;
typedef SNG_HTABLE_VALUE SngHTableValue;

typedef struct SngHTable SngHTable;
typedef struct SngHTableEntry SngHTableEntry;

struct SngHTableEntry {
	SngHTableHash hash;
	SngHTableKey key;
	SngHTableValue value;
	SngHTableEntry *next;
};

struct SngHTable {
	SngHTableEntry *buckets[SNG_HTABLE_BUCKET_COUNT];
};

// sngHTableInit
SNG_HTABLE_API void sngHTableInit(SngHTable *h);

// sngHTableClear
SNG_HTABLE_API void sngHTableClear(SngHTable *h);

// sngHTableGet
SNG_HTABLE_API b32 sngHTableGet(SngHTable *h, SngHTableKey key, SngHTableValue *value);

// sngHTablePut
SNG_HTABLE_API void sngHTablePut(SngHTable *h, SngHTableKey key, SngHTableValue value);

// sngHTableDelete
SNG_HTABLE_API b32 sngHTableDelete(SngHTable *h, SngHTableKey key, SngHTableValue *value);

#endif // SNG_HTABLE_H

#ifdef SNG_HTABLE_IMPLEMENTATION

SNG_HTABLE_API void sngHTableInit(SngHTable *h) {
	memset(h, 0, sizeof(*h));
}

SNG_HTABLE_API void sngHTableClear(SngHTable *h) {
	for (uptr i = 0; i < SNG_HTABLE_BUCKET_COUNT; i++) {
		SngHTableEntry *entry = h->buckets[i];
		while (entry) {
			SngHTableEntry *next = entry->next;
			SNG_HTABLE_FREE(entry);
			entry = next;
		}
	}
	memset(h, 0, sizeof(*h));
}

SNG_HTABLE_API b32 sngHTableGet(SngHTable *h, SngHTableKey key, SngHTableValue *value) {
	SngHTableHash hash = SNG_HTABLE_HASH_FUNC(key);
	SngHTableEntry *entry = h->buckets[hash & SNG_HTABLE_BUCKET_MASK];
	for (; entry; entry = entry->next) {
		if (entry->hash != hash) {
			continue;
		}
		if (entry->key == key) {
			*value = entry->value;
			return 1;
		}
	}
	return 0;
}

SNG_HTABLE_API void sngHTablePut(SngHTable *h, SngHTableKey key, SngHTableValue value) {
	SngHTableHash hash = SNG_HTABLE_HASH_FUNC(key);
	SngHTableEntry **bucket = &h->buckets[hash & SNG_HTABLE_BUCKET_MASK];
	SngHTableEntry *head = *bucket;
	for (SngHTableEntry *entry = head; entry; entry = entry->next) {
		if (entry->hash != hash) {
			continue;
		}
		if (entry->key == key) {
			entry->value = value;
			return;
		}
	}
	SngHTableEntry *entry = (SngHTableEntry *)SNG_HTABLE_MALLOC(sizeof(SngHTableEntry));
	entry->hash = hash;
	entry->key = key;
	entry->value = value;
	entry->next = head;
	*bucket = entry;
}

SNG_HTABLE_API b32 sngHTableDelete(SngHTable *h, SngHTableKey key, SngHTableValue *value) {
	SngHTableHash hash = SNG_HTABLE_HASH_FUNC(key);
	SngHTableEntry **link = &h->buckets[hash & SNG_HTABLE_BUCKET_MASK];
	SngHTableEntry *head = *link;
	for (
		SngHTableEntry *entry = head;
		entry;
		link = &entry->next, entry = entry->next
	) {
		if (entry->hash != hash) {
			continue;
		}
		if (entry->key == key) {
			*link = entry->next;
			if (value) {
				*value = entry->value;
			}
			SNG_HTABLE_FREE(entry);
			return 1;
		}
	}
	return 0;
}

#endif // SNG_HTABLE_IMPLEMENTATION

// TODO(james4k): undef everything, so we can define multiple hash table
// implementaions in one translation unit.
//#undef SNG_HTABLE_IMPLEMENTATION
//
//#undef SngHTable
//#undef SngHTableEntry
//#undef SngHTableHash
//#undef SngHTableKey
//#undef SngHTableValue
//
//#undef sngHTableInit
//#undef sngHTableClear
//#undef sngHTableGet
//#undef sngHTablePut
//#undef sngHTableDelete

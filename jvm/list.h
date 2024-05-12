#ifndef _UTILS_H_
#define _UTILS_H_

#define define_list(L
typedef struct List {
	uint64_t capacity;
	uint64_t size;
	void** data;
} List;
#endif

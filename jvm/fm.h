#ifndef _FM_H_
#define _FM_H_

#include <stdint.h>

typedef struct FileHandle {
	const char* name;
	FILE* handle;
	void (*Write) (struct FileHandle*, const void*, uint64_t);
	void (*Read) (struct FileHandle*, void*, uint64_t);
	void (*Seek) (struct FileHandle*, uint8_t, int64_t);
	uint64_t (*Length) (struct FileHandle*);
	uint64_t (*Pos) (struct FileHandle*);
} FileHandle;

typedef struct FileManager {
	FileHandle* (*Open) (const char*, const char*);
	void (*Close) (FileHandle*);
} FileManager;

FileManager* GetSystemFM();

#endif

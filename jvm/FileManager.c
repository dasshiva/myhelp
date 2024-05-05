#include "log.h"
#include "fm.h"
#include <stddef.h>

static FileManager* SystemFM;
static FileHandle** files;
static uint64_t files_len = 0, used;

void Write(struct FileHandle* self, const void* buf, uint64_t len) {
	if (!buf) 
		fatal("Writing a NULL buffer to a file is not allowed");
	if (len > fwrite(buf, 1, len, self->handle)) 
		fatal("Write to file %s could not be completed", self->name);
}

void Read(struct FileHandle* self, void* buf, uint64_t len) {
	uint8_t* target = buf;
	if (!target)
		fatal("Reading into a NULL buffer from a file is not allowed");
	if (len > fread(buf, 1, len, self->handle))
		fatal("Could not read into buffer successfully");
}

void Seek(struct FileHandle* self, uint8_t from, int64_t offset) {
	fseek(self->handle, from, offset);
}

uint64_t Pos(struct FileHandle* self) {
	return ftell(self->handle);
}

uint64_t Length(struct FileHandle* self) {
	uint64_t pos = self->Pos(self);
	self->Seek(self, SEEK_END, 0);
	uint64_t len = self->Pos(self);
	self->Seek(self, SEEK_SET, pos);
	return len;
}

FileHandle* Open(const char* name, const char* mode) {
	FILE* handle = fopen(name, mode);
	if (!handle)
		return NULL;
	FileHandle* fh = malloc(sizeof(FileHandle));
	fh->handle = handle;
	fh->name = name;
	fh->Write = Write;
	fh->Read = Read;
	fh->Seek = Seek;
	fh->Pos = Pos;
	fh->Length = Length;
	if (files_len == 0) {
		files_len += 10;
		files = calloc(files_len, sizeof(FileHandle*));
	}
	else if (used + 1 == files_len) {
		for (int i = 0; i <= used; i++) {
		       if (files[i] == NULL) {
			       files[i] = fh;
			       return fh;
		       }
		}
		files_len += 10;
		files = realloc(files, files_len);
	}
	files[used++] = fh;
	return fh;
}

void Close(FileHandle* handle) {
	for (int i = 0; i <= used; i++) {
		if (files[i] == handle) {
			files[i] = NULL;
			break;
		}
	}
}

void InitSystemFM() {
	SystemFM = malloc(sizeof(FileManager));
	SystemFM->Open = Open;
	SystemFM->Close = Close;
}

FileManager* GetSystemFM() {
	if (!SystemFM) 
		InitSystemFM();
	return SystemFM;
}


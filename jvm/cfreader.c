#include "cfreader.h"
#include "log.h"

ClassFile* LoadClass(FileHandle* fh) {
	if (!fh) {
		warn("Could not load class");
		return NULL;
	}
	uint64_t len = fh->Length(fh);
	uint8_t* buf = malloc(len);
	fh->Read(fh, buf, len);
	GetSystemFM()->Close(fh);
	ClassFile* cf = malloc(sizeof(ClassFile));
	return cf;
}

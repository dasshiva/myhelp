#include "log.h"
#include "cfreader.h"
#define VERSION "0.0.1"

int main(int argc, const char** argv) {
	if (argc < 2) 
		fatal("Usage: %s [FILENAME]", argv[0]);
	FileManager* fm = GetSystemFM();
	FileHandle* fh = fm->Open(argv[1], "r");
	info("Starting Sunrise VM %s", VERSION);
	info("Loading class = %s", argv[1]);
	ClassFile* class = LoadClass(fh);
	return 0;
}

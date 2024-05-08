#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>
#include <string.h>
#include "log.h"

// Very naive and error-prone Hash Function that works for "most" cases
static uint64_t Hash(const char* s) {
	int len = strlen(s);
	if (len == 1) return s[0];
	else if (!len) {
		fatal("BUG: strlen(s) returned 0");
		return 0;
	}
	else {
		char c = s[0];
		char d = s[len - 1];
		if (c != d) return c * 0xCAFEBABE + d * 0xCAFEDEAD;
		else return c * 0xCAFEBABE + d * 0xCAFEDEAD + s[len/2]  * 0xBEEF;
	}	
}

#endif

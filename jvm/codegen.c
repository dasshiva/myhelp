#include "codegen.h"
#include "log.h"
#ifdef __x86_64__

#include "codegen_x64.h"

uint8_t* codegen(ClassFile* cf, FM* method) {
	info("%lx", MOV_L32_TO_R64(RAX, 900));
	return NULL;
}

#else
#error "We only support x86-64 for now"
#endif

#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "consts.h"

#ifdef __x86_64__ 
#include "x86.inc"
#else 
#error "Only x86 is supported for now"
#endif


#endif

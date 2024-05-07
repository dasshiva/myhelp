#ifndef __CODEGEN_X64_H__
#define __CODEGEN_X64_H__
#define REX_EMPTY (0b01000000)
#define REX_W (1 << 3)
#define REX_R (1 << 2)
#define REX_X (1 << 1)
#define REX_B (1 << 0)

enum Registers8 {
	AL = 0,
	CL = 1,
	DL = 2,
	BL = 3,
	AH = 4,
	CH = 5,
	DH = 6,
	BH = 7,
// When any REX prefix is used use these instead of [A|B|C|D]H
 	SPL = 4,
	BPL = 5,
	SIL = 6,
	DIL = 7,
// The below can only be accessed with the REX.R field
	R8L = 0,
	R9L = 1,
	R10L = 2,
	R11L = 3,
	R12L = 4,
	R13L = 5,
	R14L = 6,
	R15L = 7
};

enum Registers16 {
	AX = 0,
	CX = 1,
	DX = 2,
	BX = 3,
	SP = 4,
	BP = 5,
	SI = 6,
	DI = 7,
// The below can only be accessed with the REX.R field
	R8W = 0,
	R9W = 1,
	R10W = 2,
	R11W = 3,
	R12W = 4,
	R13W = 5,
	R14W = 6,
	R15W = 7
};

enum Registers32 {
	EAX = 0,
	ECX = 1,
	EDX = 2,
	EBX = 3,
	ESP = 4,
	EBP = 5,
	ESI = 6,
	EDI = 7,
// The below can only be accessed with the REX.R field
	R8D = 0,
	R9D = 1,
	R10D = 2,
	R11D = 3,
	R12D = 4,
	R13D = 5,
	R14D = 6,
	R15D = 7
};

enum Registers64 {
	RAX = 0,
	RCX = 1,
	RDX = 2,
	RBX = 3,
	RSP = 4,
	RBP = 5,
	RSI = 6,
	RDI = 7,
// The below can only be accessed with the REX.R field
	R8 = 0,
	R9 = 1,
	R10 = 2,
	R11 = 3,
	R12 = 4,
	R13 = 5,
	R14 = 6,
	R15 = 7
};

#include <stdint.h>
#endif

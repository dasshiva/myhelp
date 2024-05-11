#include <stdint.h>
#define INSTRUCTION(ins) void arch_##ins(uint8_t** buffer, uint32_t* index, uint32_t* ins_size) 

INSTRUCTION(nop) {
	write_u8(0x90);
}

INSTRUCTION(return) {
	write_u8(0xC3);
}

#include "interp.h"
#include "log.h"

int Run(ClassFile*cf, FM* method) {
	auto code = method->attributes->code;
	uint32_t pc = 0;
	while (pc < code.ins_length) {
		uint8_t opcode = code.ins[pc];
		pc++;
	}
	return 0;
}

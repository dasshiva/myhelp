#include "interp.h"
#include "consts.h"
#include "log.h"

enum Tag {
	INT8,
	INT16,
	INT32,
	INT64,
	FLOAT32,
	FLOAT64,
	REF
};

#define is_int(x, t) if ((!method->checked) && (x.tag != t)) { warn("Method failed type verification"); return 0; }
struct value {
	uint8_t tag;
	union {
		int8_t int8;
		int16_t int16;
		int32_t int32;
		int64_t int64;
		float float32;
		double float64;
		void* ref;
	};	
};

struct Frame {
	struct value* locals;
	struct value* stack;
	uint16_t top;
};

int Run(ClassFile* cf, FM* method) {
	struct Attribute a = method->attributes->FindAttribute(method->attributes, method->attribute_count, "Code");
	Code code = a.code;
	struct Frame* frame = malloc(sizeof(struct Frame));
	frame->locals = malloc(sizeof(struct value) * code.max_locals);
	frame->stack = malloc(sizeof(struct value) * code.max_stack);
	frame->top = -1;
	uint32_t pc = 0;
	while (pc < code.ins_length) {
		uint8_t opc = code.ins[pc];
		switch (opc) {
			case opcode(nop): break;
			case opcode(return): return 1;
			case opcode(bipush): {
				frame->stack[++frame->top].int32 = code.ins[++pc]; 
				frame->stack[frame->top].tag = INT32;
				break;
			}
			case opcode(pop): frame->top -= 1; break;
			case opcode(pop2): frame->top -= 2; break;
			case opcode(istore_0):
			case opcode(istore_1):
			case opcode(istore_2):
			case opcode(istore_3):
			{
				struct value val = frame->stack[frame->top--];
				frame->locals[opc - opcode(istore_0)].tag = INT32;
				frame->locals[opc - opcode(istore_0)].int32 = val.int32;
				break;
			}
			default: {
				if (!method->checked && (opc > opcode(MAX))) {
					warn("Illegal opcode = %d", opc);
					return 1;
				}	
				fatal("Unknown opcode = %d", opc); 
			}
		}
		pc++;
	}
	return 1;
}

#include "interp.h"
#include "log.h"

enum Tag { INT8, INT16, INT32, INT64, FLOAT32, FLOAT64, REF };

#define is_int(x, t)                                                           \
  if (x.tag != t) {                                    \
    warn("Method failed type verification");                                   \
    return 0;                                                                  \
  }
struct value {
  uint8_t tag;
  union {
    int8_t int8;
    int16_t int16;
    int32_t int32;
    int64_t int64;
    float float32;
    double float64;
    void *ref;
  };
};

struct Frame {
  struct value *locals;
  struct value *stack;
  int32_t top;
};

#define write_u8(ins)                                                          \
  {                                                                            \
    if (*index + 1 == *ins_size) {                                               \
      *ins_size += 20;                                                          \
      *buffer = realloc(*buffer, *ins_size);                                      \
    }                                                                          \
    *buffer[*index++] = ins;                                                   \
  }

#define write_u16(ins)                                                         \
  {                                                                            \
    write_u8(ins & 0xFF);                                                      \
    write_u8(ins >> 8);                                                        \
  }

#define write_u32(ins)                                                         \
{  \                                                                            \
    write_u8(ins & 0xFF);                                                      \
    write_u8((ins >> 8) & 0xFF);                                               \
    write_u8((ins >> 16) & 0xFF);                                              \
    write_u8(ins >> 24);                                                       \
}

#include "codegen.h"
#include <sys/mman.h>
#define gen(opcode) arch_##opcode(&buffer, &index, &ins_size)
#define verify(cond, s) if (cond) { warn(s); munmap(buffer, ins_size); return 0;}
#define OVERFLOW_MSG "Method failed type verification : Stack overflow"
#define UNDERFLOW_MSG "Method failed type verification : Stack underflow"
#define INV_LTABLE "Method failed type verification : Invalid access to local variable"

// Run will first validate the method to be run if it is not already validated,
// while JIT compiling it at the same time. If the method is already validated,
// it directly runs the compiled method
int Run(ClassFile *cf, FM *method) {
  struct Attribute a = method->attributes->FindAttribute(method->attributes, method->attribute_count, "Code");
  auto code = a.code;
  struct Frame *frame = malloc(sizeof(struct Frame));
  frame->locals = malloc(sizeof(struct value) * code.max_locals);
  frame->stack = malloc(sizeof(struct value) * code.max_stack);
  frame->top = -1;
  uint32_t pc = 0;
  if (!method->checked) {
    uint32_t ins_size = 4096, index = 0;
    uint8_t *buffer = mmap(NULL, ins_size, PROT_READ | PROT_WRITE | PROT_EXEC, 0x20 | MAP_PRIVATE, 0, 0);
    if (buffer == MAP_FAILED) {
	    perror("mmap");
	    fatal("Could not allocate executable buffer for native code");
    }
    method->code = buffer;
    while (pc < code.ins_length) {
      uint8_t opc = code.ins[pc];
      switch (opc) {
      case opcode(nop):
	gen(nop);
        break;

      case opcode(return):
	gen(return);
	break;

      case opcode(bipush): {
	verify(frame->top + 1 == code.max_stack, OVERFLOW_MSG); 
        frame->stack[++frame->top].int32 = code.ins[++pc];
        frame->stack[frame->top].tag = INT32;
        break;
      }

      case opcode(pop): 
	verify(frame->top == -1, UNDERFLOW_MSG);
        frame->top -= 1;
        break;

      case opcode(pop2):
        verify(frame->top < 1, UNDERFLOW_MSG);
	frame->top -= 2;
        break;

      case opcode(istore_0):
      case opcode(istore_1):
      case opcode(istore_2):
      case opcode(istore_3): {
	verify((frame->top == -1), UNDERFLOW_MSG);
        struct value val = frame->stack[frame->top--];
        is_int(val, INT32);
	verify((opc - opcode(istore_0)) >= code.max_locals, INV_LTABLE);
        frame->locals[opc - opcode(istore_0)].tag = INT32;
        frame->locals[opc - opcode(istore_0)].int32 = val.int32;
        break;
      }

      default: {
          if (opc > opcode(MAX)) {
             warn("Illegal opcode = %d", opc);
             return 1;
          }
          fatal("Unknown opcode = %d", opc);
      	}
      }

      pc++;
    }
    void (*do_something) () = method->code;
    do_something();
  }
  else {
	  void (*do_that) () = method->code;
	  do_that();
  }
  return 1;
}

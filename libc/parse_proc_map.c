#include <asm-generic/fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>

#ifndef __clang__
#error "DO NOT USE ANYTHING OTHER THAN CLANG FOR COMPILING THIS"
#endif 

#define PF_R 1
#define PF_W 2
#define PF_X 4
#define PF_P 8
#define PF_HEAP 16
#define PF_STACK 32

struct map {
	uint64_t low;
	uint64_t high;
	uint8_t perms;
};

// gcc ends up using rbp on x86 when we use var args for syscall _without_ setting up the frame pointer
// I can't say they are wrong because naked functions cannot have a prologue. I had two options either to separate the assembly or use clang
// Since I didn't want to separate the assembly from the source , we always use clang because clang in naked functions
// does not generate any code other than what is inside the asm block.
__attribute__((naked))
long syscall(long syscall, ...) {
#ifdef __x86_64__
	asm volatile(
		"movq %%rdi, %%rax\n"
		"movq %%rsi, %%rdi\n"
		"movq %%rdx, %%rsi\n"
		"movq %%rcx, %%rdx\n"
		"movq %%r8, %%r10\n"
		"movq %%r9, %%r8\n"
		"syscall\n"
		"ret"
		: 
		:
		: 

	);
#elif __aarch64__
	asm volatile(
		"uxtw x8, w0\n"
		"mov x0, x1\n"
		"mov x1, x2\n"
		"mov x2, x3\n"
		"mov x3, x4\n"
		"mov x4, x5\n"
		"mov x5, x6\n"
		"mov x6, x7\n"
		"svc #0\n"
		"b lr"
		: 
		: 
		:
	);
#endif
}

#include <syscall.h>
void* sbrk(long increment) {
	uint64_t current = (uint64_t) syscall(SYS_brk, 0);
	if (!increment)
		return (void*)current;
	uint64_t new = syscall(SYS_brk, current + increment);
	if (new == current)
		return NULL;
	return (void*)new;
}

long open(const char* name, uint32_t mode) {
	return syscall(SYS_open, name, mode);
}

long read(int fd, void* buf, uint64_t len)  {
	return syscall(SYS_read, fd, buf, len);
}

long close(int fd) {
	return syscall(SYS_close, fd);
}

long write(int fd, void* buf, uint64_t len) {
	return syscall(SYS_write, fd, buf, len);
}

void* memcpy(void* to, const void* src, uint64_t len) {
	uint8_t* dest = to;
	const uint8_t* from = src;
	for (uint64_t i = 0; i < len; i++) {
		dest[i] = from[i];
	}
	return to;
}

void* memset(void* ptr, int val, uint64_t n) {
	uint8_t* dest = ptr;
	while (n--) {
		dest[n] = val;
	}
	return ptr;
}

uint64_t parse_number(int fd) {
	int buf[16] = {0};
	int i = 0;
	uint64_t num = 0;
	for (; i < 16; i++) {
		char c = 0;
		if (!read(fd, &c, 1)) 
			return ULONG_MAX;
		if (c >= 48 && c <= 57) {
			buf[i] = c - 48;
		}
		else if (c >= 97 && c <= 122) {
			buf[i] = (c - 97) + 10;
		}
		else 
			break;
	}
	i--;
	for (int j = 0; i >= 0; i--, j++) {
		num += buf[i] * (1ul << (4 * j));
	}
	return num;
}

uint8_t parse_perms(int fd) {
	char p1, p2, p3, p4;
	read(fd, &p1, 1);
	read(fd, &p2, 1);
	read(fd, &p3, 1);
	read(fd, &p4, 1);
	uint8_t perms = 0;
	perms |= (p1 == 'r') ? PF_R : 0;
	perms |= (p2 == 'w') ? PF_W : 0;
	perms |= (p3 == 'x') ? PF_X : 0;
	perms |= (p4 == 'p') ? PF_P : 0;
	return perms;
}

void skip_to_newline(int fd) {
	char c = 0;
	while (read(fd, &c, 1)) {
		if (c == '\n') break;
	}
}

struct map* parse_proc_map() {
	int fd = open("/proc/self/maps", O_RDONLY);
	uint64_t begin = (uint64_t) sbrk(0);
	uint64_t rsp = 0;
#ifdef __x86_64__
	asm volatile ("movq %%rsp, %0" : "=r"(rsp) ::);
#elif __aarch64__
	asm volatile ("mov sp, %0" : "=r" (rsp) ::);
#endif
	uint64_t* entries = sbrk(8);
	uint64_t* heap_ptr = sbrk(sizeof(struct map*));
	uint64_t* stack_ptr = sbrk(sizeof(struct map*));
	*entries = 0;
	while (1) {
		struct map m = {0};
		m.low = parse_number(fd);
		if (m.low == ULONG_MAX) 
			break;
		m.high = parse_number(fd);
		m.perms = parse_perms(fd);
		struct map* store = sbrk(sizeof(struct map));
		if (rsp >= m.low && rsp <= m.high) {
			*stack_ptr = (uint64_t) store;
			m.perms |= PF_STACK;
		}
		else if (begin >= m.low && begin <= m.high) {
			m.perms |= PF_HEAP;
			*heap_ptr = (uint64_t) store;
		}
		skip_to_newline(fd);
		memcpy(store, &m, sizeof(struct map));
		*entries += 1;
	}
	struct map* heap = (struct map*)*heap_ptr;
	heap->high = (uint64_t) sbrk(0);
	close(fd);
	return (struct map*)begin;
}

void _start(int argc, const char** argv, const char** env) {
	uint8_t* maps = parse_proc_map();
	uint64_t entries = *((uint64_t*)maps);
	struct map* begin = maps + 24;
	write(1, "Hello World\n", 13);
	for (int i= 0; i < entries; i++) {
		//printf("0x%lx 0x%lx 0x%x\n", begin->low, begin->high, begin->perms);
		begin++;
	}
	syscall(SYS_exit, 0);
}

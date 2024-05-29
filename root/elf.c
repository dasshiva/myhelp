#define _GNU_SOURCE
#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#ifdef __x86_64__
#include "zydis.h"
#include "zydis.c"
#endif

extern int errno;
#undef assert
#define assert(cond, msg) if (!(cond)) { fprintf(stderr, msg "\n" ); if (!errno) printf("%d\n", errno); perror("Error"); exit(-1); }

void sig_hand(int sig, siginfo_t* info, void* ctx) {
	ucontext_t* context = ctx;
	//fprintf(stdout, "%llx", ctx->uc_mcontext.gregs[REG_RIP]);
	exit(1);
}

int main(int argc, const char** argv) {
	if (argc < 2)
		return -1;
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		return -2;
	struct stat st;
	stat(argv[1], &st);
	void* file = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED) 
		return 1;
	Elf64_Ehdr* ehdr = file;
	assert(ehdr->e_ident[0] == 0x7F, "Invalid elf magic");
	assert(ehdr->e_ident[1] == 'E',  "Invalid elf magic");
	assert(ehdr->e_ident[2] == 'L',  "Invalid elf magic");
	assert(ehdr->e_ident[3] == 'F',  "Invalid elf magic");
	assert(ehdr->e_ident[4] == ELFCLASS64, "Elf file must be 64 bits");
	assert(ehdr->e_ident[5] == ELFDATA2LSB, "Elf file must be little endian");
	assert(ehdr->e_ident[6] == EV_CURRENT, "Invalid elf file version");
	assert(ehdr->e_ident[7] == ELFOSABI_LINUX || ehdr->e_ident[7] == ELFOSABI_SYSV, "Invalid OS ABI");

	assert(ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN, "Elf file has to be executable");
#ifdef __x86_64__
	ZydisDecoder decoder;
	ZydisDecoderContext context;
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
	decoder.decoder_mode = ZYDIS_DECODER_MODE_MINIMAL;
	assert(ehdr->e_machine == EM_X86_64, "Unknown machine type");
#elif __aarch64__
	assert(ehdr->e_machine == EM_AARCH64, "Unknown machine type");
#endif
	// Don't check e_version again it was checked in e_ident already
	
	assert(ehdr->e_ehsize == 0x40, "Invalid size of elf header");
	assert(ehdr->e_phentsize == 0x38, "Invalid size of program header table entry");
	assert(ehdr->e_phnum != 0, "Elf file must have program headers");
	// We don't care about section headers (for now)
	
	Elf64_Phdr* phdr = file + ehdr->e_phoff;
	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr->p_type == PT_LOAD) {
			if ((phdr->p_flags & 1) != 0) {
#ifdef __x86_64__
				ZyanUSize offset = 0;
				ZydisDecodedInstruction ins;
				ZyanU8* data = file + phdr->p_offset;
				while (ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, &context, data + offset, phdr->p_filesz - offset, &ins))) {
					if (ins.mnemonic == ZYDIS_MNEMONIC_SYSCALL) {
						uint8_t* ins = data + offset;
						ins[0] = 0x90; // NOP
						ins[1] = 0xCC; // INT3
					}
					offset += ins.length;
				}
#elif __aarch64__
				for (uint64_t offset = 0; offset < phdr->p_filesz; offset += 4) {
					uint32_t opcode = ((uint32_t*)(file + phdr->p_offset))[offset];
					if (((opcode & 0b11111) == 1) && (((opcode >> 21) & 0b111) == 0) && ((opcode >> 24) == 0b11010100)) `{ // SVC
						((uint32_t*)(file + phdr->p_offset))[offset] = 0b11010100001000000000000000000000; // BRK #0
					}
				}

#endif	
			}
			int prot = 0;
			if ((phdr->p_flags & PF_X) != 0)
				prot |= PROT_EXEC;
			if ((phdr->p_flags & PF_W) != 0)
				prot |= PROT_WRITE;
			if ((phdr->p_flags & PF_R) != 0)
				prot |= PROT_READ;
		        void* dest = mmap((void*)phdr->p_vaddr, phdr->p_memsz, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			assert(dest != MAP_FAILED, "could not allocate memory for loading executable");
			memcpy(dest, file + phdr->p_offset, phdr->p_filesz);
			mprotect(dest, phdr->p_memsz, prot);
		}
		phdr++;
	}
	sigset_t set;
	sigemptyset(&set);
	struct sigaction act;
	act.sa_sigaction = sig_hand;
	act.sa_mask = set;
	act.sa_flags = SA_NODEFER | SA_SIGINFO;
	sigaction(SIGTRAP, &act, NULL);
	void (*start)() = (void*)ehdr->e_entry;
	start();
	return 0;
}

#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __x86_64__
#include "zydis.h"
#include "zydis.c"
#endif

#define assert(cond, msg) if (!(cond)) { fprintf(stderr, msg); exit(-1); }

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
	assert(ehdr->e_ident[8] == 0, "EI_ABIVERSION should be zero");

	assert(ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN, "Elf file has to be executable");
#ifdef __x86_64__
	ZydisDecoder decoder;
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
	assert(ehdr->e_machine == EM_X86_64, "Unknown machine type");
#elif __aarch64__
	assert(ehdr->e_machine == EM_AARCH64, "Unknown machine type");
#endif
	// Don't check e_version again it was checked in e_ident already
	
	assert(ehdr->e_ehsize == 0x40, "Invalid size of elf header");
	assert(ehdr->e_phentsize == 0x38, "Invalid size of program header table entry");
	assert(ehdr->e_phnum != 0, "Elf file must have program headers");
	// We don't care about section headers
	
	Elf64_Phdr* phdr = file + ehdr->e_phoff;
	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr->p_type == PT_LOAD && ((phdr->p_flags & 1) != 0)) {
#ifdef __x86_64__
			ZyanUSize offset = 0;
			const ZyanUSize length = phdr->p_filesz;
			ZydisDecodedInstruction ins;
			ZyanU8* data = file + phdr->p_offset;
			ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
			while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, data + offset, length - offset, &ins, ops))) {
				if (ins.opcode == ZYDIS_MNEMONIC_SYSCALL) {
					uint8_t* ins = data + offset;
					ins[1] = 0xB;
				}
				offset += ins.length;
			}
#elif __aarch64__

#endif
		}
		phdr++;
	}		
	return 0;
}

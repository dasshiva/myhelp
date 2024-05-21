BITS 64

section .elf_header
; ELF MAGIC 0x7F ELF
db 0x7F 
db "ELF"
; ELFCLASS64
db 2
; ELFDATA2LSB
db 1
; EV_CURRENT
db 1
; EI_OSABI - LINUX
db 3
; EI_PAD
dq 0

; ET_EXEC
dw 2
; E_MACHINE - x64
dw 0x3E
; E_VERSION
dd 1
; E_ENTRY
dq _start
; E_PHOFF
dq 0x40
; E_SHOFF
dq 0
; E_FLAGS
dd 0
; E_EHSIZE
dw 0x40
; E_PHENTSIZE
dw 0x38
; E_PHNUM
dw 1
; E_SHENTSIZE
dw 0x40
; E_SHNUM
dw 0
; E_SHSTRNDX
dw 0

section .program_header
; P_TYPE
dd 1
; P_FLAGS
dd 0b111
; P_OFFSET
dq 120
; P_VADDR
dq _start
; P_PADDR
dq _start
; P_FILESZ
dq _end - _start
; P_MEMSZ
dq _end - _start
; P_ALIGN
dq 0

section .program
org 1 << 20

_start:
	jmp _start
_end:

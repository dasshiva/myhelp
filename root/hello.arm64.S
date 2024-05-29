.global _start
_start:
	eor x0, x0, x0
	mov x8, 93
	brk #0
	svc #0


; SPDX-License-Identifier: GPL-3.0-only
; Copyright (c) 2025 Marc Gonzalez

SECTION .note.GNU-stack noalloc noexec nowrite progbits
SECTION .text
GLOBAL init
GLOBAL spin

init:
	ret

	times ((init-$ + spin-loop) % 16) int3 ; align loop at 16
spin:
	xor eax, eax
	mov ecx, 1000
loop:
	times 60 inc eax ; 120 bytes
	dec ecx
	jnz loop ; 8-bit offset
	ret

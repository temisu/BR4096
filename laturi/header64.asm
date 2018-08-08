; Copyright (C) Teemu Suutari

; x86-64
	bits 64
section .text

%include "symbols.asm"

; MACH header
MACHHeader:
	dd 0xfeedfacf				; magic
	dd SOLVE(CPU_TYPE_X86_64)		; cpu type
	dd SOLVE(CPU_SUBTYPE_X86_64_ALL)	; cpu subtype
	dd SOLVE(MH_EXECUTE)			; filetype
	dd 3					; number of commands
	dd MACHCommandsEnd-MACHCommandsStart	; size of commands
;	dd SOLVE(MH_PREBOUND)|SOLVE(MH_TWOLEVEL)|SOLVE(MH_NOFIXPREBINDING)
	dd SOLVE(MH_PREBOUND)|SOLVE(MH_TWOLEVEL)
						; flags
	dd 0					; reserved
MACHHeaderEnd:
; by definition commands follow right after the header
MACHCommandsStart:
Command3Start:
	dd SOLVE(LC_SEGMENT_64)			; this is segment
	dd Command3End-Command3Start		; size
SegNameStart:

	times 16-$+SegNameStart db 0		; section name

	dq 0					; vmaddr
global HeapSize
HeapSize:
	dq 0					; vmsize
	dq 0					; fileoff
global CodeSize
CodeSize:
	dq 0					; filesize, will be written
						; by the linker
; gdb hates us if we set this to zero, but it compresses better
global GDBCompat
GDBCompat:
	dd SOLVE(VM_PROT_READ)|SOLVE(VM_PROT_WRITE)|SOLVE(VM_PROT_EXECUTE)
						; maxprot
	dd SOLVE(VM_PROT_READ)|SOLVE(VM_PROT_WRITE)|SOLVE(VM_PROT_EXECUTE)
;						; initprot
	dd 0					; nsects
;	dd SOLVE(SG_NORELOC)			; flags
	dd 0
Command3End:
Command2Start:
	dd SOLVE(LC_UNIXTHREAD)			; this is thread
	dd Command2End-Command2Start		; size (must be exact)
	dd SOLVE(x86_THREAD_STATE64)		; flavor
	dd SOLVE(x86_THREAD_STATE64_COUNT)	; count
RegSpace:
;	dq 0					; rax
;	dq 0					; rbx
;	dq 0					; rcx
;	dq 0					; rdx
;	dq 0					; rdi
;	dq 0					; rsi
;	dq 0					; rbp
	times 56-$+RegSpace db 0
Global ESPRegister
ESPRegister:
	dq 0					; rsp
RegSpace2:
;	dq 0					; r8
;	dq 0					; r9
;	dq 0					; r10
;	dq 0					; r11
;	dq 0					; r12
;	dq 0					; r13
;	dq 0					; r14
;	dq 0					; r15
	times 64-$+RegSpace2 db 0
Global __LATURI__CodeStart
__LATURI__CodeStart:
	dq 0					; rip
	dq 0					; rflags
	dq 0					; cs
	dq 0					; fs
	dq 0					; gs
Command2End:
Command4Start:
	dd SOLVE(LC_LOAD_DYLINKER)		; dyld
	dd Command4End-Command4Start		; size
	dd DyldName-Command4Start		; offset
	db 0,0 ; good for compression
DyldName:
	db '/usr/lib/dyld',0
Command4End:
MACHCommandsEnd:

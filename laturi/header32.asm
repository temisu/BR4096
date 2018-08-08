; Copyright (C) Teemu Suutari

; x86
	bits 32
section .text

%include "symbols.asm"

; MACH header
MACHHeader:
	dd 0xfeedface				; magic
	dd SOLVE(CPU_TYPE_X86)			; cpu type
	dd 0					; cpu subtype (wrong, but works)
	dd SOLVE(MH_EXECUTE)			; filetype
	dd 6					; number of commands
	dd MACHCommandsEnd-MACHCommandsStart	; size of commands
;	dd SOLVE(MH_PREBOUND)|SOLVE(MH_TWOLEVEL)|SOLVE(MH_NOFIXPREBINDING)
;	dd SOLVE(MH_PREBOUND)|SOLVE(MH_TWOLEVEL)
;	dd SOLVE(MH_PREBOUND)|SOLVE(MH_TWOLEVEL)|SOLVE(MH_NOFIXPREBINDING)|SOLVE(MH_NOUNDEFS)
	dd 0					; flags
MACHHeaderEnd:
; by definition commands follow right after the header
MACHCommandsStart:

Command2Start:
	dd SOLVE(LC_SEGMENT)			; this is segment
	dd Command2End-Command2Start		; size
Seg2NameStart:
	times 16-$+Seg2NameStart db 0		; section name

	dd 0					; vmaddr
global HeapSize
HeapSize:
	dd 0					; vmsize
	dd 0					; fileoff
global CodeSize
CodeSize:
	dd 0					; filesize, will be written
						; by the linker
	dd 0					; maxprot
	dd SOLVE(VM_PROT_READ)|SOLVE(VM_PROT_WRITE)|SOLVE(VM_PROT_EXECUTE)
	dd 0					; nsects
;	dd SOLVE(SG_NORELOC)			; flags
	dd 0
Command2End:

;%ifdef OSX_12 OSX_13

Command4Start:
	dd SOLVE(LC_SEGMENT)			; this is segment
	dd Command4End-Command4Start		; size
Seg4NameStart:
	db '__LINKEDIT'
	times 16-$+Seg4NameStart db 0		; section name
	dd 0					; vmaddr
	dd 0					; vmsize
	dd 0x1000				; fileoff (insane)
	dd 0					; filesize

	dd 0					; maxprot
	dd 0					; initport
	dd 0					; nsects
	dd 0					; flags
Command4End:

Command5Start:
	dd SOLVE(LC_DYLD_INFO_ONLY)
	dd Command5End-Command5Start 		; size
	dd 0,0,0,0,0
	dd 0,0,0,0,0
Command5End:

Command6Start:
	dd SOLVE(LC_DYSYMTAB)
	dd Command6End-Command6Start 		; size
	dd 0,0,0,0,0,0,0,0,0
	dd 0,0,0,0,0,0,0,0,0
Command6End:

;%endif

Command1Start:
 	dd SOLVE(LC_MAIN)			; this is main
	dd Command1End-Command1Start		; size
Global __LATURI__CodeStart
__LATURI__CodeStart:
	dd 0					; eip
	dd 0
	dd 0					; stack
	dd 0
Command1End:

%if 0
; default VSYNC
Command7Start:
	dd SOLVE(LC_VERSION_MIN_MACOSX)		; min version
	dd Command7End-Command7Start 		; size
	dd 0					; min version
	dd 0x0a0a00				; sdk
Command7End:
%endif

Command3Start:
	dd SOLVE(LC_LOAD_DYLINKER)		; dyld
	dd Command3End-Command3Start		; size
	dd DyldName-Command3Start		; offset
DyldName:
	db '/usr/lib/dyld',0			; 14 bytes
Command3End:

MACHCommandsEnd:

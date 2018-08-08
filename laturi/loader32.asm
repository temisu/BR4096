; Copyright (C) Teemu Suutari

; x86
	bits 32
section .text

struc nlist
.stri		resd 1
.type		resb 1
.sect		resb 1
.desc		resw 1
.addr		resd 1
endstruc

struc m_header
.magic		resd 1
.cputype	resd 1
.cpusubtype 	resd 1
.filetype	resd 1
.ncmds		resd 1
.sizeofcmds	resd 1
.flags		resd 1
endstruc

struc m_cmd
.cmd		resd 1
.len		resd 1
endstruc

struc seg_cmd
.cmd		resd 1
.len		resd 1
.segname	resb 16
.vmaddr		resd 1
.vmsize		resd 1
.fileoff	resd 1
.filesize	resd 1
.maxprot	resd 1
.initprot	resd 1
.nsects		resd 1
.flags		resd 1
endstruc

struc symtab_cmd
.cmd		resd 1
.len		resd 1
.symoff		resd 1
.nsyms		resd 1
.stroff		resd 1
.strsize	resd 1
endstruc

struc dysymtab_cmd
.cmd		resd 1
.len		resd 1
.ilocalsym	resd 1
.nlocalsym	resd 1
.iextdefsym	resd 1
.nextdefsym	resd 1
.iundefsym	resd 1
.nundefsym	resd 1
.tocoff		resd 1
.ntoc		resd 1
.modtaboff	resd 1
.nmodtab	resd 1
.extrefsymoff	resd 1
.nextrefsyms	resd 1
.indirectsymoff	resd 1
.nindirectsyms	resd 1
.extreloff	resd 1
.nextrel	resd 1
.locreloff	resd 1
.nlocrel	resd 1
endstruc

; stack layout after pushad
struc ad
.edi		resd 1
.esi		resd 1
.ebp		resd 1
.esp		resd 1
.ebx		resd 1
.edx		resd 1
.ecx		resd 1
.eax		resd 1
endstruc

;%define DYLD_ADDRESS (0x8fe00000)

%ifndef LATURI_ASM_ONLY
%define FUNC_OFFSET 1
%define FUNC_SIZE 6
%else
%define FUNC_OFFSET 0
%define FUNC_SIZE 4
%endif

%include "symbols.asm"

%xdefine HashValueCount 0


global __LATURI__CodeStart
__LATURI__CodeStart:
	popad
	pop ebp
	push ebp
%ifdef HASH_LEN_4 HASH_LEN_3
	push byte 1
	pop eax
%else
	; we popped number of arguments (1) into ebx, lets use it
	xchg eax,ebx
%endif
	mov ebx,__LATURI__JumpTable;-FUNC_SIZE
	mov edi,__LATURI__HashStart
	mov [ebx],esi

.lib_hash_done:

;	eax symcount
;	ebx dest
;	ecx 0
;	edx
;	esi
;	edi hashmap
;	ebp lib

.lib_found:

;	parse lib

;	eax symcount
;	ebx dest
;	ecx
;	edx
;	esi
;	edi hashmap
;	ebp lib header
; we need:
; vmslide
; seg.vmaddr (from __LINKEDIT)
; seg.fileoff (from __LINKEDIT)
; symtab.symoff
; symtab.stroff
; dysymtab.iextdefsym
;
; we will output:
; nlist		slide+seg.vmaddr-seg.fileoff+symtab.symoff+dysymtab.iextdefsym*nlist_size	edx
;		or
;		slide+seg.vmaddr-seg.fileoff+symtab.symoff
; strings	slide+seg.vmaddr-seg.fileoff+symtab.stroff					esi

.browse_mach_loop:
	add ebp,[byte ebp+m_header_size+m_cmd.len] ; first load command is text
	mov ecx,[byte ebp+m_header_size+m_cmd.cmd]
;	cmp cl,byte SOLVE(LC_SEGMENT)
	loop .browse_mach_no_seg
%if 0
	cmp [byte ebp+m_header_size+seg_cmd.segname+2],byte 0x4c ; 'L'
	jnz short .browse_mach_loop
%endif
	; last load command is always linkedit?
	mov esi,[byte ebp+m_header_size+seg_cmd.vmaddr]
	sub esi,[byte ebp+m_header_size+seg_cmd.fileoff]
	add esi,[ebx]
.browse_mach_no_seg:
;	cmp cl,byte SOLVE(LC_SYMTAB)
	loop .browse_mach_loop
	lea edx,[byte esi-4]
	add esi,[byte ebp+m_header_size+symtab_cmd.stroff]
	add edx,[byte ebp+m_header_size+symtab_cmd.symoff]
	xchg eax,ecx

;	eax 0
;	ebx dest
;	ecx symcount
;	edx nlist
;	esi strings
;	edi hashmap
;	ebp
.load_funcs_loop:
	push edx
.browse_lib:
	mov ebp,[edx]
	pushad

;	add esi,[byte edx+nlist.stri+4] ; nlist.stri==0
	add esi,[byte edx+4]
; eax < 256
	cdq
.hashloop:
global HashValue0
HashValue0 equ $+2
	imul edx,edx,strict byte 0
	lodsb
	add edx,eax
	daa
	jnz short .hashloop
%ifdef HASH_LEN_4
	sub edx,[edi]
;	nothing else, full 4 bytes matter.
%elifdef HASH_LEN_3
	xor edx,[edi]
;	shl eax,12
	and edx,0x7f7f7f
%else
	sub dx,[edi]
%endif
	popad

	; all image symbols lib item is the same size as nlist == 12
	; yay!!!
.do_hash_trampoline:
	lea edx,[byte edx+nlist_size]
	jnz short .browse_lib

%ifdef HASH_LEN_4
	scasd
%elifdef HASH_LEN_3
	scasd
	dec edi
%elifdef HASH_LEN_2
	inc edi
	inc edi
%else
%error Wrong hash length
%endif

	jecxz .lib_hash_done
	; store the address in jump map
;	mov ebp,[byte edx+nlist.addr-nlist_size+4]
	mov ebp,[edx]
	add [ebx],ebp
	pop edx
%ifndef LATURI_ASM_ONLY
	mov [byte ebx+4],word 0x68c3	; opcode for 'ret' followed by 'push dword'
%endif
	lea ebx,[byte ebx+FUNC_SIZE]

	loop .load_funcs_loop

;	parse lib done


;	eax 0
;	ebx dest
;	ecx 0
;	edx
;	esi
;	edi hashmap
;	ebp

	mov edx,[_dyld_all_image_infos+FUNC_OFFSET]
	mov edx,[byte edx+8]
	sub esi,esi
	xchg al,[edi]
	scasb
	jnz short .do_hash_trampoline

.main_loop_done:

;_trampoline

%ifdef TRAN_1
global CallTranslateCount
CallTranslateCount equ $+1
	mov ecx,0
	mov edi,CodeEnd
	mov al,0xe8
.call_translate_loop:
	repnz scasb
	sub [edi],edi
	scasd
	loop .call_translate_loop
%endif

%ifndef LATURI_ASM_ONLY
	call _main
%endif

CodeEnd:

; loader
extern __LATURI__HashStart
extern __LATURI__JumpTable
; here will be the beef
extern _main
; dyld
extern _dyld_all_image_infos

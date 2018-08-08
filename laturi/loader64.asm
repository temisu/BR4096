; Copyright (C) Teemu Suutari

; x86-64
	bits 64
section .text

struc nlist
.stri		resd 1
.type		resb 1
.sect		resb 1
.desc		resw 1
.addr		resq 1
endstruc

struc m_header
.magic		resd 1
.cputype	resd 1
.cpusubtype 	resd 1
.filetype	resd 1
.ncmds		resd 1
.sizeofcmds	resd 1
.flags		resd 1
.reserved	resd 1
endstruc

struc m_cmd
.cmd		resd 1
.len		resd 1
endstruc

struc seg_cmd
.cmd		resd 1
.len		resd 1
.segname	resb 16
.vmaddr		resq 1
.vmsize		resq 1
.fileoff	resq 1
.filesize	resq 1
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
.tocoff		resq 1
.ntoc		resd 1
.modtaboff	resq 1
.nmodtab	resd 1
.extrefsymoff	resq 1
.nextrefsyms	resd 1
.indirectsymoff	resq 1
.nindirectsyms	resd 1
.extreloff	resq 1
.nextrel	resd 1
.locreloff	resq 1
.nlocrel	resd 1
endstruc

%define DYLD_ADDRESS (0x7fff5fc00000)
%define DYLD_FUNC_COUNT (3)

%ifdef OSX_8 OSX_9 OSX_10 OSX_11 OSX_12 OSX_13
%define DYLD_NLIST_TYPE (0x1e)
%elifdef OSX_7
%define DYLD_NLIST_TYPE (0x1e)
%elifdef OSX_6
%define DYLD_NLIST_TYPE (0xe)
%else
%error Unknown OS
%endif

%include "symbols.asm"

%ifndef LATURI_ASM_ONLY
%define __LATURI__relof____dyld_get_image_header 0
%define __LATURI__relof____dyld_get_image_name 12
%define __LATURI__relof____dyld_get_image_vmaddr_slide 24
%else
%define __LATURI__relof____dyld_get_image_header 0
%define __LATURI__relof____dyld_get_image_name 8
%define __LATURI__relof____dyld_get_image_vmaddr_slide 16
%endif

CalculateHash:
	xor edx,edx
.hashloop:
%ifdef HASH_LEN_4 HASH_LEN_3
	imul edx,dword 0xfeedfacf
%elifdef HASH_LEN_2
	imul edx,dword 0x31e9
%else
%error Wrong hash length
%endif
	lodsb
	xor dl,al
	test al,al
	jnz short .hashloop
	xor edx,[r14]
%ifdef HASH_LEN_4
	; we are good
%elifdef HASH_LEN_3
;	shl edx,8
	and edx,0x7f7f7f
%else
	shl edx,16
%endif
	ret
; calling convention: parameters in rdi,rsi,rdx,rcx,r8,r9
; saves rbx,rbp,r12-r15 in calls
Global __LATURI__CodeStart
__LATURI__CodeStart:
;	and rsp,byte ~15
	push rax
	push byte DYLD_FUNC_COUNT
	pop rbx
	mov cl,DYLD_NLIST_TYPE
%ifdef OSX_11 OSX_10 OSX_9 OSX_8 OSX_7
; hack. lets take the return address for the __dyld_start
; after call the rsp is added 16 bytes
;	mov rbp,[byte rsp-16]
	mov rbp,[rsp]
	sub rbp,qword 0x1000 ; one page out
	and rbp,qword ~0xfff
	mov r15,qword -DYLD_ADDRESS
	add r15,rbp
%else
	mov rbp,qword DYLD_ADDRESS
	xor r15,r15
%endif
; currently broken
;	mov r12,dword __LATURI__JumpTable
	mov r13,r12
;	mov r14,dword __LATURI__HashStart
.main_loop:
;	rax
;	rbx func count
;	rcx mask
;	rdx
;	rsi
;	rdi
;	rbp lib header
;	r8
;	r9
;	r10
;	r11
;	r12 dest
;	r13 jumptable
;	r14 hashmap
;	r15 vmslide
; we need:
; vmslide
; seg.vmaddr (from __LINKEDIT)
; seg.fileoff (from __LINKEDIT)
; symtab.symoff
; symtab.stroff
; dysymtab.iextdefsym
;
; we will output:
; nlist		slide+seg.vmaddr-seg.fileoff+symtab.symoff+dysymtab.iextdefsym*nlist_size	rdx
;		or
;		slide+seg.vmaddr-seg.fileoff+symtab.symoff
; strings	slide+seg.vmaddr-seg.fileoff+symtab.stroff					rdi

	push rbp
	mov rdx,r15
.browse_mach_loop:
	mov eax,[byte rbp+m_header_size+m_cmd.len] ; zero extend to rax
	add rbp,rax				   ; first load command is text
	mov eax,[byte rbp+m_header_size+m_cmd.cmd]
	cmp al,byte SOLVE(LC_SEGMENT_64)
	jnz short .browse_mach_no_seg
	cmp [byte rbp+m_header_size+seg_cmd.segname+2],byte 0x4c ; 'L'
;	cmp [byte rbp+m_header_size+seg_cmd.segname+2],word 0x494c ; 'LI' backwards
;	cmp [byte rbp+m_header_size+seg_cmd.segname+2],dword 0x4b4e494c ; 'LINK' backwards
	jnz short .browse_mach_no_seg
	add rdx,[byte rbp+m_header_size+seg_cmd.vmaddr]
	sub rdx,[byte rbp+m_header_size+seg_cmd.fileoff]
.browse_mach_no_seg:
	cmp al,byte SOLVE(LC_SYMTAB)
	jnz short .browse_mach_no_sym
	mov edi,[byte rbp+m_header_size+symtab_cmd.stroff]
	add rdi,rdx
	mov esi,[byte rbp+m_header_size+symtab_cmd.symoff]
	add rdx,rsi
.browse_mach_no_sym:
	cmp al,byte SOLVE(LC_DYSYMTAB)
	jnz short .browse_mach_loop

	cmp cl,DYLD_NLIST_TYPE
	jz short .browse_mach_is_dyld
	imul eax,[byte rbp+m_header_size+dysymtab_cmd.iextdefsym],byte nlist_size
	add rdx,rax
.browse_mach_is_dyld:
	pop rbp

;	rax
;	rbx func count
;	rcx mask
;	rdx nlist
;	rsi
;	rdi strings
;	rbp lib header
;	r8
;	r9
;	r10
;	r11
;	r12 dest
;	r13 jumptable
;	r14 hashmap
;	r15 vmslide
.load_funcs_loop:
	push rdx
.browse_lib:
	cmp [byte rdx+nlist.type],cl
	jnz short .browse_lib_continue
;	mov esi,[byte rdx+nlist.stri]
	mov esi,[rdx]
	add rsi,rdi
	push rdx
	call near CalculateHash
	pop rdx
.browse_lib_continue:
	lea rdx,[byte rdx+nlist_size]
	jnz short .browse_lib
	; store the address in jump map
	mov rax,[byte rdx+nlist.addr-nlist_size]
	add rax,r15
	xchg rdi,r12
	stosq
%ifndef LATURI_ASM_ONLY
	mov eax,0xb848e0ff	; jmp rax ; mov rax,imm64
	stosd
%endif
	xchg rdi,r12
%ifdef HASH_LEN_4
	lea r14,[byte r14+4]
%elifdef HASH_LEN_3
	lea r14,[byte r14+3]
%else
	lea r14,[byte r14+2]
%endif
	pop rdx
	dec ebx
	jnz short .load_funcs_loop

	mov bl,[r14]
	inc r14
	test ebx,ebx

%ifdef TRAN_1
	jnz short .no_main

global CallTranslateCount
CallTranslateCount equ $+3
	mov rcx,dword 0
	lea rdi,[rel CodeEnd]
	mov al,0xe8
.call_translate_loop:
	repnz scasb
	sub [rdi],rdi
	scasd
	loop .call_translate_loop

	jmp near _main
.no_main:
%else
	jz near _main
%endif

;	rax
;	rbx func count
;	rcx
;	rdx
;	rsi
;	rdi
;	rbp
;	r8
;	r9
;	r10
;	r11
;	r12 dest
;	r13 jumptable
;	r14 hashmap
;	r15
	xor ebp,ebp
.browse_images_loop:
	inc ebp
	push rbp
	pop rdi
	call qword near [byte r13+__LATURI__relof____dyld_get_image_name]

	push rax
	pop rsi
	call near CalculateHash
	jnz short .browse_images_loop

	push rbp
	pop rdi
	call qword near [byte r13+__LATURI__relof____dyld_get_image_vmaddr_slide]
	mov r15,rax
	push rbp
	pop rdi
	call qword near [byte r13+__LATURI__relof____dyld_get_image_header]
	push rax
	pop rbp

%ifdef HASH_LEN_4
	lea r14,[byte r14+4]
%elifdef HASH_LEN_3
	lea r14,[byte r14+3]
%else
	lea r14,[byte r14+2]
%endif

	mov cl,0xf
	jmp near .main_loop
CodeEnd:

; loader
extern __LATURI__HashStart
extern __LATURI__JumpTable
; here will be the beef
extern _main
; functions
extern __dyld_get_image_name
extern __dyld_get_image_header
extern __dyld_get_image_vmaddr_slide
; jump tables
;extern __LATURI__relof____dyld_get_image_name
;extern __LATURI__relof____dyld_get_image_header
;extern __LATURI__relof____dyld_get_image_vmaddr_slide

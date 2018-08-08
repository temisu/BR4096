; Copyright (C) Teemu Suutari

%macro RAW_LIBCALL 1
; edi first param
; esi second param
; edx third param
; ecx fourth param
; return value in eax
	push ecx
	push edx
	push esi
	push edi

	call %1

	pop edi
	pop esi
	pop edx
	pop ecx
%endm

%ifndef NO_LATURI

%macro GETSYM 2
extern %2
	mov %1,[dword %2]
%endm

extern __LATURI__JumpTable

%macro LIBCALL 1
extern %1
	mov eax,%1
	RAW_LIBCALL [eax]
%endm

%macro LIBCALL_NOSTACK 1
extern %1
	mov eax,%1
	call [eax]
%endm

%else

%macro GETSYM 2
	[section .data]
%%sym_r_%2:
extern %2
	dd %2
	__SECT__

	mov %1,[%%sym_r_%2]
%endm

%macro LIBCALL 1
	[section .data]
%%func_r_%1:
extern %1
	dd %1
	__SECT__

	push dword %%func_r_%1
	pop eax
	RAW_LIBCALL [eax]
%endm

%macro LIBCALL_NOSTACK 1
	[section .data]
%%func_r_%1:
extern %1
	dd %1
	__SECT__

	push dword %%func_r_%1
	pop eax
	call [eax]
%endm

%endif

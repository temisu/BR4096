; Copyright (C) Teemu Suutari

%macro DEBUG_PLAIN 1
%ifdef DEBUG_BUILD
	[section .data]
%%dbg_text:
	db %1
	db 0
	__SECT__
	mov edi,%%dbg_text
	LIBCALL _printf
%endif
%endm

%macro DEBUG 1
%ifdef DEBUG_BUILD
	pushad
	mov esi,__LINE__
	DEBUG_PLAIN {"[",__FILE__,":%d] ",%1,10}
	popad
%endif
%endm

%macro DEBUG 2
%ifdef DEBUG_BUILD
	pushad
	push dword %2
	pop edx
	mov esi,__LINE__
	DEBUG_PLAIN {"[",__FILE__,":%d] ",%1,10}
	popad
%endif
%endm

%macro DEBUG 3
%ifdef DEBUG_BUILD
	pushad
	push dword %3
	push dword %2
	pop edx
	pop ecx
	mov esi,__LINE__
	DEBUG_PLAIN {"[",__FILE__,":%d] ",%1,10}
	popad
%endif
%endm

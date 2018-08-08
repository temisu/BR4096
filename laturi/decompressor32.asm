; Copyright (C) Teemu Suutari

; x86
	bits 32
section .text

global __ONEKPAQ__CodeStart
__ONEKPAQ__CodeStart:
	sub esp,byte 12

%ifdef COMPRESSION_1 COMPRESSION_2 COMPRESSION_3 COMPRESSION_4

	mov ebx,__ONEKPAQ_source
	mov edi,__ONEKPAQ_destination
	push edi

%ifdef COMPRESSION_1
%define ONEKPAQ_DECOMPRESSOR_MODE 1
%elifdef COMPRESSION_2
%define ONEKPAQ_DECOMPRESSOR_MODE 2
%elifdef COMPRESSION_3
%define ONEKPAQ_DECOMPRESSOR_MODE 3
%elifdef COMPRESSION_4
%define ONEKPAQ_DECOMPRESSOR_MODE 4
%endif
%include "onekpaq/onekpaq_decompressor32.asm"

global __ONEKPAQ_shift
__ONEKPAQ_shift:	equ onekpaq_decompressor.shift

	ret
%endif


extern __ONEKPAQ_source
extern __ONEKPAQ_destination

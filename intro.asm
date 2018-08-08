; Copyright (C) Teemu Suutari

; x86
	bits 32
section .text

%include "common/symbols.asm"
%include "common/structs.asm"
%include "common/debug.asm"
%include "common/exit.asm"
%include "common/libcall.asm"

%include "video/gl_libcall.asm"
%include "video/framerate.asm"


global _main
_main:
%ifdef NO_LATURI
	and esp,byte ~15

	LIBCALL _load_all_syms
%endif
	DEBUG "Intro 2013"
	DEBUG ""
	DEBUG "---------------------"
	DEBUG "--- Program start ---"
	DEBUG "---------------------"
	DEBUG ""
	DEBUG "ESP 0x%08x",esp
	DEBUG ""

%include "audio/player_main.asm"
%include "video/gl_init.asm"

	mov edi,[audio_device]
	mov esi,AudioProc
	LIBCALL _AudioDeviceStart
	DEBUG "_AudioDeviceStart %d",eax
intro_loop:

%include "video/gl_display.asm"

;	mov	edi, [alsource]
;	mov	esi, SOLVE(AL_SEC_OFFSET)
;	mov 	edx,esp
;	push dword [sample_position]
;	fild dword [esp]
;	pop eax
;	push dword 52838
;	fidiv dword [esp]
;	fstp dword [esp]
;	pop edi
	mov edi,[sample_position]
	LIBCALL	_glTexCoord1i

;	pop edi
;	mov esi,esp
;	push edi
;	LIBCALL	_MusicPlayerGetTime
;	add dword [byte esi+6],byte -128
;	mov edi,esi
;	LIBCALL _glColor3dv

;	LIBCALL _CGEventSourceKeyState
	mov edi,esp
%ifdef NO_LATURI OLD_OSX
	LIBCALL _CGSGetKeys
%else
	LIBCALL _SLSGetKeys
%endif

;	dec dword [byte edi+4]
;	js intro_loop

	shr byte [byte edi+6],6
	jnc intro_loop

	DEBUG "done, exiting"
intro_exit:
	; no exit here for non-debug, will obviously crash later :D
%ifdef NO_LATURI
	EXIT
%endif

player_modules_start:
%include "audio/audio_proc.asm"
%include "audio/modules.asm"
player_modules_end:

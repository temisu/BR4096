; Copyright (C) Teemu Suutari

%ifdef NO_LATURI
	[section .data]
gl_init_attribs:
;	dd SOLVE(kCGLPFADoubleBuffer)
	; seems to work with only double buffering
	dd 0
	__SECT__
%endif

extern _gl_main_fragment_shader

gl_init:
	DEBUG "Capture displays"
;	LIBCALL_NOSTACK _CGCaptureAllDisplays
%ifdef NO_LATURI OLD_OSX
	LIBCALL_NOSTACK _CGDisplayHideCursor
%else
	LIBCALL_NOSTACK _SLDisplayHideCursor
%endif

%ifdef MODE_SWITCHING
	DEBUG "Switch display mode"
	push byte 0
	push byte 0
	push byte 0
	push byte 0
	push dword SCREEN_HEIGHT
	push dword SCREEN_WIDTH
	push byte 32
	push byte 0
	LIBCALL_NOSTACK _CGDisplayBestModeForParameters
	push byte 0
	push byte 0
	push eax
	push byte 0
	LIBCALL_NOSTACK _CGDisplaySwitchToMode
%endif
	DEBUG "CGL init"
	mov edi,esp
	push eax
	push esp
	push edi
%ifdef NO_LATURI
	push dword gl_init_attribs
%else
	push byte 8	; first empty spot in header
%endif
	LIBCALL_NOSTACK _CGLChoosePixelFormat
	push eax
	push edi
	push eax
	push dword [edi]
	LIBCALL_NOSTACK _CGLCreateContext

%if 1
	pop ebp
	push eax
%ifdef NO_LATURI OLD_OSX
	LIBCALL_NOSTACK _CGDisplayIDToOpenGLDisplayMask
%else
	LIBCALL_NOSTACK _SLDisplayIDToOpenGLDisplayMask
%endif
%endif

	push eax
%if 0
	inc eax
%endif
	push edi
	push eax
	push dword [edi]
	LIBCALL_NOSTACK _CGLSetCurrentContext


.mask_loop:
	DEBUG "CGL Fullscreen on mask %x",[byte esp+40]
	LIBCALL_NOSTACK _CGLSetFullScreenOnDisplay

; Without _CGDisplayIDToOpenGLDisplayMask
%if 0
	rol dword [byte esp+4],1
	dec eax
	jns short .mask_loop
	inc eax
%endif

;	DEBUG "VSYNC"
;	mov [edi],dword 2
;	mov [byte esp+4],byte SOLVE(kCGLCPSwapInterval)
;	LIBCALL_NOSTACK _CGLSetParameter

	DEBUG "Initializing shader"
;	push esp
;	pop edi
	pop ebp
	push dword _gl_main_fragment_shader
	mov edi,esp
	push eax
	push edi
	inc eax
	push eax
	push SOLVE(GL_FRAGMENT_SHADER)
	LIBCALL_NOSTACK _glCreateShader
	DEBUG "Shader is %d",eax
	pop ebp
	push eax

	LIBCALL_NOSTACK _glShaderSource
	LIBCALL_NOSTACK _glCompileShader

%if 0
	; shader == 1 -> next things are not stricly necessary
	pop ebp
	pop eax
	push eax
	push eax
%endif

	LIBCALL_NOSTACK _glCreateProgram
	DEBUG "Program is %d",eax
	pop ebp
	push eax

	LIBCALL_NOSTACK _glAttachShader
	LIBCALL_NOSTACK _glLinkProgram
	LIBCALL_NOSTACK _glUseProgram

;	pop edi
;	push byte 'm' ; 'tex'
;	mov esi,esp
;	GL_LIBCALL _glGetUniformLocation
;	push byte 1
;	push byte 0
;	mov edx,esp
;	push eax
;	push eax
;	xchg eax,edi
;	push byte 2
;	pop esi
;	GL_LIBCALL _glUniform1iv

	DEBUG "GL init done"

	mov ecx,0x10000
;	xor ecx,ecx
;	dec cx
	sub esp,ecx
	mov edi,esp
	mov ebp,esp
.random_loop:
	LIBCALL _rand
	stosb
	loop .random_loop

	DEBUG "Random generated"

;	pushad
	push byte 1
	pop edi
	mov esi,esp
	GL_LIBCALL _glGenTextures
;	popad
; Works without texture binding.
;	mov esi,edi
	mov edi,SOLVE(GL_TEXTURE_3D)
;	GL_LIBCALL _glBindTexture

	mov esi,SOLVE(GL_TEXTURE_MIN_FILTER)
	mov edx,SOLVE(GL_LINEAR)
	GL_LIBCALL _glTexParameteri

	xor esi,esi
	push byte 1
	pop edx
	mov cl,32
	push ecx
	push ecx
	push ebp
	push dword SOLVE(GL_UNSIGNED_BYTE)
	push dword SOLVE(GL_RED)
	push byte 0
	push ecx
	push ecx
	GL_LIBCALL _glTexImage3D

	DEBUG "Noise texture done"
gl_init_end:

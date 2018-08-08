; Copyright (C) Teemu Suutari

gl_display:
%ifdef NO_LATURI
	pushad
	LIBCALL _CGMainDisplayID
	mov edi,eax
	LIBCALL _CGDisplayPixelsWide
	mov edx,eax
	LIBCALL _CGDisplayPixelsHigh
	mov ecx,eax

	xor edi,edi
	xor esi,esi
	GL_LIBCALL _glViewport
	pushad
	mov edi,SOLVE(GL_COLOR_BUFFER_BIT)
	GL_LIBCALL _glClear
	popad

	xor edi,edi
	xor esi,esi
	mov edx,SCREEN_WIDTH
	mov ecx,SCREEN_HEIGHT
	GL_LIBCALL _glViewport
	popad
%else
;	mov edi,SOLVE(GL_COLOR_BUFFER_BIT)
;	GL_LIBCALL _glClear
%endif

	push byte 1
	push byte 1
	push byte -1
	push byte -1
;	pop edi
;	pop esi
;	pop edx
;	pop ecx
	GL_LIBCALL_NOSTACK _glRecti

	FRAMERATE

	GL_LIBCALL_NOSTACK _glSwapAPPLE

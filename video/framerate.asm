; Copyright (C) Teemu Suutari

%macro FRAMERATE 0
%ifdef DEBUG_BUILD
	[section .data]
%%million:
	dq 1e6
%%text_format:
	db "FPS: %2.0llf",0
%%text_store:
	times 32 db 0
	__SECT__
	[section .bss]
%%frametime:
	resq 1
%%LastFrameTime:
	resq 1
%%frametime_old:
	resq 1
%%tmp:
	resq 1
	__SECT__

	pushad
; current time to oldtime
	mov esi,%%frametime
	mov edi,%%frametime_old
	movsd
	movsd
; calc a new timestamp
	mov edi,%%frametime
	xor esi,esi
	LIBCALL _gettimeofday
	fild dword [edi]
	fild dword [byte edi+4]
	fld qword [%%million]
	fdivp st1
	faddp st1
	fst qword [%%frametime]
; and substract them
	fld qword [%%frametime_old]
	fsubp st1
	fld1
	fdivrp st1
	fstp qword [%%tmp]
; make the string
	mov edi,%%text_store
	mov esi,%%text_format
	mov edx,[%%tmp]
	mov ecx,[%%tmp+4]
	LIBCALL _sprintf
; draw
	mov edi,SOLVE(GL_PROJECTION)
	GL_LIBCALL _glMatrixMode
	GL_LIBCALL _glPushMatrix
	GL_LIBCALL _glLoadIdentity
	xor edi,edi
	xor esi,esi
	mov edx,__float64__(1.0)&0xffffffff
	mov ecx,__float64__(1.0)>>32
	push dword __float64__(1.0)>>32
	push dword __float64__(1.0)&0xffffffff
	push byte 0
	push byte 0
	GL_LIBCALL _gluOrtho2D
	add esp,byte 4*4
	mov edi,SOLVE(GL_MODELVIEW)
	GL_LIBCALL _glMatrixMode
	GL_LIBCALL _glPushMatrix
	GL_LIBCALL _glLoadIdentity
	mov edi,SOLVE(GL_COLOR_BUFFER_BIT)
	GL_LIBCALL _glPushAttrib
	mov edi,__float32__(1.0)
	mov esi,__float32__(1.0)
	mov edx,__float32__(1.0)
	GL_LIBCALL _glColor3f
	xor edi,edi
	xor esi,esi
	GL_LIBCALL _glRasterPos2f

	mov edi,SOLVE(GL_CURRENT_PROGRAM)
	pushad
	mov esi,esp
	GL_LIBCALL _glGetIntegerv
	popad
	mov ebx,edi
	xor edi,edi
	GL_LIBCALL _glUseProgram
; finally the bitmap
	mov ecx,%%text_store
	GETSYM edi,_glutBitmapHelvetica12
%%loop:
	movzx esi,byte [ecx]
	LIBCALL _glutBitmapCharacter
	inc ecx
	cmp [ecx],byte 0
	jnz %%loop

	mov edi,ebx
	GL_LIBCALL _glUseProgram

	GL_LIBCALL _glPopAttrib
	GL_LIBCALL _glPopMatrix
	mov edi,SOLVE(GL_PROJECTION)
	GL_LIBCALL _glMatrixMode
	GL_LIBCALL _glPopMatrix

	GL_LIBCALL _glColor3f
; done
	popad
%endif
%endm

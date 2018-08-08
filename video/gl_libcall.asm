; Copyright (C) Teemu Suutari

%macro GL_LIBCALL 1
	LIBCALL %1
%ifdef DEBUG_BUILD
	pushad
	LIBCALL _glGetError
	test eax,eax
	jz short %%gl_no_error
	pushad
	mov edi,eax
	LIBCALL _gluErrorString
%defstr %%glfunc %1
	DEBUG {%%glfunc," failed (%s)"},eax
	popad
%%gl_no_error:
	popad
%endif
%endm

%macro GL_LIBCALL_NOSTACK 1
	LIBCALL_NOSTACK %1
%endm

%macro GL_NOERR_LIBCALL 1
	LIBCALL %1
%ifdef DEBUG_BUILD
	pushad
	LIBCALL _glGetError
	popad
%endif
%endm

; Copyright (C) Teemu Suutari

%macro EXIT 0
%ifdef NO_LATURI
	xor edi,edi
	LIBCALL _exit
%else
	int3
%endif
%endm

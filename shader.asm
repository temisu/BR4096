; Copyright (C) Teemu Suutari

; x86
	bits 32
section .text

	[section .data]

; !!!! make sure these match with shader after minimizing
;global _uniform_variable
;global _uniform_units
;_uniform_variable db "tex",0
;_uniform_units dd 0,1


global _gl_main_fragment_shader
_gl_main_fragment_shader:
%if (SCREEN_WIDTH==1920) && (SCREEN_HEIGHT==1080)
	incbin "data/main_shader_1920_1080.fsh"
%elif (SCREEN_WIDTH==1280) && (SCREEN_HEIGHT==800)
	incbin "data/main_shader_1280_800.fsh"
%elif (SCREEN_WIDTH==1280) && (SCREEN_HEIGHT==720)
	incbin "data/main_shader_1280_720.fsh"
%else
%error NORESOLUTION
%endif
; end of program, will be zero
%ifdef NO_LATURI
	db 0
%endif
	__SECT__

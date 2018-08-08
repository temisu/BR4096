;;
;; Komposter player (c) 2011 Firehawk/TDA
;;
;; This code is licensed under the MIT license:                             
;; http://www.opensource.org/licenses/mit-license.php
;;
;; This is a reference implementation for a Komposter playroutine
;; in x86 assembly language.
;;

; macros SONGDATA(address) and SONGBSS(address) are used for
; getting a pointer address to the data. change these macros
; if you relocate them

; output sample rate
%define 	OUTPUTFREQ	44100

; how many samples of audio we're rendering
;%define		SONG_BUFFERLEN	5*OUTPUTFREQ
;%define		SONG_BUFFERLEN	163*OUTPUTFREQ

; maximum number of modules per synth. 64 means 256 bytes of modulator data per channel.
%define		MAX_MODULES		40 ;64

; delay buffer addresses are max. 18bit offsets
%define 	DELAYBUFFERSIZE		262144

;.pan ____
;0000 0000

; flags in voice data byte
%define		FLAG_GATE		1
%define		FLAG_TRIG		2
%define		FLAG_ACCENT		32; 4
%define		FLAG_NOTEOFF		16; 8
;%define		FLAG_RESTART_ENV	16
;%define		FLAG_RESTART_VCO	32
;%define		FLAG_RESTART_LFO	64
%define		FLAG_LOAD_PATCH		128



;
; DATA
;
        [section .data]

;outputfreq	dd	44100.0  ; outputfreq as a float value
;noise_x1	dd	0x67452301
;noise_x2	dd	0xefcdab89
noise_div	dd	2147483648.0 ;0x4f000000

;midi0           dd      0.00018539 ;(8.1757989156 / 44100.0) ; C0
%define		midi0	(patchdata.p02+2*4)
midisemi        dd      1.059463094 ; ratio between notes a semitone apart

; small constants used around the code - use data in song if applicable
;zero		dd	0.0
%define		zero	patchdata.p00
;one		dd	1.0
%define		one	(patchdata.p04+4*4)
;minus_one	dd	-1.0
;half		dd	0.5
%define		half	(patchdata.p02+19*4)
;threefourths	dd	0.75
;four		dd	4.0
;five		dd	5.0

;; these are for the 24db/oct lpf, comment out if not used
;three_point_foureight   dd      3.48
;minus_point_onefive     dd      -0.15
;lpf_feedback_coef       dd      0.35013  
;point_three             dd      0.3


; include the song itself from an external file
songdata_start:
%include "audio/song.inc"
songdata_end:

        __SECT__

;
; BSS
;

struc audio_stream_basic_desription
.sample_rate		resq 1
.format			resd 1
.format_flags		resd 1
.bytes_per_packet	resd 1
.frames_per_packet	resd 1
.bytes_per_frame	resd 1
.channels_per_frame	resd 1
.bits_per_channel	resd 1
.reserved		resd 1
endstruc


	[section .bss]
;render_pos:
;	resd 1
audio_device:
	resd 1
sample_position:
	resd 1
sample:
	resd	1
noise:
	resd 	1
delaycount:
	resd 1 ;resb	1

	resb 256-5*4

audio_format:
	resb 256 ; resb audio_stream_basic_desription_size

pitch		resb 256 ;resd	NUM_CHANNELS
flags		resb 256 ;resb	NUM_CHANNELS
patchptr	resb 256 ;resd	NUM_CHANNELS

songdataptr	resb 256 ;resd	NUM_CHANNELS
restcount	resb 256 ;resb	NUM_CHANNELS

	resb (256-7)*256

;global songbuffer
;songbuffer	resb 65536 ;resw	SONG_BUFFERLEN
moddata		resb 65536*16 ;resd	NUM_CHANNELS*MAX_MODULES*32 ; output,mod,moddata*16,reserved*14
delaybuffer	resb 1024*1024*4*4 ;resd NUM_DELAYS*DELAYBUFFERSIZE


;
; TEXT
;
        __SECT__

; input ebx - sample number
; output st0 - output sample
playercode_start:
render_sample:

	; start the loop to fill the outputbuffer with rendered audio
	;xor	ebx, ebx ; ebx = sample number
.sample_loop:
	push	ebx ; save sample number

LIBCALL _rand
mov [noise], eax

	mov	eax, ebx
	xor	edx, edx
	idiv	dword [tickdivider]
	and	edx, edx
	jnz 	.synth
	mov	ecx, eax ; tick number in ecx

	; play notes
	xor	edx, edx
.channel_loop:
	mov	bl, byte [flags+edx] ; get flags to bl
	mov 	esi, [channelptr+edx] ;*4]
        and     cl, 63
        jnz     .tick0_end
.tick0:
	xor	eax, eax

	; test rest count to whether to load new word
	or	al, [restcount+edx]
	jz 	.load_flags
	dec 	byte [restcount+edx]
	jmp 	.channel_done

.load_flags:
	lodsw
	and	al, al
        jz      .test_accent
	jns	.midi_note
	and 	al, 0x7f
	mov	[restcount+edx], al
	jmp	.channel_done

.midi_note:
        fld     dword [midi0]
.pitchmul:
        fmul    dword [midisemi]
	mov 	bl, al ; bl remains 1 on exit from loop (FLAG_GATE)
        dec     al
        jnz     .pitchmul
        fstp    dword [pitch+edx] ;*4]


.test_accent:
	xchg 	al, ah
;	test	al, 0x20 ;0x40
;        jz      .test_noteoff
;	or	bl, FLAG_ACCENT
;.test_noteoff:
;	test 	al, 0x10 ;0x80
;        jz      .test_patch
;	or	bl, FLAG_NOTEOFF
mov bh,al
and bh,0xf0
or bl,bh

.test_patch:
        and     al, 0x0f ;0x3f
        jz      .tick0_end

	; calc patch data start position and store for the channel
	mov 	eax, [patchstart+eax*4-4]
        mov     [patchptr+edx], eax ; edx*4
	or	bl, FLAG_LOAD_PATCH

.tick0_end:
        cmp     cl, 60
        jnz     .channel_done
	test	bl, FLAG_NOTEOFF
        jz      .channel_done
	and	bl, FLAG_TRIG|FLAG_ACCENT
.channel_done:
mov [channelptr+edx], esi ; put channel pointer back ;edx*4
	mov	[flags+edx], bl ; put flags back
	add	esi, (SONG_LEN-1)*2
;        inc     edx
;        cmp     edx, NUM_CHANNELS
add edx, byte 4
cmp edx, NUM_CHANNELS*4
	jnz	.channel_loop

	; process synthesizer voices
.synth:
	xor	edx, edx ; edx=voice
	mov	[sample], edx ; set mixed sample to zero
.voice_loop:

	; process the signal stack by looping through all the
	; modules
.mod_process:
	xor	ecx, ecx ; ecx = module index
	mov	edi, edx ; edx = channel
	shl 	edi, 13  ; 8192 bytes per channel, 128 bytes per module
	add 	edi, moddata
	mov	esi, edi
.mod_process_loop:
	mov 	ebp, [voicefunctions+edx] ;*4]
	mov 	ebp, [ebp+ecx]
	; ebp is ptr to module function

	and 	ebp, ebp 
	jz 	.mod_process_end ; zero=no more modules in synth

	; load a new patch?
	test	byte [flags+edx], FLAG_LOAD_PATCH ;*4
	jz	.mod_process_prepare
	; load value from patch to modulator

	mov	eax, [patchptr+edx] ;*4]
	mov	eax, [eax+ecx] ;*4]
	mov	[edi+4], eax

	; store modulator first to fpu
.mod_process_prepare:
	; esi = start of channel data area
	; edi = start of current module data area
	fninit
	fld	dword [edi+4] ; modulator at +4

	; collect input signal voltages to fpu
	mov 	eax, [voiceinputs+edx] ;*4]
	mov 	eax, [eax+ecx]
.mod_signal_loop:
	xor	ebx, ebx

	mov	bh, al ; ebx = input module index
	fld 	dword [esi+ebx] ; output at offset 0
	shr	eax, 8 ; next byte to al

	mov	bh, al ; ebx = input module index
	fld 	dword [esi+ebx] ; output at offset 0
	shr	eax, 8 ; next byte to al

	mov	bh, al ; ebx = input module index
	fld 	dword [esi+ebx] ; output at offset 0
	shr	eax, 8 ; next byte to al

	mov	bh, al ; ebx = input module index
	fld 	dword [esi+ebx] ; output at offset 0
	shr	eax, 8 ; next byte to al

	add ecx, byte 4

	pushad
	mov 	eax, [edi+4]		; eax = modulator as integer
	mov	cl, [flags+edx] ;*4         ; cl = flags
	lea 	esi, [edi+8]            ; esi = data area starts at +8
	call 	ebp
	popad
	fst 	dword [edi] ; store output

	add 	edi, 256
	jmp 	.mod_process_loop

.mod_process_end:
;	and	byte [flags+edx*4], 0x0f ; clear hard restart flags

.skip_voice:
	fadd	dword [sample]          ; mix previous channels in
	fstp	dword [sample]          ; and pop back to temp storage

;	inc	edx			; next voice
;	cmp	edx, NUM_CHANNELS
add edx, byte 4
cmp edx, NUM_CHANNELS*4
	jl	.voice_loop
	; all voices done, sample now has the final channel mix

.skip_synth:
	pop	ebx ; restore sample number
	fld	dword [sample]
	fmul	dword [samplemul]
playercode_end:

;;
;; eof
;;
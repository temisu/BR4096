AudioProc:
	pushad
	mov edx,[sample_position]

;DEBUG "buffering audio at 0x%08x",edx

	mov edi,[esp+5*4+ad_size]
; struct AudioBufferList { UInt32 mNumberBuffers; AudioBuffer mBuffers[1]; }
; struct AudioBuffer { UInt32 mNumberChannels; UInt32 mDataByteSize; void *mData; }
	mov ecx,[byte edi+4+4]
	shr ecx,3
	mov ebp,ecx
;	DEBUG "callback for %u samples",ecx
	mov edi,[byte edi+8+4]

	xor ebx,ebx
loop_samples:	
	pushad
	add ebx,edx
%include "audio/player.asm"
	popad

	fst	dword [edi+ebx*8]
	fstp	dword [edi+ebx*8+4]

	inc ebx
	cmp ecx,ebx
	jnz loop_samples

	add [sample_position],ebp
	popad
	xor eax,eax
	ret

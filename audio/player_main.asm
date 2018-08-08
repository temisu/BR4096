;;
;;
;; Here is the actual main program which renders the song, creates a
;; playback device, calls the player to render the audio and finally plays
;; the audio.
;;
;; This example code uses MacOS X ABI and OpenAL but adapting to other x86
;; systems should be trivial.
;;
;;


;	[section .bss]
;        __SECT__

	[section .text]

Player_main:
%ifdef DEBUG_BUILD
	mov eax,(playercode_end-playercode_start)+(player_modules_end-player_modules_start)
	DEBUG "Player code uncompressed length is %d bytes",eax
	mov eax,songdata_end-songdata_start
	DEBUG "Song uncompressed length is %d bytes",eax
%endif

	DEBUG "Initing CoreAudio..."

	pop ebp
	push byte 4

	mov edi,SOLVE(kAudioHardwarePropertyDefaultOutputDevice)
	mov esi,esp
	mov edx,audio_device
	LIBCALL _AudioHardwareGetProperty
	DEBUG "_AudioHardwareGetProperty %d",eax

	mov edi,esp
	push byte audio_stream_basic_desription_size
	push byte 0
	push dword audio_format
	push edi

;	mov edi,[audio_device]
	mov edi,[edx]
	xor esi,esi
	xor edx,edx
	mov ecx,SOLVE(kAudioDevicePropertyStreamFormat)
	LIBCALL _AudioDeviceGetProperty
	DEBUG "_AudioDeviceGetProperty %d",eax


	push byte 0
	push dword audio_format
	push byte audio_stream_basic_desription_size
	push dword SOLVE(kAudioDevicePropertyStreamFormat)

%ifdef DEBUG_BUILD
	mov ecx, [audio_format+audio_stream_basic_desription.bits_per_channel]
	DEBUG "Audio device has %d bits per channel", ecx
	mov ecx, [audio_format+audio_stream_basic_desription.bytes_per_frame]
	DEBUG "Audio device has %d bytes per frame", ecx
	mov ecx, [audio_format+audio_stream_basic_desription.format_flags]
	DEBUG "Audio device format flags %08x", ecx
%endif

	xor ecx,ecx
	mov [audio_format+audio_stream_basic_desription.sample_rate],dword __float64__(44100.0)&0xffffffff
	mov [audio_format+audio_stream_basic_desription.sample_rate+4],dword __float64__(44100.0)>>32
	mov [audio_format+audio_stream_basic_desription.channels_per_frame],dword 2
	LIBCALL _AudioDeviceSetProperty
	DEBUG "_AudioDeviceSetProperty %d",eax

	pop eax
	push dword SOLVE(kAudioDevicePropertyBufferSize)
	mov [audio_format+audio_stream_basic_desription],dword (44100/300)*8
	LIBCALL _AudioDeviceSetProperty
	DEBUG "_AudioDeviceSetProperty %d",eax


	mov esi,AudioProc
	xor edx,edx
	LIBCALL _AudioDeviceAddIOProc
	DEBUG "_AudioDeviceAddIOProc %d",eax

	DEBUG "CoreAudio Done"

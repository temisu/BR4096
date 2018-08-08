/* Copyright (C) Teemu Suutari */
#include <objc/message.h>

void load_all_syms(void)
{
}


void *_LATURI_Mangle__$2d$5bNSAttributedString$28NSAttributedStringUIFoundationAdditions$29$20initWithRTF$3adocumentAttributes$3a$5d(void*a,void*b,void*c,void*d) {
	return objc_msgSend(a,sel_getUid("initWithRTF:documentAttributes:"),c,d);
}

void *_LATURI_Mangle__$2b$5bNSImage$20imageWithSize$3aflipped$3adrawingHandler$3a$5d(void*a,void*b,void*c,void*d,void*e,void*f,void*g) {
	return objc_msgSend((id)objc_lookUpClass("NSImage"),sel_getUid("imageWithSize:flipped:drawingHandler:"),c,d,e,f,g);
}

void *_LATURI_Mangle__$2d$5bNSImage$20lockFocus$5d(void*a,void*b) {
	return objc_msgSend(a,sel_getUid("lockFocus"));
}

void *_LATURI_Mangle__$2d$5bNSAttributedString$28NSStringDrawing$29$20drawInRect$3a$5d(void*a,void*b,void*c,void*d,void*e,void*f) {
	return objc_msgSend(a,sel_getUid("drawInRect:"),c,d,e,f);
}

void *_LATURI_Mangle__$2d$5bNSImage$20unlockFocus$5d(void*a,void*b) {
	return objc_msgSend(a,sel_getUid("unlockFocus"));
}

void *_LATURI_Mangle__$2d$5bNSImage$20TIFFRepresentation$5d(void*a,void*b) {
	return objc_msgSend(a,sel_getUid("TIFFRepresentation"));
}

void *_LATURI_Mangle__$2b$5bNSOpenGLContext$20currentContext$5d(void) {
	return objc_msgSend((id)objc_lookUpClass("NSOpenGLContext"),sel_getUid("currentContext"));
}

void *_LATURI_Mangle__$2d$5bNSOpenGLContext$20setValues$3aforParameter$3a$5d(void*a,void*b,void*c,void*d) {
	return objc_msgSend(a,sel_getUid("setValues:forParameter:"));
}

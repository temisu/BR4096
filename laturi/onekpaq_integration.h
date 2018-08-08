/* Copyright (C) Teemu Suutari */

#ifndef ONEKPAQ_INTEGRATION_H
#define ONEKPAQ_INTEGRATION_H
#include "common.h"

void onekpaq_compress(uchar *data,uint len,uint *linker_blocks,uchar **dest,uint *destlen,uchar **header,uint *headerlen,uint *shift,uint mode);

#endif

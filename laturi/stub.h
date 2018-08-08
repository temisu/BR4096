/* Copyright (C) Teemu Suutari */

#ifndef STUB_H
#define STUB_H

#include "common.h"

extern const uchar header_obj[];

// [OSX_VERSION - 11 ] [ COMPRESSION ]
extern const uchar *decompressor_objs[3][5];

// [OSX_VERSION - 11 ] [ TRANSLATE ] [ HASH_LEN - 2 ]
extern const uchar *loader_objs[3][2][3];

#endif

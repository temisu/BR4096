/* Copyright (C) Teemu Suutari */

#ifndef COMMON_H
#define COMMON_H
#include "laturi_api.h"

typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef long double dfloat;

#define INFO_ERR(str,...) log_call(0,str,## __VA_ARGS__)
#define INFO_LOG(str,...) log_call(1,str,## __VA_ARGS__)
#if 0
#define INFO_DEBUG(str,...) log_call(2,str,## __VA_ARGS__)
#define DBG_ASSERT(x,str,...) ({if (x) log_call(-1,"[Assert failure] " str,## __VA_ARGS__);})
#else
#define INFO_DEBUG(str,...)
#define DBG_ASSERT(x,str,...)
#endif
#define ASSERT(x,str,...) ({if (x) log_call(-1,"[Assert failure] " str,## __VA_ARGS__);})

#endif

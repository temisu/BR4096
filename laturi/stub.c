/* Copyright (C) Teemu Suutari */

#include "common.h"

#ifndef __x86_64__

const uchar header_obj[]={
#include "header32.o.inc"
};

static const uchar d90[]={
#include "decompressor32_X11_C0.o.inc"
};

static const uchar d91[]={
#include "decompressor32_X11_C1.o.inc"
};

static const uchar d92[]={
#include "decompressor32_X11_C2.o.inc"
};

static const uchar d93[]={
#include "decompressor32_X11_C3.o.inc"
};

static const uchar d94[]={
#include "decompressor32_X11_C4.o.inc"
};

static const uchar da0[]={
#include "decompressor32_X12_C0.o.inc"
};

static const uchar da1[]={
#include "decompressor32_X12_C1.o.inc"
};

static const uchar da2[]={
#include "decompressor32_X12_C2.o.inc"
};

static const uchar da3[]={
#include "decompressor32_X12_C3.o.inc"
};

static const uchar da4[]={
#include "decompressor32_X12_C4.o.inc"
};

static const uchar db0[]={
#include "decompressor32_X13_C0.o.inc"
};

static const uchar db1[]={
#include "decompressor32_X13_C1.o.inc"
};

static const uchar db2[]={
#include "decompressor32_X13_C2.o.inc"
};

static const uchar db3[]={
#include "decompressor32_X13_C3.o.inc"
};

static const uchar db4[]={
#include "decompressor32_X13_C4.o.inc"
};

static const uchar l900[]={
#include "loader32_X11_T0_H2.o.inc"
};

static const uchar l901[]={
#include "loader32_X11_T0_H3.o.inc"
};

static const uchar l902[]={
#include "loader32_X11_T0_H4.o.inc"
};

static const uchar l910[]={
#include "loader32_X11_T1_H2.o.inc"
};

static const uchar l911[]={
#include "loader32_X11_T1_H3.o.inc"
};

static const uchar l912[]={
#include "loader32_X11_T1_H4.o.inc"
};

static const uchar la00[]={
#include "loader32_X12_T0_H2.o.inc"
};

static const uchar la01[]={
#include "loader32_X12_T0_H3.o.inc"
};

static const uchar la02[]={
#include "loader32_X12_T0_H4.o.inc"
};

static const uchar la10[]={
#include "loader32_X12_T1_H2.o.inc"
};

static const uchar la11[]={
#include "loader32_X12_T1_H3.o.inc"
};

static const uchar la12[]={
#include "loader32_X12_T1_H4.o.inc"
};

static const uchar lb00[]={
#include "loader32_X13_T0_H2.o.inc"
};

static const uchar lb01[]={
#include "loader32_X13_T0_H3.o.inc"
};

static const uchar lb02[]={
#include "loader32_X13_T0_H4.o.inc"
};

static const uchar lb10[]={
#include "loader32_X13_T1_H2.o.inc"
};

static const uchar lb11[]={
#include "loader32_X13_T1_H3.o.inc"
};

static const uchar lb12[]={
#include "loader32_X13_T1_H4.o.inc"
};

#else

const uchar header_obj[]={
#include "header64.o.inc"
};

static const uchar d90[]={
#include "decompressor64_X11_C0.o.inc"
};

static const uchar d91[]={
#include "decompressor64_X11_C1.o.inc"
};

static const uchar d92[]={
#include "decompressor64_X11_C2.o.inc"
};

static const uchar d93[]={
#include "decompressor64_X11_C3.o.inc"
};

static const uchar d94[]={
#include "decompressor64_X11_C4.o.inc"
};

static const uchar da0[]={
#include "decompressor64_X12_C0.o.inc"
};

static const uchar da1[]={
#include "decompressor64_X12_C1.o.inc"
};

static const uchar da2[]={
#include "decompressor64_X12_C2.o.inc"
};

static const uchar da3[]={
#include "decompressor64_X12_C3.o.inc"
};

static const uchar da4[]={
#include "decompressor64_X12_C4.o.inc"
};

static const uchar db0[]={
#include "decompressor64_X13_C0.o.inc"
};

static const uchar db1[]={
#include "decompressor64_X13_C1.o.inc"
};

static const uchar db2[]={
#include "decompressor64_X13_C2.o.inc"
};

static const uchar db3[]={
#include "decompressor64_X13_C3.o.inc"
};

static const uchar db4[]={
#include "decompressor64_X13_C4.o.inc"
};

static const uchar l900[]={
#include "loader64_X11_T0_H2.o.inc"
};

static const uchar l901[]={
#include "loader64_X11_T0_H3.o.inc"
};

static const uchar l902[]={
#include "loader64_X11_T0_H4.o.inc"
};

static const uchar l910[]={
#include "loader64_X11_T1_H2.o.inc"
};

static const uchar l911[]={
#include "loader64_X11_T1_H3.o.inc"
};

static const uchar l912[]={
#include "loader64_X11_T1_H4.o.inc"
};

static const uchar la00[]={
#include "loader64_X12_T0_H2.o.inc"
};

static const uchar la01[]={
#include "loader64_X12_T0_H3.o.inc"
};

static const uchar la02[]={
#include "loader64_X12_T0_H4.o.inc"
};

static const uchar la10[]={
#include "loader64_X12_T1_H2.o.inc"
};

static const uchar la11[]={
#include "loader64_X12_T1_H3.o.inc"
};

static const uchar la12[]={
#include "loader64_X12_T1_H4.o.inc"
};

static const uchar lb00[]={
#include "loader64_X13_T0_H2.o.inc"
};

static const uchar lb01[]={
#include "loader64_X13_T0_H3.o.inc"
};

static const uchar lb02[]={
#include "loader64_X13_T0_H4.o.inc"
};

static const uchar lb10[]={
#include "loader64_X13_T1_H2.o.inc"
};

static const uchar lb11[]={
#include "loader64_X13_T1_H3.o.inc"
};

static const uchar lb12[]={
#include "loader64_X13_T1_H4.o.inc"
};

#endif

const uchar *decompressor_objs[3][5]=
{
	{d90,d91,d92,d93,d94},
	{da0,da1,da2,da3,da4},
	{db0,db1,db2,db3,db4}
};

const uchar *loader_objs[3][2][3]=
{
	{{l900,l901,l902},{l910,l911,l912}},
	{{la00,la01,la02},{la10,la11,la12}},
	{{lb00,lb01,lb02},{lb10,lb11,lb12}}
};

/* Copyright (C) Teemu Suutari */

#include <fstream>

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "laturi_api.h"
#include "common.h"

}

#define LATURI_INTEGRATION 1
#include "onekpaq/onekpaq_common.h"
#include "onekpaq/StreamCodec.hpp"

//#define SAVE_BLOCKS 1

#ifdef SAVE_BLOCKS
static void writeFile(const std::string &fileName,const std::vector<u8> &src) {
	std::ofstream file(fileName.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (file.is_open()) {
		file.write(reinterpret_cast<const char*>(src.data()),src.size());
		file.close();
	}
}
#endif

extern "C" {

void DebugPrint(const char *str,...)
{
	char tmp[1024];
	va_list ap;

	va_start(ap,str);
	vsprintf(tmp,str,ap);
	va_end(ap);

	INFO_LOG("%s",tmp);
}

void DebugPrintAndDie(const char *str,...)
{
	char tmp[1024];
	va_list ap;

	va_start(ap,str);
	vsprintf(tmp,str,ap);
	va_end(ap);

	INFO_LOG("%s",tmp);
	abort();
}

void onekpaq_compress(uchar *data,uint len,uint *linker_blocks,uchar **dest,uint *destlen,uchar **header,uint *headerlen,uint *shift,uint mode)
{
	std::vector<std::vector<u8>> blocks;
	uint i=0,pos=0,tot_len=0;
	while (tot_len!=len) {
		blocks.push_back(std::vector<u8>(data+pos,data+pos+linker_blocks[i]));
		pos+=linker_blocks[i];
		tot_len+=linker_blocks[i];
		i++;
	}
#ifdef SAVE_BLOCKS
	uint i=1;
	for (auto &it : blocks) {
		writeFile("block" + std::to_string(i++) + ".data",it);
	}
#endif
	StreamCodec sc;
	// TODO: real option
	sc.Encode(blocks,StreamCodec::EncodeMode(mode),StreamCodec::EncoderComplexity::Low,"onekpaq_context.cache");
	auto dst1=sc.GetAsmDest1();
	auto dst2=sc.GetAsmDest2();

	*destlen=(uint)dst2.size();
	*dest=(uchar*)malloc(dst2.size());
	memcpy(*dest,dst2.data(),dst2.size());

	*headerlen=(uint)dst1.size();
	*header=(uchar*)malloc(dst1.size());
	memcpy(*header,dst1.data(),dst1.size());

	*shift=sc.GetShift();
}

}
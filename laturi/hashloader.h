/* Copyright (C) Teemu Suutari */

#ifndef HASHLOADER_H
#define HASHLOADER_H

#include "laturi_api.h"

#ifndef __x86_64__

#ifndef LATURI_ASM_ONLY
#define JUMPTBL_ITEM_SIZE (6)
#define JUMPTBL_ADDR_OFFSET (1)
#else
#define JUMPTBL_ITEM_SIZE (4)
#define JUMPTBL_ADDR_OFFSET (0)
#endif

#else

#ifndef LATURI_ASM_ONLY
#define JUMPTBL_ITEM_SIZE (12)
#define JUMPTBL_ADDR_OFFSET (2)
#else
#define JUMPTBL_ITEM_SIZE (8)
#define JUMPTBL_ADDR_OFFSET (0)
#endif

#endif

/* system stuff */

typedef struct mheader_t {
uint		magic;
uint		cputype;
uint		cpusubtype;
uint		filetype;
uint		ncmds;
uint		sizeofcmds;
/* easy workaroung for 64bit
  should be uint for 32bit
  should be uint flags, uint reserved from 64bit
*/
ulong		flags;
} mheader;

typedef struct section_t
{
char		sectname[16];
char		segname[16];
ulong		addr;
ulong		size;
uint		offset;
uint		align;
uint		reloff;
uint		nreloc;
uint		flags;
uint		reserved1;
uint		reserved2;
} section;

struct seg_t {
char		segname[16];
ulong		vmaddr;
ulong		vmsize;
ulong		fileoff;
ulong		filesize;
uint		maxprot;
uint		initprot;
uint		nsects;
uint		flags;
section		sects[0];
};

struct symtab_t
{
uint		symoff;
uint		nsyms;
uint		stroff;
uint		strsize;
};

struct dysymtab_t
{
uint		ilocalsym;
uint		nlocalsym;
uint		iextdefsym;
uint		nextdefsym;
uint		iundefsym;
uint		nundefsym;
ulong		tocoff;
uint		ntoc;
ulong		modtaboff;
uint		nmodtab;
ulong		extrefsymoff;
uint		nextrefsyms;
ulong		indirectsymoff;
uint		nindirectsyms;
ulong		extreloff;
uint		nextrel;
ulong		locreloff;
uint		nlocrel;
};

struct dylib_t
{
uint		offset;
uint		timestamp;
uint		current_version;
uint		compatibility_version;
};

typedef struct mcmd_hdr_t {
uint		cmd;
uint		len;
} mcmd_hdr;

typedef struct mcmd_t {
uint		cmd;
uint		len;
union
{
	struct seg_t seg;
	struct symtab_t symtab;
	struct dysymtab_t dysymtab;
	struct dylib_t dylib;
};
} mcmd;

typedef struct nlistitem_t
{
uint		stri;
uchar		type;
uchar		sect;
ushort		desc;
ulong		addr;
} __attribute__((packed)) nlistitem;

/* Who invented this shit? */
typedef struct reloc_t {
union
{
	struct
	{
		uint	r_address;
		uint	r_symbolnum:24,
			r_pcrel:1,
			r_length:2,
			r_extern:1,
			r_type:4;
	};
	struct 
	{
		uint	s_address:24,
			s_type:4,
			s_length:2,
			s_pcrel:1,
			s_scattered:1;
		uint	s_value;
	};
};
} __attribute__((packed)) reloc;

/* end of system stuff */

#define MAX_SECTS (1024)
#define MAX_DEPS (1024)
#define MAX_LIBS (4096)
#define MAX_HASH_SIZE (65536)
#define MAX_BINARY_SIZE (262144*4)
#define MAX_TRANSLATIONS (65536)

typedef struct hashitem_t
{
const char*	str;
uint		hash;
} hashitem;

typedef struct function_t
{
hashitem	name;
const void	*addr;
ulong		uaddr;
uint		type;
uint		sect;
int		suspect;
int		isdyld;
} function;

typedef enum secttypes_t
{
	ST_DONOTWANT=0, /* To be demolished */
	ST_TEXT,
	ST_DATA,
	ST_BSS
} secttypes;

typedef struct linksect_t
{
void*		addr;
ulong		uaddr;
ulong		offset;
ulong 		length;
secttypes	type;
const reloc*	reloc;
uint		nrelocs;
ulong		indirect_off;
/* linker stuff */
uint		sect_num;
ulong		abs_offset;
ulong		vm_shift;	/* shift for decompressor */
int		isheader;
int		isloader;
uint		obj_number;
int		iscompressible;
int		isjumptable;
int		ispointertable;
} linksect;

typedef struct linkdata_t
{
void*		addr;
uint		nsects;
linksect	l[MAX_SECTS];
} linkdata;


typedef struct library_t
{
hashitem	name;
ulong		slide;
uint		nlist_cnt;
int		nlistkey;
const nlistitem* nlist;
const char*	strings;
uint*		indirects;
linkdata*	ldata;
uint		isdyld;
/* deps */
const char*	dlname;
const char*	fullpath;
uint		child_cnt;
const char*	child_names[MAX_DEPS];
struct library_t* childs[MAX_DEPS];
uint		parent_cnt;
struct library_t* parents[MAX_DEPS];
uint		used;
} library;

typedef struct linkmap_t
{
library*	lib;
function*	func;
} linkmap;

typedef struct library_deps_t
{
uint		count;
char		*dlnames[MAX_LIBS_COUNT];
char		*fullpaths[MAX_LIBS_COUNT];
uint		iswanted[MAX_LIBS_COUNT];
} library_deps;

typedef struct symbol_translation_t
{
uint		offset;
void		*address;
uint		index;
} symbol_translation;

#endif

/* Copyright (C) Teemu Suutari */

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/reloc.h>
#include <mach-o/x86_64/reloc.h>
#include <mach-o/nlist.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <ctype.h>
#include <CoreServices/CoreServices.h>

#include "laturi_api.h"
#include "common.h"
#include "hashloader.h"
#include "kompura.h"
#include "stub.h"
#include "dyld_address.h"
#include "onekpaq_integration.h"


#ifndef __x86_64__
#define LC_SEGMENT_BITFIT LC_SEGMENT
#else
#define LC_SEGMENT_BITFIT LC_SEGMENT_64
#endif

#define NUM_LOADER_SECTIONS (3)
#define LDLOAD_SECTION (2)

static void calculate_hash(LATURI_data *lconf,hashitem *hi)
{
	int h;

	const char *name=hi->str;
	h=0;
	int hval=((char)(lconf->hash_value));
#ifndef __x86_64__
/*	if (name[1]=='_')
	{
		hi->hash=0;
		return;
	}*/
	/*
	h*=(2654435769U)&0xffffff;	// == 2^31 * ((sqrt(5)-1), Cormen, Knuth
        */
	while (*name)
	{
		h+=*name;
		h*=hval;
		name++;
	}
#else
	do {
		h*=hval;
		h^=*name;
	} while (*(name++));
#endif
	if (lconf->hash_type==0)
	{
		hi->hash=((uint)h)&0xffff;
	} else if (lconf->hash_type==1) {
		hi->hash=((uint)h)&0x7f7f7f;
	} else {
		hi->hash=h;
	}
}

/* load library data by using index
   do it exactly as the stub would do
*/
static library *load_library(LATURI_data *lconf,uint lindex,library_deps *deps)
{
	void *header;
	ulong slide;
	library *lib=malloc(sizeof(library));
	bzero(lib,sizeof(library));
	if (lindex)
	{
		header=(void*)_dyld_get_image_header(lindex);
		slide=0;
		lib->nlistkey=N_SECT|N_EXT;
	} else {
		ulong dyld_a,dyld_s;
		dyld_address(lconf,&dyld_a,&dyld_s);
		ASSERT(!dyld_a,"Cant find dyld address\n");
		INFO_DEBUG("LL: dyld is at 0x%x:%x\n",dyld_a,dyld_s);
		header=(void*)dyld_a;
		slide=dyld_s;
		lib->nlistkey=N_SECT|N_PEXT;
		lib->isdyld=1;
	}
	if (header)
	{
		INFO_DEBUG("LL: Got header for index %d\n",lindex);
		if (lindex)
		{
			lib->name.str=strdup(_dyld_get_image_name(lindex));
			INFO_DEBUG("LL: Got name '%s' for index %d\n",lib->name.str,lindex);
			lib->slide=_dyld_get_image_vmaddr_slide(lindex);
		} else {
			lib->name.str="dyld";
			lib->slide=slide;
		}
		calculate_hash(lconf,&lib->name);
		/* now dependency stuff */
		lib->fullpath=lib->name.str;
		uint i;
		INFO_DEBUG("LL: Dependencies...\n");
		for (i=0;i<deps->count;i++)
		{
			if (!strcmp(lib->fullpath,deps->fullpaths[i]))
			{
				lib->dlname=deps->dlnames[i];
				INFO_DEBUG("LL: Dependency %s\n",lib->dlname);
				break;
			}
		}
		INFO_DEBUG("LL: Dependencies done\n");

		INFO_DEBUG("LL: header address is 0x%08x\n",header);
		/* now browse thru the header, get nlist & symtab address */
		mheader *mhdr=(void*)header;
		uint ccnt=mhdr->ncmds;
		void *reloc_addr=0;
		ulong reloc2=0;
		ulong nlist_off=0;
		ulong string_off=0;
		mcmd *tmp=(void*)(mhdr+1);
		INFO_DEBUG("LL: %d load commands\n",ccnt);
		for (i=0;i<ccnt;i++)
		{
			/* linksect, we could also do memcmp, but this is better because if
			   this will fail, it will fail the same way as stub will
			*/
			INFO_DEBUG("LL: load command %d (%llx)\n",i,tmp->cmd);
			if (tmp->cmd==LC_SEGMENT_BITFIT && *(uint*)(void*)(tmp->seg.segname+2)==0x4b4e494c)
			{
				reloc_addr=(void*)tmp->seg.vmaddr;
				reloc2=tmp->seg.fileoff;
			}
			else if (tmp->cmd==LC_SYMTAB)
			{
				nlist_off=tmp->symtab.symoff;
				string_off=tmp->symtab.stroff;

				lib->nlist_cnt=tmp->symtab.nsyms;
				INFO_DEBUG("LL: symbols %u\n",lib->nlist_cnt);
			}
#if 0
			else if (tmp->cmd==LC_DYSYMTAB)
			{
				if (lindex)
				{
					/* small optimization for snow leopard */
/* TODO: barf, this still does not work */
					/*if (lconf->running_osx_version!=6)*/ nlist_off+=tmp->dysymtab.iextdefsym*sizeof(nlistitem);
					lib->nlist_cnt=tmp->dysymtab.nextdefsym;
				} else {
					lib->nlist_cnt=tmp->dysymtab.nlocalsym;
				}
			
			}
#endif
			else if (tmp->cmd==LC_LOAD_DYLIB)
			{
				ASSERT(lib->child_cnt==MAX_DEPS-1,"Library '%s' has too many depencies\n",lib->name.str);
				lib->child_names[lib->child_cnt++]=strdup((char*)(void*)tmp+tmp->dylib.offset);
			}
			else if (tmp->cmd==LC_REEXPORT_DYLIB)
			{
				ASSERT(lib->child_cnt==MAX_DEPS-1,"Library '%s' has too many depencies\n",lib->name.str);
				lib->child_names[lib->child_cnt++]=strdup((char*)(void*)tmp+tmp->dylib.offset);
			}
			tmp=((void*)tmp)+tmp->len;
		}
		INFO_DEBUG("LL: load commands done\n");
		lib->nlist=lib->slide+reloc_addr-reloc2+nlist_off;
		lib->strings=lib->slide+reloc_addr-reloc2+string_off;
		return lib;
	}
	if (lib->name.str) free(&lib->name.str);
	free(lib);
	return 0;
}

int LATURI_check_library_bits(const void *addr)
{
	if (!addr) return 0;

	const mheader *hdr=addr;
#ifndef __x86_64__
	if (hdr->magic==MH_MAGIC_64)
	{
		/* sucks */
		return -1;
	}
#else
	if (hdr->magic==MH_MAGIC)
	{
		/* re-execute with right architecture |-( */
		return -1;
	}
#endif
	return 0;
}

static library *load_object(LATURI_data *lconf,const void *addr,uint obj_number)
{
	if (!addr) return 0;

	/* check that the file is what it claims to be */
	const mheader *hdr=addr;

#ifndef __x86_64__
	if (hdr->magic==MH_MAGIC_64)
	{
		return 0;
	}
	if (hdr->magic!=MH_MAGIC || hdr->cputype!=CPU_TYPE_X86 || hdr->filetype!=MH_OBJECT)
#else
	if (hdr->magic==MH_MAGIC)
	{
		return 0;
	}
	if (hdr->magic!=MH_MAGIC_64 || hdr->cputype!=CPU_TYPE_X86_64 || hdr->filetype!=MH_OBJECT)
#endif
	{
		ASSERT(1,"Not proper Mach-O object\n");
	}
	/* this will segfault if the malloc fails, it is error handling enough.. */
	library *lib=malloc(sizeof(library));
	bzero(lib,sizeof(library));
	lib->ldata=malloc(sizeof(linkdata));
	bzero(lib->ldata,sizeof(linkdata));
	lib->ldata->addr=(void*)addr;

	int segfound=0,symtabfound=0,dysymtabfound=0;
	uint i;
	uint ncmd=hdr->ncmds;
	mcmd *cmd=(void*)(hdr+1);
	for (i=0;i<ncmd;i++)
	{
		switch (cmd->cmd)
		{
			case LC_SEGMENT_BITFIT:
			{
				ASSERT(segfound,"Double segment\n");
				segfound=1;
				
				uint j;
				uint nsects=lib->ldata->nsects=cmd->seg.nsects;
				ASSERT(nsects>=(MAX_SECTS-1),"Too many sections\n");
				for (j=0;j<nsects;j++)
				{
					secttypes st=ST_DONOTWANT;
					if (strcmp(cmd->seg.sects[j].sectname,"__eh_frame") &&
						!strcmp(cmd->seg.sects[j].segname,SEG_TEXT))
					{
						st=ST_TEXT;
					} else if (!strcmp(cmd->seg.sects[j].sectname,SECT_BSS) &&
						!strcmp(cmd->seg.sects[j].segname,SEG_DATA))
					{
						st=ST_BSS;
					} else if (/*!strcmp(cmd->seg.sects[j].sectname,SECT_DATA) && */
						!strcmp(cmd->seg.sects[j].segname,SEG_DATA))
					{
						st=ST_DATA;
					} else if (!strcmp(cmd->seg.sects[j].sectname,"__jump_table") &&
						!strcmp(cmd->seg.sects[j].segname,SEG_IMPORT))
					{
						lib->ldata->l[j].isjumptable=1;
					} else if (!strcmp(cmd->seg.sects[j].sectname,"__pointers") &&
						!strcmp(cmd->seg.sects[j].segname,SEG_IMPORT))
					{
						lib->ldata->l[j].ispointertable=1;
					} 
					if (!cmd->seg.sects[j].size) st=ST_DONOTWANT;
					lib->ldata->l[j].type=st;
					lib->ldata->l[j].sect_num=j+1;		/* sections are numbered from 1 (not 0) in relocs */
					lib->ldata->l[j].isheader=obj_number?0:1;
					lib->ldata->l[j].isloader=(obj_number<NUM_LOADER_SECTIONS)?1:0;
					lib->ldata->l[j].obj_number=obj_number;
					lib->ldata->l[j].iscompressible=(obj_number<LDLOAD_SECTION||!lconf->onekpaq_mode)?0:1;
					lib->ldata->l[j].uaddr=cmd->seg.sects[j].addr;
					if (st!=ST_BSS)
					{
						lib->ldata->l[j].addr=(void*)addr+cmd->seg.sects[j].offset;
						lib->ldata->l[j].offset=cmd->seg.sects[j].offset;
						lib->ldata->l[j].reloc=addr+cmd->seg.sects[j].reloff;
						lib->ldata->l[j].nrelocs=cmd->seg.sects[j].nreloc;
						lib->ldata->l[j].indirect_off=cmd->seg.sects[j].reserved1;
					}
					lib->ldata->l[j].length=cmd->seg.sects[j].size;
					/*if (st!=ST_DONOTWANT)
					{
						INFO_LOG("Linking section '%s/%s', type %s and size 0x%08x, %d relocs\n",
							cmd->seg.sects[j].segname,cmd->seg.sects[j].sectname,
							(st==ST_TEXT)?"text":(st=ST_DATA)?"data":"bss",
							lib->ldata->l[j].length,lib->ldata->l[j].nrelocs);
					}*/
				}
			}
			break;

			case LC_SYMTAB:
			{
				ASSERT(symtabfound,"Double symtab\n");
				symtabfound=1;

				lib->nlist_cnt=cmd->symtab.nsyms;
				lib->nlist=addr+cmd->symtab.symoff;
				lib->strings=addr+cmd->symtab.stroff;
			}
			break;

			case LC_DYSYMTAB:
			{
				ASSERT(dysymtabfound,"Double dysymtab\n");
				dysymtabfound=1;
				lib->indirects=(void*)addr+cmd->dysymtab.indirectsymoff;
			}
			break;
			
			default:
			break;
		}
		cmd=(void*)cmd+cmd->len;
	}
	ASSERT(!segfound,"No segment in object\n");
	ASSERT(!symtabfound,"No symtab in object\n");
	lib->nlistkey=0;
	return lib;
}

static const char *demangle_symbol(const char *name,char *buffer)
{
#define MANGLE_LEN (17)
	if (strlen(name)>MANGLE_LEN && !memcmp(name,"__LATURI_Mangle__",MANGLE_LEN))
	{
		char *ret=buffer;
		// demangling
		name+=MANGLE_LEN;
		while (*name)
		{
			if (*name=='$')
			{
				char ch=0;
				if (name[1]>='0' && name[1]<='9')
				{
					ch+=16*(name[1]-'0');
				} else if (name[1]>='A' && name[1]<='F') {
					ch+=16*(name[1]-'A'+10);
				} else if (name[1]>='a' && name[1]<='f') {
					ch+=16*(name[1]-'a'+10);
				} else {
					ASSERT(true,"Invalid mangled symbol\n");
				}
				if (name[2]>='0' && name[2]<='9')
				{
					ch+=name[2]-'0';
				} else if (name[2]>='A' && name[2]<='F') {
					ch+=name[2]-'A'+10;
				} else if (name[2]>='a' && name[2]<='f') {
					ch+=name[2]-'a'+10;
				} else {
					ASSERT(true,"Invalid mangled symbol\n");
				}
				*(buffer++)=ch;
				name+=3;
			} else {
				*(buffer++)=*(name++);
			}
		}
		*buffer=0;
		return ret;
	}
	return name;
}


static function *load_funcs(LATURI_data *lconf,library *lib)
{
	uint i,j;

	ulong size=sizeof(function)*(lib->nlist_cnt+1);		/* no way there is this much real symbols, but I dont care :D
								   +1 because empty last one
								*/
	function *funcs=malloc(size);
	bzero(funcs,size);
	j=0;
	for (i=0;i<lib->nlist_cnt;i++)
	{
		char demangle_buffer[4096];
//		if (lib->nlistkey && lib->nlist[i].type!=lib->nlistkey) continue;
		funcs[j].name.str=strdup(demangle_symbol(lib->nlist[i].stri+lib->strings,demangle_buffer));
		calculate_hash(lconf,&funcs[j].name);
		if (lib->nlist[i].addr) funcs[j].addr=(void*)(lib->nlist[i].addr+lib->slide);
			else funcs[j].addr=0;
		funcs[j].uaddr=lib->nlist[i].addr;
		//void *ds=dlsym(RTLD_DEFAULT,funcs[j].name.str+1);
		funcs[j].type=lib->nlist[i].type;
		funcs[j].sect=lib->nlist[i].sect;
// Variants blow this
//		if (lib->nlist[i].type==(N_SECT|N_EXT)) funcs[j].suspect=!(ds==funcs[j].addr);
//		if (lib->nlist[i].type==(N_SECT|N_EXT)) funcs[j].suspect=!ds;
		funcs[j].suspect=!funcs[j].addr;
		if (lib->isdyld) funcs[j].isdyld=1;
		INFO_DEBUG("LL: function %016lx %s\n",(ulong)funcs[j].addr,funcs[j].name.str);
		j++;
	}
/*	INFO_LOG("Found %d syms on '%s' of which %d are ignored\n",j,lib->name.str,lib->nlist_cnt-j); */
	return funcs;
}

static uint func_count(function *funcs)
{
	uint r=0;
	while (funcs[r].name.str) r++;
	return r;
}

static function *insert_dyld_symbol(LATURI_data *lconf,function *funcs,const char *name)
{
	uint cnt=func_count(funcs);
	ulong size=sizeof(function)*(cnt+2);
	funcs=realloc(funcs,size);
	bzero(funcs+cnt,sizeof(function)*2);
	funcs[cnt].name.str=name;
	calculate_hash(lconf,&(funcs[cnt].name));
	funcs[cnt].type=N_EXT;
	funcs[cnt].isdyld=1;
	return funcs;
}

static int linkmap_sort(const void *_a,const void *_b)
{
	const linkmap *a=_a;
	const linkmap *b=_b;
	if (a->func->isdyld>b->func->isdyld) return -1;
	if (a->func->isdyld<b->func->isdyld) return 1;
	if (a->lib->name.hash<b->lib->name.hash) return -1;
	if (a->lib->name.hash>b->lib->name.hash) return 1;
	return (strcmp(a->func->name.str,b->func->name.str));
}

static linkmap *map_funcs(LATURI_data *lconf,library **libmap,function **funcmap,uint libcount,function **objfuncs,uint num_objfuncs)
{
	uint i,j,k,l;

	linkmap *lmap=0;

	uint real_objfcnt=0;


	for (l=0;l<num_objfuncs;l++)
	{
		uint objfcnt=func_count(objfuncs[l]);

		/* this might be sloooow.... :D */
		for (i=0;i<objfcnt;i++)
		{
			int found=0;
			uint foundlib=0;
			uint foundfunc=0;

			if (objfuncs[l][i].type!=N_EXT) continue;

			/* LATURI internal symbols are thought to be resolved at this point */
			if (strlen(objfuncs[l][i].name.str)>10)
			{
				if (!memcmp(objfuncs[l][i].name.str,"__LATURI__",10)) continue;
			}
			
			/* check whether it is resolved by inter-object lookup */
			int sect_found=0;
			for (j=0;j<num_objfuncs;j++)
			{
				uint objfcount2=func_count(objfuncs[j]);

				for (k=0;k<objfcount2;k++)
				{
					if (!strcmp(objfuncs[l][i].name.str,objfuncs[j][k].name.str) && objfuncs[l][i].isdyld==objfuncs[j][k].isdyld && (objfuncs[j][k].type&~N_PEXT)>N_EXT)
					{
						sect_found=1;
						break;
					}
				}
				if (sect_found) break;
			}
			if (sect_found)
			{
				/*INFO_LOG("Function '%s' solved internally\n",objfuncs[l][i].name.str);*/
				continue;
			}
			
			for (j=0;j<libcount;j++)
			{
				uint libfcount=func_count(funcmap[j]);
				for (k=0;k<libfcount;k++)
				{
					if (!strcmp(objfuncs[l][i].name.str,funcmap[j][k].name.str) &&
						objfuncs[l][i].isdyld==funcmap[j][k].isdyld &&
							!funcmap[j][k].suspect)
					{
						if (!found)
						{
							foundlib=j;
							foundfunc=k;
							found=1;
						} else {
							Dl_info nfo;
							if (dladdr(funcmap[j][k].addr,&nfo))
							{
								if (!strcmp(libmap[j]->name.str,nfo.dli_fname))
								{
									foundlib=j;
									foundfunc=k;
								}
							}
							found++;
						}
					}
				}
			}
			if (found)
			{
				/* check hash collisions */
				uint libfcount=func_count(funcmap[foundlib]);
				uint myhash=funcmap[foundlib][foundfunc].name.hash;
				for (k=0;k<libfcount;k++)
				{
					if (lconf->hash_type==1 || lconf->hash_type==2)
					{
						// full check on non-weak hash
						if (k==foundfunc) continue;
					} else {
						// semi-check on weak hash
						if (k>=foundfunc) break;
					}
					ASSERT(funcmap[foundlib][k].name.hash==myhash,
						"Functions '%s' and '%s' clash in %s\n",
						funcmap[foundlib][foundfunc].name.str,funcmap[foundlib][k].name.str,
						libmap[foundlib]->name.str);
				}
				/* no hash collision, good */
				lmap=realloc(lmap,sizeof(linkmap)*(real_objfcnt+2));
				bzero(lmap+real_objfcnt,sizeof(linkmap)*2);
				lmap[real_objfcnt].lib=libmap[foundlib];
				lmap[real_objfcnt].func=&funcmap[foundlib][foundfunc];
				real_objfcnt++;
			} else {
				ASSERT(1,"Could not resolve %s\n",objfuncs[l][i].name.str);
			}
		}
	}
	/* now sort */
	qsort(lmap,real_objfcnt,sizeof(linkmap),linkmap_sort);
	/* Final check: Check that bound library hashes do not overlap...
	   (Of course we can do it in very slow way)
	*/
	for (i=0;i<real_objfcnt;i++) for (j=0;j<libcount;j++)
	{
		if (lmap[i].lib==libmap[j]) continue;
		ASSERT(lmap[i].lib->name.hash==libmap[j]->name.hash,
			"Libraries %s and %s clash\n",lmap[i].lib->name.str,libmap[j]->name.str);
	}
	return lmap;
}

static uint linkmap_size(linkmap *lmap)
{
	uint i=0;
	while (lmap[i].lib) i++;
	return i;
}

static uint find_named_parents_r(library *child,library **parents,uint parent_index)
{
	uint i;
	if (child->dlname)
	{
		int found=0;

		for (i=0;i<parent_index;i++)
		{
			if (parents[i]==child)
			{
				found=1;
				break;
			}
		}
		if (!found)
		{
			ASSERT(parent_index==MAX_DEPS,"Deps table full\n");
			parents[parent_index++]=child;
		}
	}
	child->used=1;
	for (i=0;i<child->parent_cnt;i++)
	{
		if (!(child->parents[i]->used))
		{
			parent_index=find_named_parents_r(child->parents[i],parents,parent_index);
		}
	}
	return parent_index;
}

static uint find_named_parents(library *child,library **parents,library **libs,uint libcount)
{
	uint i;

	for (i=0;i<libcount;i++)
	{
		libs[i]->used=0;
	}
	return find_named_parents_r(child,parents,0);
}

static int map_libs(linkmap *lmap,library **libs,uint libcount)
{
	uint i;
	/* first resolve the names of childs to real libraries.
	   ugly code again
	*/
	for (i=0;i<libcount;i++)
	{
		if (libs[i]->isdyld) continue;

		uint j;
		for (j=0;j<libcount;j++)
		{
			if (libs[j]->isdyld) continue;

			uint k;
			for (k=0;k<libs[j]->child_cnt;k++)
			{
				if (!strcmp(libs[i]->fullpath,libs[j]->child_names[k]))
				{
					ASSERT(libs[j]->childs[k],"Deps mismatch on %s\n",libs[j]->name.str);
					libs[j]->childs[k]=libs[i];
				}
			}
		}
	}
	/* then translate them to parents map */
	for (i=0;i<libcount;i++)
	{
		if (libs[i]->isdyld) continue;

		uint j;
		for (j=0;j<libs[i]->child_cnt;j++)
		{
			library *child=libs[i]->childs[j];
			if (!child) continue; /*  seems to happen with libgcc_s, strange */
			
			ASSERT(child->parent_cnt==MAX_DEPS,"Deps list full\n");
			child->parents[(child->parent_cnt)++]=libs[i];
		}
	}

	/* found out all possibilities. */
	library *old_lib=0;
	uint cnt=linkmap_size(lmap);
	uint dep_cnt=0;
	library **depmap[MAX_DEPS];
	uint cnt_map[MAX_DEPS];
	for (i=0;i<cnt;i++)
	{
		if (lmap[i].lib==old_lib || lmap[i].lib->isdyld) continue;
		old_lib=lmap[i].lib;
		ASSERT(dep_cnt==MAX_DEPS,"Deps resolver list full\n");
		library *parents[MAX_DEPS];
		cnt_map[dep_cnt]=find_named_parents(lmap[i].lib,parents,libs,libcount);
		if (!cnt_map[dep_cnt])
		{
			INFO_ERR("Could not resolve '%s' properly (missing library from link flags?)\n",lmap[i].lib->name.str);
			return -1;
		}
		depmap[dep_cnt]=malloc(cnt_map[dep_cnt]*sizeof(library*));
		memcpy(depmap[dep_cnt],parents,cnt_map[dep_cnt]*sizeof(library*));

		dep_cnt++;
	}
	ASSERT(!dep_cnt,"Object does not have externals!\n");

	/* clear out previous markers */
	for (i=0;i<libcount;i++)
	{
		libs[i]->used=0;
	}

	/* reduce the set by brute force */
	uint combs[MAX_DEPS];
	uint best_combs[MAX_DEPS];
	uint best_len=~0U;
	for (i=0;i<dep_cnt;i++)
	{
		best_combs[i]=combs[i]=0;
	}
	/* Big list here -> big problem here. */
	for(;;)
	{
		uint j;
		uint len=0;

		/* calculate size of this set */
		for (j=0;j<dep_cnt;j++)
		{
			uint k;
			int found=0;


			for (k=0;k<j;k++)
			{
				if (depmap[j][combs[j]]==depmap[k][combs[k]])
				{
					found=1;
					break;
				}
			}
			/* not added before */
			if (!found) len+=(uint)(strlen(depmap[j][combs[j]]->dlname)+1);
		}

		if (len<best_len)
		{
			bcopy(combs,best_combs,dep_cnt*sizeof(uint));
			best_len=len;
		}

		int done=0;
	
		for (j=0;j<dep_cnt;j++)
		{
			if (++(combs[j])==cnt_map[j])
			{
				combs[j]=0;
				if (j==(dep_cnt-1))
				{
					done=1;
				}
			} else {
				break;
			}
		}
		if (done) break;
	}

	/* mark our favorite line */
	for (i=0;i<dep_cnt;i++)
	{
		depmap[i][best_combs[i]]->used=1;
	}

	return 0;
}

static char *write_hash(LATURI_data *lconf,char *dest,uint hash,uint *len)
{
	dest[0]=(hash&0xff);
	dest[1]=((hash>>8)&0xff);
	if (lconf->hash_type==2)
	{
		dest[2]=((hash>>16)&0xff);
		dest[3]=((hash>>24)&0xff);
		*len+=4;
		return dest+4;
	} else if (lconf->hash_type==1) {
		dest[2]=((hash>>16)&0xff);
		*len+=3;
		return dest+3;
	} else {
		*len+=2;
		return dest+2;
	}
}

static uint create_map(LATURI_data *lconf,char *dest,linkmap *lmap)
{
	uint len=0;

	uint i;
	library *old_lib=0;
	i=0;
	int not_first=0;
	while(lmap[i].lib)
	{
		if (old_lib!=lmap[i].lib)
		{
			if (!(lmap[i].lib->isdyld)) INFO_LOG("\nMappings for %s\n",lmap[i].lib->name.str);
			old_lib=lmap[i].lib;
			/* Count the funcs using this lib */
			uint j=1;
			while (lmap[i+j].lib && lmap[i+j].lib==old_lib) j++;
			ASSERT(j>255,"Internal format failure (more than 255 functions per library) HARDLIMIT!\n");
			if (old_lib!=lmap[0].lib) /* no hash & count for dyld */
			{
				*(dest++)=j;
				len++;
				dest=write_hash(lconf,dest,lmap[i].lib->name.hash,&len);
			}
			not_first=0;
		} else not_first=1;
		if (!(lmap[i].lib->isdyld)) INFO_LOG("%s %s",(not_first)?",":" ",lmap[i].func->name.str);
		ASSERT(!(lmap[i].lib->isdyld)&&(lmap[i].lib->slide),"\nVM-sliding not supported in lib loading :(\n");
		dest=write_hash(lconf,dest,lmap[i].func->name.hash,&len);
		i++;
	}
	INFO_LOG("\n");
	*(dest++)=0;
	len++;
	INFO_LOG("Total length for hashes is %d\n",len);
	return len;
}

static int sect_sort(const void *_a,const void *_b)
{
	const linksect *a=_a;
	const linksect *b=_b;
	if (a->type==ST_DONOTWANT && b->type==ST_DONOTWANT) return 0;
	if (b->type==ST_DONOTWANT) return -1;
	if (a->type==ST_DONOTWANT) return 1;

	/* header first */
	if (a->isheader>b->isheader) return -1;
	if (a->isheader<b->isheader) return 1;

	/* non-compressible stuff first */
	if (a->iscompressible<b->iscompressible) return -1;
	if (a->iscompressible>b->iscompressible) return 1;

	if (a->type==ST_TEXT && b->type==ST_TEXT)
	{
		if (a->isloader>b->isloader) return -1;
		if (a->isloader<b->isloader) return 1;
	}
	if (a->type==ST_DATA && b->type==ST_DATA)
	{
#if 1
		if (a->isloader>b->isloader) return -1;
		if (a->isloader<b->isloader) return 1;
#endif
	}
	if (a->type==ST_BSS && b->type==ST_BSS)
	{
		if (a->isloader>b->isloader) return -1;
		if (a->isloader<b->isloader) return 1;
	}
#if 1
	if (a->type<b->type) return -1;
	if (a->type>b->type) return 1;
#else
	if (a->type==ST_DATA && a->isloader && !b->isloader) return 1;
	if (b->type==ST_DATA && b->isloader && !a->isloader) return -1;
	static const uint compart[]={0,2,1,3};
	if (compart[a->type]<compart[b->type]) return -1;
	if (compart[a->type]>compart[b->type]) return 1;
#endif
	if (a->obj_number<b->obj_number) return -1;
	if (a->obj_number>b->obj_number) return 1;
	if (a->sect_num<b->sect_num) return -1;
	if (a->sect_num>b->sect_num) return 1;
	return 0;
}

static const char *is_special_laturi_symbol(const char *name,int *is_rel,int *is_idx,int *is_end_rel)
{
	*is_rel=0;
	*is_idx=0;
	*is_end_rel=0;
	const char *laturi_rel_string="__LATURI__relof__";
	const char *laturi_rel_ebx_string="__LATURI__ebx_relof__";
	const char *laturi_idx_string="__LATURI__index__";

	if (strlen(name)>17)
	{
		if (!memcmp(name,laturi_rel_string,17))
		{
			*is_rel=1;
			name+=17;
		}
		if (!memcmp(name,laturi_rel_ebx_string,21))
		{
			*is_end_rel=1;
			name+=21;
		}
		if (!memcmp(name,laturi_idx_string,17))
		{
			*is_idx=1;
			name+=17;
		}
	}
	return name;
}

static long table_lookup(library *this_lib,function **object_funcs,uint objects_count,linkmap *lmap,uint lmap_cnt,linksect *raw_sects,uint sect_count,uint lindex,ulong tbl_offset,ulong jtbl_off)
{
	if (raw_sects[lindex].isjumptable || raw_sects[lindex].ispointertable)
	{
		/* Find the function that is referenced in the jumptable/pointertable.
		   replace call to jumptable/pointertable by direct call/reference to extern func.

		   this will be hackish
		*/
		uint multiplier;
		if (raw_sects[lindex].isjumptable) multiplier=5;
			else multiplier=4;

		uint tbl_index=this_lib->indirects[tbl_offset/multiplier+raw_sects[lindex].indirect_off];

		/* now search the symbol like standard extern. */
		const char *fname=this_lib->nlist[tbl_index].stri+this_lib->strings;

		uint i;
		for (i=0;i<objects_count;i++)
		{
			uint objfcnt=func_count(object_funcs[i]);
			uint j;
			for (j=0;j<objfcnt;j++)
			{
				if ((object_funcs[i][j].type&~N_PEXT)>N_EXT && !strcmp(fname,object_funcs[i][j].name.str))
				{
					ulong sect_start=0;
					uint k;

					/* we need to first de-mangle the current process layout... */
					for (k=0;k<sect_count;k++)
					{
						if (object_funcs[i][j].sect==raw_sects[k].sect_num && i+NUM_LOADER_SECTIONS==raw_sects[k].obj_number && !raw_sects[k].isloader)
						{
							ASSERT(raw_sects[k].type==ST_DONOTWANT,"Symbol in killed section for extern\n");
							sect_start=raw_sects[k].abs_offset;
							return object_funcs[i][j].uaddr+sect_start-raw_sects[k].uaddr;
						}
					}
					ASSERT(!sect_start,"No proper section found for extern '%s'\n",fname);
				}
			}
		}

		for (i=0;i<lmap_cnt;i++)
		{
			if (!strcmp(fname,lmap[i].func->name.str))
			{
				ulong offset=jtbl_off+i*JUMPTBL_ITEM_SIZE;
				if (raw_sects[lindex].ispointertable) offset++;
				return offset;
			}
		}
	}
	return 0;
}

static void insert_libraries(linksect *l,library **libs,uint libcount)
{
	if (!libcount) return;

#define MAX_STRINGS (65536)
	uint dylib_size=sizeof(mcmd_hdr)+sizeof(struct dylib_t);
	void *addr=malloc(l->length+dylib_size*libcount+MAX_STRINGS);
	memcpy(addr,l->addr,l->length);
	memset(addr+l->length,0,dylib_size*libcount+MAX_STRINGS);

	
	uint i;
	uint real_libs=0;
	for (i=0;i<libcount;i++)
	{
		if (!(libs[i]->used)) continue;
		real_libs++;
	}
	
//	uint str_len=0;
//	void *str_addr=addr+l->length+dylib_size*real_libs;
	uint real_size=0;//dylib_size*real_libs;
//	if (1)
//	{
//		str_addr+=sizeof(mcmd_hdr);
//	}

	mcmd *cmd=(mcmd*)(addr+l->length);
	for (i=0;i<libcount;i++)
	{
		if (!(libs[i]->used)) continue;
		ulong libnamelen=strlen(libs[i]->dlname)+1;
//		memcpy(str_addr+str_len,libs[i]->dlname,libnamelen);
//		libnamelen=(libnamelen+1)&~1U;

		cmd->cmd=LC_LOAD_DYLIB;
		real_size+=(cmd->len=(uint)(sizeof(mcmd_hdr)+sizeof(struct dylib_t))+libnamelen);
		cmd->dylib.offset=sizeof(mcmd_hdr)+sizeof(struct dylib_t);
		memcpy((void*)cmd+sizeof(mcmd_hdr)+sizeof(struct dylib_t),libs[i]->dlname,libnamelen);

		cmd=(mcmd*)((void*)cmd+cmd->len);
//		str_len+=libnamelen;
	}
	/* Add minversion for vsync (is garbage) */
//	uint fake_size=0;
//	if (1) {
//		cmd->cmd=LC_VERSION_MIN_MACOSX;
//		cmd->len=sizeof(mcmd_hdr)+2*sizeof(uint);
//		real_size+=sizeof(mcmd_hdr);
//		fake_size=2*sizeof(uint);
//		real_libs++;
//	}
	((mheader*)addr)->ncmds+=real_libs;
	((mheader*)addr)->sizeofcmds+=real_size;//+fake_size;
	l->addr=addr;
	l->length+=real_size;
}

static uint link_binary(LATURI_data *lconf,void *dest,uint *linker_blocks,library **objects,function **object_funcs,uint objects_count,linkmap *lmap,void *hashmap,uint hashlen,library **libs,uint libcount,ulong **length_ptr)
{
	library *h_object=load_object(lconf,header_obj,0);
	ASSERT(!h_object,"Could not find a proper header\n");
	library *d_object=load_object(lconf,decompressor_objs[lconf->running_osx_version-11][lconf->onekpaq_mode],1);
	ASSERT(!d_object,"Could not find a proper decompressor\n");
	library *l_object=load_object(lconf,loader_objs[lconf->running_osx_version-11][((lconf->translate_type)>>1)&1][lconf->hash_type],LDLOAD_SECTION);
	ASSERT(!l_object,"Could not find a proper loader\n");
	function *h_object_funcs=load_funcs(lconf,h_object);
	ASSERT(!h_object_funcs,"Could not read symbols from header\n");
	function *d_object_funcs=load_funcs(lconf,d_object);
	ASSERT(!d_object_funcs,"Could not read symbols from decompressor\n");
	function *l_object_funcs=load_funcs(lconf,l_object);
	ASSERT(!l_object_funcs,"Could not read symbols from loader\n");

	uint lmap_cnt=0;
	while(lmap[++lmap_cnt].lib);

	/* insert dylib commands*/
	insert_libraries(h_object->ldata->l,libs,libcount);

	/* insert hash data as section into loader */
	linksect *l=l_object->ldata->l+l_object->ldata->nsects;
	l->addr=hashmap;
	l->length=hashlen;
	l->type=ST_DATA;
	l->nrelocs=0;
	l->indirect_off=0;
	l->obj_number=LDLOAD_SECTION;
	l->sect_num=MAX_SECTS*2;
	l->isloader=1;
	l->iscompressible=0;
	l++;
	l->addr=0;
	l->length=JUMPTBL_ITEM_SIZE*lmap_cnt+JUMPTBL_ADDR_OFFSET; /* offset bytes are being overwritten in the end */
	l->type=ST_BSS;
	l->nrelocs=0;
	l->indirect_off=0;
	l->obj_number=LDLOAD_SECTION;
	l->sect_num=MAX_SECTS*2+1;
	l->isloader=1;
	l->iscompressible=(lconf->onekpaq_mode)?1:0;
	l_object->ldata->nsects+=2;

	/* translation */
	symbol_translation translations[MAX_TRANSLATIONS];
#ifndef __x86_64__
	uint translate_count=0;
#endif

	/* first sort out sections: sort header text first, then others */
	linksect raw_sects[MAX_SECTS+NUM_LOADER_SECTIONS+1];

	uint sect_count=h_object->ldata->nsects;
	bcopy(h_object->ldata->l,raw_sects,h_object->ldata->nsects*sizeof(linksect));

	bcopy(d_object->ldata->l,raw_sects+sect_count,d_object->ldata->nsects*sizeof(linksect));
	sect_count+=d_object->ldata->nsects;

	bcopy(l_object->ldata->l,raw_sects+sect_count,l_object->ldata->nsects*sizeof(linksect));
	sect_count+=l_object->ldata->nsects;
	
	uint i;
	for (i=0;i<objects_count;i++)
	{
		bcopy(objects[i]->ldata->l,raw_sects+sect_count,objects[i]->ldata->nsects*sizeof(linksect));
		sect_count+=objects[i]->ldata->nsects;
	}
	qsort(raw_sects,sect_count,sizeof(linksect),sect_sort);

	ulong c_off=0;
	ulong jtbl_off=0;
	ulong jtbl_len=0;
	ulong f_len=0;
	ulong b_len=0;
	INFO_LOG("Final mapping\n");
	ulong loader_addr=0;
	ulong decompr_addr=0;
	ulong vm_shift=0;
	for (i=0;i<sect_count;i++)
	{
		if (raw_sects[i].type==ST_DONOTWANT) break;
		/* align BSS really sky high */
		ulong nudge=(raw_sects[i].type!=ST_BSS)?0:(65536-c_off)&65535;
		if (nudge)
		{
			if (i && !raw_sects[i-1].isloader) nudge=0;
		}
#if 0
		if (i && raw_sects[i-1].isheader && raw_sects[i-1].length<256)
		{
			nudge=256-raw_sects[i-1].length;
		}
#endif
		if (raw_sects[i].type==ST_BSS && raw_sects[i].isloader)
		{
			nudge=1;
			while (nudge<c_off) nudge<<=1;
			nudge=nudge-c_off;
		}
		raw_sects[i].abs_offset=c_off-vm_shift+nudge;

		if (lconf->onekpaq_mode)
		{
			if (!vm_shift && raw_sects[i].iscompressible)
			{
				ASSERT(raw_sects[i].abs_offset>lconf->decompress_offset,"Too small area for compressed data, increase it\n");
				vm_shift=lconf->decompress_offset-raw_sects[i].abs_offset;
				c_off+=vm_shift;
			}
			raw_sects[i].vm_shift=vm_shift;
		}

		b_len+=raw_sects[i].length+nudge;
		if (raw_sects[i].type==ST_TEXT || raw_sects[i].type==ST_DATA)
		{
			f_len+=raw_sects[i].length+nudge;
			memcpy(dest+raw_sects[i].abs_offset,raw_sects[i].addr,raw_sects[i].length);
			linker_blocks[i]=(uint)(raw_sects[i].length);
			if (i) linker_blocks[i-1]+=nudge;
		}
#if 0
		if (raw_sects[i].type==ST_DATA && raw_sects[i].isloader)
		{
			if (lconf->onekpaq_mode)
			{
				/* magic nonsave hash byte */
				c_off++;
			}
		}
#endif
		if (raw_sects[i].type==ST_BSS && raw_sects[i].isloader)
		{
			jtbl_off=raw_sects[i].abs_offset+raw_sects[i].vm_shift;
			jtbl_len=raw_sects[i].length;
		}
		c_off+=raw_sects[i].length+nudge;

		if (raw_sects[i].isheader)
		{
			INFO_LOG("HEADER  : 0x%08lx - 0x%08lx\n",
				raw_sects[i].abs_offset,raw_sects[i].abs_offset+raw_sects[i].length);
		} else {
			INFO_LOG("%s(%c)%c: 0x%08lx - 0x%08lx\n",
				((char*[]){"","TEXT","DATA","BSS "})[raw_sects[i].type],raw_sects[i].isloader?'l':'o',
				lconf->onekpaq_mode?(raw_sects[i].iscompressible?'+':'-'):' ',
				raw_sects[i].abs_offset+raw_sects[i].vm_shift,raw_sects[i].abs_offset+raw_sects[i].vm_shift+raw_sects[i].length);
		}
		// TODO: fix this mess
		if (raw_sects[i].type==ST_TEXT && !raw_sects[i].isheader && raw_sects[i].isloader && decompr_addr && !loader_addr) loader_addr=raw_sects[i].abs_offset;
		if (raw_sects[i].type==ST_TEXT && !raw_sects[i].isheader && raw_sects[i].isloader && !decompr_addr) decompr_addr=raw_sects[i].abs_offset;
		//INFO_LOG("decompr %p loader %p\n",decompr_addr,loader_addr);
	}

	/* data is there, now relocate */

	/* insert filesize to the header, address is in CodeSize-symbol */
	int cs_found=0;
	uint h_objfcnt=func_count(h_object_funcs);
	ulong *filesize_address=0;
	for (i=0;i<h_objfcnt;i++)
	{
		if (h_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("CodeSize",h_object_funcs[i].name.str))
		{
			filesize_address=(ulong*)(dest+h_object_funcs[i].uaddr);
			cs_found=1;
			break;
		}
	}
	ASSERT(!cs_found,"No CodeSize in loader\n");

	if (lconf->heap_size<65536) lconf->heap_size=65536;
	ulong final_hs=b_len+vm_shift+lconf->heap_size;
	for (i=65536;i>0;i<<=1)
	{
		if (final_hs<=i)
		{
			final_hs=i;
			break;
		}
	}
	INFO_LOG("HEAP    : 0x%08lx - 0x%08lx\n",b_len+vm_shift,final_hs);

	INFO_LOG("Uncompressed file length is %ld\n",f_len);

	int hs_found=0;
	for (i=0;i<h_objfcnt;i++)
	{
		if (h_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("HeapSize",h_object_funcs[i].name.str))
		{
			*((ulong*)(dest+h_object_funcs[i].uaddr))=final_hs;
			hs_found=1;
			break;
		}
	}
	ASSERT(!hs_found,"No HeapSize in loader\n");
#if 0
	hs_found=0;
	for (i=0;i<h_objfcnt;i++)
	{
		if (h_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("ESPRegister",h_object_funcs[i].name.str))
		{
			*((ulong*)(dest+h_object_funcs[i].uaddr))=final_hs;
			hs_found=1;
			break;
		}
	}
	ASSERT(!hs_found,"No ESPRegister in loader\n");
#endif
	/* debug */
/*	uint *gdb_address=0;
	for (i=0;i<h_objfcnt;i++)
	{
		if (h_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("GDBCompat",h_object_funcs[i].name.str))
		{
			gdb_address=dest+h_object_funcs[i].uaddr;
			break;
		}
	}
	ASSERT(!gdb_address,"No GDBCompat in loader\n");
*/
	/* hash value */
	uint l_objfcnt=func_count(l_object_funcs);
	uchar *hash_values[4]={0,0,0,0};
	{
		uint j=0;
		for (i=0;i<l_objfcnt;i++)
		{
			if (l_object_funcs[i].type==(N_SECT|N_EXT) && !memcmp("HashValue",l_object_funcs[i].name.str,9))
			{
				ASSERT(j==4,"Too many hash values in loader");
				hash_values[j++]=dest+loader_addr+l_object_funcs[i].uaddr;
				//INFO_LOG("hash loc %x\n",(uint)hash_values[j-1]);
			}
		}
	}
	/* Translations */
	uint *ctc_address=0;
	for (i=0;i<l_objfcnt;i++)
	{
		if (l_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("CallTranslateCount",l_object_funcs[i].name.str))
		{
			ctc_address=dest+loader_addr+l_object_funcs[i].uaddr;
			break;
		}
	}

	/* Codestart, loader objects are not relocatable, this is manual work ;( */
	ulong codestart_address=0;
	uint d_objfcnt=func_count(d_object_funcs);
	for (i=0;i<d_objfcnt;i++)
	{
		if (d_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("__ONEKPAQ__CodeStart",d_object_funcs[i].name.str))
		{
			codestart_address=(ulong)(decompr_addr+d_object_funcs[i].uaddr);
			break;
		}
	}
	ASSERT(!codestart_address,"No CodeStart in decompressor\n");

	ulong shift_address=0;
	for (i=0;i<d_objfcnt;i++)
	{
		if (d_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("__ONEKPAQ_shift",d_object_funcs[i].name.str))
		{
			shift_address=(ulong)(decompr_addr+d_object_funcs[i].uaddr);
			break;
		}
	}
//	ASSERT(!shift_address,"No Shift in decompressor\n");

	int found_codestart=0;
	for (i=0;i<h_objfcnt;i++)
	{
		if (h_object_funcs[i].type==(N_SECT|N_EXT) && !strcmp("__LATURI__CodeStart",h_object_funcs[i].name.str))
		{
			*(ulong*)(dest+h_object_funcs[i].uaddr)=codestart_address;
			found_codestart=1;
			break;
		}
	}
	ASSERT(!found_codestart,"No CodeStart in header\n");

	ulong *onekpaq_source_address=0;

	/* this will be ugly */
	for (i=0;i<sect_count;i++)
	{
		if (raw_sects[i].type==ST_DONOTWANT) continue;
		
		library *ffl;
		function *ff;
		if (raw_sects[i].isloader)
		{
			// TODO: raw index again
			if (i==0) {
				ffl=h_object;
				ff=h_object_funcs;
			} else if (i==1) {
				ffl=d_object;
				ff=d_object_funcs;
			} else {
				ffl=l_object;
				ff=l_object_funcs;
			}
		} else {
			uint num=raw_sects[i].obj_number-NUM_LOADER_SECTIONS;
			ffl=objects[num];
			ff=object_funcs[num];
		}
		uint j;
		for (j=0;j<raw_sects[i].nrelocs;j++)
		{
			/* demangle the symbol idiotism */
			uint rx_scattered;
			uint rx_pcrel;
			uint rx_type;
			uint rx_length;
			uint rx_extern;
			uint rx_address;
			uint rx_value;
			uint rx_symbolnum;
			uint rx_pairval;

			if (raw_sects[i].reloc[j].s_scattered)
			{
				rx_scattered=1;
				rx_extern=0;
				rx_symbolnum=0;
				rx_pcrel=raw_sects[i].reloc[j].s_pcrel;
				rx_length=raw_sects[i].reloc[j].s_length;
				rx_type=raw_sects[i].reloc[j].s_type;
				rx_address=raw_sects[i].reloc[j].s_address;
				rx_value=raw_sects[i].reloc[j].s_value;
				rx_pairval=0;
			} else {
				rx_scattered=0;
				rx_address=raw_sects[i].reloc[j].r_address;
				rx_symbolnum=raw_sects[i].reloc[j].r_symbolnum;
				rx_pcrel=raw_sects[i].reloc[j].r_pcrel;
				rx_length=raw_sects[i].reloc[j].r_length;
				rx_extern=raw_sects[i].reloc[j].r_extern;
				rx_type=raw_sects[i].reloc[j].r_type;
				rx_value=0;
				rx_pairval=0;
			}

			ulong target_offset=0;
			ulong ta_pc_addr=raw_sects[i].abs_offset+raw_sects[i].vm_shift+rx_address;
			long *target_address_q=(long*)(dest+raw_sects[i].abs_offset+rx_address);
			int *target_address_d=(int*)(dest+raw_sects[i].abs_offset+rx_address);
			short *target_address_w=(short*)(dest+raw_sects[i].abs_offset+rx_address);
			char *target_address_b=(char*)(dest+raw_sects[i].abs_offset+rx_address);
			long int ta_contents=0;
#ifndef __x86_64__
			/* Sanity checking.... */
			if (rx_type==GENERIC_RELOC_PAIR) continue; /* whatever, handled elsewhere */
			ASSERT((rx_type!=GENERIC_RELOC_VANILLA && rx_type!=GENERIC_RELOC_LOCAL_SECTDIFF && rx_type!=GENERIC_RELOC_SECTDIFF) || rx_length>2,
				"Me no like this kind of reloc. Me too simple. File a bug\n");
			/* now decode paired symbol scheisse */
			if (rx_type==GENERIC_RELOC_LOCAL_SECTDIFF || rx_type==GENERIC_RELOC_SECTDIFF)
			{
				ASSERT(j+1==raw_sects[i].nrelocs || !raw_sects[i].reloc[j+1].s_scattered || raw_sects[i].reloc[j+1].s_type!=GENERIC_RELOC_PAIR,
					"Broken reloc\n");
				rx_pairval=raw_sects[i].reloc[j+1].s_value;
			}
#else
			ASSERT(/*rx_type<X86_64_RELOC_UNSIGNED || */rx_type>X86_64_RELOC_SIGNED_4 || rx_length>3,
				"Broken reloc\n");

			switch (rx_type)
			{
				case X86_64_RELOC_UNSIGNED:
				case X86_64_RELOC_SIGNED:
				case X86_64_RELOC_BRANCH:
				break;

				case X86_64_RELOC_SIGNED_1:
				ta_pc_addr++;
				break;
			
				case X86_64_RELOC_SIGNED_2:
				ta_pc_addr+=2;
				break;
			
				case X86_64_RELOC_SIGNED_4:
				ta_pc_addr+=4;
				break;
				
				case X86_64_RELOC_GOT_LOAD:
				case X86_64_RELOC_GOT:
				case X86_64_RELOC_SUBTRACTOR:
				default:
				ASSERT(1,"Unknown relocation type! File a bug (with .o)\n");
				return 0;
				break;
			}
#endif
			switch (rx_length)
			{
				case 0:
				ta_contents=*target_address_b;
				ta_pc_addr++;
				break;

				case 1:
				ta_contents=*target_address_w;
				ta_pc_addr+=2;
				break;

				case 2:
				ta_contents=*target_address_d;
				ta_pc_addr+=4;
				break;

				case 3:
				ta_contents=*target_address_q;
				ta_pc_addr+=8;
				break;
				
				default:
				ta_contents=0;
				break;
			}
			
			if (rx_extern)
			{
				/* find the symbol name, in following order.
				   1. check if it is DynloaderDestAddrs or HashStart 
				   1b. or other relevant address
				   2. lmap and indirect reference
				   3. object funcs
				   nobody refers loader. ]:D
				*/
				const char *fname=ff[rx_symbolnum].name.str;
				int found=0;

				/* 1 */
				if (!strcmp(fname,"__LATURI__DynloaderDestAddrs"))
				{
					found=1;
					target_offset=jtbl_off;
				}
				else if (!strcmp(fname,"__LATURI__HashStart"))
				{
					found=1;
					uint k;
					for (k=0;;k++)
					{
						if (raw_sects[k].isloader && raw_sects[k].obj_number==LDLOAD_SECTION && raw_sects[k].type==ST_DATA)
						{
							target_offset=raw_sects[k].abs_offset+raw_sects[k].vm_shift;
							break;
						}
					}
				}
				else if (!strcmp(fname,"__LATURI__JumpTable"))
				{
					found=1;
					target_offset=jtbl_off+JUMPTBL_ADDR_OFFSET;
				}
				else if (!strcmp(fname,"__ONEKPAQ_source"))
				{
					found=1;
#ifndef __x86_64__
					onekpaq_source_address=(ulong*)target_address_d;
#else
					onekpaq_source_address=(ulong*)target_address_q;
#endif
				}
				else if (!strcmp(fname,"__ONEKPAQ_destination"))
				{
					found=1;
					target_offset=lconf->decompress_offset;
				}
				/* 2 */
				else {
					int is_rel,is_idx,is_end_rel;
					char demangle_buffer[4096];
					fname=demangle_symbol(is_special_laturi_symbol(fname,&is_rel,&is_idx,&is_end_rel),demangle_buffer);

					uint k;
					for (k=0;k<lmap_cnt;k++)
					{
						if (!strcmp(fname,lmap[k].func->name.str))
						{
							found=1;
							if (is_rel)
							{
								target_offset=k*JUMPTBL_ITEM_SIZE;
							} else if (is_end_rel) {
								target_offset=(k-lmap_cnt)*JUMPTBL_ITEM_SIZE;
							} else if (is_idx) {
								target_offset=k;
							} else {
#ifndef __x86_64__
								if ((lconf->translate_type&1) && !raw_sects[i].isloader && rx_length==2 && ((uchar*)target_address_b)[-1]==0xe8U && k<=255 && jtbl_off+k*JUMPTBL_ITEM_SIZE+JUMPTBL_ADDR_OFFSET<0x10000U)
								{
									ASSERT(translate_count==MAX_TRANSLATIONS,"Max limit of call translations reached\n");
									/*INFO_LOG("Translatable call '%s' found at 0x%08lx\n",fname,raw_sects[i].abs_offset+rx_address-1);*/
									translations[translate_count].offset=raw_sects[i].abs_offset+rx_address;
									translations[translate_count].address=((void*)target_address_d)-1;
									translations[translate_count].index=jtbl_off+k*JUMPTBL_ITEM_SIZE+JUMPTBL_ADDR_OFFSET;
									translate_count++;
								}
#endif
								target_offset=jtbl_off+k*JUMPTBL_ITEM_SIZE;
							}
							break;
						}
					}
				}
				
				/* 3 */
				if (!found)
				{
					uint n;

					for (n=0;n<objects_count;n++)
					{
						uint objfcnt=func_count(object_funcs[n]);
						uint k;
						for (k=0;k<objfcnt;k++)
						{
							if ((object_funcs[n][k].type&~N_PEXT)>N_EXT && !strcmp(fname,object_funcs[n][k].name.str))
							{
								if ((object_funcs[n][k].type&N_TYPE)==N_ABS) {
									target_offset=object_funcs[n][k].uaddr;
								} else {
									ulong sect_start=0;
									uint m;

									/* we need to first de-mangle the current process layout... */
									for (m=0;m<sect_count;m++)
									{
										if (object_funcs[n][k].sect==raw_sects[m].sect_num && n+NUM_LOADER_SECTIONS==raw_sects[m].obj_number && !raw_sects[m].isloader)
										{
											ASSERT(raw_sects[m].type==ST_DONOTWANT,"Symbol in killed section for extern\n");
											sect_start=raw_sects[m].abs_offset+raw_sects[m].vm_shift;
											break;
										}
									}
									ASSERT(!sect_start,"No proper section found for extern '%s'\n",fname);
									target_offset=object_funcs[n][k].uaddr+sect_start-raw_sects[m].uaddr;
								}
								found=1;
								/*INFO_LOG("Symbol '%s' is 0x%08lx,0x%08lx,0x%08lx,0x%08x\n",fname,target_offset,sect_start,raw_sects[m].uaddr,n);*/
								break;
							}
						}
						if (found) break;
					}
				}

				ASSERT(!found,"Did not find symbol '%s' anywhere\n",fname);
			} else {
				uint k;
#ifndef __x86_64__
				if (rx_scattered)
				{
					int found=0;
					/* this is total and utter bullshit:
					   why to give the relocation so inconvinient form?
					  
					   1. Find where section rx_value (is address) belongs (in original address space)
					   2. Solve the difference between real world and imaginary
					   3. Proceed normally

					   1 & 2.
					*/
					for (k=0;k<sect_count;k++)
					{
						if (raw_sects[k].uaddr<=rx_value && rx_value<raw_sects[k].uaddr+raw_sects[k].length && raw_sects[i].obj_number==raw_sects[k].obj_number && raw_sects[i].isloader==raw_sects[k].isloader)
						{
							ASSERT(raw_sects[k].type==ST_DONOTWANT && !raw_sects[k].isjumptable && !raw_sects[k].ispointertable,
								"Symbol in killed section for reloc (scatter)\n");
							if (raw_sects[k].isjumptable || raw_sects[k].ispointertable)
							{
								/* TODO?!?: no pcrel here */
								ulong tbl_offset=ta_contents+rx_pairval-raw_sects[k].uaddr;
								target_offset=table_lookup(ffl,object_funcs,objects_count,lmap,lmap_cnt,raw_sects,sect_count,k,tbl_offset,jtbl_off);
								if (target_offset)
								{
									found=1;
									/* one big hack */
									target_offset-=raw_sects[i].abs_offset+raw_sects[i].vm_shift-raw_sects[i].uaddr+rx_pairval+ta_contents;
								}
							} else {
								/*target_offset=raw_sects[k].abs_offset-raw_sects[k].uaddr-(raw_sects[i].abs_offset-raw_sects[i].uaddr);*/
								target_offset=raw_sects[k].abs_offset+raw_sects[k].vm_shift-raw_sects[k].uaddr;
								found=1;
								break;
							}
						}
					}
					ASSERT(!found,"Could not resolve scattered symbol\n");

					/* 2. */
					/* My brain dislocated when I tried to understand this... */
					ASSERT(rx_pcrel,"Symbol type that is not supported encountered! File a bug\n");
				} else
#endif
				{
					/* de-mangle, again... */
					int found=0;
					for (k=0;k<sect_count;k++)
					{
						if (rx_symbolnum==raw_sects[k].sect_num && raw_sects[i].obj_number==raw_sects[k].obj_number && raw_sects[i].isloader==raw_sects[k].isloader)
						{
							ASSERT(raw_sects[k].type==ST_DONOTWANT && !raw_sects[k].isjumptable && !raw_sects[k].ispointertable,
								"Symbol in killed section for reloc\n");
/* TODO: check got logic for 64 */
							if (raw_sects[k].isjumptable || raw_sects[k].ispointertable)
							{
#ifndef __x86_64__
								ulong tbl_offset=ta_contents-raw_sects[i].abs_offset+raw_sects[i].vm_shift+((rx_pcrel)?ta_pc_addr:0)-raw_sects[k].uaddr;
#else
								ulong tbl_offset=ta_contents-raw_sects[i].abs_offset+raw_sects[i].vm_shift-raw_sects[k].uaddr;
#endif
								target_offset=table_lookup(ffl,object_funcs,objects_count,lmap,lmap_cnt,raw_sects,sect_count,k,tbl_offset,jtbl_off);
								if (target_offset)
								{
									found=1;
#ifndef __x86_64__
									target_offset-=ta_pc_addr+ta_contents;
#else
									target_offset-=ta_contents;
#endif
									rx_pcrel=0; /* hackish */
									break;
								} else {
									ASSERT(1,"Could not relocate indirect reloc\n");
								}
							} else {
								target_offset=raw_sects[k].abs_offset+raw_sects[k].vm_shift-raw_sects[k].uaddr;
								found=1;
								break;
							}
						}
					}
					ASSERT(!found,"No proper section found for reloc\n");
				}
			}

			/* reloc properly... */
			if (rx_pcrel)
			{
#ifndef __x86_64__
				/* in 32bit the rel-addresses point to seg start */
				ta_contents+=target_offset-raw_sects[i].abs_offset-raw_sects[i].vm_shift;
#else
				/* in 64bit the rel-addresses point to next instr. */
				ta_contents+=target_offset-ta_pc_addr;
#endif
			} else {
				ta_contents+=target_offset;
			}

			switch (rx_length)
			{
				case 0:
				if (ta_contents<-0x80 && ta_contents>0x7f)
				{
					INFO_ERR("Byte reloc out of bounds\n");
					return 0;
				}
				*target_address_b=ta_contents;
				break;

				case 1:
				if (ta_contents<-0x8000 && ta_contents>0x7fff)
				{
					INFO_ERR("Word reloc out of bounds\n");
					return 0;
				}
				*target_address_w=ta_contents;
				break;

				case 2:
				*target_address_d=(uint)ta_contents;
				break;

				case 3:
				*target_address_q=ta_contents;
				break;
				
				default:
				break;
			}
		}
	}
//	if (!lconf->debug) *gdb_address=0;
	for (i=0;hash_values[i];i++)
	{
//		INFO_LOG("hash value @ %p\n",(void*)(hash_values[i])-dest);
		*hash_values[i]=lconf->hash_value;
	}

	/* Fancy translations could be put here */

#ifndef __x86_64__
	if ((lconf->translate_type&1) && translate_count)
	{
		INFO_LOG("Doing %d relative-to-indirect call translations\n",translate_count);
		for (i=0;i<translate_count;i++)
		{
			uchar *tdest=translations[i].address;
/*
			0x67FF160000        call dword near [word 0x0]
*/
			*(tdest++)=0x67;
			*(tdest++)=0xff;
			*(tdest++)=0x16;
			*(tdest++)=translations[i].index&0xffU;
			*(tdest++)=translations[i].index>>8;
		}
	}
#endif
	if (lconf->translate_type&2)
	{
		ulong low_addr=0;
		ulong high_addr=0;

		for (i=0;i<sect_count;i++)
		{
			if (raw_sects[i].type==ST_TEXT && !raw_sects[i].isloader)
			{
				ulong tmp_low=raw_sects[i].abs_offset+raw_sects[i].vm_shift;
				ulong tmp_high=raw_sects[i].abs_offset+raw_sects[i].vm_shift+raw_sects[i].length;
				if (!low_addr || tmp_low<low_addr) low_addr=tmp_low;
				if (tmp_high>high_addr) high_addr=tmp_high;
			}
		}
		INFO_LOG("Doing relative to absolute call conversion from 0x%08lx to 0x%08lx\n",low_addr,high_addr);

		uint tr_count=0;
		uint tr_bad_count=0;
		uint last_count=0;
		uint last_bad_count=0;
		ulong tmp_i=0;
		if (high_addr>=5) for (tmp_i=low_addr;tmp_i<high_addr-5;tmp_i++)
		{
			if (*((uchar*)dest+tmp_i)==0xe8U)
			{
				uint *tmp_addr=dest+tmp_i+1;
				ulong real_addr=*tmp_addr+(tmp_i+5);

				ASSERT(tr_count==MAX_TRANSLATIONS,"Max limit of call translations reached\n");
				translations[tr_count].offset=(uint)(tmp_i+1);
				tr_count++;

				/*INFO_LOG("%08x:%08x\n",i,real_addr);*/
				if ((real_addr>=low_addr && real_addr<high_addr) || (real_addr>=jtbl_off && real_addr<jtbl_off+jtbl_len))
				{
					if (tr_count-last_count>2*(tr_bad_count-last_bad_count))
					{
						/*INFO_LOG("*\n");*/
						last_count=tr_count;
						last_bad_count=tr_bad_count;
					}
				} else {
					tr_bad_count++;
				}
				tmp_i+=4;
				continue;
			}
		}
		ASSERT(!last_count,"Translation defined with 0 translations, failing\n");
		INFO_LOG("Found %d good replacements, %d bad replacements\n",last_count-last_bad_count,last_bad_count);

		uint last_addr=0;
		for (i=0;i<last_count;i++)
		{
			uint tmp_off=translations[i].offset;
			uint *tmp_addr=dest+tmp_off;
			*tmp_addr=*tmp_addr+tmp_off;

			last_addr=tmp_off;
		}

		ASSERT(!ctc_address,"No CallTranslateCount in loader\n");
		*ctc_address=(uint)(last_addr+4-low_addr)-last_count*3;
	}

	/*for (i=0;i<sect_count;i++)
	{
		if (raw_sects[i].type==ST_DONOTWANT) break;
		if (raw_sects[i].type==ST_BSS || raw_sects[i].isloader) continue;
		
		bwt(dest+raw_sects[i].abs_offset,(uint)raw_sects[i].length);
	}*/

	/*for (i=0;i<sect_count;i++)
	{
		if (raw_sects[i].type==ST_DONOTWANT) break;
		if (raw_sects[i].type!=ST_TEXT || raw_sects[i].isloader) continue;
		ulong x;
		
		for (x=raw_sects[i].abs_offset;x<raw_sects[i].abs_offset+raw_sects[i].length;x++)
		{
			if (((uchar*)dest)[x]==0xe8)
			{
				*((uint*)(dest+x+1))+=(uint)(x+1);
				x+=4;
			}
		}
	}*/

	if (lconf->onekpaq_mode)
	{
		uchar *hdr=0,*data=0;
		uint hdrlen=0,datalen=0;

		uint rf_len=(uint)f_len;
		uint *r_linker_blocks=linker_blocks;
		uint start_offset=0;

		for (i=0;i<sect_count;i++)
		{
			if (raw_sects[i].iscompressible) break;
			rf_len-=*r_linker_blocks;
			start_offset+=*r_linker_blocks;
			r_linker_blocks++;
		}

		uint shift=0;
		onekpaq_compress(dest+start_offset,rf_len,r_linker_blocks,&data,&datalen,&hdr,&hdrlen,&shift,lconf->onekpaq_mode);

		if (shift_address) *(uchar*)(dest+shift_address)=shift;

		memcpy(dest+start_offset,hdr,hdrlen);
		memcpy(dest+start_offset+hdrlen,data,datalen);
		*(r_linker_blocks++)=hdrlen;
		*(r_linker_blocks++)=datalen;
		*(r_linker_blocks++)=0;
		f_len=start_offset+hdrlen+datalen;

		if (onekpaq_source_address) {
			*onekpaq_source_address=start_offset+hdrlen;
		}
	}
	*filesize_address=f_len;
	*length_ptr=filesize_address;
	return (uint)f_len;
}

void LATURI_initialize(LATURI_data *lconf)
{
	bzero(lconf,sizeof(LATURI_data));

/*	int mib[2];
	int tmp_number_of_cores=0;
	size_t len=sizeof(int); 
	mib[0]=CTL_HW;
	mib[1]=HW_NCPU;
	sysctl(mib,2,&tmp_number_of_cores,&len,0,0);
	if (tmp_number_of_cores<1) tmp_number_of_cores=1;
	lconf->number_of_cores=tmp_number_of_cores;*/

	SInt32 versMaj,versMin;
	Gestalt(gestaltSystemVersionMajor, &versMaj);
	Gestalt(gestaltSystemVersionMinor, &versMin);
	if (versMaj==10) lconf->running_osx_version=versMin;

	lconf->clean_stub=1;
}

static char *getlibpathname(void *library_addr)
{
	/* this is insane, would love to have some API for this */
	char **name=library_addr+sizeof(ulong);
	return strdup(*name);
}

unsigned char *LATURI_generate(LATURI_data *lconf,const char *filename,unsigned int *ret_length)
{
	uint i;
	library_deps deps;
	bzero(&deps,sizeof(library_deps));
	for (i=0;i<lconf->num_libraries;i++)
	{
		deps.dlnames[i]=lconf->library_names[i];
	}
	deps.count=lconf->num_libraries;

	library *objects[MAX_INPUT_FILES];
	for (i=0;i<lconf->num_input_files;i++)
	{
		objects[i]=load_object(lconf,lconf->input_files[i],i+NUM_LOADER_SECTIONS);
		if (!objects[i])
		{
			INFO_ERR("Could not read object\n");
			return 0;
		}
	}

	if (lconf->running_osx_version<11 || lconf->running_osx_version>13)
	{
		INFO_ERR("10.11 -> 10.13 required\n");
		return 0;
	}

	if (!lconf->hash_value || lconf->hash_value>0xffU) lconf->hash_value=0xff;

	for (i=0;i<lconf->num_input_files;i++)
	{
		INFO_LOG("--- Input:   %s\n",lconf->input_file_names[i]);
	}
	for (i=0;i<deps.count;i++) INFO_LOG("--- Library: %s\n",deps.dlnames[i]);
	INFO_LOG("--- Building for 10.%d\n",lconf->running_osx_version);
	INFO_LOG("--- Options: ");
	if (!lconf->no_compress)
	{
		if (lconf->clean_stub) INFO_LOG("[CLEAN_STUB] ");
			else INFO_LOG("[DIRTY_STUB] ");
	} else {
		INFO_LOG("[NO_COMPRESS] ");
	}
#ifndef __x86_64__
	INFO_LOG("[X86]");
#else
	INFO_LOG("[X86_64]");
#endif
#ifdef LATURI_ASM_ONLY
	INFO_LOG(" [ASM]");
#endif
	ASSERT(lconf->hash_type>2,"wrong hash length\n");
	INFO_LOG(" [HASH_LENGTH=%d]",lconf->hash_type+2);
	INFO_LOG(" [HASH_VALUE=0x%x]",lconf->hash_value);
	ASSERT(lconf->onekpaq_mode>4,"Wrong onekpaq mode\n");
	if (lconf->onekpaq_mode)
	{
		if (lconf->onekpaq_mode==1 || lconf->onekpaq_mode==3) INFO_LOG(" [ONEKPAQ_SINGLE]");
			else INFO_LOG(" [ONEKPAQ_MULTI]");
		INFO_LOG(" [DECOMPRESS_OFFSET=%p]",lconf->decompress_offset);
	}
	INFO_LOG("\n");


	function *object_funcs[MAX_INPUT_FILES];
	for (i=0;i<lconf->num_input_files;i++)
	{
		object_funcs[i]=load_funcs(lconf,objects[i]);
		if (!object_funcs[i])
		{
			INFO_ERR("Could not read symbols\n");
			return 0;
		}
	}

	/* Hackish. These need to be defined in order to get picked up by the hash-creator */
	// TODO: header is now 0, loader 1. maybe change needed....
//	object_funcs[0]=insert_dyld_symbol(lconf,object_funcs[0],"__dyld_get_image_name");
//	object_funcs[0]=insert_dyld_symbol(lconf,object_funcs[0],"__dyld_get_image_header");
//	object_funcs[0]=insert_dyld_symbol(lconf,object_funcs[0],"__dyld_get_image_vmaddr_slide");
	object_funcs[0]=insert_dyld_symbol(lconf,object_funcs[0],"_dyld_all_image_infos");

	for (i=0;i<deps.count;i++)
	{
		void *addr;
		if (!(addr=dlopen(deps.dlnames[i],RTLD_NOW|RTLD_NODELETE)))
		{
			INFO_ERR("Could not open library or framework '%s'\n",deps.dlnames[i]);
			return 0;
		} else {
			deps.fullpaths[i]=getlibpathname(addr);
		}
	}

	library *lib[MAX_LIBS];
	function *funcs[MAX_LIBS];
	uint libcount=0;
	for(i=0;i<MAX_LIBS;i++)
	{
		lib[i]=load_library(lconf,i,&deps);
		if (!lib[i])
		{
			libcount=i;
			break;
		}
		funcs[i]=load_funcs(lconf,lib[i]);
	}
/*	INFO_LOG("Libcount is %d\n",libcount); */

	linkmap *lmap=map_funcs(lconf,lib,funcs,libcount,object_funcs,lconf->num_input_files);
	if (!lmap) return 0;
	if (map_libs(lmap,lib,libcount)) return 0;

	char hash_data[MAX_HASH_SIZE];
	bzero(hash_data,MAX_HASH_SIZE);
	uint hash_len=create_map(lconf,hash_data,lmap);
	if (!hash_len)
	{
		INFO_ERR("Hash generation error. zero size\n");
		return 0;
	}
	if (hash_len>MAX_HASH_SIZE)
	{
		INFO_ERR("Resulting hash table too large: HARD_LIMIT!\n");
		return 0;
	}
	/* Visual feedback for inspecting format */
#if 0
	INFO_LOG("RAW: ---------------------------------------------------------\n");
	for (i=0;i<((hash_len+15)&~15);i++)
	{
		if (!(i&15)) INFO_LOG("%08x: ",i);
		if (i<hash_len) INFO_LOG("%02x",(uint)(uchar)(hash_data[i]));
			else INFO_LOG("  ");
		if ((i&15)==7) INFO_LOG(" ");
			else if ((i&15)==15)
		{
			uint j;

			INFO_LOG("  \"");
			for (j=(i&~15);j<i;j++)
			{
				if (j<hash_len) INFO_LOG("%c",isprint(hash_data[j])?hash_data[j]:'.');
			}
			INFO_LOG("\"\n");
		}
	}
	INFO_LOG("--------------------------------------------------------------\n");
#endif

	uchar *binary_data=malloc(MAX_BINARY_SIZE);
	bzero(binary_data,MAX_BINARY_SIZE);
	uint linker_blocks[MAX_SECTS];
	bzero(linker_blocks,MAX_SECTS*sizeof(uint));
	ulong *length_ptr=0;
	uint binary_len=link_binary(lconf,binary_data,linker_blocks,objects,object_funcs,lconf->num_input_files,lmap,hash_data,hash_len,lib,libcount,&length_ptr);
	if (!binary_len)
	{
		return 0;
	}
	if (binary_len>MAX_BINARY_SIZE)
	{
		INFO_ERR("Internal implementation error. File too large\n");
		return 0;
	}
	if (lconf->running_osx_version>=10 && binary_len<4096) // stupid new check in the kernel
	{
		memset(binary_data+binary_len,0,4096-binary_len);
		uint j=0;
		while (linker_blocks[j]) j++;
		linker_blocks[j++]=4096-binary_len;
		linker_blocks[j++]=0;
		binary_len=4096;
		*length_ptr=4096;
	}

	uint final_length=0;
	uchar *final_data=0;

	if (lconf->no_compress)
	{
		final_data=binary_data;
		final_length=binary_len;
	} else {
		uint comp_data_len;
		int has_spaces=0;
		char *tmp;
		if ((tmp=rindex(filename,' ')))
		{
			if (!index(tmp,'/')) has_spaces=1;
		}
		void *comp_data=kompura(binary_data,binary_len,linker_blocks,&comp_data_len,1/*lconf->running_osx_version>=9*/,lconf->clean_stub,has_spaces);
		if (!comp_data || !comp_data_len)
		{
			INFO_ERR("Error compressing data\n");
			return 0;
		}
		if (comp_data_len>=binary_len)
		{
			INFO_LOG("No point using compression (%d>=%d), writing plain executable\n",comp_data_len,binary_len);
			free(comp_data);
			final_data=binary_data;
			final_length=binary_len;
		} else {
			free(binary_data);
			final_data=comp_data;
			final_length=comp_data_len;
		}
	}
	if (final_data) *ret_length=final_length;
	return final_data;
}

const char *LATURI_version(void)
{
	return "3.2";
}

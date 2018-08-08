/* Copyright (C) Teemu Suutari */

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <getopt.h>
#include <spawn.h>
#include <stdarg.h>
#include <CoreServices/CoreServices.h>

#include "laturi_api.h"

static int enable_info_log=0;

void log_call(int level,const char *str,...)
{
	va_list ap;

	va_start(ap,str);
	if (enable_info_log || level<=0) vfprintf(stderr,str,ap);
	va_end(ap);
	if (level<0) abort();
}

static void usage(void)
{
	log_call(0,"\n--- LATURI ------------------------------------------------------------------\n");
	log_call(0,"--- Mach-O object linker/packer intended for 1k/4k intros                 ---\n");
	log_call(0,"--- Copyright (C) TS, The Digital Artist, tz@iki.fi                       ---\n");
        log_call(0,"--- Freely usable, no warranty!!!                                         ---\n");
	log_call(0,"-----------------------------------------------------------------------------\n\n");
	log_call(0,"Version: %s\n",LATURI_version());
	log_call(0,"Usage: laturi [options]\n\n");
	log_call(0,"Options:\n");
	log_call(0,"\t-i,--input file \n\t\tInput file(s) to be linked and optionally compressed\n");
	log_call(0,"\t-o,--output file\n\t\tOutput file to be generated\n");
	log_call(0,"\t-l,--library library\n\t\tLink with library\n");
	log_call(0,"\t-f,--framework framework\n\t\tLink with framework\n");
	log_call(0,"\t-H,--hash-length length\n\t\tHash length for function and library references\n");
	log_call(0,"\t\t[default: 3, valid values 2-4]\n");
	log_call(0,"\t-W,--hash-value value\n\t\tHash value for 2-byte hashing\n");
	log_call(0,"\t-m,--heap-size size\n\t\tTarget program heap size incl. code, stack on top of heap\n");
	log_call(0,"\t\t[default: minimum 128k, rounded to next largest 2^n]\n");
	log_call(0,"\t-p,--onekpaq-mode mode\n\t\tSpecify onekpaq ppm comprission mode\n");
	log_call(0,"\t\t0 - no onekpaq compression\n");
	log_call(0,"\t\t1 - onekpaq single section mode\n");
	log_call(0,"\t\t2 - onekpaq multi section mode\n");
	log_call(0,"\t\t3 - onekpaq single section mode, fast decoder\n");
	log_call(0,"\t\t4 - onekpaq multi section mode, fast decoder\n");
	log_call(0,"\t\t[default: no onekpaq compression]\n");
	log_call(0,"\t-P,--decompress-offset offset\n\t\tTarget decompression offset (inside heap)\n");
	log_call(0,"\t\t[default: default 4k, uncompressed binary must fit into this]\n");
	log_call(0,"\t-t,--tgt-trans call-translate-type\n\t\tTranslate relative calls to indirect and/or absolute calls\n");
	log_call(0,"\t\t0 - no call translation\n");
	log_call(0,"\t\t1 - translate relative calls to external libraries to indirect\n");
	log_call(0,"\t\t2 - translate relative calls to absolute calls\n");
	log_call(0,"\t\t3 - use both translations\n");
	log_call(0,"\t\t[default: no translation]\n");
	log_call(0,"\t\t[note: 64-bit mode only support rel-to-abs conversion]\n");
	log_call(0,"\t-d,--debug create gdb-debuggable binary\n");
	log_call(0,"\t\t[default: no debug]\n");
	log_call(0,"\t   --tgt-noclean\n\t\tUse smaller, messy start-stub\n");
	log_call(0,"\t\t[default: clean-version]\n");
	log_call(0,"\t   --tgt-nopack\n\t\tDo not compress with gzip\n");
	log_call(0,"\t\t[default: gzip-compression enabled]\n");
	log_call(0,"\t-v,--verbose\n\t\tBe verbose\n");
	log_call(0,"\t-h,--help\n\t\tThis page\n");
	exit(-1);
}

static void toomanylibs(void)
{
	log_call(0,"Too many libraries/frameworks specified\n");
	exit(-1);
}

#ifdef __x86_64__
static int spawn_in_32bit(int __attribute__((unused)) argc, char **argv, char **envp)
{
	/* API for doing this is crap */
	posix_spawnattr_t attr;
	cpu_type_t ct=CPU_TYPE_X86;
	size_t ocount;

	if (posix_spawnattr_init(&attr))
	{
		log_call(0,"posix_spawnattr_init error\n");
		return -1;
	}

	if (posix_spawnattr_setflags(&attr,POSIX_SPAWN_SETEXEC))
	{
		log_call(0,"posix_spawnattr_setflags error\n");
		return -1;
	}

	if (posix_spawnattr_setbinpref_np(&attr,1,&ct,&ocount))
	{
		log_call(0,"posix_spawnattr_setbinpref_np error\n");
		return -1;
	}

	if (posix_spawnp(0,argv[0],0,&attr,argv,envp))
	{
		log_call(0,"posix_spawnp error\n");
		return -1;
	}
	return -1;
}
#endif

static void *map_file(const char *name)
{
	int fd;
	unsigned long size;
	void *addr;
	struct stat st;

	/* this leaves stuff unfreed & unclosed eventually
	   I dont care...
	*/
	fd=open(name,O_RDONLY);
	if (fd<0) return 0;
	fstat(fd,&st);
	size=(uint)st.st_size;
	size=(size+(PAGE_SIZE-1))&~(PAGE_SIZE-1);
	addr=mmap(0,size,PROT_READ,MAP_FILE|MAP_SHARED,fd,0);
	if (addr==(void*)~0) addr=0;
	return addr;
}

static int write_file(const char *name,void *data,uint len)
{
	int fd;
	unlink(name);
	fd=open(name,O_WRONLY|O_CREAT|O_TRUNC,0755);
	if (fd<0)
	{
		return -1;
	}
	if (write(fd,data,len)!=(int)len)
	{
		close(fd);
		unlink(name);
		return -1;
	}
	close(fd);
	return 0;
}

#ifdef __x86_64__
int main(int argc,char **argv,char **envp)
#else
int main(int argc,char **argv)
#endif
{
	LATURI_data lconf;

	LATURI_initialize(&lconf);

	int tmp_heap_size=0;
	int tmp_clean_stub=1;
	int tmp_no_compress=0;
	int tmp_debug=0;
	int tmp_hash_length=3;
	unsigned int tmp_hash_value=0x7b;
	int tmp_onekpaq_mode=0;
	unsigned long tmp_decompr_offset=0x1000;
	/* get options: */
	struct option lo[] = {
		{ "input",	required_argument,	0,		'i' },
		{ "output",	required_argument,	0,		'o' },
		{ "library",	required_argument,	0,		'l' },
		{ "framework",	required_argument,	0,		'f' },
		{ "hash-length",required_argument,	0,		'H'},
		{ "hash-value", required_argument,	0,		'W'},
		{ "heap-size",	required_argument,	0,		'm' },
		{ "tgt-noclean", no_argument,		&tmp_clean_stub, 0  },
		{ "tgt-nopack",	no_argument,		&tmp_no_compress,1  },
		{ "tgt-trans",	required_argument,      0,              't' },
		{ "debug",	no_argument,		&tmp_debug,	'd' },
		{ "onekpaq-mode", required_argument,	0,		'p' },
		{ "decompress-offset", required_argument, 0,		'P' },
		{ "help",	no_argument,		0,		'h' },
		{ "verbose",	no_argument,		0,		'v' },
		{ 0,		0,			0,		0   }
	};

	char *output_file=0;
	unsigned int i;

	int ch;
	while ((ch=getopt_long(argc,argv,"i:o:l:f:H:W:m:t:p:P:dvh",lo,0))!=-1)
	{
		switch(ch)
		{
			case 'i':
			if (lconf.num_input_files==MAX_INPUT_FILES)
			{
				log_call(0,"Too many input files");
				return -1;
			}
			lconf.input_file_names[lconf.num_input_files++]=strdup(optarg);
			break;

			case 'o':
			output_file=strdup(optarg);
			break;

			case 'l':
			if (lconf.num_libraries==MAX_LIBS_COUNT) toomanylibs();
			lconf.library_names[lconf.num_libraries++]=strdup(optarg);
			break;

			case 'f':
			{
				const char *fw_padding=".framework/";
				int fw_len=(unsigned int)strlen(optarg);
				if (lconf.num_libraries==MAX_LIBS_COUNT) toomanylibs();
				lconf.library_names[lconf.num_libraries]=malloc((fw_len*2+strlen(fw_padding)+1)*sizeof(char));
				memcpy(lconf.library_names[lconf.num_libraries],optarg,fw_len);
				memcpy(lconf.library_names[lconf.num_libraries]+fw_len,fw_padding,strlen(fw_padding));
				strcpy(lconf.library_names[lconf.num_libraries++]+fw_len+strlen(fw_padding),optarg);
			}
			break;
			
			case 'H':
			tmp_hash_length=atoi(optarg);
			break;

			case 'W':
			tmp_hash_value=atoi(optarg);
			break;

			case 't':
			lconf.translate_type=atoi(optarg);
			if (lconf.translate_type<0 || lconf.translate_type>3)
			{
				log_call(0,"Unknown call translation type\n");
				return -1;
			}
			break;

			case 'm':
			tmp_heap_size=atoi(optarg);
			break;
			
			case 'p':
			tmp_onekpaq_mode=atoi(optarg);
			break;

			case 'P':
			tmp_decompr_offset=atol(optarg);
			break;

			case 'd':
			tmp_debug=1;
			break;

			case 'h':
			usage();
			break;

			case 'v':
			enable_info_log=1;
			break;

			case 0:
			break;

			default:
			usage();
			break;
		}
	}
	if (tmp_hash_length<2 || tmp_hash_length>4) usage();
	if (tmp_onekpaq_mode>4) usage();
	if (!lconf.num_input_files || !output_file || !lconf.num_libraries) usage();
/*	argc-=optind;
	argv+=optind;
*/
	if (argc-optind) usage();
	lconf.hash_type=tmp_hash_length-2;
	lconf.hash_value=tmp_hash_value;
	lconf.heap_size=tmp_heap_size;
	lconf.clean_stub=tmp_clean_stub;
	lconf.no_compress=tmp_no_compress;
	lconf.onekpaq_mode=tmp_onekpaq_mode;
	lconf.decompress_offset=tmp_decompr_offset;
	lconf.debug=tmp_debug;

	for (i=0;i<lconf.num_input_files;i++)
	{
		lconf.input_files[i]=map_file(lconf.input_file_names[i]);
		if (LATURI_check_library_bits(lconf.input_files[i]))
		{
#ifdef __x86_64__
			return spawn_in_32bit(argc,argv,envp);
#else
			log_call(0,"Can't do 64bit builds on 32bit machine (or you mixed 64bit and 32bit objects)\n");
			return -1;
#endif
		}
	}

	unsigned char *final_data=0;
	unsigned int final_length=0;

	log_call(1,"--- Output:  %s\n",output_file);
	final_data=LATURI_generate(&lconf,output_file,&final_length);
	if (!final_data) return -1;

	if (write_file(output_file,final_data,final_length))
	{
		log_call(0,"Error writing target file\n");
		return -1;
	}

	return 0;
}

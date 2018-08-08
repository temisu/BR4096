/* Copyright (C) Teemu Suutari */

#ifndef LATURI_API_H
#define LATURI_API_H

#define MAX_INPUT_FILES (256)
#define MAX_LIBS_COUNT (128)

/* data blob for config and input */
typedef struct LATURI_data_t
{
/* internal conf */
int		running_osx_version;
/* options */
int		translate_type;
int		hash_type;
int		clean_stub;
int		onekpaq_mode;
int		no_compress;
int		debug;
unsigned int	hash_value;
unsigned long	heap_size;
unsigned long	decompress_offset;
/* data files */
unsigned int	num_input_files;
char		*input_file_names[MAX_INPUT_FILES];
void		*input_files[MAX_INPUT_FILES];
unsigned int	num_libraries;
char		*library_names[MAX_LIBS_COUNT];
} LATURI_data;

/* User implements this */
extern void log_call(int level,const char *str,...);

/* These are provided */
extern int LATURI_check_library_bits(const void *addr);
extern void LATURI_initialize(LATURI_data *lconf);
extern const char *LATURI_version(void);
extern unsigned char *LATURI_generate(LATURI_data *lconf,const char *filename,unsigned int *ret_length);

#endif

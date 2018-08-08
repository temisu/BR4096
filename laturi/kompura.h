/* Copyright (C) Teemu Suutari */

#ifndef KOMPURA_H
#define KOMPURA_H
#include "common.h"

#define Z_ABET (288)
#define Z_DIST (32)
#define Z_TAB (19)

enum {
	CW_NONE=0,
	CW_LITERAL=1,
	CW_LENGTH=2,
	CW_DISTANCE=3,
	CW_END=4,
	CW_TABLE=5,
	CW_RAW=6
};

enum {
	CB_RAW=0,
	CB_STATIC=1,
	CB_DYNAMIC=2,
};

typedef struct z_itemdata_t {
ushort		length;
ushort		dist;
ushort		c_length;
ushort		c_dist;
ushort		coverage;
} __attribute__((packed)) z_itemdata;

typedef struct z_codeword_t {
/* uncoded symbol */
uchar		type;
uchar		lit;
ushort		ld;

/* coded symbol */
ushort		code;
ushort		num_extra_bits;
ushort		extra_bits;
} __attribute__((packed)) z_codeword;

typedef struct z_blockentry_t {
uint		length;
uint		count;
uint		chosen_length;
int		addressable;
} __attribute__((packed)) z_blockentry;

typedef struct z_huff_item_t {
/* uncoded info */
uint		code;	/* when sorted, code needs to be known */
uint		count;

/* probability stuff */
dfloat		w;
dfloat		opt_length;
dfloat		lc_per_budget;
int		decremented;

/* coded info */
uint		bit_length;
uint		symbol;
} z_huff_item;

typedef struct z_huff_map_t {
/* uncoded map */
uint		type;	/* CB_ ... */
uint		num_tot_items;
uint		num_lit_items;
uint		num_dist_items;
z_huff_item	items[Z_ABET];

/* coded distance */
uint		distances_length;
z_huff_item	distances[Z_DIST];

/* coded map (use the same principles as in main data) */
uint		table_lit_length;
uint		table_dist_length;
uint		table_enc_data_length;
z_codeword	table_enc_data[Z_ABET+Z_DIST];
z_huff_item	table[Z_TAB];
uchar		table_lengths[Z_TAB];
uchar		table_lengths_length;

/* optimization */
uint		*raw_cost_map;
} z_huff_map;

typedef z_codeword *(*compress_func)(ushort*,uchar*,uint,uint,uint*,z_huff_map*,uint);

typedef struct compress_workload_t
{
/* parameters */
uint		start_offset;
uint		end_offset;
uint		extra;
/* return data */
uint		length;
z_codeword	*c_map;
z_huff_map	*h_map;
} compress_workload;

typedef struct gziphdr_t {
ushort		magic;
uchar		method;
uchar		flags1;
uchar		mtime[4];
uchar		flags2;
uchar		os;
} __attribute__((packed)) gziphdr;

/* Interface */

void *kompura(uchar *data,uint len,uint *linker_blocks,uint *retlen,int apple_gzip,int clean_stub,int has_spaces);

#endif

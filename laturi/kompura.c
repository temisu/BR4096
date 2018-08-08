/* Copyright (C) Teemu Suutari */

/* I used only rfc1951 to implement this */

#include "kompura.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dispatch/dispatch.h>

#define KOMPURA_MAX_DEPTH (1024)

//#define DEBUG_BLOCKS 1

static const uchar huffman_lengths_order[Z_TAB]=
	{16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

static void find_length_code(z_codeword *c)
{
	uint code,tmp_num_bits,tmp_bits,tmp_symbol;

	code=c->ld-3;
	if (code&0xf0)
	{
		if (code&0xc0)
		{
			if (code&0x80)
			{
				if (code==0xff)
				{
					tmp_symbol=285;
					tmp_bits=0;
					tmp_num_bits=0;
				} else {
					tmp_symbol=((code&0x60)>>5)+281;
					tmp_bits=code&0x1f;
					tmp_num_bits=5;
				}
			} else {
				tmp_symbol=((code&0x30)>>4)+277;
				tmp_bits=code&0xf;
				tmp_num_bits=4;
			}
		} else {
			if (code&0x20)
			{
				tmp_symbol=((code&0x18)>>3)+273;
				tmp_bits=code&0x7;
				tmp_num_bits=3;
			} else {
				tmp_symbol=((code&0xc)>>2)+269;
				tmp_bits=code&0x3;
				tmp_num_bits=2;
			}
		}
	} else {
		if (code&0x8)
		{
			tmp_symbol=((code&0x6)>>1)+265;
			tmp_bits=code&0x1;
			tmp_num_bits=1;
		} else {
			tmp_symbol=(code&0x7)+257;
			tmp_bits=0;
			tmp_num_bits=0;
		}
	}
	c->num_extra_bits=tmp_num_bits;
	c->extra_bits=tmp_bits;
	c->code=tmp_symbol;
}

static void find_distance_code(z_codeword *c)
{
	uint code,tmp_num_bits,tmp_bits,tmp_symbol;

	code=c->ld-1;
	if (code&0x7f00)
	{
		if (code&0x7000)
		{
			if (code&0x4000)
			{
				tmp_symbol=((code&0x2000)>>13)+28;
				tmp_bits=code&0x1fff;
				tmp_num_bits=13;
			} else {
				if (code&0x2000)
				{
					tmp_symbol=((code&0x1000)>>12)+26;
					tmp_bits=code&0xfff;
					tmp_num_bits=12;
				} else {
					tmp_symbol=((code&0x800)>>11)+24;
					tmp_bits=code&0x7ff;
					tmp_num_bits=11;
				}
			}
		} else {
			if (code&0xc00)
			{
				if (code&0x800)
				{
					tmp_symbol=((code&0x400)>>10)+22;
					tmp_bits=code&0x3ff;
					tmp_num_bits=10;
				} else {
					tmp_symbol=((code&0x200)>>9)+20;
					tmp_bits=code&0x1ff;
					tmp_num_bits=9;
				}
			} else {
				if (code&0x200)
				{
					tmp_symbol=((code&0x100)>>8)+18;
					tmp_bits=code&0xff;
					tmp_num_bits=8;
				} else {
					tmp_symbol=((code&0x80)>>7)+16;
					tmp_bits=code&0x7f;
					tmp_num_bits=7;
				}
			}
		}
	} else {
		if (code&0xf0)
		{
			if (code&0xc0)
			{
				if (code&0x80)
				{
					tmp_symbol=((code&0x40)>>6)+14;
					tmp_bits=code&0x3f;
					tmp_num_bits=6;
				} else {
					tmp_symbol=((code&0x20)>>5)+12;
					tmp_bits=code&0x1f;
					tmp_num_bits=5;
				}
			} else {
				if (code&0x20)
				{
					tmp_symbol=((code&0x10)>>4)+10;
					tmp_bits=code&0xf;
					tmp_num_bits=4;
				} else {
					tmp_symbol=((code&0x8)>>3)+8;
					tmp_bits=code&0x7;
					tmp_num_bits=3;
				}
			}
		} else {
			if (code&0xc)
			{
				if (code&0x8)
				{
					tmp_symbol=((code&0x4)>>2)+6;
					tmp_bits=code&0x3;
					tmp_num_bits=2;
				} else {
					tmp_symbol=((code&0x2)>>1)+4;
					tmp_bits=code&0x1;
					tmp_num_bits=1;
				}
			} else {
				tmp_symbol=code;
				tmp_bits=0;
				tmp_num_bits=0;
			}
		}
	}
	c->num_extra_bits=tmp_num_bits;
	c->extra_bits=tmp_bits;
	c->code=tmp_symbol;
}

static void encode_codeword(z_codeword *c)
{
	switch (c->type)
	{
		case CW_LITERAL:
		c->code=c->lit;
		/*c->num_extra_bits=0;
		c->extra_bits=0;*/
		break;

		case CW_LENGTH:
		DBG_ASSERT(c->ld<3 || c->ld>258,"Wrong length of encode data (%d)\n",c->ld);
		find_length_code(c);
		break;

		case CW_DISTANCE:
		DBG_ASSERT(c->ld<1 || c->ld>32768,"Wrong distance of encode data (%d)\n",c->ld);
		find_distance_code(c);
		break;
	
		case CW_END:
		c->code=256;
		/*c->num_extra_bits=0;
		c->extra_bits=0;*/
		break;

		case CW_TABLE:
		if (c->ld==1)
		{
			c->code=c->lit;
			/*c->num_extra_bits=0;
			c->extra_bits=0;*/
		} else {
			if (c->lit)
			{
				DBG_ASSERT(c->ld<3 || c->ld>6,"Wrong length of table data repeat x (%d)\n",c->ld);

				c->code=16;
				c->num_extra_bits=2;
				c->extra_bits=c->ld-3;
			} else {
				DBG_ASSERT(c->ld<3 || c->ld>138,"Wrong length of table data repeat 0 (%d)\n",c->ld);

				if (c->ld<11)
				{
					c->code=17;
					c->num_extra_bits=3;
					c->extra_bits=c->ld-3;
				} else {
					c->code=18;
					c->num_extra_bits=7;
					c->extra_bits=c->ld-11;
				}
			}
		}
		break;
		
		case CW_RAW:
		c->code=c->lit;
		/*c->num_extra_bits=0;
		c->extra_bits=0;*/
		break;
		
		default:
		DBG_ASSERT(1,"Not defined codeword\n");
		break;
	}
}

static int calculate_block_saving(const uchar *data,uint offset,uint length,uint distance,z_huff_map *map,int only_len)
{
	z_codeword tmp;
	int tmp_bits;

	tmp_bits=0;
	if (length>=3 && length<=258)
	{
		/*bzero(&tmp,sizeof(z_codeword));*/
		tmp.type=CW_LENGTH;
		tmp.ld=length;
		encode_codeword(&tmp);
		tmp_bits-=tmp.num_extra_bits;
		if (map)
		{
			tmp_bits-=map->items[tmp.code].bit_length;
			if (!map->items[tmp.code].bit_length) tmp_bits-=8;
		} else {
			tmp_bits-=8;
		}

		tmp.type=CW_DISTANCE;
		tmp.ld=distance;
		encode_codeword(&tmp);
		tmp_bits-=tmp.num_extra_bits;
		if (map)
		{
			tmp_bits-=map->distances[tmp.code].bit_length;
			if (!map->distances[tmp.code].bit_length) tmp_bits-=5;
		} else {
			tmp_bits-=5;
		}
	} else if (length==1) {
		if (map)
		{
			tmp_bits=-map->items[data[offset]].bit_length;
		} else {
			tmp_bits=-8;
		}
		if (only_len) return -tmp_bits;
		return tmp_bits;
	} else {
		DBG_ASSERT(1,"Unknown length for cost calculate (%d)\n",length);
	}

	if (!only_len)
	{
		if (map)
		{
			if (offset)
			{
				tmp_bits+=map->raw_cost_map[offset-1+length]-map->raw_cost_map[offset-1];
			} else {
				tmp_bits+=map->raw_cost_map[length-1];
			}
		} else {
			tmp_bits+=length*8;
		}
	} else tmp_bits=-tmp_bits;
	return tmp_bits;
}

static uint find_len_block(uint *target_length,ushort *shadow_map,const uchar *data,uint offset,uint block_length,z_huff_map *map)
{
	uint best_offset,best_length,max_length,tmp_dist,max_depth;
	int best_saving,lit_saving;

	best_offset=0;
	best_length=1;
	max_length=*target_length;
	lit_saving=best_saving=calculate_block_saving(data,offset,1,0,map,0);

	tmp_dist=shadow_map[offset];
	if (offset+3>block_length) tmp_dist=0;
	max_depth=KOMPURA_MAX_DEPTH;
	while (tmp_dist && max_depth)
	{
		int saving;
		uint len;

		/*if (!hashmap_get_value(offset,tmp_dist,&len))
		{*/
		len=3;
		while (len<max_length && offset+len<block_length && data[offset-tmp_dist+len]==data[offset+len]) len++;
		/*hashmap_set_value(offset,tmp_dist,len);
		}
		if (len>max_length) len=max_length;*/

		saving=calculate_block_saving(data,offset,len,tmp_dist,map,0);
		if (saving>best_saving && (len==max_length || max_length==258))
		{
			best_offset=tmp_dist;
			best_length=len;
			best_saving=saving;
			/*if (len==258 && tmp_dist==1) break;*/
		}
		if (shadow_map[offset-tmp_dist])
		{
			max_depth--;
			tmp_dist+=shadow_map[offset-tmp_dist];
			if (tmp_dist>32768) tmp_dist=0;
		} else {
			tmp_dist=0;
		}
	}

	if (best_length==max_length || max_length==258)
	{
		*target_length=best_length;
		return best_offset;
	} else {
		*target_length=1;
		return 0;
	}
}

static void find_best_repeat_block(z_itemdata *c,ushort *shadow_map,const uchar *data,uint offset,uint block_length,z_huff_map *map)
{
	uint target_length;

	target_length=258;
	c->dist=find_len_block(&target_length,shadow_map,data,offset,block_length,map);
	c->length=target_length;
}

static uint find_specific_len_block(uint target_length,ushort *shadow_map,const uchar *data,uint offset,uint block_length,z_huff_map *map)
{
	uint tmp=target_length;

	DBG_ASSERT(target_length>=258,"Too long block length\n");
	return find_len_block(&tmp,shadow_map,data,offset,block_length,map);
}

#define BLOCK_FUDGE (2)

static uint find_nth_block_dest(z_blockentry *blks,uint bcount,uint bindex,uint n)
{
	uint i,len;

	i=0;
	len=0;
	do
	{
		if (bindex+i>=bcount) return 0;
		if (blks[bindex].length>=len && len+blks[bindex+i].count>blks[bindex].length && n<=BLOCK_FUDGE)
		{
			if (blks[bindex].length<=len+n || n>=blks[bindex].length) return 0;
				else return blks[bindex].length-n;
		}
		len+=blks[bindex+i].count;
		i++;
	} while (n--);

	if (len>blks[bindex].length) return 0;
	return len;
}

static uint process_block(ushort *shadow_map,const uchar *data,uint start_off,uint block_length,z_itemdata *i_map,z_blockentry *blks,uint bcount,z_huff_map *zmap,int emit_block)
{
	uint i,cost,blk_index,len_decr;

	i=0;
	cost=0;
	blk_index=0;
	len_decr=0;
	while (i<bcount)
	{
		uint tmp_b,real_len;

		tmp_b=blk_index;
		DBG_ASSERT(len_decr>=blks[i].chosen_length,"Length mismatch in block processing (%d>=%d)\n",len_decr,blks[i].chosen_length);
		real_len=blks[i].chosen_length-len_decr;
		blks[i].addressable=1;
		if (real_len==1)
		{
			if (emit_block)
			{
				i_map[blk_index].c_length=1;
				i_map[blk_index].c_dist=0;
				INFO_DEBUG("1:");
			} else {
				cost+=calculate_block_saving(data,start_off+blk_index,1,0,zmap,1);
			}
			blk_index++;
		} else if (real_len==2) {
			if (emit_block)
			{
				i_map[blk_index].c_length=1;
				i_map[blk_index].c_dist=0;
				i_map[blk_index+1].c_length=1;
				i_map[blk_index+1].c_dist=0;
				INFO_DEBUG("2:");
			} else {
				cost+=calculate_block_saving(data,start_off+blk_index,1,0,zmap,1);
				cost+=calculate_block_saving(data,start_off+blk_index+1,1,0,zmap,1);
			}
			blk_index+=2;
		} else if (real_len==3 && i_map[blk_index].length>=3) {
			uint tmp_dist;
			uint bcost,lcost;

			if (real_len==i_map[blk_index].length)
			{
				tmp_dist=i_map[blk_index].dist;
			} else {
				tmp_dist=find_specific_len_block(real_len,shadow_map,data,start_off+blk_index,block_length,zmap);
				if (!tmp_dist) tmp_dist=i_map[blk_index].dist;
			}
			bcost=calculate_block_saving(data,start_off+blk_index,real_len,tmp_dist,zmap,1);
			lcost=calculate_block_saving(data,start_off+blk_index,1,0,zmap,1) +
				calculate_block_saving(data,start_off+blk_index+1,1,0,zmap,1) +
				calculate_block_saving(data,start_off+blk_index+2,1,0,zmap,1);
			if (bcost>lcost)
			{
				if (emit_block)
				{
					INFO_DEBUG("1:1:1:");
					i_map[blk_index].c_length=1;
					i_map[blk_index].c_dist=0;
					i_map[blk_index+1].c_length=1;
					i_map[blk_index+1].c_dist=0;
					i_map[blk_index+2].c_length=1;
					i_map[blk_index+2].c_dist=0;
				} else {
					cost+=lcost;
				}
			} else {
				if (emit_block)
				{
					i_map[blk_index].c_length=real_len;
					i_map[blk_index].c_dist=tmp_dist;
					INFO_DEBUG("3:");
				} else {
					cost+=bcost;
				}
			}
			blk_index+=3;
		} else {
			uint tmp_dist;
			uint tmp_len;
			
			tmp_len=real_len;
			if (tmp_len==i_map[blk_index].length && tmp_len<=258)
			{
				tmp_dist=i_map[blk_index].dist;
			} else {
				while (tmp_len>258)
				{
					/* we need to divide the blocks
					   leave the last block to common case */
					uint shard_len;
					uint shard_dist;
					if (tmp_len>=261 || tmp_len==259) shard_len=258;
						else shard_len=257;
					if (shard_len==258)
					{
						DBG_ASSERT(i_map[blk_index].length!=258,"Max repeat length is not 258 (%d)\n",i_map[blk_index].length);
						shard_dist=i_map[blk_index].dist;
					} else {
						shard_dist=find_specific_len_block(shard_len,shadow_map,data,start_off+blk_index,block_length,zmap);
						if (!shard_dist)
						{
							shard_dist=i_map[blk_index].dist;
							if (!shard_dist) shard_len=1;
						}
					}
					if (emit_block)
					{
						i_map[blk_index].c_length=shard_len;
						i_map[blk_index].c_dist=shard_dist;
						INFO_DEBUG("%d:",shard_len);
					} else {
						cost+=calculate_block_saving(data,start_off+blk_index,shard_len,shard_dist,zmap,1);
					}
					tmp_len-=shard_len;
					blk_index+=shard_len;
				}
				if (tmp_len!=1)
				{
					if (i_map[blk_index].length==tmp_len)
					{
						tmp_dist=i_map[blk_index].dist;
					} else {
						tmp_dist=find_specific_len_block(tmp_len,shadow_map,data,start_off+blk_index,block_length,zmap);
						if (!tmp_dist)
						{
							tmp_dist=i_map[blk_index].dist;
							if (!tmp_dist) tmp_len=1;
						}
					}
				} else tmp_dist=0;
			}
			if (emit_block)
			{
				i_map[blk_index].c_length=tmp_len;
				i_map[blk_index].c_dist=tmp_dist;
				INFO_DEBUG("%d:",tmp_len);
			} else {
				cost+=calculate_block_saving(data,start_off+blk_index,tmp_len,tmp_dist,zmap,1);
			}
			blk_index+=tmp_len;
		}
		tmp_b=blk_index-tmp_b+len_decr;
		while (i<bcount && tmp_b>=blks[i].count) tmp_b-=blks[i++].count;
		if (tmp_b)
		{
			len_decr=tmp_b;
		} else {
			len_decr=0;
		}
	}
	if (emit_block) { INFO_DEBUG("\n"); }
	return cost;
}

#define ITERATIVE_LOOP_MAX (8)

static void minimize_block(ushort *shadow_map,const uchar *data,uint start_off,uint block_length,z_itemdata *i_map,z_blockentry *blks,uint bcount,z_huff_map *zmap)
{
	uint i,best_cost;
	uint *best_indexes;
	uint *trial_indexes;
	
	best_indexes=malloc(bcount*sizeof(uint));
	trial_indexes=malloc(bcount*sizeof(uint));	
	bzero(best_indexes,bcount*sizeof(uint));
	bzero(trial_indexes,bcount*sizeof(uint));

	/*DBG_ASSERT(bcount==1,"Trivial case provided for resolver\n");*/

	for (i=0;i<bcount;i++) blks[i].chosen_length=find_nth_block_dest(blks,bcount,i,0);

	best_cost=~0U;
	/* brute force */
	for (;;)
	{
		uint tmp_cost,not_found_next,done;

		/* do trial */
		for (i=0;i<bcount;i++) blks[i].addressable=0;
		tmp_cost=process_block(shadow_map,data,start_off,block_length,i_map,blks,bcount,zmap,0);
		if (tmp_cost<=best_cost)
		{
			bcopy(trial_indexes,best_indexes,bcount*sizeof(uint));
			best_cost=tmp_cost;
		}


		/* calculate next attempt */
		i=bcount-1;
		done=0;
		do {
			uint n,cl;

			while (!blks[i].addressable && i)
			{
				trial_indexes[i]=0;
				blks[i].chosen_length=find_nth_block_dest(blks,bcount,i,0);
				i--;
			}
			n=trial_indexes[i];
			cl=find_nth_block_dest(blks,bcount,i,n+1);
			if (cl)
			{
				trial_indexes[i]=n+1;
				blks[i].chosen_length=cl;
				not_found_next=0;
			} else {
				trial_indexes[i]=0;
				blks[i].chosen_length=find_nth_block_dest(blks,bcount,i,0);
				if (!i)
				{
					done=1;
				} else {
					i--;
					not_found_next=1;
				}
			}
		} while(not_found_next && !done);
		if (done) break;
		/* this might get nasty, so bail out at number 1 if so */
		if (bcount>=ITERATIVE_LOOP_MAX) break;
	}

	for (i=0;i<bcount;i++) blks[i].chosen_length=find_nth_block_dest(blks,bcount,i,best_indexes[i]);
	process_block(shadow_map,data,start_off,block_length,i_map,blks,bcount,zmap,1);
	free(trial_indexes);
	free(best_indexes);
}

static z_codeword *create_codeword_map(ushort *shadow_map,const uchar *data,uint start_off,uint end_off,uint *ret_length,z_huff_map *zmap)
{
	uint i,j,maxlen,blen;
	z_itemdata *i_map;
	z_codeword *c_map;
	
	maxlen=(end_off-start_off);
	i_map=malloc(maxlen*sizeof(z_itemdata));
	bzero(i_map,maxlen*sizeof(z_itemdata));
	/* worst case mem use */
	c_map=malloc((maxlen+1)*sizeof(z_codeword));
	bzero(c_map,(maxlen+1)*sizeof(z_codeword));

	i=0;
	while (i<maxlen)
	{
		/* optimization for big trivial blocks */
		j=0;
		if (start_off+i>=1) while (i+j<maxlen && data[start_off+i-1]==data[start_off+i+j]) j++;
		while (j>=258 || (i+j==maxlen && j>=3))
		{
			i_map[i].length=(j>=258)?258:j;
			i_map[i].dist=1;
			i++;
			j--;
		}
		if (i==maxlen) break;
		find_best_repeat_block(i_map+i,shadow_map,data,start_off+i,end_off,zmap);
		i++;
	}

/*	if (!start_off)
	{
		for (i=0;i<maxlen;i++)
		{
			INFO_LOG("%02x:",i_map[i].length);
			if ((i&15)==15) INFO_LOG("\n");
		}
		DBG_ASSERT(1,"\n");
	}*/

	/* now we have all the possible variations of the length codes.
	   Find out the best way to use them.
	*/

	for (i=0;i<maxlen;i++)
	{
		if (i_map[i].length!=1) for (j=0;j<i_map[i].length;j++)
		{
			DBG_ASSERT(i+j>maxlen,"posh\n");
			i_map[i+j].coverage++;
		}
	}

	blen=0;
	for (i=0;i<maxlen;i+=blen)
	{
		if (i_map[i].coverage)
		{
			blen=i_map[i].length;
			while (i+blen<maxlen && (i_map[i+blen].coverage>1 || (i_map[i+blen].coverage!=0 && i_map[i+blen].length==1))) blen++;
		} else blen=1;

		DBG_ASSERT(blen==2,"Wrong pattern length!\n");

		if (blen==1)
		{
			/* literal */
			i_map[i].c_length=1;
			i_map[i].c_dist=0;
		} else if (blen==i_map[i].length) {
			/* trivial case */
			i_map[i].c_length=i_map[i].length;
			i_map[i].c_dist=i_map[i].dist;
			if (blen==3)
			{
				uint bcost,lcost;

				bcost=calculate_block_saving(data,start_off+i,3,i_map[i].dist,zmap,1);
				lcost=calculate_block_saving(data,start_off+i,1,0,zmap,1) +
					calculate_block_saving(data,start_off+i+1,1,0,zmap,1) +
					calculate_block_saving(data,start_off+i+2,1,0,zmap,1);
				if (bcost>lcost)
				{
					i_map[i].c_length=1;
					i_map[i].c_dist=0;
					i_map[i+1].c_length=1;
					i_map[i+1].c_dist=0;
					i_map[i+2].c_length=1;
					i_map[i+2].c_dist=0;
				}
			}
		} else {
			/* nasty case i.e. the usual one */
			uint tmp_len,bcount;
			z_blockentry *blks;
			
			blks=malloc(blen*sizeof(z_blockentry));
			bzero(blks,blen*sizeof(z_blockentry));
			
			bcount=0;
			j=0;
			while(j<blen)
			{
				uint k,m;

				tmp_len=i_map[i+j].length;
				m=0;
				/* this handles blocks that are longer than 258 bytes */
				while (j+m<blen && i_map[i+j+m].length==258) m++;
				if (m) m--;
				k=1;
				while (j+m+k<blen && i_map[i+j+m+k].length<=tmp_len-k) k++;
				if (m)
				{
					blks[bcount].length=m+258;
				} else {
					blks[bcount].length=tmp_len;
				}
				blks[bcount].count=m+k;
				bcount++;
				j+=m+k;
			}

			/* now we have blocks, now minimize */
			minimize_block(shadow_map,data,start_off+i,end_off,i_map+i,blks,bcount,zmap);
			
			free(blks);
		}
	}

	i=0;
	j=0;
	while (i<maxlen)
	{
		uint tmp_len,tmp_dist;

		tmp_len=i_map[i].c_length;
		tmp_dist=i_map[i].c_dist;

		/* encode codeword(s) */
		if (tmp_len==1)
		{
			/* it is literal */
			c_map[j].type=CW_LITERAL;
			c_map[j].lit=data[start_off+i];
			encode_codeword(c_map+j);
			j++;
		} else if (tmp_len==2) {
			c_map[j].type=CW_LITERAL;
			c_map[j].lit=data[start_off+i];
			encode_codeword(c_map+j);
			j++;
			c_map[j].type=CW_LITERAL;
			c_map[j].lit=data[start_off+i+1];
			encode_codeword(c_map+j);
			j++;
		} else {
			/* it is repeat block */
			c_map[j].type=CW_LENGTH;
			c_map[j].ld=tmp_len;
			encode_codeword(c_map+j);
			j++;
			c_map[j].type=CW_DISTANCE;
			c_map[j].ld=tmp_dist;
			encode_codeword(c_map+j);
			j++;
		}

		i+=tmp_len;
	}

	c_map[j].type=CW_END;
	encode_codeword(c_map+j);
	*ret_length=j+1;

	free(i_map);
	return c_map;
}


static int huff_item_compare(const void *_a, const void *_b)
{
	const z_huff_item *a=_a;
	const z_huff_item *b=_b;

	if (a->count>b->count) return -1;
	if (a->count<b->count) return 1;
	if (a->code>b->code) return 1;
	if (a->code<b->code) return -1;
	DBG_ASSERT(1,"Double code symbol\n");
	return 0;
}

static int huff_item_compare_blen(const void *_a, const void *_b)
{
	const z_huff_item *a=_a;
	const z_huff_item *b=_b;

	if (!a->bit_length && b->bit_length) return 1;
	if (!b->bit_length && a->bit_length) return -1;
	if (a->bit_length>b->bit_length) return 1;
	if (a->bit_length<b->bit_length) return -1;
	if (a->code>b->code) return 1;
	if (a->code<b->code) return -1;
	DBG_ASSERT(1,"Double code symbol\n");
	return 0;
}

static int huff_item_compare_unsort(const void *_a, const void *_b)
{
	const z_huff_item *a=_a;
	const z_huff_item *b=_b;

	if (a->code>b->code) return 1;
	if (a->code<b->code) return -1;
	DBG_ASSERT(1,"Double code symbol\n");
	return 0;
}

static uint bit_rotate(uint value,uint bits)
{
	/* huffman codes are MSB first -> rotate bits */
	uint retval,i;

	if (bits==1) return value;

	retval=0;
	for (i=0;i<bits;i++)
	{
		retval<<=1;
		if (value&1) retval|=1;
		value>>=1;
	}
	return retval;
}


static void calculate_raw_cost_map(uchar *data,uint start_offset,uint end_offset,z_huff_map *h_map)
{
	uint i,len,cost;
	
	len=end_offset-start_offset;
	h_map->raw_cost_map=malloc(end_offset*sizeof(uint));
	bzero(h_map->raw_cost_map,end_offset*sizeof(uint));

	cost=0;
	for (i=0;i<len;i++)
	{
		cost+=h_map->items[data[start_offset+i]].bit_length;
		h_map->raw_cost_map[start_offset+i]=cost;
	}
}

static dfloat calculate_lc_per_budget(uint count,uint bits)
{
	return (dfloat)(count)/(dfloat)(1<<(15-bits));
}

static uint optimize_huffman_coeffs(z_codeword *c,z_huff_item *items,uint filter,uint num_items,uint *ret_items,uint map_size,uint max_bit_len,uint smooth_level)
{
	uint i,retries,symval,totlen,real_num_items;
	int budget,im_done;

	totlen=0;

	real_num_items=0;
	for (i=0;i<num_items;i++)
	{
		if (filter==c[i].type || (filter==CW_LITERAL && (c[i].type==CW_LENGTH || c[i].type==CW_END)))
		{
			items[c[i].code].count++;
			totlen+=c[i].num_extra_bits;
			real_num_items++;
		}
	}

	DBG_ASSERT(!real_num_items && filter!=CW_DISTANCE,"Zero items in huffman table creation\n");
	if (!real_num_items && filter==CW_DISTANCE)
	{
		/*INFO_LOG("Literal only map\n");*/
		*ret_items=1;
		items[0].bit_length=0;
		return 0;
	}

	for (i=0;i<map_size;i++)
	{
		/* set code num */
		items[i].code=i;

		/* these two variables are stored into the struct for debugging purposes only */

		/* item weight */
		items[i].w=((dfloat)items[i].count)/((dfloat)(real_num_items));
		/* the optimal length in bits */
		items[i].opt_length=-log2l(items[i].w);

		/* pessimistic rounding */
		items[i].bit_length=ceill(items[i].opt_length);
		if (!items[i].bit_length && items[i].count) items[i].bit_length=1;
		if (items[i].bit_length>max_bit_len) items[i].bit_length=max_bit_len;

		/* saving for decreasing bit length */
		if (items[i].count && items[i].bit_length!=1) items[i].lc_per_budget=calculate_lc_per_budget(items[i].count,items[i].bit_length);
			else items[i].lc_per_budget=0;
	}
	
	/* sort data */
	qsort(items,map_size,sizeof(z_huff_item),huff_item_compare);

	/* now the difficult piece. optimize the mathematically correct
	   lengths into best possible huffman lengths

	   the maximum lenth of a symbol defined is 15 bits. use this to construct our budget
	   i.e. budget is 32768
	   1 bit symbol eats 16384 out of it
	   2 bit symbol eats 8192
	   ...
	   8 bit symbol eats 128
	   ...
	   15 bit symbol eats 1
	*/

	budget=32768;
	for (i=0;i<map_size;i++)
	{
		if (!items[i].count) break;
		budget-=1<<(15-items[i].bit_length);
	}
	DBG_ASSERT(budget<0,"Pessimistic budget is below zero (%d)\n",budget);
	

	/* completely remove bit lengths we can */
	if (budget && smooth_level)
	{
		uint maxb,j;

		maxb=0;
		for (i=0;i<map_size;i++)
		{
			if (items[i].bit_length>maxb) maxb=items[i].bit_length;
		}
		
		for (j=maxb;j>=smooth_level;j--)
		{
			int decr;

			decr=0;
			for (i=0;i<map_size;i++)
			{
				if (items[i].bit_length==j) decr+=1<<(15-items[i].bit_length);
			}
			
			if (decr<budget)
			{
				for (i=0;i<map_size;i++)
				{
					if (items[i].bit_length==j) items[i].bit_length=j-1;
				}
				budget-=decr;
			} else break;
		}
	}

	/* now spread goodwill (==leftover budget) to the possible receivers starting from top */
	retries=65536;
	im_done=0;
	while (budget && retries--)
	{
		dfloat best_saving;
		int best_index;

		best_saving=0;
		best_index=-1;
		for (i=0;i<map_size;i++)
		{
			if (!items[i].count) break;
			if (items[i].lc_per_budget>best_saving)
			{
				if (budget<(1<<(15-items[i].bit_length))/* || items[i].decremented*/)
				{
					items[i].lc_per_budget=0;
				} else {
					best_saving=items[i].lc_per_budget;
					best_index=i;
				}
			}
		}
		if (best_index>=0)
		{
			budget-=1<<(15-items[best_index].bit_length);
			items[best_index].decremented++;
			items[best_index].bit_length--;
			items[best_index].lc_per_budget=calculate_lc_per_budget(items[best_index].count,items[best_index].bit_length);
		}

	}

	DBG_ASSERT(budget&&!(budget==16384 && i==1),"Fixed budget is not zero (%d)\n",budget);

	/* sort data again */
	qsort(items,map_size,sizeof(z_huff_item),huff_item_compare_blen);


	/* calculate symbol values and update totlen */
	symval=0;
	for (i=0;i<map_size;i++)
	{
		if (!items[i].bit_length) break;
		items[i].symbol=bit_rotate(symval>>(15-items[i].bit_length),items[i].bit_length);
		symval+=1<<(15-items[i].bit_length);
		totlen+=items[i].count*items[i].bit_length;
	}

	/* unsort data */
	qsort(items,map_size,sizeof(z_huff_item),huff_item_compare_unsort);

	*ret_items=real_num_items;
	return totlen;
}

static void create_static_huffman_map(z_huff_map *h)
{
	uint i;

	bzero(h,sizeof(z_huff_map));
	h->type=CB_STATIC;

	for (i=0;i<Z_ABET;i++)
	{
		h->items[i].code=i;
		if (i<144)
		{
			h->items[i].bit_length=8;
			h->items[i].symbol=bit_rotate(i+48,8);
		} else if (i<256) {
			h->items[i].bit_length=9;
			h->items[i].symbol=bit_rotate(i+256,9);	/* 400-144 */
		} else if (i<280) {
			h->items[i].bit_length=7;
			h->items[i].symbol=bit_rotate(i-256,7);
		} else {
			h->items[i].bit_length=8;
			h->items[i].symbol=bit_rotate(i-88,8);	/* 280-192 */
		}
	}

	for (i=0;i<Z_DIST;i++)
	{
		h->distances[i].code=i;
		h->distances[i].bit_length=5;
		h->distances[i].symbol=bit_rotate(i,5);
	}
}

static uint calculate_static_huffman_map(z_huff_map *h,z_codeword *c,uint c_length)
{
	uint i,len;

	len=0;
	h->num_tot_items=c_length;
	for (i=0;i<c_length;i++)
	{
		switch(c[i].type)
		{
			case CW_LENGTH:
			len+=c[i].num_extra_bits;
			case CW_LITERAL:
			case CW_END:
			len+=h->items[c[i].code].bit_length;
			h->items[c[i].code].count++;
			break;
			
			case CW_DISTANCE:
			len+=c[i].num_extra_bits+5;
			h->distances[c[i].code].count++;
			break;

			default:
			DBG_ASSERT(1,"Unknown type for static huffman map\n");
			break;
		}
	}
	len+=1+2;
	
	return len;
}

static uint create_best_huffman_map(z_huff_map *h,z_codeword *c,uint c_length,int do_data_map,uint smooth_level)
{
	uint i,j,totlen,tmp_table_len,tmp_table_len2,enc_data_len,tmp_len;
	uchar tmp_table[Z_ABET+Z_DIST];

	if (do_data_map)
	{
		bzero(h,sizeof(z_huff_map));
	} else {
		z_huff_item tmp_items[Z_ABET];
		memcpy(tmp_items,h->items,sizeof(tmp_items));
		bzero(h,sizeof(z_huff_map));
		memcpy(h->items,tmp_items,sizeof(tmp_items));
	}
	h->type=CB_DYNAMIC;

	totlen=0;
	h->num_tot_items=c_length;

	/* main data huffman table */
	if (do_data_map)
	{
		totlen+=optimize_huffman_coeffs(c,h->items,CW_LITERAL,c_length,&tmp_len,Z_ABET,15,smooth_level);
		h->num_lit_items=tmp_len;
	} else {
		tmp_len=0;
		for (i=0;i<c_length;i++)
		{
			if (c[i].type==CW_LITERAL || c[i].type==CW_LENGTH || c[i].type==CW_END)
			{
				totlen+=h->items[c[i].code].bit_length;
				totlen+=c[i].num_extra_bits;
				tmp_len++;
			}
		}
		h->num_lit_items=tmp_len;
	}

	/* distance huffman table */
	totlen+=optimize_huffman_coeffs(c,h->distances,CW_DISTANCE,c_length,&tmp_len,Z_DIST,7,0);
	h->num_dist_items=tmp_len;

	/* now calculate encoded huffman representation */

	tmp_table_len=0;
	for (i=0;i<Z_ABET;i++)
	{
		tmp_table[i]=h->items[i].bit_length;
		if (tmp_table[i]) tmp_table_len=i+1;
	}
	h->table_lit_length=tmp_table_len;
	tmp_table_len2=0;
	for (i=0;i<Z_DIST;i++)
	{
		tmp_table[i+tmp_table_len]=h->distances[i].bit_length;
		if (tmp_table[i+tmp_table_len]) tmp_table_len2=i+1;
	}
	if (!tmp_table_len2) tmp_table_len2++;
	h->table_dist_length=tmp_table_len2;
	tmp_table_len+=tmp_table_len2;

	/*for (i=0;i<tmp_table_len;i++)
	{
		INFO_LOG("%02x(%02x):",tmp_table[i],(i<tmp_table_len-tmp_table_len2)?h->items[i].count:h->distances[i-tmp_table_len+tmp_table_len2].count);
		if ((i&15)==15) INFO_LOG("\n");
	}
	INFO_LOG("\n");*/

	enc_data_len=0;
	for (i=0;i<tmp_table_len;i+=j)
	{
		/* RLE */

		j=i;
		while (tmp_table[i]==tmp_table[j] && (++j)<tmp_table_len);
		if (j==tmp_table_len) j--;
		j-=i;
		if (!j) j=1;

		if (tmp_table[i])
		{
			h->table_enc_data[enc_data_len].type=CW_TABLE;
			h->table_enc_data[enc_data_len].ld=1;
			h->table_enc_data[enc_data_len].lit=tmp_table[i];
			encode_codeword(h->table_enc_data+enc_data_len);
			enc_data_len++;

			if (j>=4)
			{
				uint k;

				k=1;
				while (k!=j)
				{
					h->table_enc_data[enc_data_len].type=CW_TABLE;
					h->table_enc_data[enc_data_len].lit=tmp_table[i];
					if (j-k<=6)
					{
						h->table_enc_data[enc_data_len].ld=j-k;
						k+=j-k;
					} else if (j-k<9) {
						h->table_enc_data[enc_data_len].ld=j-k-3;
						k+=j-k-3;
					} else {
						h->table_enc_data[enc_data_len].ld=6;
						k+=6;
					}
					encode_codeword(h->table_enc_data+enc_data_len);
					enc_data_len++;
				}
			} else j=1;
		} else {
			if (j>=3)
			{
				uint k;

				k=0;
				while (k!=j)
				{
					h->table_enc_data[enc_data_len].type=CW_TABLE;
					h->table_enc_data[enc_data_len].lit=0;
					if (j-k<=138)
					{
						h->table_enc_data[enc_data_len].ld=j-k;
						k+=j-k;
					} else if (j-k<(138+3)) {
						h->table_enc_data[enc_data_len].ld=j-k-3;
						k+=j-k-3;
					} else {
						h->table_enc_data[enc_data_len].ld=138;
						k+=138;
					}
					encode_codeword(h->table_enc_data+enc_data_len);
					enc_data_len++;
				}
			} else {
				h->table_enc_data[enc_data_len].type=CW_TABLE;
				h->table_enc_data[enc_data_len].lit=0;
				h->table_enc_data[enc_data_len].ld=1;
				encode_codeword(h->table_enc_data+enc_data_len);
				enc_data_len++;
				j=1;
			}
		}
	}
	h->table_enc_data_length=enc_data_len;

	/* huffman table of huffman table */
	totlen+=optimize_huffman_coeffs(h->table_enc_data,h->table,CW_TABLE,h->table_enc_data_length,&tmp_len,Z_TAB,7,0);


	h->table_lengths_length=0;
	for (i=0;i<Z_TAB;i++)
	{
		h->table_lengths[i]=h->table[huffman_lengths_order[i]].bit_length;
		if (h->table_lengths[i]) h->table_lengths_length=i+1;
	}

	totlen+=1+2+5+5+4+h->table_lengths_length*3;

	DBG_ASSERT(h->table_lit_length<257 || h->table_lit_length>286,"Wrong length of literals: %d\n",h->table_lit_length);
	DBG_ASSERT(h->table_dist_length<1 || h->table_dist_length>32,"Wrong length of distances: %d\n",h->table_dist_length);
	DBG_ASSERT(h->table_lengths_length<4 || h->table_lengths_length>19,"Wrong length of bit lengths %d\n",h->table_lengths_length);

	return totlen;
}

static uint write_bits(uchar *dest,uint bindex,uint data,uint bits)
{
	uint bit_i,byte_i,mask;

	if (bits>8)
	{
		bindex=write_bits(dest,bindex,data&0xff,8);
		data>>=8;
		bits-=8;
	}
	bit_i=bindex&7;
	byte_i=bindex>>3;
	mask=(1<<bits)-1;
	dest[byte_i]|=((data&mask)<<bit_i)&0xff;
	if (bit_i+bits>8) dest[byte_i+1]|=((data&mask)<<bit_i)>>8;
	return bindex+bits;
}

#ifdef DEBUG_BLOCKS
static void write_debug(z_codeword *c,uchar *data,uint i)
{
	static uint counter=0;
	uint tmp_ld=c[i].ld;
	uint orig_ld=tmp_ld;
	do
	{
		int nf=0;

		if (c[i].type==CW_LENGTH)
		{
			if (tmp_ld==orig_ld) INFO_LOG("+  ");
				else INFO_LOG("-  ");
//			INFO_LOG("%02x",data[counter]);
			counter++;
			if (tmp_ld) tmp_ld--;
		} else if (c[i].type==CW_LITERAL) {
			INFO_LOG(".");
			INFO_LOG("%02x",data[counter]);
			counter++;
			tmp_ld=0;
		} else {
			tmp_ld=0;
			nf=1;
		}
		if (!nf && !(counter&15))
		{
			INFO_LOG("\n");
		}
	} while (tmp_ld);
}
#endif

static uint write_compressed_data(z_huff_map *h,z_codeword *c,uchar *dest,uint pos,int is_final,int apple_gzip)
{
	uint i;
	
	pos=write_bits(dest,pos,(is_final?1:0),1);
	pos=write_bits(dest,pos,h->type,2);
	if (h->type==CB_RAW)
	{
		uint tpos;
		
		tpos=pos&7;
		if (tpos) pos=write_bits(dest,pos,0,8-tpos);
	}
	if (h->type==CB_DYNAMIC)
	{
		pos=write_bits(dest,pos,h->table_lit_length-257,5);
		pos=write_bits(dest,pos,h->table_dist_length-1,5);
		pos=write_bits(dest,pos,h->table_lengths_length-4,4);
		for (i=0;i<h->table_lengths_length;i++)
		{
			pos=write_bits(dest,pos,h->table_lengths[i],3);
		}
		for (i=0;i<h->table_enc_data_length;i++)
		{
				pos=write_bits(dest,pos,h->table[h->table_enc_data[i].code].symbol,h->table[h->table_enc_data[i].code].bit_length);
				if (h->table_enc_data[i].num_extra_bits) pos=write_bits(dest,pos,h->table_enc_data[i].extra_bits,h->table_enc_data[i].num_extra_bits);
		}
	}
	for (i=0;i<h->num_tot_items;i++)
	{
		if (is_final && c[i].type==CW_END)
		{
			/* write final end only if there is a space for it in the current byte*/
			/* mavericks  (a.k.a apple gzip 2) needs this in any case */
			if (!apple_gzip && (!(pos&7) || (pos&7)+h->items[c[i].code].bit_length+c[i].num_extra_bits>8)) break;
		}
		if (c[i].type!=CW_DISTANCE)
		{
			pos=write_bits(dest,pos,h->items[c[i].code].symbol,h->items[c[i].code].bit_length);
		} else {
			pos=write_bits(dest,pos,h->distances[c[i].code].symbol,h->distances[c[i].code].bit_length);
		}
		if (c[i].num_extra_bits) pos=write_bits(dest,pos,c[i].extra_bits,c[i].num_extra_bits);
	}
	return pos;
}

static void free_huffman_map(z_huff_map *h_map)
{
	if (h_map)
	{
		free(h_map->raw_cost_map);
	}
}

static z_codeword *compress_with_static(ushort *shadow_map,uchar *data,uint start_offset,uint end_offset,uint *ret_len,z_huff_map *h_map,uint __attribute__((unused)) dummy)
{
	uint cwmap_len,len;
	z_codeword *c_map;

	create_static_huffman_map(h_map);
	calculate_raw_cost_map(data,start_offset,end_offset,h_map);
	c_map=create_codeword_map(shadow_map,data,start_offset,end_offset,&cwmap_len,h_map);
	INFO_LOG(".");
	len=calculate_static_huffman_map(h_map,c_map,cwmap_len);
	*ret_len=len;
	return c_map;
}

static z_codeword *compress_with_dynamic(ushort *shadow_map,uchar *data,uint start_offset,uint end_offset,uint *ret_len,z_huff_map *h_map,uint smooth_level)
{
	uint i,cwmap_len,len,best_len;
	z_codeword *c_map,*c_best;
	z_huff_map h_best;

	c_best=create_codeword_map(shadow_map,data,start_offset,end_offset,&cwmap_len,0);
	best_len=create_best_huffman_map(h_map,c_best,cwmap_len,1,smooth_level);
	calculate_raw_cost_map(data,start_offset,end_offset,h_map);
	memcpy(&h_best,h_map,sizeof(z_huff_map));
	INFO_LOG(".");
	for (i=0;i<4;i++)
	{
		c_map=create_codeword_map(shadow_map,data,start_offset,end_offset,&cwmap_len,h_map);
		len=create_best_huffman_map(h_map,c_map,cwmap_len,1,smooth_level);
		calculate_raw_cost_map(data,start_offset,end_offset,h_map);
		if (len<best_len)
		{
			free_huffman_map(&h_best);
			memcpy(&h_best,h_map,sizeof(z_huff_map));
			best_len=len;
			free(c_best);
			c_best=c_map;
		} else free(c_map);
		INFO_LOG(".");
	}
	memcpy(h_map,&h_best,sizeof(z_huff_map));
	*ret_len=best_len;
	return c_best;
}

static z_codeword *compress_with_nocompress(ushort __attribute__((unused)) *shadow_map,uchar *data,uint start_offset,uint end_offset,uint *ret_len,z_huff_map *h_map,uint __attribute__((unused)) dummy)
{
	uint i,len;
	z_codeword *c_map;

	len=end_offset-start_offset;

	/* h_map must be present... */
	bzero(h_map,sizeof(z_huff_map));
	if (len>65535)
	{
		/* not supported due to the fact how I made top-level architecture.
		   this should anyway compress so discourage strongly to use this
		*/
		*ret_len=len*16;
		return 0;
	}

	h_map->type=CB_RAW;
	h_map->num_tot_items=len+4;
	for (i=0;i<256;i++)
	{
		h_map->items[i].bit_length=8;
		h_map->items[i].symbol=i;
	}

	c_map=malloc((len+4)*sizeof(z_codeword));
	bzero(c_map,(len+4)*sizeof(z_codeword));

	c_map[0].type=CW_RAW;
	c_map[0].lit=(len)&0xff;
	encode_codeword(c_map);
	c_map[1].type=CW_RAW;
	c_map[1].lit=(len>>8)&0xff;
	encode_codeword(c_map+1);
	c_map[2].type=CW_RAW;
	c_map[2].lit=(~len)&0xff;
	encode_codeword(c_map+2);
	c_map[3].type=CW_RAW;
	c_map[3].lit=(~(len>>8))&0xff;
	encode_codeword(c_map+3);
	for (i=0;i<len;i++)
	{
		c_map[i+4].type=CW_RAW;
		c_map[i+4].lit=data[start_offset+i];
		encode_codeword(c_map+i+4);
	}
	INFO_LOG(".");
	*ret_len=1+2+7+(len+4)*8;	/* we cant know byte orientation yet, thus +7 i.e. pessimistic */
	return c_map;
}

static compress_workload *process_compress_workload(compress_func cf,ushort *shadow_map,uchar *data,uint start_offset,uint end_offset,uint extra)
{
	compress_workload *ret;
	
	ret=malloc(sizeof(compress_workload));
	bzero(ret,sizeof(compress_workload));

	uint ret_len;
	ret->h_map=malloc(sizeof(z_huff_map));
	ret->extra=extra;
	ret->start_offset=start_offset;
	ret->end_offset=end_offset;
	ret->c_map=cf(shadow_map,data,start_offset,end_offset,&ret_len,ret->h_map,extra);
	ret->length=ret_len;

	return ret;
}

static void delete_workitem(compress_workload *w)
{
	free_huffman_map(w->h_map);
	free(w->h_map);
	free(w->c_map);
}

/*
static const char *unclean_hdr="HOME=/tmp/Z;cp $0 ~;sed 1d $0|zcat>~\n";
static const char *unclean_hdr_gzip_insert="\n~\nfi\n";
*/

/*
static const char *unclean_hdr="cp $0 /tmp/Z;(sed 1d $0|zcat)>$_;$_\n";
static const char *unclean_hdr_gzip_insert="\nfi\n\4\1";
*/

/*
static const char *unclean_hdr="cp $0 /tmp/z;((sed 1d $0|zcat)>$_\n";
static const char *unclean_hdr_gzip_insert=");$_\n)";
*/

//static const char *unclean_hdr="cp $0 /tmp/z;(sed 1d $0|zcat\n";
//static const char *unclean_hdr_gzip_insert=")>$_;$_\n)\n";

/*
static const char *unclean_hdr="HOME=/tmp/z;cp $0 ~;sed 1d $0|zcat>~\n";
static const char *unclean_hdr_gzip_insert="\n~\n)\n";
*/


//static const char *unclean_nospace_hdr="z=/tmp/z;cp $0 $z;sed 1d $0|zcat>$z\n";
//static const char *unclean_nospace_hdr_gzip_insert="\n$z\n)\n";

//static const char   *unclean_space_hdr="z=/tmp/z;cp \"$0\" $z;sed 1d \"$0\"|zcat>$z\n";
//static const char   *unclean_space_hdr_gzip_insert="\n$z\n)\n";

static const char *unclean_nospace_hdr="cp $0 /tmp/z;(sed 1d $0|zcat\n";
static const char *unclean_nospace_hdr_gzip_insert=")>$_;$_\n)\n";

static const char   *unclean_space_hdr="cp \"$0\" /tmp/z;(sed 1d \"$0\"|zcat\n";
static const char   *unclean_space_hdr_gzip_insert=")>$_;$_\n)\n";

/*
static const char *unclean_hdr="a=/tmp/Z;tail -n+2 $0|zcat>$a\n";
static const char *unclean_hdr_gzip_insert="\nchmod +x $a;$a\n";

static const char *unclean_hdr="HOME=/tmp/Z;cp $0 ~&&zsh ~;zcat<<''>~;~\n";
static const char *unclean_hdr_gzip_insert="\nTDA\n\1"; * Amiga rules *

static const char *unclean_hdr="HOME=/tmp/Z;cp $0 ~;tail -n+2 $0|zcat>~\n";
static const char *unclean_hdr_gzip_insert="\n\n~\n\n\1";
*/

/*
static const char   *clean_hdr="HOME=/tmp/Z;cp $0 ~&&zsh ~||zcat<<''>~;~;rm ~\n";
static const char   *clean_hdr="HOME=/tmp/Z;tail -n+2 $0|zcat>~;chmod +x ~;~;rm ~;exit\n";
static const char   *clean_hdr_gzip_insert="\nTDA\n\1"; * Amiga rules *

static const char   *clean_hdr="HOME=/tmp/Z;cp $0 ~;tail -n+2 $0|zcat>~;~;rm ~\n";
static const char   *clean_hdr_gzip_insert="\nexit\n";

static const char   *clean_hdr="HOME=/tmp/Z;tail -n+2 $0|zcat>~;chmod +x ~;~;rm ~\n";
static const char   *clean_hdr_gzip_insert="\nexit\n";

static const char   *clean_hdr="#!/bin/zsh\nHOME=/tmp/Z;cp $0 ~;zcat<<''>~;~;rm ~\n";
static const char   *clean_hdr_gzip_insert="\nTDA\n\1"; * Amiga rules *
*/

//static const char   *clean_hdr="HOME=/tmp/Z;cp $0 ~;sed 1d $0|zcat>~;~\n";

static const char   *clean_nospace_hdr="HOME=/tmp/Z;cp $0 ~;sed 1d $0|zcat>~;~\n";
static const char   *clean_nospace_hdr_gzip_insert="\nrm ~\n)\n";

static const char     *clean_space_hdr="HOME=/tmp/Z;cp \"$0\" ~;sed 1d \"$0\"|zcat>~;~\n";
static const char     *clean_space_hdr_gzip_insert="\nrm ~\n)\n";

static uint stub_and_gzip_header_insert(uchar *dest,int clean_stub,int has_spaces)
{
	uint chosen_hdr_len,chosen_insert_len;
	gziphdr *gzhdr;
	const char *chosen_hdr;
	const char *chosen_hdr_gzip_insert;

	if (!has_spaces)
	{
		chosen_hdr=(clean_stub)?clean_nospace_hdr:unclean_nospace_hdr;
		chosen_hdr_gzip_insert=(clean_stub)?clean_nospace_hdr_gzip_insert:unclean_nospace_hdr_gzip_insert;
	} else {
		chosen_hdr=(clean_stub)?clean_space_hdr:unclean_space_hdr;
		chosen_hdr_gzip_insert=(clean_stub)?clean_space_hdr_gzip_insert:unclean_space_hdr_gzip_insert;
	}
	chosen_hdr_len=(uint)strlen(chosen_hdr);
	chosen_insert_len=(uint)strlen(chosen_hdr_gzip_insert);
	chosen_insert_len=(chosen_insert_len>=6)?chosen_insert_len-6:0;
	if (chosen_insert_len) chosen_insert_len++; /* must have null at the end */
	memcpy(dest,chosen_hdr,chosen_hdr_len);

	gzhdr=(void*)(dest+chosen_hdr_len);
	gzhdr->magic=0x8b1f;
	gzhdr->method=8;
	gzhdr->flags1=(chosen_insert_len)?16:0;
	memcpy(gzhdr->mtime,chosen_hdr_gzip_insert,6);
	if (chosen_insert_len) memcpy(dest+chosen_hdr_len+(uint)sizeof(gziphdr),chosen_hdr_gzip_insert+6,chosen_insert_len);

	return chosen_hdr_len+(uint)sizeof(gziphdr)+chosen_insert_len;
}

void *kompura(uchar *data,uint len,uint *linker_blocks,uint *retlen,int apple_gzip,int clean_stub,int has_spaces)
{
	uint i,hdr_len,tmp_len,lb_count,pos;
	compress_workload **w;
	uchar *comp_data;
	ushort *shadow_map;

	/*bzero(hash_table,sizeof(hash_table));*/

	ASSERT(!len,"Zero length provided for compressor\n");
	tmp_len=0;
	lb_count=0;
	while(linker_blocks[lb_count])
	{
		tmp_len+=linker_blocks[lb_count];
		lb_count++;
	}
	ASSERT(tmp_len!=len,"Linker_blocks do not match total length\n");

	shadow_map=malloc(len*sizeof(ushort));
	bzero(shadow_map,len*sizeof(ushort));

	w=malloc(lb_count*lb_count*sizeof(compress_workload*));
	bzero(w,lb_count*lb_count*sizeof(compress_workload*));

	INFO_LOG("Compressing..");

	if (len>3) for (i=1;i<=len-3;i++)
	{
		uint j;
	
		j=i-1;
		while(i-j<=32768)
		{
			if (data[i]==data[j] && data[i+1]==data[j+1] && data[i+2]==data[j+2])
			{
				shadow_map[i]=i-j;
				break;
			}
			if (!j) break;
			j--;
		}
	}
#define CW_BLOCK_ITEMS (8)

	uint witems=lb_count*lb_count*CW_BLOCK_ITEMS;
	w=malloc(witems*sizeof(compress_workload*));
	memset(w,0,witems*sizeof(compress_workload*));


	dispatch_apply(witems,dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0),^(size_t x){
		uint i_blk;
		i_blk=(uint)x/(lb_count*CW_BLOCK_ITEMS);
		i_blk++;
		uint j;
		j=(uint)x%(lb_count*CW_BLOCK_ITEMS);
		j/=CW_BLOCK_ITEMS;

		uint k,blen,tmp_len_blk;
		k=0;
		tmp_len_blk=0;
		while (k<j)
		{
			tmp_len_blk+=linker_blocks[k];
			k++;
		}
		k=j;
		blen=0;
		while (k<lb_count && k<j+i_blk)
		{
			blen+=linker_blocks[k];
			k++;
		}
		if (k==j+i_blk)
		{
			static const compress_func cf_map[CW_BLOCK_ITEMS]={compress_with_nocompress,compress_with_static,
				compress_with_dynamic,compress_with_dynamic,compress_with_dynamic,
				compress_with_dynamic,compress_with_dynamic,compress_with_dynamic};
			static const uint extra_map[CW_BLOCK_ITEMS]={2,1,0,14,13,12,11,10};
			uint r=(uint)x%CW_BLOCK_ITEMS;
			
			w[x]=process_compress_workload(cf_map[r],shadow_map,data,tmp_len_blk,tmp_len_blk+blen,extra_map[r]);
		}
	});

	for (i=0;i<witems/CW_BLOCK_ITEMS;i++) if (w[i*CW_BLOCK_ITEMS])
	{
		uint j;
		for (j=1;j<CW_BLOCK_ITEMS;j++)
		{
			if (w[i*CW_BLOCK_ITEMS]->length>w[i*CW_BLOCK_ITEMS+j]->length)
			{
				compress_workload *tmp;
				
				tmp=w[i*CW_BLOCK_ITEMS];
				w[i*CW_BLOCK_ITEMS]=w[i*CW_BLOCK_ITEMS+j];
				w[i*CW_BLOCK_ITEMS+j]=tmp;
			}
		}
	}

	INFO_LOG("\n");

	/* header */
	comp_data=malloc(len*2*sizeof(uchar));
	bzero(comp_data,len*2*sizeof(uchar));
	hdr_len=stub_and_gzip_header_insert(comp_data,clean_stub,has_spaces);
	pos=0;

	/* lovely, now we just need to find the best combination... */
	{
		uint best_length;	       
		uint comb[lb_count];       
		uint best_comb[lb_count];

		best_length=~0U;
		bzero(comb,sizeof(comb));
		bzero(best_comb,sizeof(best_comb));

		for (;;)
		{
			int done;
			uint tmp_length;

			i=0;
			tmp_length=0;
			while (i<lb_count)
			{
				tmp_length+=w[(comb[i]*lb_count+i)*CW_BLOCK_ITEMS]->length;
				i+=comb[i]+1;
			}

			if (tmp_length<best_length)
			{
				best_length=tmp_length;
				memcpy(best_comb,comb,sizeof(best_comb));
			}

			/* next one */
			i=lb_count-1;
			done=0;
			for(;;)
			{
				comb[i]+=1;
				if (comb[i]==lb_count-i)
				{
					if (!i)
					{
						done=1;
						break;
					}
					comb[i--]=0;
					continue;
				}
				i=0;
				while (i<lb_count)
				{
					i+=comb[i]+1;
				}
				if (i==lb_count) break;
				i=lb_count-1;
			}
			if (done) break;
		}
		
		i=0;
		INFO_LOG("Compression blocks\n");
		while(i<lb_count)
		{
			compress_workload *tmp_w;

			tmp_w=w[(best_comb[i]*lb_count+i)*CW_BLOCK_ITEMS];
			if (tmp_w->extra==1)
			{
				INFO_LOG("STATIC    ");
			} else if (tmp_w->extra==2) {
				INFO_LOG("RAW       ");
			} else {
				INFO_LOG("DYNAM%02d   ",tmp_w->extra);
			}
			{
				float compratio;

				compratio=(float)(tmp_w->length)*100.0f/(float)((tmp_w->end_offset-tmp_w->start_offset)*8);
				INFO_LOG("| Complete block: 0x%08x - 0x%08x (%.1f/%d - %.1f%%)\n",tmp_w->start_offset,tmp_w->end_offset,(float)tmp_w->length/8.0,tmp_w->end_offset-tmp_w->start_offset,compratio);
			}

			{
				uint slength;

				uint cur_pos;
				uint tot_length;
				
				uint k;
				tot_length=tmp_w->end_offset-tmp_w->start_offset;
				
				{
					uint j;
					uint c_len;
					
					c_len=0;
					for (j=0;j<tmp_w->h_map->num_tot_items;j++) {
						z_codeword *c;
						c=tmp_w->c_map;

						if (c[j].type!=CW_DISTANCE)
						{
							c_len+=tmp_w->h_map->items[c[j].code].bit_length;
						} else {
							c_len+=tmp_w->h_map->distances[c[j].code].bit_length;
						}
						c_len+=c[j].num_extra_bits;
					}
					slength=c_len;
				}

				INFO_LOG("          + Header length %.1f\n",(float)(tmp_w->length-slength)/8);

				k=0;
				for (cur_pos=0;cur_pos<tot_length;cur_pos+=linker_blocks[i+(k++)]) {
					uint j;
					uint c_len;
					uint c_pos;
					
					c_len=0;
					c_pos=0;
					if (tmp_w->extra==2) {
						c_len=linker_blocks[i+k]*8;
					} else {
						for (j=0;j<tmp_w->h_map->num_tot_items;j++) {
							z_codeword *c;
							c=tmp_w->c_map;

							if (c_pos>=cur_pos&&c_pos<cur_pos+linker_blocks[i+k]) {
								if (c[j].type!=CW_DISTANCE)
								{
									c_len+=tmp_w->h_map->items[c[j].code].bit_length;
								} else {
									c_len+=tmp_w->h_map->distances[c[j].code].bit_length;
								}
								c_len+=c[j].num_extra_bits;
							}

							if (c[j].type==CW_LITERAL)
							{
								c_pos++;
							} else if (c[j].type==CW_LENGTH) {
								c_pos+=c[j].ld;
							}
						}
					}
					{
						float compratio;

						compratio=(float)(c_len)*100.0f/(float)(linker_blocks[i+k]*8);
						INFO_LOG("          +      sub-block: 0x%08x - 0x%08x (%.1f/%d - %.1f%%)\n",tmp_w->start_offset+cur_pos,tmp_w->start_offset+cur_pos+linker_blocks[i+k],
							(float)c_len/8.0,linker_blocks[i+k],compratio);
					}
				}
			}
			pos=write_compressed_data(tmp_w->h_map,tmp_w->c_map,comp_data+hdr_len,pos,(i+best_comb[i]+1==lb_count)?1:0,apple_gzip);
			i+=best_comb[i]+1;
		}
#ifdef DEBUG_BLOCKS
		i=0;
		while(i<lb_count)
		{
			uint j;
			compress_workload *tmp_w;

			tmp_w=w[(best_comb[i]*lb_count+i)*CW_BLOCK_ITEMS];
			for (j=0;j<tmp_w->h_map->num_tot_items;j++) write_debug(tmp_w->c_map,data,j);
			INFO_LOG("\n");
			i+=best_comb[i]+1;
		}
#endif
	}

	*retlen=((pos+7)>>3)+hdr_len;
	INFO_LOG("Compressed length %d (hdr=%d)\n",*retlen,hdr_len);
	if (!clean_stub || has_spaces)
	{
		INFO_LOG("!!!!!!!!!!\n");
		if (!clean_stub)
		{
			INFO_LOG("Unclean stub chosen. This will leave a file in the /tmp directory when executed!\n");
		}
		if (has_spaces)
		{
			INFO_LOG("whitespace in the filename, larger stub used!\n");
		}
		INFO_LOG("!!!!!!!!!!\n");
	}
	free(shadow_map);
	for (i=0;i<witems;i++) if (w[i]) delete_workitem(w[i]);
	free(w);
	return comp_data;
}

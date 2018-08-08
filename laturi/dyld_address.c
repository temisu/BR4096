/* Copyright (C) Teemu Suutari */

#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/vm_region.h>
#include <mach/vm_map.h>

#include <mach-o/loader.h>

#include "common.h"

#ifndef __x86_64__
#define DYLD_ADDRESS ((void*)0x8fe00000UL)
#else
#define DYLD_ADDRESS ((void*)0x7fff5fc00000UL)
#endif


void dyld_address(LATURI_data *lconf,ulong *dyld_addr,ulong *dyld_slide)
{
	if (lconf->running_osx_version==11)
	{
		kern_return_t kr;

		vm_size_t vmsize;
		vm_address_t address=(ulong)DYLD_ADDRESS;
		vm_region_basic_info_data_64_t info;
		mach_msg_type_number_t info_count;
		memory_object_name_t object;

		info_count=VM_REGION_BASIC_INFO_COUNT_64;
		kr=vm_region_64(mach_task_self(),&address,&vmsize,VM_REGION_BASIC_INFO,(vm_region_info_t)&info,&info_count,&object);
		if (kr == KERN_SUCCESS)
	 	{
			*dyld_addr=address;
			*dyld_slide=address-(ulong)DYLD_ADDRESS;
		} else {
			*dyld_addr=0;
			*dyld_slide=0;
		}
	} else for (vm_address_t base=0;;) {
		kern_return_t kr;
		vm_size_t vmsize;
		vm_address_t address=base;
		vm_region_basic_info_data_64_t info;
		mach_msg_type_number_t info_count;
		memory_object_name_t object;

		info_count=VM_REGION_BASIC_INFO_COUNT_64;
		kr=vm_region_64(mach_task_self(),&address,&vmsize,VM_REGION_BASIC_INFO,(vm_region_info_t)&info,&info_count,&object);
		if (kr == KERN_SUCCESS)
		{
			if (info.protection&1)
			{
				if (*((ulong*)address)==0xfeedfaceU && ((ulong*)address)[1]==7 && ((ulong*)address)[3]==MH_DYLINKER)
				{
					*dyld_addr=address;
					*dyld_slide=address;//address-0x3f000;
					return;
				}
			}
		} else {
			*dyld_addr=0;
			*dyld_slide=0;
			return;
		}
		base+=0x4000;
	}
}

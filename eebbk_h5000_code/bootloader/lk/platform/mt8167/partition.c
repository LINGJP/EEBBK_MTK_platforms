/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stdint.h>
#include <printf.h>
#include <malloc.h>
#include <string.h>
#include <platform/errno.h>
#include <platform/mmc_core.h>
#include <platform/partition.h>
#include <target.h>
#include <platform/partition_fastboot.h>


#define TAG "[PART_LK]"

#define LEVEL_ERR   (0x0001)
#define LEVEL_INFO  (0x0004)

#define DEBUG_LEVEL (LEVEL_ERR | LEVEL_INFO)   

#define part_err(fmt, args...)   \
do {    \
    if (DEBUG_LEVEL & LEVEL_ERR) {  \
        dprintf(CRITICAL, fmt, ##args); \
    }   \
} while (0)

#define part_info(fmt, args...)  \
do {    \
    if (DEBUG_LEVEL & LEVEL_INFO) {  \
        dprintf(CRITICAL, fmt, ##args);    \
    }   \
} while (0)


part_t tempart;
struct part_meta_info temmeta;

part_t *partition;

part_t* get_part(char *name)
{
    part_t *part = partition;
    part_t *ret = NULL;
	
	part_info("%s[%s] %s\n", TAG, __FUNCTION__, name);
	while (part->nr_sects) {
        if (part->info) {
		    if (!strcmp(name, (char *)part->info->name)) {
                memcpy(&tempart, part, sizeof(part_t));
                memcpy(&temmeta, part->info, sizeof(struct part_meta_info));
                tempart.info = &temmeta;
                ret = &tempart;
                break;
            }
        }
		part++;
		
        //part_info("%s[%s] 0x%lx\n", TAG, __FUNCTION__, tempart.start_sect);
	}
    return ret;
}

void put_part(part_t *part)
{
    if (!part) {
        return;
    }

    if (part->info) {
        free(part->info);
    }
    free(part);
}


part_t *mt_part_get_partition(char *name)
{
	if (!strcmp(name, ""))
		return NULL;
	part_dev_t *dev = mt_part_get_device();

    if (!strcmp(name, "PRELOADER") || !strcmp(name, "preloader")) {
        tempart.start_sect = 0x0;
        tempart.nr_sects = 0x200;
		tempart.part_id = dev->blkdev->part_boot1;
        return &tempart;
    }
    return get_part(name);
}

#ifdef MTK_EMMC_SUPPORT
void mt_part_dump(void)
{
    part_t *part = partition;
    part_dev_t *dev = mt_part_get_device();
    
    part_info("Part Info.(1blk=%dB):\n", dev->blkdev->blksz);
    while (part->nr_sects) {
    	 part_info("[0x%016llx-0x%016llx] (%.8ld blocks): \"%s\"\n", 
               (u64)part->start_sect * dev->blkdev->blksz,
              (u64)(part->start_sect + part->nr_sects) * dev->blkdev->blksz - 1,
                               part->nr_sects, (part->info) ? (char *)part->info->name : "unknown");
        part++;
    }
    part_info("\n");
}


#elif defined(MTK_NAND_SUPPORT)

void mt_part_dump(void)
{
    part_t *part = &partition_layout[0];
    part_dev_t *dev = mt_part_get_device();
    
    part_info("\nPart Info from compiler.(1blk=%dB):\n", dev->blkdev->blksz);
    part_info("\nPart Info.(1blk=%dB):\n", dev->blkdev->blksz);
    while (part->name) {
        part_info("[0x%.8x-0x%.8x] (%.8ld blocks): \"%s\"\n", 
               part->start_sect * dev->blkdev->blksz,
              (part->start_sect + part->nr_sects) * dev->blkdev->blksz - 1,
			   part->nr_sects, part->name);
        part++;
    }
    part_info("\n");
}

#else

void mt_part_dump(void)
{
}


#endif


/**/
/*fastboot*/
/**/
unsigned int write_partition(unsigned size, unsigned char *partition)
{
    return 0;
}

int partition_get_index(const char * name)
{
	int index;

	for (index = 0; index < PART_MAX_COUNT; index++) {
		if(index >= (int)(sizeof(g_part_name_map)/sizeof(g_part_name_map[0])) )
		    return -1;
		if (!strcmp(name, g_part_name_map[index].fb_name)) {
			part_info("[%s]find %s %s index %d\n", __FUNCTION__, name, g_part_name_map[index].r_name, g_part_name_map[index].partition_idx);
			return g_part_name_map[index].partition_idx;
		}
	}

	return -1;
}

unsigned int partition_get_region(int index)
{
    part_t *p; 
    if(index >= (int)(sizeof(g_part_name_map)/sizeof(g_part_name_map[0])) )
		return -1;
	
    p = mt_part_get_partition(g_part_name_map[index].r_name);	
    if (p == NULL) {
        return -1;
    }
    return p->part_id;
}   

u64 partition_get_offset(int index)
{
	part_t *p;
	part_dev_t *dev = mt_part_get_device();
    if(index >= (int)(sizeof(g_part_name_map)/sizeof(g_part_name_map[0])) )
		return -1;
	
    p = mt_part_get_partition(g_part_name_map[index].r_name);	
	if (p == NULL) {
		return -1;
    }
	return (u64)p->start_sect * dev->blkdev->blksz;
}

u64 partition_get_size(int index)
{
	part_t *p;
	part_dev_t *dev = mt_part_get_device();
    if(index >= (int)(sizeof(g_part_name_map)/sizeof(g_part_name_map[0])) )
		return -1;
	
	p = mt_part_get_partition(g_part_name_map[index].r_name);	
	if (p == NULL) {
		return -1;
    }
	return (u64)p->nr_sects * dev->blkdev->blksz;
}

int partition_get_type(int index, char **p_type)
{
	*p_type = g_part_name_map[index].partition_type;
	return 0;
}

int partition_get_name(int index, char **p_name)
{
	*p_name = g_part_name_map[index].fb_name;
	return 0;
}

int is_support_erase(int index)
{
	return g_part_name_map[index].is_support_erase;
}

int is_support_flash(int index)
{
	return g_part_name_map[index].is_support_dl;
}
extern int read_gpt(part_t *part);
int part_init(part_dev_t *dev)
{
	part_t *part_ptr;

	dprintf(CRITICAL, "[partition init]\n");

	part_ptr = calloc(128 + 1, sizeof(part_t));
	if (!part_ptr)
		return -1;
	partition = part_ptr + 1;
	partition->part_id = dev->blkdev->part_user;
	read_gpt(partition);
	return 0;
}

#ifdef MTK_EMMC_SUPPORT
u64 emmc_write(u32 part_id, u64 offset, void *data, u64 size)
{
	part_dev_t *dev = mt_part_get_device();
	return (u64)dev->write(dev,data,offset,(int)size, part_id);
}

u64 emmc_read(u32 part_id, u64 offset, void *data, u64 size)
{
	part_dev_t *dev = mt_part_get_device();
	return (u64)dev->read(dev,offset,data,(int)size, part_id);
}

int emmc_erase(u32 part_id, u64 offset, u64 size)
{
	part_dev_t *dev = mt_part_get_device();
	return dev->erase(0, offset, size, part_id);
}
#endif

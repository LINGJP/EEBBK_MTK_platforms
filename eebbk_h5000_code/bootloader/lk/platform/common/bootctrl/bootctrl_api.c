/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2016. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */
#include <stdint.h>
#include "bootctrl.h"
#include "debug.h"

#define BOOTCTR_PARTITION "para"

static const char* suffix[2] = {BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B};

int check_suffix_with_slot(char *suffix)
{
    int slot = -1;

    if(suffix == NULL) {
        dprintf(CRITICAL, "input suffix is NULL\n");
        return -1;
    }

    if(!strcmp(suffix, BOOTCTRL_SUFFIX_A)) {
        slot = 0;
    }
    else if(!strcmp(suffix, BOOTCTRL_SUFFIX_B)) {
        slot = 1;
    }
    else {
        dprintf(CRITICAL, "unknow slot suffix\n");
    }

   return slot;
}

static int read_write_partition_info(boot_ctrl_t *bctrl ,int mode)
{
    ssize_t boot_ctrl_size;
    int ret = -1;
    boot_ctrl_size = sizeof(boot_ctrl_t);

    if(bctrl == NULL) {
        dprintf(CRITICAL, "read_write_partition_info failed, bctrl is NULL\n");
        return ret;
    }

    if(mode == READ_PARTITION) {
        ret = partition_read(BOOTCTR_PARTITION, (off_t)OFFSETOF_SLOT_SUFFIX, (char *)bctrl, (size_t)boot_ctrl_size);
        if (ret < 0) {
            dprintf(CRITICAL, "partition_read failed, ret: 0x%x\n", ret);
            return ret;
        }
    }
    else if(mode == WRITE_PARTITION) {
        ret = partition_write(BOOTCTR_PARTITION, (off_t)OFFSETOF_SLOT_SUFFIX, (char *)bctrl, (size_t)boot_ctrl_size);
        if (ret < 0) {
            dprintf(CRITICAL, "read_write_partition_info partition_write failed, unknown mode\n");
            return ret;
        }
    }
    else {
        dprintf(CRITICAL, " unknown mode, ret: 0x%x\n", ret);
        return ret;
    }

    return 0;
}

const char *get_suffix(void)
{
    int slot = 0, ret = -1;

    boot_ctrl_t metadata;

    ret = read_write_partition_info(&metadata, READ_PARTITION);
    if (ret < 0) {
        dprintf(CRITICAL, "read_partition_info failed, ret: 0x%x\n", ret);
        return NULL;
    }

    if(metadata.magic != BOOTCTRL_MAGIC) {
        dprintf(CRITICAL, "boot_ctrl magic number is wrong, use default value\n");
        slot = 0;
        set_active_slot(BOOTCTRL_SUFFIX_A);
    }
    else {
        dprintf(CRITICAL, "boot_ctrl magic number is match, compare priority\n");

        if(metadata.slot_info[0].priority >= metadata.slot_info[1].priority)
           slot = 0;
        else if (metadata.slot_info[0].priority < metadata.slot_info[1].priority)
           slot = 1;
    }

    return suffix[slot];
}

int set_active_slot(char *suffix)
{
    int slot = 0 ,slot1 = 0;
    int ret = -1;
    slot_metadata_t *slotp;

    boot_ctrl_t metadata;

    slot = check_suffix_with_slot(suffix);
    if(slot == -1) {
        dprintf(CRITICAL, "set_active_slot failed, slot: 0x%x\n", slot);
        return -1;
    }

    if(suffix == NULL) {
        dprintf(CRITICAL, "input suffix is NULL\n");
        return -1;
    }

    ret = read_write_partition_info(&metadata, READ_PARTITION);
    if(ret < 0) {
        dprintf(CRITICAL, "partition_read failed, ret: 0x%x\n", ret);
        return -1;
    }

    metadata.magic = BOOTCTRL_MAGIC;

    /* Set highest priority and reset retry count */
    slotp = &metadata.slot_info[slot];
    slotp->successful_boot = 0;
    slotp->priority = 7;
    slotp->retry_count = 3;
    slotp->normal_boot = 1;

    /* Re-set arg to another slot */
    slot1 = (slot == 0) ? 1 : 0;
    slotp = &metadata.slot_info[slot1];
    slotp->successful_boot = 0;
    slotp->priority = 6;
    slotp->retry_count = 3;
    slotp->normal_boot = 1;

    ret = read_write_partition_info(&metadata, WRITE_PARTITION);
    if (ret < 0) {
        dprintf(CRITICAL, "partition_write failed, ret: 0x%x\n", ret);
        return -1;
    }

    return 0;
}

uint8_t get_retry_count(char *suffix)
{
    int slot = 0;
    int ret = -1;
    slot_metadata_t *slotp;
    boot_ctrl_t metadata;

    slot = check_suffix_with_slot(suffix);
    if(slot == -1) {
        dprintf(CRITICAL, "get_retry_count failed, slot: 0x%x\n", slot);
        return 0;
    }

    ret = read_write_partition_info(&metadata, READ_PARTITION);
    if (ret < 0) {
        dprintf(CRITICAL, "partition_read failed, ret: 0x%x\n", ret);
        return 0;
    }

    slotp = &metadata.slot_info[slot];
    return slotp->retry_count;
}

int set_normal_boot(char *suffix, int boot_mode)
{
    int slot = 0, ret = -1;
    slot_metadata_t *slotp;
    boot_ctrl_t metadata;

    slot = check_suffix_with_slot(suffix);
    if(slot == -1) {
        dprintf(CRITICAL, "set_not_normal_boot failed, slot: 0x%x\n", slot);
        return -1;
    }

    ret = read_write_partition_info(&metadata, READ_PARTITION);
    if(ret < 0) {
        dprintf(CRITICAL, "partition_read failed, ret: 0x%x\n", ret);
        return -1;
    }

    slotp = &metadata.slot_info[slot];
    slotp->normal_boot = boot_mode;

    ret = read_write_partition_info(&metadata, WRITE_PARTITION);
    if(ret < 0) {
        dprintf(CRITICAL, "partition_write failed, ret: 0x%x\n", ret);
        return -1;
    }

    return 0;
}

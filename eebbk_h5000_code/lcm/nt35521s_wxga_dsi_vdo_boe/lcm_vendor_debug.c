/* 
 * 2010 - 2012 Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#include <linux/kobject.h>
#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/byteorder/generic.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>

#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#include "disp_lcm.h"

#define CONFIG_LCM_VENDOR_ID_DEBUG

#ifdef CONFIG_LCM_VENDOR_ID_DEBUG

struct kobject *lcm_vendor_id_kobj;

static ssize_t lcm_sysfs_vendor_id_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int val, id = -1;
    const char *lcm_manufactor_name[] = {"boe nt35523_ic", "innolux jd9365_ic", "boe jd9364_ic", "other lcm"};

    id = disp_get_lcm_vendor_id();
    if (id > 2 || id < 0) {
        val = sprintf(buf, "%s\n",  "error lcm" );
        return -1;
    }
    val = sprintf(buf, "%s\n", lcm_manufactor_name[id] );
    printk("lcm->show->vendor_id = %d\n", id);

    return val;
}

static DEVICE_ATTR(vendor_id, S_IRUGO, lcm_sysfs_vendor_id_show, NULL);

int lcm_sysfs_vendor_id_init(void)
{
    s32 ret = -1;

    lcm_vendor_id_kobj = kobject_create_and_add("lcm", NULL) ;

    if (lcm_vendor_id_kobj == NULL)
    {
        printk("%s: subsystem_register failed\n", __func__);
        return -ENOMEM;
    }

    ret = sysfs_create_file(lcm_vendor_id_kobj, &dev_attr_vendor_id.attr);
    if (ret)
    {
        printk("%s: sysfs_create_gesture_file failed\n", __func__);
        return ret;
    }

    printk("sysfs create lcm_vendor_id success!\n");

    return ret;
}

void lcm_sysfs_vendor_id_deinit(void)
{
    sysfs_remove_file(lcm_vendor_id_kobj, &dev_attr_vendor_id.attr);
    kobject_del(lcm_vendor_id_kobj);
}
#endif


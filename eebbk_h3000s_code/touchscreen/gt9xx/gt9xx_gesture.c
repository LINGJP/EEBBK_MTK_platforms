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

#include "include/gt9xx_gesture.h"
#include "include/tpd_gt9xx_common.h"

struct kobject *tp_gesture_kobj;

#ifdef CONFIG_GTP_GESTURE_WAKEUP

int get_tp_gesture_flag(void)
{
    GTP_INFO("%s()->g_gesture_switch = %d\n", __func__, gesture_data.enabled);
	return gesture_data.enabled;
}

static ssize_t tp_sysfs_gesture_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int val;

	val = sprintf(buf, "%d\n", gesture_data.enabled);
    GTP_INFO("sysfs->show->g_gesture_switch = %d\n", gesture_data.enabled);

    return val;
}

static ssize_t tp_sysfs_gesture_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	int val = count;
	
  GTP_INFO("sysfs->store->buf = %s\n", buf);

	val = simple_strtol(buf, NULL, 10);
	if (val == 1)
		gesture_data.enabled = 1;
	else
		gesture_data.enabled = 0;

    return count;
}

static DEVICE_ATTR(gestureflag, S_IRUGO|S_IWUSR, tp_sysfs_gesture_show, tp_sysfs_gesture_store);

int tp_sysfs_gesture_init(void)
{
    s32 ret = -1;

    tp_gesture_kobj = kobject_create_and_add("touchscreen", NULL) ;

    if (tp_gesture_kobj == NULL)
    {
        GTP_ERROR("%s: subsystem_register failed\n", __func__);
        return -ENOMEM;
    }

    ret = sysfs_create_file(tp_gesture_kobj, &dev_attr_gestureflag.attr);
    if (ret)
    {
        GTP_ERROR("%s: sysfs_create_gesture_file failed\n", __func__);
        return ret;
    }

    GTP_INFO("sysfs create gesture success!\n");

    return ret;
}

void tp_sysfs_gesture_deinit(void)
{
    sysfs_remove_file(tp_gesture_kobj, &dev_attr_gestureflag.attr);
    kobject_del(tp_gesture_kobj);
}
#endif


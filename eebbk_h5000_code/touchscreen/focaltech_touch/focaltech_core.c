/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2017, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*****************************************************************************
*
* File Name: focaltech_core.c

* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract: entrance for focaltech ts driver
*
* Version: V1.0
*
*****************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
//#include <linux/rtpm_prio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/wakelock.h>
#include "focaltech_core.h"
#include "tpd.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_DRIVER_NAME                     "fts_ts"
#define INTERVAL_READ_REG                   20  //interval time per read reg unit:ms
#define TIMEOUT_READ_REG                    300 //timeout of read reg unit:ms
#define FTS_I2C_SLAVE_ADDR                  0x38
#define FTS_READ_TOUCH_BUFFER_DIVIDED       0
/*****************************************************************************
* Static variables
*****************************************************************************/
struct i2c_client *fts_i2c_client;
struct input_dev *fts_input_dev;
struct task_struct *thread_tpd;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_flag;

#if FTS_DEBUG_EN
int g_show_log = 1;
#else
int g_show_log = 0;
#endif

unsigned int tpd_rst_gpio_number = 0;
unsigned int tpd_int_gpio_number = 1;
static unsigned int ft_touch_irq = 0;
static unsigned int ft_irq_disable = 0;
static spinlock_t irq_lock;
struct mutex report_mutex;


#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))
static int tpd_def_calmat_local_normal[8]  = TPD_CALIBRATION_MATRIX_ROTATION_NORMAL;
static int tpd_def_calmat_local_factory[8] = TPD_CALIBRATION_MATRIX_ROTATION_FACTORY;
#endif

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static void tpd_resume(struct device *h);
static void tpd_suspend(struct device *h);
static void fts_release_all_finger(void);

/*****************************************************************************
* Focaltech ts i2c driver configuration
*****************************************************************************/
static const struct i2c_device_id fts_tpd_id[] = {{FTS_DRIVER_NAME, 0}, {} };
static const struct of_device_id fts_dt_match[] =
{
    {.compatible = "mediatek,cap_touch"},
    {},
};
MODULE_DEVICE_TABLE(of, fts_dt_match);

static struct i2c_driver tpd_i2c_driver =
{
    .driver = {
        .name = FTS_DRIVER_NAME,
        .of_match_table = of_match_ptr(fts_dt_match),
    },
    .probe = tpd_probe,
    .remove = tpd_remove,
    .id_table = fts_tpd_id,
    .detect = tpd_i2c_detect,
};

/*****************************************************************************
*  Name: fts_wait_tp_to_valid
*  Brief:   Read chip id until TP FW become valid, need call when reset/power on/resume...
*           1. Read Chip ID per INTERVAL_READ_REG(20ms)
*           2. Timeout: TIMEOUT_READ_REG(400ms)
*  Input:
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int fts_wait_tp_to_valid(struct i2c_client *client)
{
    int ret = 0;
    int cnt = 0;
    u8  reg_value = 0;

    do
    {
        ret = fts_i2c_read_reg(client, FTS_REG_CHIP_ID, &reg_value);
        if ((ret < 0) || (reg_value != chip_types.chip_idh))
        {
            FTS_INFO("TP Not Ready, ReadData = 0x%x", reg_value);
        }
        else if (reg_value == chip_types.chip_idh)
        {
            FTS_INFO("TP Ready, Device ID = 0x%x", reg_value);
            return 0;
        }
        cnt++;
        msleep(INTERVAL_READ_REG);
    }
    while ((cnt * INTERVAL_READ_REG) < TIMEOUT_READ_REG);

    /* error: not get correct reg data */
    return -1;
}
/*add by lmh start*/
/*****************************************************************************
*  Name: fts_check_tp_valid
*  Brief:   Read chip id until TP FW become valid, need call when reset/power on/resume...
*           1. Read Chip ID per INTERVAL_READ_REG(20ms)
*           2. Timeout: TIMEOUT_READ_REG(400ms)
*  Input:
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int fts_check_tp_valid(struct i2c_client *client)
{
    int ret = 0;
    int cnt = 0;
    u8  reg_value = 0;

	//固件debug调试版本可能会将chip id 改成0x5422,在这里对这部分机器进行兼容
    do
    {
        ret = fts_i2c_read_reg(client, FTS_REG_CHIP_ID, &reg_value);
		if ((ret < 0) || (reg_value != chip_types.chip_idh))
		//if ((ret < 0) || ((reg_value != chip_types.chip_idh) && (reg_value != 0x54) && (reg_value != 0xef) ))
        {
            FTS_INFO("ret = %d, TP Not Ready, ReadData = 0x%x", ret, reg_value);
        }
        //else if ( (reg_value == chip_types.chip_idh) || (reg_value == 0x54) || (reg_value == 0xef) )
        else if ( (reg_value == chip_types.chip_idh) )
        {
            FTS_INFO("TP Ready, Device ID = 0x%x", reg_value);
            return 0;
        }
        cnt++;
        msleep(INTERVAL_READ_REG);
    }
    while ((cnt * INTERVAL_READ_REG) < TIMEOUT_READ_REG);

    /* error: not get correct reg data */
    return -1;
}
/*add by lmh end*/

/*****************************************************************************
*  Name: fts_recover_state
*  Brief: Need execute this function when reset
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_tp_state_recovery(struct i2c_client *client)
{
    /* wait tp stable */
    fts_wait_tp_to_valid(client);
    /* recover TP charger state 0x8B */
    /* recover TP glove state 0xC0 */
    /* recover TP cover state 0xC1 */
    fts_ex_mode_recovery(client);

#if FTS_PSENSOR_EN
    fts_proximity_recovery(client);
#endif

    /* recover TP gesture state 0xD0 */
#if FTS_GESTURE_EN
    fts_gesture_recovery(client);
#endif
}

/*****************************************************************************
*  Name: fts_reset_proc
*  Brief: Execute reset operation
*  Input: hdelayms - delay time unit:ms
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int fts_reset_proc(int hdelayms)
{
    tpd_gpio_output(tpd_rst_gpio_number, 0);
    msleep(20);
    tpd_gpio_output(tpd_rst_gpio_number, 1);
    msleep(hdelayms);

    return 0;
}

/*****************************************************************************
*  Name: fts_irq_disable
*  Brief: disable irq
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_irq_disable(void)
{
    unsigned long irqflags;

	FTS_FUNC_ENTER();

    spin_lock_irqsave(&irq_lock, irqflags);

    if (!ft_irq_disable)
    {
        disable_irq_nosync(ft_touch_irq);
        ft_irq_disable = 1;
    }

    spin_unlock_irqrestore(&irq_lock, irqflags);
}

/*****************************************************************************
*  Name: fts_irq_enable
*  Brief: enable irq
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_irq_enable(void)
{
    unsigned long irqflags = 0;
	
	FTS_FUNC_ENTER();

    spin_lock_irqsave(&irq_lock, irqflags);

    if (ft_irq_disable)
    {
        enable_irq(ft_touch_irq);
        ft_irq_disable = 0;
    }

    spin_unlock_irqrestore(&irq_lock, irqflags);
}

#if FTS_POWER_SOURCE_CUST_EN
/*****************************************************************************
* Power Control
*****************************************************************************/
int fts_power_init(void)
{
    int retval;
	
	FTS_FUNC_ENTER();
	
    /*set TP volt*/
    tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
    retval = regulator_set_voltage(tpd->reg, 3300000, 3300000);
    if (retval != 0)
    {
        FTS_ERROR("[POWER]Failed to set voltage of regulator,ret=%d!", retval);
        return retval;
    }

    retval = regulator_enable(tpd->reg);
    if (retval != 0)
    {
        FTS_ERROR("[POWER]Fail to enable regulator when init,ret=%d!", retval);
        return retval;
    }

    return 0;
}

void fts_power_suspend(void)
{
    int retval;
	
	FTS_FUNC_ENTER();

    retval = regulator_disable(tpd->reg);
    if (retval != 0)
        FTS_ERROR("[POWER]Failed to disable regulator when suspend ret=%d!", retval);
}

int fts_power_resume(void)
{
    int retval = 0;

	FTS_FUNC_ENTER();
	
    retval = regulator_enable(tpd->reg);
    if (retval != 0)
        FTS_ERROR("[POWER]Failed to enable regulator when resume,ret=%d!", retval);

    return retval;
}
#endif


/*****************************************************************************
*  Reprot related
*****************************************************************************/
/*****************************************************************************
*  Name: fts_release_all_finger
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_release_all_finger(void)
{
#if FTS_MT_PROTOCOL_B_EN
    unsigned int finger_count=0;
#endif

    FTS_FUNC_ENTER();

    mutex_lock(&report_mutex);

#if FTS_MT_PROTOCOL_B_EN
    for (finger_count = 0; finger_count < tpd_dts_data.touch_max_num; finger_count++)
    {
        input_mt_slot(fts_input_dev, finger_count);
        input_mt_report_slot_state( fts_input_dev, MT_TOOL_FINGER, false);
    }
#else
    input_mt_sync ( fts_input_dev );
#endif

    input_report_key(fts_input_dev, BTN_TOUCH, 0);
    input_sync (fts_input_dev );

    mutex_unlock(&report_mutex);

    FTS_FUNC_EXIT();
}

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
char g_sz_debug[1024] = {0};

#define FTS_ONE_TCH_LEN     FTS_TOUCH_STEP
static void fts_show_touch_buffer(u8 *buf, int point_num)
{
    int len = point_num * FTS_ONE_TCH_LEN;
    int count = 0;
    int i;

    memset(g_sz_debug, 0, 1024);
    if (len > (POINT_READ_BUF-3))
    {
        len = POINT_READ_BUF-3;
    }
    else if (len == 0)
    {
        len += FTS_ONE_TCH_LEN;
    }
    count += sprintf(g_sz_debug, "%02X,%02X,%02X", buf[0], buf[1], buf[2]);
    for (i = 0; i < len; i++)
    {
        count += sprintf(g_sz_debug+count, ",%02X", buf[i+3]);
    }
    FTS_DEBUG("buffer: %s", g_sz_debug);
}
#endif

#if (!FTS_MT_PROTOCOL_B_EN)
static void tpd_down(int x, int y, int p, int id)
{
    if ((id < 0) || (id > 9))
        return;
    input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
    input_report_key(tpd->dev, BTN_TOUCH, 1);

#if (FTS_REPORT_PRESSURE_EN)
    if (p <= 0)
    {
        FTS_ERROR("[A]Illegal pressure: %d", p);
        p = 1;
    }

    input_report_abs(tpd->dev, ABS_MT_PRESSURE, p);
#endif

    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);

    FTS_DEBUG("[A]P%d(%4d,%4d)[p:%d] DOWN!", id, x, y, p);

}

static void tpd_up(int x, int y)
{
    FTS_DEBUG("[A]All Up!");
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    input_mt_sync(tpd->dev);
}

static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
    int i = 0;
    int ret;
    u8 data[POINT_READ_BUF] = { 0 };
    u16 high_byte, low_byte;
    char writebuf[10]= { 0 };

#if FTS_READ_TOUCH_BUFFER_DIVIDED
    u8 pointnum;
    memset(data, 0xff, POINT_READ_BUF);
    ret = fts_i2c_read(fts_i2c_client, writebuf, 1, data, 3 + FTS_TOUCH_STEP);
    if (ret < 0)
    {
        FTS_ERROR("[A]Read touchdata failed, ret: %d", ret);
        return ret;
    }

    pointnum = data[2] & 0x0f;
    if (pointnum > 1)
    {
        writebuf[0] = 9;
        ret = fts_i2c_read(fts_i2c_client, writebuf, 1, data+9, (pointnum-1) * FTS_TOUCH_STEP);
        if (ret < 0)
        {
            FTS_ERROR("[A]Read touchdata failed, ret: %d", ret);
            return ret;
        }
    }
#else
    ret = fts_i2c_read(fts_i2c_client, writebuf, 1, data, POINT_READ_BUF);
    if (ret < 0)
    {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        FTS_FUNC_EXIT();
        return ret;
    }
#endif

    if ((data[0] & 0x70) != 0)
        return false;

    memcpy(pinfo, cinfo, sizeof(struct touch_info));
    memset(cinfo, 0, sizeof(struct touch_info));
    for (i = 0; i < tpd_dts_data.touch_max_num; i++)
        cinfo->p[i] = 1;    /* Put up */

    /*get the number of the touch points*/
    cinfo->count = data[2] & 0x0f;
    FTS_DEBUG("Number of touch points = %d", cinfo->count);

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
    fts_show_touch_buffer(data, cinfo->count);
#endif
    for (i = 0; i < cinfo->count; i++)
    {
        cinfo->p[i] = (data[3 + 6 * i] >> 6) & 0x0003; /* event flag */
        cinfo->id[i] = data[3+6*i+2]>>4;                        // touch id

        /*get the X coordinate, 2 bytes*/
        high_byte = data[3 + 6 * i];
        high_byte <<= 8;
        high_byte &= 0x0F00;

        low_byte = data[3 + 6 * i + 1];
        low_byte &= 0x00FF;
        cinfo->x[i] = high_byte | low_byte;

        /*get the Y coordinate, 2 bytes*/
        high_byte = data[3 + 6 * i + 2];
        high_byte <<= 8;
        high_byte &= 0x0F00;

        low_byte = data[3 + 6 * i + 3];
        low_byte &= 0x00FF;
        cinfo->y[i] = high_byte | low_byte;

        FTS_DEBUG(" cinfo->x[%d] = %d, cinfo->y[%d] = %d, cinfo->p[%d] = %d", i,
                  cinfo->x[i], i, cinfo->y[i], i, cinfo->p[i]);
    }

    return true;
};
#else
/************************************************************************
* Name: fts_read_touchdata
* Brief: report the point information
* Input: event info
* Output: get touch data in pinfo
* Return: success is zero
***********************************************************************/
static int fts_read_touchdata(struct ts_event *data)
{
    u8 buf[POINT_READ_BUF] = { 0 };
    int ret = -1;
    int i = 0;
    u8 pointid = FTS_MAX_ID;

	/************ add by lmh start *************/
	//u8 state;
	//static u8 buf_addr[2] = { 0 };
    //static u8 buf_value[2] = { 0 };
    //buf_addr[0] = FTS_REG_CHARGER_MODE_EN; //charger control
	/************ add by lmh end *************/

    FTS_FUNC_ENTER();
#if FTS_READ_TOUCH_BUFFER_DIVIDED
    memset(buf, 0xff, POINT_READ_BUF);
    buf[0] = 0x0;
    ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 3 + FTS_TOUCH_STEP);
    if (ret < 0)
    {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        return ret;
    }
    ret = data->touchs;
    memset(data, 0, sizeof(struct ts_event));
    data->touchs = ret;
    data->touch_point_num = buf[FT_TOUCH_POINT_NUM] & 0x0F;

    if (data->touch_point_num > 1)
    {
        buf[9] = 9;
        ret = fts_i2c_read(fts_i2c_client, buf+9, 1, buf+9, (data->touch_point_num-1) * FTS_TOUCH_STEP);
        if (ret < 0)
        {
            FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
            return ret;
        }
    }
#else
    ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, POINT_READ_BUF);
    if (ret < 0)
    {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        FTS_FUNC_EXIT();
        return ret;
    }
    ret = data->touchs;
    memset(data, 0, sizeof(struct ts_event));
    data->touchs = ret;
    data->touch_point_num = buf[FT_TOUCH_POINT_NUM] & 0x0F;
#endif

	/************ add by lmh start *************/

	//catch the interrupt when reset case the esd fail 	
	if(data->touch_point_num >= 15)
	{
		/* Before read/write TP register, need wait TP to valid */
    	fts_tp_state_recovery(fts_i2c_client);

		return -1;
	}
	/************ add by lmh end *************/
	
	data->touch_point = 0;

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
    fts_show_touch_buffer(buf, data->touch_point_num);
#endif

    for (i = 0; i < tpd_dts_data.touch_max_num; i++)
    {
        pointid = (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
        if (pointid >= FTS_MAX_ID)
            break;
        else
            data->touch_point++;
        data->au16_x[i] =
            (s16) (buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
        data->au16_y[i] =
            (s16) (buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
        data->au8_touch_event[i] =
            buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] >> 6;
        data->au8_finger_id[i] =
            (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;

        data->pressure[i] =
            (buf[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);//cannot constant value
        data->area[i] =
            (buf[FTS_TOUCH_MISC + FTS_TOUCH_STEP * i]) >> 4;
        if ((data->au8_touch_event[i]==0 || data->au8_touch_event[i]==2)&&(data->touch_point_num==0))
        {
            FTS_DEBUG("abnormal touch data from fw");
            return -1;
        }
    }
    if(data->touch_point == 0)
    {
        return -1;
    }
    //FTS_FUNC_EXIT();
    return 0;
}
/************************************************************************
* Name: fts_report_key
* Brief: report key event
* Input: event info
* Output: no
* Return: 0: is key event, -1: isn't key event
***********************************************************************/
static int fts_report_key(struct ts_event *data)
{
    int i = 0;

    if (1 != data->touch_point)
        return -1;

    for (i = 0; i < tpd_dts_data.touch_max_num; i++)
    {
        if (data->au16_y[i] <= TPD_RES_Y)
        {
            return -1;
        }
        else
        {
            break;
        }
    }
    if (data->au8_touch_event[i]== 0 ||
        data->au8_touch_event[i] == 2)
    {
        tpd_button(data->au16_x[i], data->au16_y[i], 1);
        FTS_DEBUG("[B]Key(%d, %d) DOWN!", data->au16_x[i], data->au16_y[i]);
    }
    else
    {
        tpd_button(data->au16_x[i], data->au16_y[i], 0);
        FTS_DEBUG("[B]Key(%d, %d) UP!", data->au16_x[i], data->au16_y[i]);
    }

    input_sync(tpd->dev);

    return 0;
}

/************************************************************************
* Name: fts_report_touch
* Brief: report the point information
* Input: event info
* Output: no
* Return: success is zero
***********************************************************************/
static int fts_report_touch(struct ts_event *data)
{
    int i = 0;
    int up_point = 0;
    int touchs = 0;

    for (i = 0; i < data->touch_point; i++)
    {
        if (data->au8_finger_id[i] >= tpd_dts_data.touch_max_num)
        {
            break;
        }

        input_mt_slot(tpd->dev, data->au8_finger_id[i]);

        if (data->au8_touch_event[i]== 0 || data->au8_touch_event[i] == 2)
        {
            input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER,true);
#if FTS_REPORT_PRESSURE_EN
            if (data->pressure[i] <= 0)
            {
                FTS_ERROR("[B]Illegal pressure: %d", data->pressure[i]);
                data->pressure[i] = 1;
            }
            
            input_report_abs(tpd->dev, ABS_MT_PRESSURE, data->pressure[i]);
#endif
            if (data->area[i] <= 0)
            {
                FTS_ERROR("[B]Illegal touch-major: %d", data->area[i]);
                data->area[i] = 1;
            }
            input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, data->area[i]);

            input_report_abs(tpd->dev, ABS_MT_POSITION_X,data->au16_x[i]);
            input_report_abs(tpd->dev, ABS_MT_POSITION_Y,data->au16_y[i]);
            touchs |= BIT(data->au8_finger_id[i]);
            data->touchs |= BIT(data->au8_finger_id[i]);

            //FTS_DEBUG("[B]P%d(%4d,%4d)[p:%d,tm:%d] DOWN!", data->au8_finger_id[i], data->au16_x[i], data->au16_y[i], data->pressure[i], data->area[i]);
            FTS_DEBUG("[B]P%d [pressure:%d,tm:%d],  position=(%4d,%4d) DOWN!", data->au8_finger_id[i], data->pressure[i], data->area[i], data->au16_x[i], data->au16_y[i]);
        }
        else
        {
            up_point++;
            input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);

            data->touchs &= ~BIT(data->au8_finger_id[i]);
            FTS_DEBUG("[B]P%d UP fw!", data->au8_finger_id[i]);
        }

    }
    for (i = 0; i < tpd_dts_data.touch_max_num; i++)
    {
        if (BIT(i) & (data->touchs ^ touchs))
        {
            FTS_DEBUG("[B]P%d UP driver!", i);
            data->touchs &= ~BIT(i);
            input_mt_slot(tpd->dev, i);
            input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
        }
    }
    data->touchs = touchs;

    if ((data->touch_point == up_point) || !data->touch_point_num)
    {
        FTS_DEBUG("[B]Points All UP!");
        input_report_key(tpd->dev, BTN_TOUCH, 0);
    }
    else
    {
        input_report_key(tpd->dev, BTN_TOUCH, 1);
    }

    input_sync(tpd->dev);
    return 0;
}
#endif


/*****************************************************************************
*  Name: tpd_eint_interrupt_handler
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
{
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
    return IRQ_HANDLED;
}

/*****************************************************************************
*  Name: tpd_irq_registration
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int tpd_irq_registration(struct i2c_client *client)
{
    struct device_node *node = NULL;
    int ret = 0;
	/**** modify by lmh start ****/
    //u32 ints[2] = {0,0};
	unsigned int irq_gpio;
	/**** modify by lmh end ****/

    FTS_FUNC_ENTER();
    node = of_find_matching_node(node, touch_of_match);
    if (node)
    {
		/**** modify by lmh start ****/
        //of_property_read_u32_array(node,"debounce", ints, ARRAY_SIZE(ints));
        //gpio_set_debounce(ints[0], ints[1]);
		irq_gpio = of_get_named_gpio(node, "int-gpio", 0); 
        printk("%s irq_gpio=%d\n",__func__,irq_gpio);
        gpio_set_debounce(irq_gpio, 10);	
		/**** modify by lmh end ****/

        ft_touch_irq = irq_of_parse_and_map(node, 0);
        FTS_DEBUG("ft_touch_irq = %d\n", ft_touch_irq);
        ret = request_irq(ft_touch_irq, tpd_eint_interrupt_handler,
                          IRQF_TRIGGER_FALLING, "TOUCH_PANEL-eint", NULL);
        if (ret == 0)
        {
            FTS_INFO("IRQ request succussfully, irq=%d", ft_touch_irq);
            client->irq = ft_touch_irq;
			enable_irq_wake(ft_touch_irq);
        }
        else
            FTS_ERROR("tpd request_irq IRQ LINE NOT AVAILABLE!.");

    }
    else
    {
        FTS_ERROR("Can not find touch eint device node!");
    }
    FTS_FUNC_EXIT();
    return 0;
}


#if !(FTS_MT_PROTOCOL_B_EN)
static int fts_report_key_a(struct touch_info *cinfo, struct touch_info *pinfo, struct touch_info *finfo)
{
    if (tpd_dts_data.use_tpd_button)
    {
        if (cinfo->p[0] == 0)
            memcpy(finfo, cinfo, sizeof(struct touch_info));
    }

    if ((cinfo->y[0] >= TPD_RES_Y) && (pinfo->y[0] < TPD_RES_Y)
        && ((pinfo->p[0] == 0) || (pinfo->p[0] == 2)))
    {
        FTS_DEBUG("All up");
        tpd_up(pinfo->x[0], pinfo->y[0]);
        input_sync(tpd->dev);
        return 0;
    }

    if (tpd_dts_data.use_tpd_button)
    {
        if ((cinfo->y[0] <= TPD_RES_Y && cinfo->y[0] != 0) && (pinfo->y[0] > TPD_RES_Y)
            && ((pinfo->p[0] == 0) || (pinfo->p[0] == 2)))
        {
            FTS_DEBUG("All key up");
            tpd_button(pinfo->x[0], pinfo->y[0], 0);
            input_sync(tpd->dev);
            return 0;
        }

        if ((cinfo->y[0] > TPD_RES_Y) || (pinfo->y[0] > TPD_RES_Y))
        {
            if (finfo->y[0] > TPD_RES_Y)
            {
                if ((cinfo->p[0] == 0) || (cinfo->p[0] == 2))
                {
                    FTS_DEBUG("Key(%d,%d) Down", pinfo->x[0], pinfo->y[0]);
                    tpd_button(pinfo->x[0], pinfo->y[0], 1);
                }
                else if ((cinfo->p[0] == 1) &&
                         ((pinfo->p[0] == 0) || (pinfo->p[0] == 2)))
                {
                    FTS_DEBUG("Key(%d,%d) Up!", pinfo->x[0], pinfo->y[0]);
                    tpd_button(pinfo->x[0], pinfo->y[0], 0);
                }
                input_sync(tpd->dev);
            }
            return 0;
        }
    }
    return -1;
}

static void fts_report_touch_a(struct touch_info *cinfo)
{
    int i;
    if (cinfo->count > 0)
    {
        for (i = 0; i < cinfo->count; i++)
        {
            tpd_down(cinfo->x[i], cinfo->y[i], i + 1, cinfo->id[i]);
        }
    }
    else
    {
        tpd_up(cinfo->x[0], cinfo->y[0]);
    }
    input_sync(tpd->dev);
}
#endif


/*****************************************************************************
*  Name: touch_event_handler
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int touch_event_handler(void *unused)
{
    int i = 0;
    int ret;

#if FTS_GESTURE_EN
    u8 state = 0;
#endif
#if FTS_MT_PROTOCOL_B_EN
    struct ts_event pevent;
#else
    struct touch_info  cinfo, pinfo;
#endif

    struct touch_info  finfo;
    //struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    struct sched_param param = { .sched_priority = 4 };
    FTS_FUNC_ENTER();
    FTS_DEBUG("RTPM_PRIO_TPD: %d", param.sched_priority);

    if (tpd_dts_data.use_tpd_button)
    {
        memset(&finfo, 0, sizeof(struct touch_info));
        for (i = 0; i < tpd_dts_data.touch_max_num; i++)
            finfo.p[i] = 1;
    }
#if !FTS_MT_PROTOCOL_B_EN
    memset(&cinfo, 0, sizeof(struct touch_info));
    memset(&pinfo, 0, sizeof(struct touch_info));
#endif
    sched_setscheduler(current, SCHED_RR, &param);

    do
    {
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(waiter, tpd_flag != 0);

        tpd_flag = 0;

        set_current_state(TASK_RUNNING);

#if FTS_GESTURE_EN
        ret = fts_i2c_read_reg(fts_i2c_client, FTS_REG_GESTURE_EN, &state);
        if (ret<0)
        {
            FTS_ERROR("[Focal][Touch] read value fail");
        }
        if (state == 1)
        {
            fts_gesture_readdata(fts_i2c_client);
            continue;
        }
#endif

#if FTS_PSENSOR_EN
        if (fts_proximity_readdata(fts_i2c_client) == 0)
            continue;
#endif

#if FTS_POINT_REPORT_CHECK_EN
        fts_point_report_check_queue_work();
#endif

        //FTS_DEBUG("touch_event_handler start");
#if FTS_ESDCHECK_EN
        fts_esdcheck_set_intr(1);
#endif
#if FTS_MT_PROTOCOL_B_EN
        {
            ret = fts_read_touchdata(&pevent);
            if (ret == 0)
            {
                mutex_lock(&report_mutex);
        		//FTS_DEBUG("tpd_dts_data.use_tpd_button = %d", tpd_dts_data.use_tpd_button);
                if (tpd_dts_data.use_tpd_button)
                {
                    ret = !fts_report_key(&pevent);
                }
                if (ret == 0)
                {
                    fts_report_touch(&pevent);
                }
                mutex_unlock(&report_mutex);
            }
        }
#else //FTS_MT_PROTOCOL_A_EN
        {
            ret = tpd_touchinfo(&cinfo, &pinfo);
            if (ret)
            {
                mutex_lock(&report_mutex);
                ret = fts_report_key_a(&cinfo, &pinfo, &finfo);
                if (ret)
                {
                    fts_report_touch_a(&cinfo);
                }
                mutex_unlock(&report_mutex);

            }
        }
#endif
#if FTS_ESDCHECK_EN
        fts_esdcheck_set_intr(0);
#endif
    }
    while (!kthread_should_stop());

    return 0;
}

/************************************************************************
* Name: tpd_i2c_detect
* Brief:
* Input:
* Output:
* Return:
***********************************************************************/
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);

    return 0;
}

/************************************************************************
* Name: fts_probe
* Brief:
* Input:
* Output:
* Return:
***********************************************************************/
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int retval = 0;

    FTS_FUNC_ENTER();

    fts_i2c_client = client;
    fts_input_dev = tpd->dev;

    if (fts_i2c_client->addr != FTS_I2C_SLAVE_ADDR)
    {
        FTS_INFO("[TPD]Change i2c addr 0x%02x to 0x38", fts_i2c_client->addr);
        fts_i2c_client->addr = FTS_I2C_SLAVE_ADDR;
        FTS_DEBUG("[TPD]fts_i2c_client->addr=0x%x\n", fts_i2c_client->addr);
    }

    spin_lock_init(&irq_lock);
    mutex_init(&report_mutex);

    /* Configure gpio to irq and request irq */
    tpd_gpio_as_int(tpd_int_gpio_number);
    tpd_irq_registration(client);
    fts_irq_disable();

    /* Init I2C */
    fts_i2c_init();

    fts_ctpm_get_upgrade_array();
	

#if FTS_GESTURE_EN
    fts_gesture_init(tpd->dev, client);
#endif

#if FTS_PSENSOR_EN
    fts_proximity_init(client);
#endif

#if FTS_MT_PROTOCOL_B_EN
    input_mt_init_slots(tpd->dev, tpd_dts_data.touch_max_num, INPUT_MT_DIRECT);
#endif

    fts_reset_proc(200);

	/**********add by lmh start*************/
	/*retval = fts_check_tp_valid(client);
	if(retval < 0)
	{
		FTS_ERROR("[TPD] The tp is not valid,retval = %d!", retval);
		goto out;
	}*/
	/**********add by lmh end*************/
    
	fts_wait_tp_to_valid(client);

    tpd_load_status = 1;

#if FTS_SYSFS_NODE_EN
    fts_create_sysfs(client);
#endif

#if FTS_POINT_REPORT_CHECK_EN
    fts_point_report_check_init();
#endif

    fts_ex_mode_init(client);

#if FTS_APK_NODE_EN
    fts_create_apk_debug_channel(client);
#endif

    thread_tpd = kthread_run(touch_event_handler, 0, TPD_DEVICE);
    if (IS_ERR(thread_tpd))
    {
        retval = PTR_ERR(thread_tpd);
        FTS_ERROR("[TPD]Failed to create kernel thread_tpd,ret=%d!", retval);
    }

    FTS_DEBUG("[TPD]Touch Panel Device Probe %s!", (retval < 0) ? "FAIL" : "PASS");

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    fts_sensor_init();
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_init();
#endif

    fts_irq_enable(); /* need execute before upgrade */

#if FTS_AUTO_UPGRADE_EN
    fts_ctpm_upgrade_init();
#endif

#if FTS_TEST_EN
    fts_test_init(client);
#endif

    FTS_FUNC_EXIT();
	
    return 0;

	/**********add by lmh start*************/
    /*out:
		gpio_free(tpd_rst_gpio_number);
    	gpio_free(tpd_int_gpio_number); 
        return -1;*/
    /**********add by lmh end*************/
}

/*****************************************************************************
*  Name: tpd_remove
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int tpd_remove(struct i2c_client *client)
{
    FTS_FUNC_ENTER();

#if FTS_TEST_EN
    fts_test_exit(client);
#endif

#if FTS_POINT_REPORT_CHECK_EN
    fts_point_report_check_exit();
#endif


#if FTS_SYSFS_NODE_EN
    fts_remove_sysfs(client);
#endif

    fts_ex_mode_exit(client);

#if FTS_PSENSOR_EN
    fts_proximity_exit(client);
#endif

#if FTS_APK_NODE_EN
    fts_release_apk_debug_channel();
#endif

#if FTS_AUTO_UPGRADE_EN
    fts_ctpm_upgrade_exit();
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_exit();
#endif

#if FTS_GESTURE_EN
    fts_gesture_exit(client);
#endif

    fts_i2c_exit();

    FTS_FUNC_EXIT();

    return 0;
}

/*****************************************************************************
*  Name: tpd_local_init
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int tpd_local_init(void)
{
    FTS_FUNC_ENTER();
#if FTS_POWER_SOURCE_CUST_EN
    if (fts_power_init() != 0)
        return -1;
#endif

    if (i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        FTS_ERROR("[TPD]: Unable to add fts i2c driver!!");
        FTS_FUNC_EXIT();
        return -1;
    }

    if (tpd_dts_data.use_tpd_button)
    {
        tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
                           tpd_dts_data.tpd_key_dim_local);
    }

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local_factory, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local_factory, 8 * 4);

    memcpy(tpd_calmat, tpd_def_calmat_local_normal, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local_normal, 8 * 4);
#endif

    tpd_type_cap = 1;

    FTS_FUNC_EXIT();
    return 0;
}

/*****************************************************************************
*  Name: tpd_suspend
*  Brief: When suspend, will call this function
*           1. Set gesture if EN
*           2. Disable ESD if EN
*           3. Process MTK sensor hub if configure, default n, if n, execute 4/5/6
*           4. disable irq
*           5. Set TP to sleep mode
*           6. Disable power(regulator) if EN
*           7. fts_release_all_finger
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void tpd_suspend(struct device *h)
{
    int retval = 0;

    FTS_FUNC_ENTER();

#if FTS_PSENSOR_EN
    if (fts_proximity_suspend() == 0)
        return;
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_suspend();
#endif

#if FTS_GESTURE_EN
    retval = fts_gesture_suspend(fts_i2c_client);
    if (retval == 0)
    {
        /* Enter into gesture mode(suspend) */
        return;
    }
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    fts_sensor_suspend(fts_i2c_client);
#else
    fts_irq_disable();
    /* TP enter sleep mode */
    retval = fts_i2c_write_reg(fts_i2c_client, FTS_REG_POWER_MODE, FTS_REG_POWER_MODE_SLEEP_VALUE);
    if (retval < 0)
    {
        FTS_ERROR("Set TP to sleep mode fail, ret=%d!", retval);
    }

//设置TP suspend不掉电
#if FTS_POWER_SOURCE_CUST_EN
//    fts_power_suspend();
#endif

#endif

    FTS_FUNC_EXIT();
}


/*****************************************************************************
*  Name: tpd_resume
*  Brief: When suspend, will call this function
*           1. Clear gesture if EN
*           2. Enable power(regulator) if EN
*           3. Execute reset if no IDC to wake up
*           4. Confirm TP in valid app by read chip ip register:0xA3
*           5. fts_release_all_finger
*           6. Enable ESD if EN
*           7. tpd_usb_plugin if EN
*           8. Process MTK sensor hub if configure, default n, if n, execute 9
*           9. disable irq
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void tpd_resume(struct device *h)
{
    FTS_FUNC_ENTER();

#if FTS_PSENSOR_EN
    if (fts_proximity_resume() == 0)
        return;
#endif

    fts_release_all_finger();

//suspend时候没有掉电
#if FTS_POWER_SOURCE_CUST_EN
//    fts_power_resume();
#endif

/*双击唤醒时候resume进来在reset的时候会触发一次中断，而且在里面i2c读写的时候恰好reset被拉低了，导致i2c通信失败*/
/*这个reset触发的中断没什么用，因为在手势触发的中断中已经将手势动作上报了*/
/*在reset之前将中断关掉，reset之后再打开*/
	//fts_irq_disable();
#if (!FTS_CHIP_IDC)
    fts_reset_proc(200);
#endif
	//fts_irq_enable();


    /* Before read/write TP register, need wait TP to valid */
    fts_tp_state_recovery(fts_i2c_client);

#if FTS_ESDCHECK_EN
    fts_esdcheck_resume();
#endif

#if FTS_GESTURE_EN
    if (fts_gesture_resume(fts_i2c_client) == 0)
    {
        FTS_FUNC_EXIT();
        return;
    }
#endif


#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    fts_sensor_enable(fts_i2c_client);
#else
    fts_irq_enable();
#endif
}

/*****************************************************************************
*  TPD Device Driver
*****************************************************************************/
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static DEVICE_ATTR(tpd_scp_ctrl, 0664, show_scp_ctrl, store_scp_ctrl);
#endif

struct device_attribute *fts_attrs[] =
{
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
    &dev_attr_tpd_scp_ctrl,
#endif
};

static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = FTS_DRIVER_NAME,
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
    .attrs = {
        .attr = fts_attrs,
        .num  = ARRAY_SIZE(fts_attrs),
    },
};

/*****************************************************************************
*  Name: tpd_driver_init
*  Brief: 1. Get dts information
*         2. call tpd_driver_add to add tpd_device_driver
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int __init tpd_driver_init(void)
{
    FTS_FUNC_ENTER();
    FTS_INFO("Driver version: %s", FTS_DRIVER_VERSION);
    tpd_get_dts_info();
    if (tpd_driver_add(&tpd_device_driver) < 0)
    {
        FTS_ERROR("[TPD]: Add FTS Touch driver failed!!");
    }

    FTS_FUNC_EXIT();
    return 0;
}

/*****************************************************************************
*  Name: tpd_driver_exit
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void __exit tpd_driver_exit(void)
{
    FTS_FUNC_ENTER();
    tpd_driver_remove(&tpd_device_driver);
    FTS_FUNC_EXIT();
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);


MODULE_AUTHOR("FocalTech Driver Team");
MODULE_DESCRIPTION("FocalTech Touchscreen Driver for Mediatek");
MODULE_LICENSE("GPL v2");

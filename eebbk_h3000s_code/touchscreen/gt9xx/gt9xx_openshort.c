/* drivers/input/touchscreen/gt9xx_shorttp.c
 *
 * 2010 - 2012 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Version:1.0
 * Author: meta@goodix.com
 * Accomplished Date:2012/10/20
 * Revision record:
 *
 */

#include "include/gt9xx_openshort.h"


//extern s32 gtp_i2c_read(struct i2c_client *, u8 *, s32);
//extern s32 gtp_i2c_write(struct i2c_client *, u8 *, s32);
//extern void gtp_reset_guitar(struct i2c_client*, s32);
//extern s32 gup_enter_update_mode(struct i2c_client *);
//extern s32 gup_leave_update_mode(void);
extern s32 gtp_send_cfg(struct i2c_client *client);
//extern s32 gtp_read_version(struct i2c_client *client, u16* version);
//extern void gtp_irq_disable(struct goodix_ts_data *ts);
//extern void gtp_irq_enable(struct goodix_ts_data *ts);
//extern int gtp_get_sensorid();

u8  gt9xx_drv_num = MAX_DRIVER_NUM;	// default driver and sensor number
u8  gt9xx_sen_num = MAX_SENSOR_NUM;
u16 gt9xx_pixel_cnt = MAX_DRIVER_NUM * MAX_SENSOR_NUM;
u16 gt9xx_sc_pxl_cnt = MAX_DRIVER_NUM * MAX_SENSOR_NUM;
struct gt9xx_short_info *short_sum;

#if GTP_HAVE_TOUCH_KEY
    u8 gt9xx_sc_drv_num;
    u8 key_is_isolated;	// 0: no, 1: yes
    u8 key_iso_pos[5];
#endif

u16 max_limit_value = 4000;     // screen max limit
u16 min_limit_value = 500;      // screen min limit
u16 max_limit_key = 4200;       // key_val max limit
u16 min_limit_key = 625;        // key_val min limit

static s32 sample_set_num = 16;
static u32 default_test_types = _MAX_TEST | _MIN_TEST | _KEY_MAX_TEST | _KEY_MIN_TEST;
static u8  rslt_buf_idx = 0;
static s32 *test_rslt_buf;
static struct gt9xx_open_info *touchpad_sum;

#define _MIN_ERROR_NUM      (sample_set_num	* 9 / 10)

static char *result_lines[50];
static char tmp_info_line[80];
static u8 RsltIndex;
static u8 open_test_flag = 0;

static void append_info_line(void)
{
    if (strlen(tmp_info_line) != 0)
    {
        result_lines[RsltIndex] = (char *)kzalloc(strlen(tmp_info_line), GFP_KERNEL);
        memcpy(result_lines[RsltIndex], tmp_info_line, strlen(tmp_info_line));
    }
    if (RsltIndex != 49)
        ++RsltIndex;
    else {
        kfree(result_lines[RsltIndex]);
    }
}



#define SET_INFO_LINE_INFO(fmt, args...)       do{ memset(tmp_info_line, '\0', 80);\
                                                   sprintf(tmp_info_line, "<Sysfs-INFO>"fmt"\n", ##args);\
                                                   GTP_INFO(fmt, ##args);\
                                                append_info_line();} while(0)

#define SET_INFO_LINE_ERR(fmt, args...)        do { memset(tmp_info_line, '\0', 80);\
                                                   sprintf(tmp_info_line, "<Sysfs-ERR>"fmt"\n", ##args);\
                                                   GTP_ERROR(fmt, ##args);\
                                                   append_info_line();}while(0)


static u8 cfg_drv_order[MAX_DRIVER_NUM];
static u8 cfg_sen_order[MAX_SENSOR_NUM];

/*
 * Initialize cfg_drv_order and cfg_sen_order, which is used for report short channels
 *
 */

s32 gt9xx_short_parse_cfg(void)
{
    u8 i = 0;
	u8 drv_num = 0, sen_num = 0;
	drv_num = (config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT-GT9_REG_CFG_BEG] & 0x1F)
						+ (config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT+1 -GT9_REG_CFG_BEG] & 0x1F);
	sen_num = (config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT+2-GT9_REG_CFG_BEG] & 0x0F)
						+ ((config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT+2-GT9_REG_CFG_BEG]>>4) & 0x0F);

	//dpt
	//if (gtp_get_sensorid() == 0 || gtp_get_sensorid() == 2)
	{
		gt9xx_drv_num -= 1;
	}

	GTP_INFO("drv_num = %d, sen_num = %d\n", drv_num, sen_num);

	if (drv_num < MIN_DRIVER_NUM || drv_num > MAX_DRIVER_NUM)
	{
		GTP_ERROR("driver number error!");
		return FAIL;
	}
	if (sen_num < MIN_SENSOR_NUM || sen_num > MAX_SENSOR_NUM)
	{
		GTP_ERROR("sensor number error!");
		return FAIL;
	}
	// get sensor and driver order
	for (i = 0; i < sen_num; ++i)
	{
	    cfg_sen_order[i] = config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_ORD - GT9_REG_CFG_BEG + i];
	}
	cfg_sen_order[i] = 0xff;
	for (i = 0; i < drv_num; ++i)
	{
	    cfg_drv_order[i] = config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_DRV_ORD - GT9_REG_CFG_BEG + i];
	}
	cfg_drv_order[i] = 0xff;

	GTP_DEBUG_ARRAY(cfg_sen_order, MAX_SENSOR_NUM);
	GTP_DEBUG_ARRAY(cfg_drv_order, MAX_DRIVER_NUM);
    return SUCCESS;
}

/*
 * @param:
 *      phy_chnl: ic detected short channel, is_driver: it's driver or not
 * @Return:
 *      0xff: the ic channel is not used, otherwise: the tp short channel
 */
u8 gt9_get_short_tp_chnl(u8 phy_chnl, u8 is_driver)
{
    u8 i = 0;
    if (is_driver) {
        for (i = 0; i < MAX_DRIVER_NUM; ++i)
        {
            if (cfg_drv_order[i] == phy_chnl) {
                return i;
            }
            else if (cfg_drv_order[i] == 0xff) {
                return 0xff;
            }
        }
    }
    else
    {
        for (i = 0; i < MAX_SENSOR_NUM; ++i)
        {
            if (cfg_sen_order[i] == phy_chnl) {
                return i;
            }
            else if (cfg_sen_order[i] == 0xff) {
                return 0xff;
            }
        }
    }
    return 0xff;
}


u8 gt9xx_set_ic_msg(struct i2c_client *client, u16 addr, u8 val)
{
    s32 i = 0;
    u8 msg[3];

    msg[0] = (addr >> 8) & 0xff;
    msg[1] = addr & 0xff;
    msg[2] = val;

    for (i = 0; i < 5; i++)
    {
        if (gtp_i2c_write(client, msg, GTP_ADDR_LENGTH + 1) > 0)
        {
            break;
        }
    }

    if (i >= 5)
    {
        GTP_ERROR("Set data to 0x%02x%02x failed!", msg[0], msg[1]);
        return FAIL;
    }

    return SUCCESS;
}

static s32 gtp_i2c_end_cmd(struct i2c_client *client)
{
    u8  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
    s32 ret = 0;

    ret = gtp_i2c_write(client, end_cmd, 3);
    if (ret < 0)
    {
        SET_INFO_LINE_INFO("I2C write end_cmd  error!");
    }
    return ret;
}

s32 gtp_parse_config(void)
{
#if GTP_HAVE_TOUCH_KEY
    u8 i = 0;
    u8 key_pos = 0;
#endif
    gt9xx_drv_num = (config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT-GT9_REG_CFG_BEG] & 0x1F)
                    + (config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT+1 -GT9_REG_CFG_BEG] & 0x1F);
    gt9xx_sen_num = (config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT+2-GT9_REG_CFG_BEG] & 0x0F)
                    + ((config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_SEN_DRV_CNT+2-GT9_REG_CFG_BEG]>>4) & 0x0F);

    //dpt
    //if (gtp_get_sensorid() == 0 || gtp_get_sensorid() == 2)
    {
	gt9xx_drv_num -= 1;
    }

    GTP_INFO("gt9xx_drv_num = %d, gt9xx_sen_num = %d\n", gt9xx_drv_num, gt9xx_sen_num);

    if (gt9xx_drv_num < MIN_DRIVER_NUM || gt9xx_drv_num > MAX_DRIVER_NUM)
    {
        SET_INFO_LINE_ERR("driver number error!");
        return FAIL;
    }
    if (gt9xx_sen_num < MIN_SENSOR_NUM || gt9xx_sen_num > MAX_SENSOR_NUM)
    {
        SET_INFO_LINE_ERR("sensor number error!");
        return FAIL;
    }
    gt9xx_sc_pxl_cnt = gt9xx_pixel_cnt = gt9xx_drv_num * gt9xx_sen_num;

#if GTP_HAVE_TOUCH_KEY
    gt9xx_sc_drv_num = gt9xx_drv_num - (config_tp_cfg[0x804E - GT9_REG_CFG_BEG] & 0x01);

    key_is_isolated = 0;
    key_iso_pos[0] = 0;
    for (i = 0; i < 4; ++i)
    {
        key_pos = config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_KEY_VAL - GT9_REG_CFG_BEG + i]%0x08;
        GTP_DEBUG("key_val[%d] = 0x%x", i+1, config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_KEY_VAL - GT9_REG_CFG_BEG + i]);
        if (key_pos != 0)
        {
            break;
        }
        else
        {
            key_iso_pos[0]++;
            key_iso_pos[i+1] = config_tp_cfg[GTP_ADDR_LENGTH + GT9_REG_KEY_VAL - GT9_REG_CFG_BEG + i]/8;
        }
    }
    if (i == 4)
    {
        key_is_isolated = 1;
    }
    gt9xx_sc_pxl_cnt = gt9xx_pixel_cnt - (gt9xx_drv_num-gt9xx_sc_drv_num) * gt9xx_sen_num;
    GTP_DEBUG("drv num: %d, sen num: %d, sc drv num: %d", gt9xx_drv_num, gt9xx_sen_num, gt9xx_sc_drv_num);
    if (key_is_isolated)
    {
        GTP_DEBUG("[%d key(s)]: %d, %d, %d, %d", key_iso_pos[0], key_iso_pos[1], key_iso_pos[2], key_iso_pos[3], key_iso_pos[4]);
    }
#endif

    return SUCCESS;
}



/*
 * Function:
 * 		write one byte to specified register
 * Input:
 * 		reg: the register address
 * 		val: the value to write into
 * Return:
 * 		i2c_write function return
 */
s32 gtp_write_register(struct i2c_client * client, u16 reg, u8 val)
{
    u8 buf[3];
    buf[0] = (u8) (reg >> 8);
    buf[1] = (u8) reg;
    buf[2] = val;
    return gtp_i2c_write(client, buf, 3);
}
/*
 * Function:
 * 		read one byte from specified register into buf
 * Input:
 *		reg: the register
 * 		buf: the buffer for one byte
 * Return:
 *		i2c_read function return
 */
s32 gtp_read_register(struct i2c_client * client, u16 reg, u8* buf)
{
    buf[0] = (u8)(reg >> 8);
    buf[1] = (u8)reg;
    return gtp_i2c_read(client, buf, 3);
}

/*
 * Function:
 * 		burn dsp_short code
 * Input:
 * 		i2c_client
 * Return:
 * 		SUCCESS: burning succeed, FAIL: burning failed
 */
s32 gtp_burn_dsp_short(struct i2c_client *client)
{
    s32 ret = 0;
    u8 *check_buf;
    u16 i = 0;
    u8 j = 0;

    GTP_DEBUG("Start writing dsp_short code");
    dsp_short[0] = (u8)(GTP_REG_DSP_SHORT >> 8);
    dsp_short[1] = (u8)(GTP_REG_DSP_SHORT);

    ret = gtp_i2c_write(client, dsp_short, 6144+2);
    if ( ret < 0 )
    {
        SET_INFO_LINE_ERR("write dsp_short code failed!");
        return FAIL;
    }
    // check code: 0xC000~0xCFFF
    GTP_DEBUG("Start checking dsp_short code");
    check_buf = (u8*)kmalloc(sizeof(u8) * (2048+2), GFP_KERNEL);
    if (!check_buf)
    {
        SET_INFO_LINE_ERR("failed to allocate memory for check buffer!");
        return FAIL;
    }

    while (j < 3)
    {
        msleep(20);
        check_buf[0] = (u8)((GTP_REG_DSP_SHORT+j*2048) >> 8);
        check_buf[1] = (u8)(GTP_REG_DSP_SHORT+j*2048);
        ret = gtp_i2c_read(client, check_buf, 2048+2);
        if (ret < 0)
        {
            kfree(check_buf);
            return FAIL;
        }
        for (i = 0; i < 2048; ++i)
        {
            if (check_buf[i+2] != dsp_short[i+2+j*2048])
            {
                SET_INFO_LINE_ERR("check dsp_short code failed!");
                kfree(check_buf);
                return FAIL;
            }
        }
        j++;
    }
    kfree(check_buf);
    return SUCCESS;
}
/*
 * Function:
 * 		check the resistor between shortlike channels if less than threshold confirm as short
 * INPUT:
 *		Short like Information struct pointer
 * Returns:
 *		SUCCESS: it's shorted FAIL: otherwise
 */
s32 gtp_short_resist_check(struct gt9xx_short_info *short_node)
{
    s32 short_resist = 0;
    struct gt9xx_short_info *node = short_node;
    u8 master = node->master;
    u8 slave = node->slave;
    u8 chnnl_tx[4] = { GT9_DRV_HEAD|13, GT9_DRV_HEAD|28,GT9_DRV_HEAD|29, GT9_DRV_HEAD|42 };
    s32 numberator = 0;
    u32 amplifier = 1000;  // amplify 1000 times to emulate float computing

    // Tx-ABIST & Tx_ABIST
    if ((((master > chnnl_tx[0]) && (master <= chnnl_tx[1])) &&
        ((slave > chnnl_tx[0]) && (slave <= chnnl_tx[1])) ) ||
        (((master > chnnl_tx[2]) && (master <= chnnl_tx[3])) &&
        ((slave > chnnl_tx[2]) && (master <= chnnl_tx[3]))))
    {
        numberator = node->self_data * 60 * amplifier;
        short_resist = numberator/(node->short_code) - 40 * amplifier;
    }
    // the receiver is Rx-odd(1,3,5)
    else if ((node->slave & (GT9_DRV_HEAD | 0x01)) == 0x01)
    {
        numberator = node->self_data * 40 * amplifier;
        short_resist = numberator/node->short_code - 40 * amplifier;
    }
    else
    {
        numberator = node->self_data * 60 * amplifier;
        short_resist = numberator / node->short_code - 60 * amplifier;
    }
    GTP_DEBUG("self_data = %d" ,node->self_data);
    GTP_DEBUG("master = 0x%x, slave = 0x%x", node->master, node->slave);
    GTP_DEBUG("numberator = %d, short_code = %d, base = %d, short_resist = %d", numberator, node->short_code, numberator/node->short_code,  short_resist);
    short_resist = (short_resist < 0) ? 0 : short_resist;
    if (short_resist < (GT9XX_RESIST_THRESHOLD * amplifier))
    {
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}



/*
 * Function:
 * 		compute the result, whether there are shorts or not
 * Input:
 * 		i2c_client
 * Return:
 * 		SUCCESS
 */
s32 gtp_compute_rslt(struct i2c_client *client)
{
    u16 short_code;
    u16 sh_idx = 0;		// short code index in result_buf
    u8 i = 0, j = 0;
    u16 result_addr = 0x8806;	// result first started ADDRESS
    u8 *result_buf;
    s32 ret = 0;
    u16 data_len = 3 + (MAX_DRIVER_NUM + MAX_SENSOR_NUM) * 2 + 2; // a short data frame length
    struct gt9xx_short_info short_node;
    u8 node_idx = 0; // short_sum index: 0~GT9_INFO_NODE_MAX

    result_buf = (u8*)kmalloc(sizeof(u8) * (data_len+2), GFP_ATOMIC);
    short_sum = (struct gt9xx_short_info *) kmalloc(sizeof(struct gt9xx_short_info) * GT9_INFO_NODE_MAX, GFP_ATOMIC);

    if (!result_buf || !short_sum)
    {
        SET_INFO_LINE_ERR("allocate memory for short result failed!");
        return FAIL;
    }
    // Tx(driver) first, low addr stores high byte
    for (i = 0; i < MAX_DRIVER_NUM; ++i)
    {
        result_buf[0] = (u8) (result_addr >> 8);
        result_buf[1] = (u8) (result_addr);
        ret = gtp_i2c_read(client, result_buf, data_len+2);
        if (ret < 0)
        {
            SET_INFO_LINE_ERR("read result data failed!");
        }

        short_node.master = result_buf[2];
        short_node.self_data = (u16)(result_buf[3] << 8) +	(u16) result_buf[4];

        GTP_DEBUG_ARRAY(result_buf, 2+data_len);

        // test short between Tx and Tx
        for (j = i+1, sh_idx = 5+j*2 ; j < MAX_DRIVER_NUM; ++j, sh_idx += 2)
        {
            short_code = (u16)(result_buf[sh_idx] << 8) +(u16)result_buf[sh_idx+1];
            if ( short_code > GT9XX_SHORT_THRESHOLD)
            {
                if (j > 25)
                {
                    j++;
                }
                short_node.slave = j|GT9_DRV_HEAD;
                short_node.short_code = short_code;

                ret = gtp_short_resist_check(&short_node);
                if (ret == SUCCESS && node_idx < GT9_INFO_NODE_MAX)
                {
                    short_sum[node_idx++] = short_node;
                }
            }
        }
        // test short between Tx and Rx
        for (j = 0; j < MAX_SENSOR_NUM; ++j, sh_idx += 2)
        {
            short_code = (u16)(result_buf[sh_idx] << 8) +(u16)( result_buf[sh_idx+1]);
            if (short_code > GT9XX_SHORT_THRESHOLD)
            {
                short_node.slave = j | GT9_SEN_HEAD;
                short_node.short_code = short_code;
                ret = gtp_short_resist_check(&short_node);
                if (ret == SUCCESS && node_idx < GT9_INFO_NODE_MAX)
                {
                    short_sum[node_idx++] = short_node;
                }
            }
        }
        result_addr += data_len;
    }
    // Rx(sensor) followed
    for (i = 0; i < MAX_SENSOR_NUM; i += 2)
    {
        result_buf[0] = (u8)(result_addr >> 8);
        result_buf[1] = (u8)result_addr;
        gtp_i2c_read(client, result_buf, data_len+2);

        short_node.master = result_buf[2];
        short_node.self_data = (u16)(result_buf[3] << 8) + (u16)result_buf[4];
        GTP_DEBUG_ARRAY(result_buf, data_len+2);
        // test short between Rx and Rx
        for (j = 1, sh_idx=5 + MAX_DRIVER_NUM*2+j*2; j < MAX_SENSOR_NUM; ++j, sh_idx+=2)
        {
            if (j == i || ( (j < i) && (j&0x01)==0))
            {	// if j less than i and  j is even , j less than i
                continue;
            }
            short_code = (u16)(result_buf[sh_idx] << 8) +(u16) result_buf[sh_idx+1];
            if (short_code > GT9XX_SHORT_THRESHOLD)
            {
                short_node.slave = j | GT9_SEN_HEAD;
                short_node.short_code = short_code;
                ret = gtp_short_resist_check(&short_node);
                if (ret == SUCCESS && node_idx < GT9_INFO_NODE_MAX)
                {
                    short_sum[node_idx++] = short_node;
                }
            }
        }
        result_addr += data_len;
    }

    if (node_idx == 0)
    {
        SET_INFO_LINE_INFO("There is no short channels!");
    }
    else
    {
        SET_INFO_LINE_INFO("There are %d  pair(s) short channels:", node_idx);
        for (i = 0; i < node_idx; ++i)
        {
            if ((short_sum[i].master & GT9_DRV_HEAD) == GT9_DRV_HEAD)
            {
                if (short_sum[i].master > (26 | GT9_DRV_HEAD))
                {
                    short_sum[i].master--;
                }
                SET_INFO_LINE_INFO("[%d]Drv %d", i+1, gt9_get_short_tp_chnl(short_sum[i].master-GT9_DRV_HEAD, 1));
            }
            else
            {
                SET_INFO_LINE_INFO("[%d]Sen %d", i+1, gt9_get_short_tp_chnl(short_sum[i].master, 0));
            }
            if ((short_sum[i].slave & GT9_DRV_HEAD) == GT9_DRV_HEAD)
            {
                if (short_sum[i].slave > (26 | GT9_DRV_HEAD))
                {
                    short_sum[i].slave--;
                }
                SET_INFO_LINE_INFO("   Drv %d", gt9_get_short_tp_chnl(short_sum[i].slave-GT9_DRV_HEAD, 1));
            }
            else
            {
                SET_INFO_LINE_INFO("   Sen %d", gt9_get_short_tp_chnl(short_sum[i].slave, 0));
            }
        }
    }
    kfree(short_sum);
    kfree(result_buf);
    return SUCCESS;
}


/*
 * leave short test
 */
void gt9xx_leave_short_test(struct i2c_client *client)
{
    // boot from rom and download code from flash to ram
    gtp_write_register(client, _rRW_MISCTL__BOOT_CTL_, 0x99);
    gtp_write_register(client, _rRW_MISCTL__BOOTCTL_B0_, 0x08);

    gtp_reset_guitar(client, 20);
    msleep(100);

    gtp_send_cfg(client);
    SET_INFO_LINE_INFO("---gt9 short test end---");
}


/*
 * Function:
 *		gt9 series ic short test function
 * Input:
 * 		I2c_client, i2c device
 * Return:
 * 		SUCCESS: test succeed, FAIL: test failed
 */
s32 gt9xx_short_test(struct i2c_client * client)
{
    s32 ret = 0;
    u8 i = 0;
    u8 opr_buf[10] = {0};
    u8 retry = 0;
//    struct goodix_ts_data *ts;

//    ts = i2c_get_clientdata(i2c_client_point);
    gtp_irq_disable();
    // step 1: reset guitar, delay 1ms,  hang up ss51 and dsp
    SET_INFO_LINE_INFO("---gtp short test---");
    SET_INFO_LINE_INFO("Step 1: reset guitar, hang up ss51 dsp");

    gt9xx_short_parse_cfg();
#if 0
    GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);
    msleep(40);
    GTP_GPIO_OUTPUT(GTP_INT_PORT, (client->addr == 0x14));
    msleep(6);
    GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);
#endif
    gtp_reset_guitar(client, 60);
    //msleep(60);
    //gtp_write_register(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	while(retry++ < 200)
	{
	    //step4:Hold ss51 & dsp
	    ret = gtp_write_register(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	    //step5:DSP_CK and DSP_ALU_CK PowerOn
	    ret += gtp_write_register(client, 0x4010, 0x00);
	    if(ret == SUCCESS + SUCCESS)
	    {
	       SET_INFO_LINE_INFO("Enter update Hold ss51 SUCCESS.");
		    break;
	    }
	}
	if(retry >= 200)
	{
	    SET_INFO_LINE_ERR("Enter update Hold ss51 failed.");
	}

    msleep(60);
    for (i = 0; i < 200; ++i)
    {
        msleep(2);
        ret = gtp_read_register(client, _rRW_MISCTL__SWRST_B0_, opr_buf);
        if (opr_buf[2] == 0x0c)
        {
            break;
        }
        else
        {
            gtp_write_register(client, _rRW_MISCTL__SWRST_B0_, 0x0c);
        }
    }
    if (i == 200)
    {
        GTP_DEBUG("ret = 0x%d", ret);
        SET_INFO_LINE_ERR("step1: hang up ss51 and dsp failed!");
        goto short_test_exit;
    }

    // step2: burn dsp_short code
    SET_INFO_LINE_INFO("step 2: burn dsp_short code");
    gtp_write_register(client, _bRW_MISCTL__TMR0_EN, 0x00); // clear watchdog
    gtp_write_register(client, _bRW_MISCTL__CACHE_EN, 0x00); // clear cache
    gtp_write_register(client, _rRW_MISCTL__BOOTCTL_B0_, 0x02); // boot from sram
    gtp_write_register(client, _bWO_MISCTL__CPU_SWRST_PULSE, 0x01); // reset software
    gtp_write_register(client, _bRW_MISCTL__SRAM_BANK, 0x00); // select bank 0
    gtp_write_register(client, _bRW_MISCTL__MEM_CD_EN, 0x01); // allow AHB bus accessing code sram

    // ---: burn dsp_short code
    ret = gtp_burn_dsp_short(client);
    if (ret == FAIL)
    {
        SET_INFO_LINE_ERR("step2: burn dsp_short failed!");
        goto short_test_exit;
    }

    // step3: run dsp_short, read results
    SET_INFO_LINE_INFO("step 3: run dsp_short code, confirm it's runnin'");
    gtp_write_register(client, _rRW_MISCTL__SHORT_BOOT_FLAG, 0x00);	// clear dsp_short running flag
    gtp_write_register(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x03);//set scramble

	ret = gt9xx_set_ic_msg(client, _bWO_MISCTL__CPU_SWRST_PULSE, 0x01);           //20121114
    gtp_write_register(client, _rRW_MISCTL__SWRST_B0_, 0x08);	// release dsp

    msleep(150);
    // confirm dsp is running
    gtp_read_register(client, _rRW_MISCTL__SHORT_BOOT_FLAG, opr_buf);
    if (opr_buf[2] != 0xAA)
    {
        SET_INFO_LINE_ERR("step 3: dsp is not running!");
        goto short_test_exit;
    }

    // step4: host configure ic, get test result
    SET_INFO_LINE_INFO("Step 4: host config ic, get test result");
    // write short threshold
    opr_buf[0] = (u8) (GTP_REG_SHORT_TH >> 8);
    opr_buf[1] = (u8) GTP_REG_SHORT_TH;
    opr_buf[2] = (u8) (GT9XX_SHORT_THRESHOLD >> 8);
    opr_buf[3] = (u8) (GT9XX_SHORT_THRESHOLD);
    gtp_i2c_write(client, opr_buf, 4);
    // write short double threshold(double check is not used)
    opr_buf[1] += 2;
    gtp_i2c_write(client, opr_buf, 4);

    // clear waiting flag, run dsp
    gtp_write_register(client, _rRW_MISCTL__SHORT_BOOT_FLAG, 0x04);

    // inquirying test status until it's okay
    for (i = 0; ; ++i)
    {
        gtp_read_register(client, 0x8800, opr_buf);
        if (opr_buf[2] == 0x88)
        {
            break;
        }
        msleep(20);
        if ( i > 500 )
        {
            SET_INFO_LINE_ERR("step 4: inquiry test status timeout!");
            goto short_test_exit;
        }
    }
    // step 5: compute the result
    // short flag: bit0: Rx-Rx, bit1: Tx-Tx, bit2: Tx-Rx
    //  if low 3 bits all equal to 0, there is no short channels, otherwise compute the result
    gtp_read_register(client, 0x8801, opr_buf);
    GTP_DEBUG("short_flag = 0x%x", opr_buf[2]&0x7);
    if ((opr_buf[2] & 0x7) != 0)
    {
        ret = gtp_compute_rslt(client);
    }
    else
    {
        SET_INFO_LINE_INFO("There is no short channels!");
        ret = SUCCESS;
    }
    gt9xx_leave_short_test(client);
    gtp_irq_enable();
    return ret;

short_test_exit:
    gt9xx_leave_short_test(client);
    gtp_irq_enable();
    return FAIL;
}

u32 endian_mode(void)
{
    union {s32 i; s8 c;}endian;

    endian.i = 1;

    if (1 == endian.c)
    {
        return MYBIG_ENDIAN;
    }
    else
    {
        return MYLITLE_ENDIAN;
    }
}
/*
*********************************************************************************************************
* Function:
*	send read rawdata cmd
* Input:
*	i2c_client* client: i2c device
* Return:
* 	SUCCESS: send process succeed, FAIL: failed
*********************************************************************************************************
*/
s32 gt9_read_raw_cmd(struct i2c_client* client)
{
    u8 raw_cmd[3] = {(u8)(GTP_REG_READ_RAW >> 8), (u8)GTP_REG_READ_RAW, 0x01};
    s32 ret = -1;

    ret = gtp_i2c_write(client, raw_cmd, 3);
    if(ret <= 0)
    {
        SET_INFO_LINE_ERR("i2c write failed.");
        return FAIL;
    }
    msleep(10);
    return SUCCESS;
}

s32 gt9_read_coor_cmd(struct i2c_client *client)
{
    u8 raw_cmd[3] = {(u8)(GTP_REG_READ_RAW >> 8), (u8)GTP_REG_READ_RAW, 0x0};
    s32 ret = -1;

    ret = gtp_i2c_write(client, raw_cmd, 3);
    if (ret < 0)
    {
        SET_INFO_LINE_ERR("i2c write coor cmd failed!");
        return FAIL;
    }
    msleep(10);
    return SUCCESS;
}
/*
*********************************************************************************************************
* Function:
*	read rawdata from ic registers
* Input:
*	u16* data: rawdata buffer
* 	i2c_client* client: i2c device
* Return:
* 	SUCCESS: read process succeed, FAIL:  failed
*********************************************************************************************************
*/
s32 gtp_read_rawdata(struct i2c_client* client, u16* data)
{
    s32 ret = -1;
    u16 retry = 0;
    u8 read_state[3] = {(u8)(GTP_REG_RAW_READY>>8), (u8)GTP_REG_RAW_READY, 0};
    u16 i = 0, j = 0;
    u8 *read_rawbuf;
    u8 tail, head;

    read_rawbuf = (u8*)kmalloc(sizeof(u8) * (gt9xx_pixel_cnt * 2 + GTP_ADDR_LENGTH), GFP_KERNEL);

    if (NULL == read_rawbuf)
    {
        SET_INFO_LINE_ERR("failed to allocate for read_rawbuf");
        return FAIL;
    }
    read_rawbuf[0] = (u8)( GTP_REG_RAW_DATA >> 8);
    read_rawbuf[1] = (u8)( GTP_REG_RAW_DATA );

    if(data == NULL)
    {
        SET_INFO_LINE_ERR("Invalid raw buffer.");
        return FAIL;
    }

    msleep(10);
    while (retry++ < GTP_WAIT_RAW_MAX_TIMES)
    {
        ret = gtp_i2c_read(client, read_state, 3);
        if(ret <= 0)
        {
            SET_INFO_LINE_ERR("i2c read failed.return: %d", ret);
            continue;
        }
        if(read_state[GTP_ADDR_LENGTH] == 0x80)
        {
            GTP_DEBUG("Raw data is ready.");
            break;
        }
        if ((retry/10) == 0)
        GTP_DEBUG("read_state[2] = 0x%x", read_state[GTP_ADDR_LENGTH]);
	//msleep(5);
	msleep(100);
    }
    if (retry >= GTP_WAIT_RAW_MAX_TIMES)
    {
        SET_INFO_LINE_ERR("Wait raw data ready timeout.");
        return FAIL;
    }

    ret = gtp_i2c_read(client, read_rawbuf, GTP_ADDR_LENGTH + ((gt9xx_drv_num*gt9xx_sen_num)*2));
    if(ret <= 0)
    {
        SET_INFO_LINE_ERR("i2c read rawdata failed.");
        return FAIL;
    }
    gtp_i2c_end_cmd(client);	// clear buffer state

    if (endian_mode() == MYBIG_ENDIAN)
    {
        head = 0;
        tail =1;
        GTP_DEBUG("Big Endian.");
    }
    else
    {
        head = 1;
        tail = 0;
        GTP_DEBUG("Little Endian.");
    }

    for(i=0,j = 0; i < ((gt9xx_drv_num*gt9xx_sen_num)*2); i+=2)
    {
        data[i/2] = (u16)(read_rawbuf[i+head+GTP_ADDR_LENGTH]<<8) + (u16)read_rawbuf[GTP_ADDR_LENGTH+i+tail];
	#if 1	//GTP_DEBUG_ARRAY_ON
        if (i == 0) 
        {
			printk("%d ", data[i/2]);
			++j;
			if((j%10) == 0)
			printk("\n");
        }
	#endif
    }

    kfree(read_rawbuf);
    return SUCCESS;
}
/*
*********************************************************************************************************
* Function:
*	rawdata test initilization function
* Input:
*	u32 check_types: test items
*********************************************************************************************************
*/
static void gtp_raw_test_init(u32 check_types)
{
    u16 i = 0;

    test_rslt_buf = (s32*) kmalloc(sizeof(s32)*sample_set_num, GFP_ATOMIC);
    touchpad_sum = (struct gt9xx_open_info*) kmalloc(sizeof(struct gt9xx_open_info) * 4 * _BEYOND_REC_MAX, GFP_ATOMIC);
    if (NULL == test_rslt_buf || touchpad_sum == NULL)
    {
        SET_INFO_LINE_ERR("Test result buffer allocate failed!");
    }
    memset(touchpad_sum, 0, sizeof(struct gt9xx_open_info) * 4 * _BEYOND_REC_MAX);
    for (i = 0; i < gt9xx_drv_num*gt9xx_sen_num; i++)
    {
        if (i < sample_set_num)
        {
            test_rslt_buf[i] = _CHANNEL_PASS;
        }
    }
}

/*
*********************************************************************************************************
* Function:
*	touchscreen rawdata min limit test
* Input:
*	u16* raw_buf: rawdata buffer
*********************************************************************************************************
*/
static void gtp_raw_min_test(u16 *raw_buf)
{
    u16 i, j=0;
    u8 driver, sensor;
    u8 sum_base = 1 * _BEYOND_REC_MAX;
    u8 new_flag = 0;

    //printk("=================================================\n");
    //printk("rslt_buf_idx:%d\n", rslt_buf_idx);
    //printk(">>>drv_num:%d, sen_num:%d\n", gt9xx_drv_num, gt9xx_sen_num);
    for (i = 0; i < gt9xx_sc_pxl_cnt; i++)
    {
        if (raw_buf[i] < min_limit_value)
        {
            test_rslt_buf[rslt_buf_idx] |= _BEYOND_MIN_LIMIT;
            driver = (i/gt9xx_sen_num) + 1;
            sensor = (i%gt9xx_sen_num) + 1;
            new_flag = 0;
	    //printk("raw>>>%d, %d-%d\n", raw_buf[i], driver, sensor);
	    for (j = sum_base; j < (sum_base+_BEYOND_REC_MAX); ++j)
            {
                if (touchpad_sum[j].driver == 0)
                {
                    new_flag = 1;
                    break;
                }
                if ((driver == touchpad_sum[j].driver) && (sensor == touchpad_sum[j].sensor))
                {
                    touchpad_sum[j].times++;
                    new_flag = 0;
                    break;
                }
            }
            if (new_flag)	// new one
            {
                touchpad_sum[j].driver = driver;
                touchpad_sum[j].sensor = sensor;
                touchpad_sum[j].beyond_type |= _BEYOND_MIN_LIMIT;
                touchpad_sum[j].raw_val = raw_buf[i];
                touchpad_sum[j].times = 1;
            }
            else
            {
                continue;
            }
            GTP_DEBUG("[%d, %d]rawdata: %d, raw min limit: %d", driver, sensor, raw_buf[i], min_limit_value);
        }
    }
}

/*
*********************************************************************************************************
* Function:
*	touchscreen rawdata max limit test
* Input:
*	u16* raw_buf: rawdata buffer
*********************************************************************************************************
*/
static void gtp_raw_max_test(u16 *raw_buf)
{
    u16 i, j;
    u8 driver, sensor;
    u8 sum_base = 0 * _BEYOND_REC_MAX;
    u8 new_flag = 0;

    for (i = 0; i < gt9xx_sc_pxl_cnt; i++)
    {
        if (raw_buf[i] > max_limit_value)
        {
            test_rslt_buf[rslt_buf_idx] |= _BEYOND_MAX_LIMIT;
            driver = (i/gt9xx_sen_num) + 1;
            sensor = (i%gt9xx_sen_num) + 1;
            new_flag = 0;
            for (j = sum_base; j < (sum_base+_BEYOND_REC_MAX); ++j)
            {
                if (touchpad_sum[j].driver == 0)
                {
                    new_flag = 1;
                    break;
                }
                if ((driver == touchpad_sum[j].driver) && (sensor == touchpad_sum[j].sensor))
                {
                    touchpad_sum[j].times++;
                    new_flag = 0;
                    break;
                }
            }
            if (new_flag)	// new one
            {
                touchpad_sum[j].driver = driver;
                touchpad_sum[j].sensor = sensor;
                touchpad_sum[j].beyond_type |= _BEYOND_MAX_LIMIT;
                touchpad_sum[j].raw_val = raw_buf[i];
                touchpad_sum[j].times = 1;
            }
            else
            {
                continue;
            }
            GTP_DEBUG("[%d, %d]rawdata: %d, raw max limit: %d", driver, sensor, raw_buf[i], max_limit_value);
        }
    }
}

#if GTP_HAVE_TOUCH_KEY
/*
*********************************************************************************************************
* Function:
*	key rawdata max limit test
* Input:
*	u16* raw_buf: rawdata buffer
*********************************************************************************************************
*/
static void gtp_key_max_test(u16 *raw_buf)
{
    u16 i = 0, j = 1, k = 0;
    u8 key_cnt = key_iso_pos[0];
    u8 driver, sensor;
    u8 sum_base = 2 * _BEYOND_REC_MAX;
    u8 new_flag = 0;

    driver = gt9xx_drv_num;
    for (i = gt9xx_sc_pxl_cnt; i < gt9xx_pixel_cnt; ++i)
    {
        sensor = (i%gt9xx_sen_num) + 1;
        if (key_is_isolated)
        {
            if ((key_iso_pos[j] != sensor) || (key_cnt == 0))
            {
                continue;
            }
            else	// only test key pixel rawdata
            {
                --key_cnt;
                ++j;
            }
        }
        if (raw_buf[i] > max_limit_key)
        {
            test_rslt_buf[rslt_buf_idx] |= _BEYOND_KEY_MAX_LMT;
            new_flag = 0;
            for (k = sum_base; k < (sum_base+_BEYOND_REC_MAX); ++k)
            {
                if (touchpad_sum[k].driver == 0)
                {
                    new_flag = 1;
                    break;
                }
                if (touchpad_sum[k].sensor == sensor)
                {
                    touchpad_sum[k].times++;
                    new_flag = 0;
                    break;
                }
            }
            if (new_flag)	// new one
            {
                touchpad_sum[k].driver = driver;
                touchpad_sum[k].sensor = sensor;
                touchpad_sum[k].beyond_type |= _BEYOND_KEY_MAX_LMT;
                touchpad_sum[k].raw_val = raw_buf[i];
                touchpad_sum[k].times = 1;
                if (key_is_isolated)
                {
                    touchpad_sum[k].key = j-1;
                }
            }
            else
            {
                continue;
            }
            GTP_DEBUG("[%d, %d]key rawdata: %d, key max limit: %d", driver,sensor, raw_buf[i], max_limit_key);
        }
    }
}
/*
*********************************************************************************************************
* Function:
*	key rawdata min limit test
* Input:
*	u16* raw_buf: rawdata buffer
*********************************************************************************************************
*/
static void gtp_key_min_test(u16 *raw_buf)
{
    u16 i = 0, j = 1, k = 0;
    u8 key_cnt = key_iso_pos[0];
    u8 driver, sensor;
    u8 sum_base = 3 * _BEYOND_REC_MAX;
    u8 new_flag = 0;

    driver = gt9xx_drv_num;
    for (i = gt9xx_sc_pxl_cnt; i < gt9xx_pixel_cnt; ++i)
    {
        sensor = (i%gt9xx_sen_num) + 1;
        if (key_is_isolated)
        {
            if ((key_iso_pos[j] != sensor) || (key_cnt == 0))
            {
                continue;
            }
            else	// only test key pixel rawdata
            {
                --key_cnt;
                ++j;
            }
        }

        if (raw_buf[i] < min_limit_key)
        {
            test_rslt_buf[rslt_buf_idx] |= _BEYOND_KEY_MIN_LMT;
            new_flag = 0;
            for (k = sum_base; k < (sum_base + _BEYOND_REC_MAX); ++k)
            {
                if (touchpad_sum[k].driver == 0)
                {
                    new_flag = 1;
                    break;
                }
                if (sensor == touchpad_sum[k].sensor)
                {
                    touchpad_sum[k].times++;
                    break;
                }
            }
            if (new_flag)	// new one
            {
                touchpad_sum[k].driver = driver;
                touchpad_sum[k].sensor = sensor;
                touchpad_sum[k].beyond_type |= _BEYOND_KEY_MIN_LMT;
                touchpad_sum[k].raw_val = raw_buf[i];
                touchpad_sum[k].times = 1;
                if (key_is_isolated)
                {
                    touchpad_sum[k].key = j-1;
                }
            }
            else
            {
                continue;
            }
            GTP_DEBUG("[%d, %d]key rawdata: %d, key min limit: %d", driver, sensor, raw_buf[i], min_limit_key);
        }
    }
}
#endif
/*
*********************************************************************************************************
* Function:
*	analyse rawdata retrived from ic registers
* Input:
*	u16 *raw_buf, buffer for rawdata,
*   u32 check_types, test items
* Return:
*	SUCCESS: test process succeed, FAIL: failed
*********************************************************************************************************
*/
static u32 gtp_raw_test(u16 *raw_buf, u32 check_types)
{
    if (raw_buf == NULL)
    {
        GTP_DEBUG("Invalid raw buffer pointer!");
        return FAIL;
    }
    if (0 == check_types)
    {
        check_types = default_test_types;
    #if GTP_HAVE_TOUCH_KEY
        check_types |= _KEY_MAX_TEST | _KEY_MIN_TEST;
    #endif
    }

    if (check_types & _MAX_TEST)
    {
        gtp_raw_max_test(raw_buf);		// ?\C6\C1\D7\EE\B4\F3?\B2\E2\CA\D4
    }

    if (check_types & _MIN_TEST)
    {
        gtp_raw_min_test(raw_buf);		// ?\C6\C1\D7\EE§³?\B2\E2\CA\D4
    }
#if GTP_HAVE_TOUCH_KEY
    if (check_types & _KEY_MAX_TEST)
    {
        gtp_key_max_test(raw_buf);
    }
    if (check_types & _KEY_MIN_TEST)
    {
        gtp_key_min_test(raw_buf);
    }
#endif
    return SUCCESS;
}


/*
====================================================================================================
* Function:
* 	output the test result
* Return:
* 	return the result. if result == 0, the TP is ok, otherwise list the beyonds
====================================================================================================
*/

static s32 gtp_get_test_result(void)
{
    u16 i = 0, j = 0;
    u16 beyond_max_num = 0;			// beyond max limit test times
    u16 beyond_min_num = 0;			// beyond min limit test times
#if GTP_HAVE_TOUCH_KEY
    u16 beyond_key_max = 0;			// beyond key max limit test times
    u16 beyond_key_min = 0;			// beyond key min limit test times
#endif
    s32 result = _CHANNEL_PASS;

#if GTP_DEBUG_ON
    for (i = 0; i < 4 * _BEYOND_REC_MAX; ++i)
    {
        printk("(%2d, %2d)[%2d] ", touchpad_sum[i].driver, touchpad_sum[i].sensor, touchpad_sum[i].times);
        if (i && ((i+1) % 5 == 0))
        {
            printk("\n");
        }
    }
    printk("\n");
#endif

    for (i = 0; i < sample_set_num; ++i)
    {
        if (test_rslt_buf[i] & _BEYOND_MAX_LIMIT)
        {
            beyond_max_num++;
        }
        if (test_rslt_buf[i] & _BEYOND_MIN_LIMIT)
        {
            beyond_min_num++;
        }
#if GTP_HAVE_TOUCH_KEY
        if (test_rslt_buf[i] & _BEYOND_KEY_MAX_LMT)
        {
            beyond_key_max++;
        }
        if (test_rslt_buf[i] & _BEYOND_KEY_MIN_LMT)
        {
            beyond_key_min++;
        }
#endif
    }
    GTP_DEBUG("_MIN_ERROR_NUM:%d\n", _MIN_ERROR_NUM);
    GTP_DEBUG("beyond_max_num:%d\n", beyond_max_num);
    GTP_DEBUG("beyond_min_num:%d\n", beyond_min_num);
    if (beyond_max_num > _MIN_ERROR_NUM)
    {
        result |= _BEYOND_MAX_LIMIT;
        j = 0;
        SET_INFO_LINE_INFO("Beyond Max Limit Points Info: ");
        for (i = 0; i < _BEYOND_REC_MAX; ++i)
        {
            if (touchpad_sum[i].driver == 0)
            {
                break;
            }
            SET_INFO_LINE_INFO("  [%d]Drv: %d, Sen: %d[%d]", ++j, touchpad_sum[i].driver, touchpad_sum[i].sensor, touchpad_sum[i].times);
        }
    }
    if (beyond_min_num > _MIN_ERROR_NUM)
    {
        result |= _BEYOND_MIN_LIMIT;
        SET_INFO_LINE_INFO("Beyond Min Limit Points Info:");
        j = 0;
        for (i = _BEYOND_REC_MAX; i < (2*_BEYOND_REC_MAX); ++i)
        {
            if (touchpad_sum[i].driver == 0)
            {
                break;
            }
            SET_INFO_LINE_INFO("  [%d]Drv: %d, Sen: %d[%d]", ++j, touchpad_sum[i].driver, touchpad_sum[i].sensor, touchpad_sum[i].times);
        }
    }
#if GTP_HAVE_TOUCH_KEY
    if (beyond_key_max > _MIN_ERROR_NUM)
    {
        result |= _BEYOND_KEY_MAX_LMT;
        SET_INFO_LINE_INFO("Beyond Key Max Limit Key Info:");
        for (i = 2*_BEYOND_REC_MAX; i < (3*_BEYOND_REC_MAX); ++i)
        {
            if (touchpad_sum[i].driver == 0)
            {
                break;
            }
            SET_INFO_LINE_INFO("  [%d]Drv: %d, Sen: %d[%d]", touchpad_sum[i].key, touchpad_sum[i].driver, touchpad_sum[i].sensor, touchpad_sum[i].times);
        }
    }
    if (beyond_key_min > _MIN_ERROR_NUM)
    {
        result |= _BEYOND_KEY_MIN_LMT;
        SET_INFO_LINE_INFO("Beyond Key Min Limit Key Info:");
        for (i = 3*_BEYOND_REC_MAX; i < (4*_BEYOND_REC_MAX); ++i)
        {
            if (touchpad_sum[i].driver == 0)
            {
                break;
            }
            SET_INFO_LINE_INFO("  [%d]Drv: %d, Sen: %d[%d]", touchpad_sum[i].key, touchpad_sum[i].driver, touchpad_sum[i].sensor, touchpad_sum[i].times);
        }
    }
#endif

    if (result == 0)
    {
        SET_INFO_LINE_INFO("[TEST SUCCEED]: The TP is ok!");
        return result;
    }
    SET_INFO_LINE_INFO("[TEST FAILED]:");
    if (result & _BEYOND_MAX_LIMIT)
    {
        SET_INFO_LINE_INFO("  Beyond Raw Max Limit[Max Limit: %d]", max_limit_value);
    }
    if (result & _BEYOND_MIN_LIMIT)
    {
        SET_INFO_LINE_INFO("  Beyond Raw Min Limit[Min Limit: %d]", min_limit_value);
    }
#if GTP_HAVE_TOUCH_KEY
    if (result & _BEYOND_KEY_MAX_LMT)
    {
        SET_INFO_LINE_INFO("  Beyond KeyVal Max Limit[Key Max Limit: %d]", max_limit_key);
    }
    if (result & _BEYOND_KEY_MIN_LMT)
    {
        SET_INFO_LINE_INFO("  Beyond KeyVal Min Limit[Key Min Limit: %d]", min_limit_key);
    }
#endif

    return result;
}

/*
 ===================================================
 * Function:
 * 		test gt9 series ic open test
 * Input:
 * 		client, i2c_client
 * Return:
 * 		SUCCESS: test process success, FAIL, test process failed
 *
 ===================================================
*/

s32 gt9xx_open_test(struct i2c_client * client)
{
    u16 i = 0;
    s32 ret = 0; // SUCCESS, FAIL
//    struct goodix_ts_data *ts;
    u16 *raw_buf = NULL;

//    ts = i2c_get_clientdata(i2c_client_point);

    open_test_flag = 1;

    gtp_irq_disable();
    //SET_INFO_LINE_INFO("---GT9xx Touchpad Test---");
    GTP_DEBUG("Parsing configuration...");
    ret = gtp_parse_config();
    if (ret == FAIL)
    {
        SET_INFO_LINE_ERR("failed to parse config...");
        goto open_test_exit;
    }
    raw_buf = (u16*)kmalloc(sizeof(u16)* gt9xx_pixel_cnt, GFP_KERNEL);
    if (NULL == raw_buf)
    {
        SET_INFO_LINE_ERR("failed to allocate mem for raw_buf!");
        goto open_test_exit;
    }

    GTP_DEBUG("Step 1: Send Rawdata Cmd");

//    ts->gtp_rawdiff_mode = 1;
    gtp_raw_test_init(0);
    ret = gt9_read_raw_cmd(client);
    if (ret == FAIL)
    {
        SET_INFO_LINE_ERR("Send Read Rawdata Cmd failed!");
        goto open_test_exit;
    }
    GTP_DEBUG("Step 2: Sample Rawdata");
    for (i = 0; i < sample_set_num; ++i)
    {
        rslt_buf_idx = i;
        ret = gtp_read_rawdata(client, raw_buf);
        if (ret == FAIL)
        {
            SET_INFO_LINE_ERR("Read Rawdata failed!");
            goto open_test_exit;
        }
        else
        {
			if (i == 0)
			{
				int j = 0;
				for (j = 0; j < gt9xx_pixel_cnt; j++)
				{
					if ( (j%16) == 0)
						printk("\n");
					printk("%d ", raw_buf[j]);
				}
			}
        }
        ret = gtp_raw_test(raw_buf, 0);
        if (ret == FAIL)
        {
            gtp_i2c_end_cmd(client);
            continue;
        }
    }
    GTP_DEBUG("Step 3: Analyse Result");
    gtp_get_test_result();

    ret = SUCCESS;
open_test_exit:

    kfree(raw_buf);
    if (test_rslt_buf)
    {
        kfree(test_rslt_buf);
    }
    if (touchpad_sum)
    {
        kfree(touchpad_sum);
    }
    gtp_irq_enable();

    open_test_flag = 0;
//    ts->gtp_rawdiff_mode = 0;
    gt9_read_coor_cmd(client);	// back to read coordinates data
    SET_INFO_LINE_INFO("---gtp TP test end---");
    return ret;
}

static ssize_t gtp_sysfs_shorttest_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    u8 index;
    u32 len;

#ifdef CONFIG_GTP_ESD_PROTECT
    gtp_esd_switch(i2c_client_point, SWITCH_OFF);
#endif

    gt9xx_short_test(i2c_client_point);

#ifdef CONFIG_GTP_ESD_PROTECT
    gtp_esd_switch(i2c_client_point, SWITCH_ON);
#endif

    for (index = 0, len = 0; index < RsltIndex; ++index)
    {
        sprintf(&buf[len], "%s", result_lines[index]);
        len += strlen(result_lines[index]);
        kfree(result_lines[index]);
    }
    RsltIndex = 0;
    return len;
}

static ssize_t gtp_sysfs_shorttest_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

static ssize_t gtp_sysfs_opentest_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    s32 index;
    u32 len = 0;

    if (open_test_flag == 0) {
#ifdef CONFIG_GTP_ESD_PROTECT
    gtp_esd_switch(i2c_client_point, SWITCH_OFF);
#endif

    gt9xx_open_test(i2c_client_point);

#ifdef CONFIG_GTP_ESD_PROTECT
    gtp_esd_switch(i2c_client_point, SWITCH_ON);
#endif

    for (index = 0, len = 0; index < RsltIndex; ++index)
    {
        sprintf(&buf[len], "%s", result_lines[index]);
        len += strlen(result_lines[index]);
        kfree(result_lines[index]);
    }
    RsltIndex = 0;
    }
    return len;
}

static ssize_t gtp_sysfs_opentest_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return -EPERM;
}

static DEVICE_ATTR(shorttest, S_IRUGO|S_IWUSR, gtp_sysfs_shorttest_show, gtp_sysfs_shorttest_store);
static DEVICE_ATTR(opentest, S_IRUGO|S_IWUSR, gtp_sysfs_opentest_show, gtp_sysfs_opentest_store);
/*******************************************************
Description:
	Goodix debug sysfs init function.

Parameter:
	none.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
int gtp_debug_sysfs_init(void)
{
    s32 ret ;

    if (tp_gesture_kobj == NULL)
    {
        GTP_ERROR("%s: subsystem_register failed\n", __func__);
        return -ENOMEM;
    }

    ret = sysfs_create_file(tp_gesture_kobj, &dev_attr_shorttest.attr);
    if (ret)
    {
        GTP_ERROR("%s: sysfs_create_version_file failed\n", __func__);
        return ret;
    }
    ret = sysfs_create_file(tp_gesture_kobj, &dev_attr_opentest.attr);
    if (ret)
    {
        GTP_ERROR("%s: sysfs_create_version_file failed\n", __func__);
        return ret;
    }

#if 0
    switch(gtp_get_sensorid())
    {
    case 0:
	min_limit_value = 500;
	break;
    case 1:
	min_limit_value = 500;
	break;
    }
#endif

    GTP_INFO("Goodix debug sysfs create success!\n");
    return 0 ;
}

void gtp_debug_sysfs_deinit(void)
{
    sysfs_remove_file(tp_gesture_kobj, &dev_attr_shorttest.attr);
    sysfs_remove_file(tp_gesture_kobj, &dev_attr_opentest.attr);
    kobject_del(tp_gesture_kobj);
}

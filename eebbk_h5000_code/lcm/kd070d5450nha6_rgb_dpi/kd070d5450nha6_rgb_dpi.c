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
#ifdef BUILD_LK
//#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

//#include <mach/mt_pm_ldo.h>
//#include <mach/mt_gpio.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif
extern atomic_t ESDCheck_byCPU;
#endif

#include "lcm_drv.h"

#define LOG_TAG "LCM"

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */
#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  printf("[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_FACTORY_ID_BOE 0
#define LCM_FACTORY_ID_INNOLUX 1
#define LCM_DSI_CMD_MODE	0
#define I2C_I2C_LCD_BIAS_CHANNEL 2
static LCM_UTIL_FUNCS lcm_util = { 0 };

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))

#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW   0xFFFE
#define REGFLAG_RESET_HIGH  0xFFFF

static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)		\
    lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
        lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
      lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
        lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#endif

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting[] = 
{
#if 1
	{0xF0, 2, {0x5A,0x5A}},
	{0xF1, 2, {0x5A,0x5A}},
	{0xFC, 2, {0xA5,0xA5}},
	{0xD0, 2, {0x00,0x10}},
	
	{0xB1, 1, {0x10}},
	{0xB2, 4, {0x14,0x22,0x2F,0x04}},
	{0xF2, 5, {0x02,0x08,0x08,0x40,0x10}},
	{0xB0, 1, {0x04}},
	{0xFD, 1, {0x09}},
	
	{0xF3, 10, {0x01,0x97,0x20,0x22,0x84,0x07,0x27,0x10,0x26,0x00}},
	
	{0xF4, 45, {0x02,0x02,0x1C,0x26,0x1D,0x02,0x03,0x03,0x03,0x1C,0x1C,0x03,0x00,0x0C,0x0C,0x03,0x02,0x03,0x06,0x06,0x13,0x14,0x01,0x15,0x01,0x04,0x02,0x61,0x78,0x79,0x72,0xE3,0xF3,0xF0,0x00,0x00,0x01,0x01,0x28,0x04,0x03,0x28,0x01,0xD1,0x32}},
	{0xF5, 26, {0x61,0x2F,0x2F,0x5F,0x5B,0xBB,0x53,0x0F,0x33,0x23,0x02,0x59,0x54,0x52,0x45,0x46,0x60,0x02,0x60,0x80,0x27,0x26,0x52,0x25,0x7A,0x1F}},
	
	{0xEE, 8, {0x28,0x00,0x28,0x00,0x28,0x00,0x28,0x00}},
	{0xEF, 8, {0x34,0x12,0x98,0xBA,0x20,0x00,0x24,0x80}},
	
	{0xF7, 32, {0x0E,0x0E,0x0A,0x0A,0x0F,0x0F,0x0B,0x0B,0x05,0x07,0x01,0x01,0x01,0x01,0x01,0x01,0x0C,0x0C,0x08,0x08,0x0D,0x0D,0x09,0x09,0x04,0x06,0x01,0x01,0x01,0x01,0x01,0x01}},
	
	{0xBC, 3, {0x01,0x4E,0x0B}},
	{0xE1, 5, {0x03,0x10,0x1C,0xA0,0x10}},
	
	{0xF6, 6, {0xB0,0x25,0xA6,0x00,0x00,0x00}},
	{0xFE, 6, {0x00,0x0D,0x03,0x21,0x80,0x78}},
	
	{0xFA, 17, {0x00,0x35,0x05,0x0B,0x01,0x06,0x0B,0x0A,0x0D,0x16,0x19,0x19,0x1A,0x19,0x19,0x1A,0x22}},
	{0xFB, 17, {0x00,0x35,0x05,0x0B,0x01,0x06,0x0B,0x0A,0x0D,0x16,0x19,0x19,0x1A,0x19,0x19,0x1A,0x22}},
	
	{0xC3, 3, {0x40,0x00,0x28}},
	
	{0x35,1,{0x00}},
//	{0x36,1,{0x04}},

#if 0
#ifdef BUILD_LK	
	{0x51,1,{0x80}},
#endif	

	{0x53,1,{0x24}},
	
	{0x55,1,{0x01}},
#endif	

#endif

	{0x11,1,{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,1,{0x00}},
	{REGFLAG_DELAY, 40, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

#ifdef BUILD_LK
static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#endif

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

			case REGFLAG_DELAY:
				if (table[i].count <= 10)
					MDELAY(table[i].count);
				else
					MDELAY(table[i].count);
				break;

			case REGFLAG_UDELAY:
				UDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE:
				break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

#ifndef BUILD_LK
static struct regulator *lcm_vgp_1v8;
static struct regulator *lcm_vmch_3v3;
static unsigned int GPIO_LCD_ID;
static unsigned int GPIO_LCD_RST;

/* get LDO supply */
static int lcm_get_ldo_supply(struct device *dev)
{
	int ret;
	struct regulator *lcm_vgp_ldo;
	struct regulator *lcm_vmch_ldo;

	pr_debug("lcm_get_ldo_supply is going\n");

	lcm_vgp_ldo = devm_regulator_get(dev, "reg-lcm-1v8");
	if (IS_ERR(lcm_vgp_ldo)) {
		ret = PTR_ERR(lcm_vgp_ldo);
		dev_err(dev, "failed to get reg-lcm-1v8 LDO, %d\n", ret);
		return ret;
	}
	
	lcm_vmch_ldo = devm_regulator_get(dev, "reg-lcm-3v3");
	if (IS_ERR(lcm_vmch_ldo)) {
		ret = PTR_ERR(lcm_vmch_ldo);
		dev_err(dev, "failed to get reg-lcm-3v3 LDO, %d\n", ret);
		return ret;
	}

	pr_debug("lcm get supply ok.\n");

	/* get current voltage settings */
	ret = regulator_get_voltage(lcm_vgp_ldo);
	pr_debug("lcm LDO voltage = %d in kernel stage\n", ret);
	
	ret = regulator_get_voltage(lcm_vmch_ldo);
	pr_debug("lcm LDO voltage = %d in kernel stage\n", ret);

	lcm_vgp_1v8 = lcm_vgp_ldo;
	lcm_vmch_3v3 = lcm_vmch_ldo;

	return ret;
}

static int lcm_ldo_supply_enable_boe(void)
{
	int ret;
	unsigned int vol_vgp, vol_vmch;

	pr_debug("lcm_ldo_supply_enable_boe\n");

	if (lcm_vgp_1v8 == NULL || lcm_vmch_3v3 == NULL)
		return 0;

	pr_debug("LCM: set regulator voltage lcm_vgp_1v8 voltage to 1.8V\n");
	ret = regulator_set_voltage(lcm_vgp_1v8, 1800000, 1800000);
	if (ret != 0) {
		pr_err("LCM: lcm failed to set lcm_vgp_1v8 voltage: %d\n", ret);
		return ret;
	}
	
	pr_debug("LCM: set regulator voltage lcm_vmch_3v3 voltage to 3.3V\n");
	ret = regulator_set_voltage(lcm_vmch_3v3, 3300000, 3300000);
	if (ret != 0) {
		pr_err("LCM: lcm failed to set lcm_vmch_3v3 voltage: %d\n", ret);
		return ret;
	}

	/* get voltage settings again */
	vol_vgp = regulator_get_voltage(lcm_vgp_1v8);
	vol_vmch = regulator_get_voltage(lcm_vmch_3v3);
	/*
	if (vol_vmch == 3300000)
		printk("LCM: check regulator voltage=3300000 pass!\n");
	else
		printk("LCM: check regulator voltage=3300000 fail! (voltage: %d)\n", volt);
	*/
	
	ret = regulator_enable(lcm_vmch_3v3);
	if (ret != 0) {
		pr_err("LCM: Failed to enable lcm_vmch_3v3: %d\n", ret);
		return ret;
	}

	ret = regulator_enable(lcm_vgp_1v8);
	if (ret != 0) {
		pr_err("LCM: Failed to enable lcm_vgp_1v8: %d\n", ret);
		return ret;
	}
	return ret;
}

static int lcm_vgp_supply_disable_boe(void)
{
	int ret = 0;
	unsigned int is_vgp_enable, is_vmch_enable;

	if (lcm_vgp_1v8 == NULL || lcm_vmch_3v3 == NULL)
		return 0;

	/* disable regulator */
	is_vgp_enable = regulator_is_enabled(lcm_vgp_1v8);
	is_vmch_enable = regulator_is_enabled(lcm_vmch_3v3);

	pr_debug("LCM: lcm query regulator enable status[0x%x, 0x%x]\n", is_vgp_enable, is_vmch_enable);

	if (is_vgp_enable) {
		ret = regulator_disable(lcm_vgp_1v8);
		if (ret != 0) {
			pr_err("LCM: lcm failed to disable lcm_vgp_1v8: %d\n", ret);
			return ret;
		}
		/* verify */
		is_vgp_enable = regulator_is_enabled(lcm_vgp_1v8);
		if (!is_vgp_enable)
			pr_err("LCM: lcm regulator disable lcm_vgp_1v8 pass\n");
	}
	
	if (is_vmch_enable) {
		ret = regulator_disable(lcm_vmch_3v3);
		if (ret != 0) {
			pr_err("LCM: lcm failed to disable lcm_vmch_3v3: %d\n", ret);
			return ret;
		}
		/* verify */
		is_vmch_enable = regulator_is_enabled(lcm_vmch_3v3);
		if (!is_vmch_enable)
			pr_err("LCM: lcm regulator disable lcm_vmch_3v3 pass\n");
	}

	return ret;
}

static void lcm_request_gpio_control(struct device *dev)
{
	GPIO_LCD_ID = of_get_named_gpio(dev->of_node, "gpio_lcd_id", 0);
	gpio_request(GPIO_LCD_ID, "GPIO_LCD_ID");
	
	GPIO_LCD_RST = of_get_named_gpio(dev->of_node, "gpio_lcd_rst", 0);
	gpio_request(GPIO_LCD_RST, "GPIO_LCD_RST");
	
	pr_debug("lcd_id = %d, rst = %d\n", GPIO_LCD_ID, GPIO_LCD_RST);
}

static int lcm_driver_probe(struct device *dev, void const *data)
{
	/* printk("LCM: lcm_driver_probe\n"); */

	lcm_request_gpio_control(dev);
	lcm_get_ldo_supply(dev);
	lcm_ldo_supply_enable_boe();

	return 0;
}

static const struct of_device_id lcm_platform_of_match[] = {
	{
		.compatible = "kd,kd070d5450nha6",
		.data = 0,
	}, {
		/* sentinel */
	}
};

MODULE_DEVICE_TABLE(of, platform_of_match);

static int lcm_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;

	id = of_match_node(lcm_platform_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	return lcm_driver_probe(&pdev->dev, id->data);
}

static struct platform_driver lcm_driver = {
	.probe = lcm_platform_probe,
	.driver = {
		   .name = "kd070d5450nha6_rgb_dpi",
		   .owner = THIS_MODULE,
		   .of_match_table = lcm_platform_of_match,
		   },
};

static int __init lcm_init(void)
{
	pr_notice("LCM: Register lcm driver\n");
	if (platform_driver_register(&lcm_driver)) {
		pr_err("LCM: failed to register disp driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_exit(void)
{
	platform_driver_unregister(&lcm_driver);
	pr_notice("LCM: Unregister lcm driver done\n");
}
late_initcall(lcm_init);
module_exit(lcm_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("LCM display subsystem driver");
MODULE_LICENSE("GPL");
#endif
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (1280)

#define GPIO_OUT_ONE  1
#define GPIO_OUT_ZERO 0

#ifdef BUILD_LK
/* GPIO54       DISP_PWM0 for backlight panel */
#ifdef GPIO_LCD_ID
#define GPIO_LCD_ID		GPIO_LCD_ID
#else
#define GPIO_LCD_ID		GPIO2
#endif

/* GPIO66       LCM_RST panel */
#ifdef GPIO_LCM_RST
#define GPIO_LCD_RST		GPIO_LCM_RST
#else
#define GPIO_LCD_RST		GPIO66
#endif

#endif


static int get_lcm_manufactor(void)
{
	int val = 1; //0 boe, 1 lnno
	
#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO_LCD_ID, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_ID, GPIO_DIR_IN);
	val = mt_get_gpio_in(GPIO_LCD_ID);
#else
	val = __gpio_get_value(GPIO_LCD_ID);
#endif

	LCM_LOGI("lcd_id val = %d\n", val);
	return val;
}

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, output);
#else
	gpio_set_value(GPIO, output);
#endif
}

static void lcm_init_power(void)
{
#ifdef BUILD_LK
	printf("[LK/LCM] lcm init power() enter\n");

	mt6392_upmu_set_rg_vgp1_vosel(3);
	mt6392_upmu_set_rg_vgp1_en(0x1);
	MDELAY(10);
	
	mt6392_upmu_set_rg_vmch_vosel(1);
	mt6392_upmu_set_rg_vmch_en(0x1);
	MDELAY(20);
	
	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ONE);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ZERO);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ONE);
	MDELAY(50);
#else
	pr_err("[Kernel/LCM] lcm init power() enter\n");
	lcm_ldo_supply_enable_boe();
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ONE);
	MDELAY(20);

	SET_RESET_PIN(1);
	MDELAY(50);
#endif
}

static void lcm_suspend_power(void)
{
#ifndef BUILD_LK
	pr_err("[Kernel/LCM] lcm_suspend_power() enter\n");
	
	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ZERO);
	MDELAY(120);
	
	lcm_vgp_supply_disable_boe();
	MDELAY(10);
#endif
}

static void lcm_resume_power(void)
{
#ifndef BUILD_LK
	pr_err("[Kernel/LCM] lcm_resume_power() enter\n");

	lcm_init_power();
#endif
}

/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
//	LCM_LOGI("\n\n%s, %d ========8168 platform==========\n\n");
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));
	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

//	params->physical_width = 68;
//	params->physical_height = 122;

#if LCM_DSI_CMD_MODE
	params->dsi.mode   = CMD_MODE;
#else
	//params->dsi.mode   = BURST_VDO_MODE;
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

//	params->dsi.switch_mode_enable = 0;

	// 1 Three lane or Four lane
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;

	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	params->dsi.ssc_disable = 1;
//	params->dsi.ssc_range = 8;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 4;
	params->dsi.vertical_frontporch					= 8;
	params->dsi.vertical_active_line					= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 64;
	params->dsi.horizontal_backporch				= 80;
	params->dsi.horizontal_frontporch				= 64;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	// Bit rate calculation
	// 1 Every lane speed
	params->dsi.PLL_CLOCK = 235;
//	params->dsi.cont_clock = 1;
	params->dsi.noncont_clock = 1;

#if 1
//	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
/*
	params->dsi.lcm_esd_check_table[1].cmd          = 0x7F;
	params->dsi.lcm_esd_check_table[1].count        = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x01;
*/
#endif
}

static void lcm_init_lcm(void)
{
#ifdef BUILD_LK
	printf("[LK/LCM] lcm_init() enter\n");
#else
	pr_err("[Kernel/LCM] lcm_init() enter\n");
#endif

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
#ifdef BUILD_LK
	printf("[LK/LCM] lcm_suspend() enter\n");
#else
	pr_err("[Kernel/LCM] lcm_suspend() enter\n");
#endif

	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
#ifdef BUILD_LK
	printf("[LK/LCM] lcm_resume() enter\n");
#else
	pr_err("[Kernel/LCM] lcm_resume() enter\n");
#endif

	lcm_init_lcm();
}

#ifdef BUILD_LK
static void lcm_setbacklight(unsigned int level)
#else
static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
#endif
{
#if 0
#ifdef BUILD_LK
	bl_level[0].para_list[0] = level;
	push_table(bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
#else
	unsigned int cmd = 0x51;
	unsigned char count = 1;
	unsigned char value = level;

	dsi_set_cmdq_V22(handle, cmd, count, &value, 1);	////use cmdq to send cmd
#endif
#endif
}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
	unsigned int id = 0;
	unsigned char buf[3];
	unsigned int array[16];
	
	array[0] = 0x00033700;  /* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

#ifndef BUILD_LK
	atomic_set(&ESDCheck_byCPU, 1);
#endif
	read_reg_v2(0x04, buf, 3);
#ifndef BUILD_LK
	atomic_set(&ESDCheck_byCPU, 0);
#endif
	
	id = buf[0];     /* we only need ID */

	LCM_LOGI("%s, ata: id = 0x%08x, 0x%08x\n", __func__, id, buf[1]);
	
	if (id == 0x93)
		return 1;
	else
		return 0;
}

static unsigned int lcm_compare_id(void)
{
#if 1
	if (LCM_FACTORY_ID_BOE == get_lcm_manufactor() )	
		return 1;
	else
		return 0;
#else	
	unsigned int id = 0;
	unsigned char id1 = 0;
	unsigned char id2 = 0;
	unsigned char buffer[3];
	unsigned int array[16];

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

	array[0] = 0x00033700;  /* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x04, buffer, 3);
	id1 = buffer[0];
	id2 = buffer[1];
	id = (id1 << 8) | id2; 

	LCM_LOGI("[LK/LCM] id enter = 0x%x, 0x%x, 0x%x\n", id1, id2, id);

	return 1;
#endif	
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
#if LCM_DSI_CMD_MODE
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
#endif
}

static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
	/* customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register */
	if (mode == 0) {    /* V2C */
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;    /* mode control addr */
		lcm_switch_mode_cmd.val[0] = 0x13;  /* enabel GRAM firstly, ensure writing one frame to GRAM */
		lcm_switch_mode_cmd.val[1] = 0x10;  /* disable video mode secondly */
	} else {        /* C2V */
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0] = 0x03;  /* disable GRAM and enable video mode */
	}
	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}

LCM_DRIVER kd070d5450nha6_rgb_dpi_lcm_drv = {
	.name = "kd070d5450nha6_rgb_dpi",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init_lcm,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
#ifdef BUILD_LK
	.set_backlight      = lcm_setbacklight,
#else
  .set_backlight_cmdq = lcm_setbacklight_cmdq,
#endif
	.update = lcm_update,
	.ata_check = lcm_ata_check,
	.switch_mode = lcm_switch_mode,
};

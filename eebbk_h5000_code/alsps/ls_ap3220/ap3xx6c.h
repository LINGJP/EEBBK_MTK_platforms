/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Filename: ap3xx6.h
 *
 * Summary:
 *	AP3xx6 sensor dirver header file.
 *
 * Modification History:
 * Date	By		Summary
 * -------- -------- -------------------------------------------------------
 * 05/11/12 YC		Original Creation (Test version:1.0)
 */

/*
 * Definitions for AP3xx6 als/ps sensor chip.
 */
#ifndef __AP3xx6_H__
#define __AP3xx6_H__

#include <linux/ioctl.h>
#include <linux/i2c.h>
#define SENSOR_HOR_OK                   0
#define SENSOR_VER_OK			1
#define SENSOR_HOR_FAILED		3
#define SENSOR_BOTH_FAILED		4
#define SENSOR_BOTH_OK 			5

#define AP3xx6_ENABLE						0X00
#define AP3xx6_INT_STATUS					0x01
#define AP3xx6_INT_CTL                      0x02
#define AP3xx6_ADATA_L						0X0C
#define AP3xx6_ADATA_H						0X0D
#define AP3xx6_PDATA_L						0X0E
#define AP3xx6_PDATA_H						0X0F
#define AP3xx6_INT_LOW_THD_LOW				0X2A
#define AP3xx6_INT_LOW_THD_HIGH				0X2B
#define AP3xx6_INT_HIGH_THD_LOW				0X2C
#define AP3xx6_INT_HIGH_THD_HIGH			0X2D



#define AP3xx6_SUCCESS						0
#define AP3xx6_ERR_I2C						-1
#define AP3xx6_ERR_STATUS					-3
#define AP3xx6_ERR_SETUP_FAILURE			-4
#define AP3xx6_ERR_GETGSENSORDATA			-5
#define AP3xx6_ERR_IDENTIFICATION			-6

#define STANDARD_LIGHT_VALUE_HOR	2650	//2100	//正常光raw值
#define STANDARD_LIGHT_VALUE_VER	2100	//2300	//正常光

struct current_sensor_s {
	int which_sensor;       //1 is vertical; 0 is horizontal; 2 is only horizontal, 3 is only vertial
	int sensor_als;
	int sensor_pls;
};

int ap3xx6_read_reg(struct i2c_client *client,char reg, u8 mask, u8 shift);
int ap3xx6_read_ps(struct i2c_client *client, u16 *data);
int ap3xx6_set_plthres(struct i2c_client *client, int val);
int ap3xx6_set_phthres(struct i2c_client *client, int val);
int ap3xx6_set_ALSGain(struct i2c_client *client, int val);

int change_sensor_ic(int buf);
int get_ap3xx6_init_flag(void);
/***************cail***************/
int set_hor_ps_cail(unsigned int *p);
int set_ver_ps_cail(unsigned int *p);
int set_ver_light_cail(unsigned int *p);
int set_hor_light_cail(unsigned int *p);

int get_ps_level(struct i2c_client *client,int val);
int ap3220_change_ps_threshold(struct i2c_client *client_res);
int change_als_lux(struct i2c_client *client, unsigned int val);

//void disable_two_irq(void);
void enable_two_irq(void);
#endif

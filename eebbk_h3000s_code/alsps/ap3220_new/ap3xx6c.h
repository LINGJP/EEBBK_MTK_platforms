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

/* PS Configuration Register */
#define AP3220_PS_CONFIG                0x20

#define AP3220_PS_INTG_SHIFT            4
#define AP3220_PS_INTG_MASK                     0xf0
#define AP3220_PS_INTG_TIME_1           0x00
#define AP3220_PS_INTG_TIME_2           0x01
#define AP3220_PS_INTG_TIME_3           0x02
#define AP3220_PS_INTG_TIME_9           0x08
#define AP3220_PS_INTG_TIME_16          0x0F

#define AP3220_PS_CONFIG_BIT6   (1 << 6)
#define AP3220_PS_CONFIG_BIT5   (1 << 5) 
#define AP3220_PS_CONFIG_BIT4   (1 << 4)
                
#define AP3220_PS_GAIN_SHIFT    2
#define AP3220_PS_GAIN_MASK             0xc0
#define AP3220_PS_GAIN_1                0x00
#define AP3220_PS_GAIN_2                0x01
#define AP3220_PS_GAIN_4                0x02 
#define AP3220_PS_GAIN_8                0x03
        
#define AP3220_PS_PERSIST_SHIFT         0
#define AP3220_PS_PERSIST_MASK          0x03
#define AP3220_PS_PERSIST_1                     0x00
#define AP3220_PS_PERSIST_2                     0x01
#define AP3220_PS_PERSIST_4                     0x02
#define AP3220_PS_PERSIST_8                     0x03
        
/* PS LED Control Register */
#define AP3220_LED_CTRL                 0x21

#define AP3220_LED_PULSE_SHIFT          0x4
#define AP3220_LED_PULSE_MASK           0x30
#define AP3220_PS_LED_PULSE_0           0x00
#define AP3220_PS_LED_PULSE_1           0x01
#define AP3220_PS_LED_PULSE_2           0x02
#define AP3220_PS_LED_PULSE_3           0x03
        
#define AP3220_LED_RATIO_SHIFT          0x0
#define AP3220_LED_RATIO_MASK           0x03
#define AP3220_PS_LED_RATIO_16          0x00
#define AP3220_PS_LED_RATIO_33          0x01
#define AP3220_PS_LED_RATIO_66          0x02
#define AP3220_PS_LED_RATIO_100         0x03

/* PS Interrupt Mode Register */
#define AP3220_PS_INT_MODE              0x22
#define AP3220_PS_INT_MASK              0x1
#define AP3220_PS_INT_SHIFT             0x0
#define AP3220_PS_INT_ZONE              0x0
#define AP3220_PS_INT_HYST              0x1

/* PS Mean Time Register  */
#define AP3220_PS_MEAN_TIME             0x23
#define AP3220_PS_MEAN_MASK             0x3
#define AP3220_PS_MEAN_SHIFT            0x0
#define AP3220_PS_MEAN_TIME_12          0x00
#define AP3220_PS_MEAN_TIME_25          0x01
#define AP3220_PS_MEAN_TIME_37          0x02
#define AP3220_PS_MEAN_TIME_50          0x03

/* PS LED Waiting Time Register */
#define AP3220_LED_WAITING                      0x24    //(0-63 50ms*(64-1) = 3200ms)
#define AP3220_LED_WAIT_MASK            0x3f
#define AP3220_LED_WAIT_SHIFT           0x0
#define AP3220_LED_WAIT_0                       0x0
#define AP3220_LED_WAIT_1                       0x1
#define AP3220_LED_WAIT_2                       0x2


/* PS Calibration Register */
#define AP3220_PS_CALI_L                0x28
#define AP3220_PS_CALI_H                0x29

/* PS Low Threshold L Register */
#define AP3220_PS_LTHL                          0x2a
#define AP3220_PS_LTHL_SHIFT            0x0
#define AP3220_PS_LTHL_MASK                     0x03
#define AP3220_PS_LTHL_BIT                      0x02

/* PS Low Threshold H Register */
#define AP3220_PS_LTHH                          0x2b

#endif

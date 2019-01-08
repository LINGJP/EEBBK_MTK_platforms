#ifndef __ALSPS_3220_H__
#define __ALSPS_3220__

/* System Configuration Register */
#define AP3220_MODE_COMMAND		0x00
#define AP3220_MODE_SHIFT		(0)
#define AP3220_MODE_MASK		0x07
#define AP3220_MODE_POWERDOWN	0x0
#define AP3220_MODE_ALS_EN		0x1
#define AP3220_MODE_PS_EN		0x2
#define AP3220_MODE_ALS_PS_EN	0x3
#define AP3220_MODE_SWRESET		0x4

/* INT Status Register */
#define AP3220_INT_COMMAND		0x01
#define AP3220_INT_SHIFT		(0)
#define AP3220_INT_MASK			0x03
#define AP3220_INT_PMASK		0x02
#define AP3220_INT_AMASK		0x01

/* IR Data Register */
#define AP3220_IR_ADC_LSB		0x0a
#define AP3220_IR_IROF_MASK		(1 << 7)
#define AP3220_IR_DATA_LMASK	0x3
#define AP3220_IR_DATA_LBIT		2

#define AP3220_IR_ADC_MSB		0x0b
#define AP3220_IR_DATA_HMASK	0xff

/* PS Data Register */
#define AP3220_PS_ADC_LSB		0x0e
#define AP3220_PS_OBJ_LMASK		(1 << 7)
#define AP3220_PS_IROF_LMASK	(1 << 6)
#define AP3220_PS_DATA_LMASK	0xf
#define AP3220_PS_DATA_LBIT		4

#define AP3220_PS_ADC_MSB		0x0f
#define AP3220_PS_OBJ_HMASK		(1 << 7)
#define AP3220_PS_IROF_HMASK	(1 << 6)
#define AP3220_PS_DATA_HMASK	0x3f

/* PS Configuration Register */
#define AP3220_PS_CONFIG		0x20

#define AP3220_PS_INTG_SHIFT		4
#define AP3220_PS_INTG_MASK			0xf0
#define AP3220_PS_INTG_TIME_1		0x00
#define AP3220_PS_INTG_TIME_2		0x01
#define AP3220_PS_INTG_TIME_3		0x02
#define AP3220_PS_INTG_TIME_9		0x08
#define AP3220_PS_INTG_TIME_16		0x0F

#define AP3220_PS_CONFIG_BIT6	(1 << 6)
#define AP3220_PS_CONFIG_BIT5	(1 << 5)
#define AP3220_PS_CONFIG_BIT4	(1 << 4)

#define AP3220_PS_GAIN_SHIFT	2
#define AP3220_PS_GAIN_MASK		0xc0
#define AP3220_PS_GAIN_1		0x00
#define AP3220_PS_GAIN_2		0x01
#define AP3220_PS_GAIN_4		0x02
#define AP3220_PS_GAIN_8		0x03

#define AP3220_PS_PERSIST_SHIFT		0
#define AP3220_PS_PERSIST_MASK		0x03
#define AP3220_PS_PERSIST_1			0x00
#define AP3220_PS_PERSIST_2			0x01
#define AP3220_PS_PERSIST_4			0x02
#define AP3220_PS_PERSIST_8			0x03

/* PS LED Control Register */
#define AP3220_LED_CTRL			0x21

#define AP3220_LED_PULSE_SHIFT		0x4
#define AP3220_LED_PULSE_MASK		0x30
#define AP3220_PS_LED_PULSE_0		0x00
#define AP3220_PS_LED_PULSE_1		0x01
#define AP3220_PS_LED_PULSE_2		0x02
#define AP3220_PS_LED_PULSE_3		0x03

#define AP3220_LED_RATIO_SHIFT		0x0
#define AP3220_LED_RATIO_MASK		0x03
#define AP3220_PS_LED_RATIO_16		0x00
#define AP3220_PS_LED_RATIO_33		0x01
#define AP3220_PS_LED_RATIO_66		0x02
#define AP3220_PS_LED_RATIO_100		0x03

/* PS Interrupt Mode Register */
#define AP3220_PS_INT_MODE		0x22
#define AP3220_PS_INT_MASK		0x1
#define AP3220_PS_INT_SHIFT		0x0
#define AP3220_PS_INT_ZONE		0x0
#define AP3220_PS_INT_HYST		0x1

/* PS Mean Time Register  */
#define AP3220_PS_MEAN_TIME		0x23
#define AP3220_PS_MEAN_MASK		0x3
#define AP3220_PS_MEAN_SHIFT		0x0
#define AP3220_PS_MEAN_TIME_12		0x00
#define AP3220_PS_MEAN_TIME_25		0x01
#define AP3220_PS_MEAN_TIME_37		0x02
#define AP3220_PS_MEAN_TIME_50		0x03

/* PS LED Waiting Time Register */
#define AP3220_LED_WAITING			0x24	//(0-63 50ms*(64-1) = 3200ms)
#define AP3220_LED_WAIT_MASK		0x3f
#define AP3220_LED_WAIT_SHIFT		0x0
#define AP3220_LED_WAIT_0			0x0
#define AP3220_LED_WAIT_1			0x1
#define AP3220_LED_WAIT_2			0x2


/* PS Calibration Register */
#define AP3220_PS_CALI_L		0x28
#define AP3220_PS_CALI_H		0x29

/* PS Low Threshold L Register */
#define AP3220_PS_LTHL				0x2a
#define AP3220_PS_LTHL_SHIFT		0x0
#define AP3220_PS_LTHL_MASK			0x03
#define AP3220_PS_LTHL_BIT			0x02

/* PS Low Threshold H Register */
#define AP3220_PS_LTHH				0x2b
#define AP3220_PS_LTHH_SHIFT		(0)
#define AP3220_PS_LTHH_MASK			0xff

/* PS High Threshold L Register */
#define AP3220_PS_HTHL				0x2c
#define AP3220_PS_HTHL_SHIFT		(0)
#define AP3220_PS_HTHL_MASK			0x3
#define AP3220_PS_HTHL_BIT			0x02

/* PS High Threshold H Register */
#define AP3220_PS_HTHH				0x2d
#define AP3220_PS_HTHH_SHIFT		(0)
#define AP3220_PS_HTHH_MASK			0xff

#define AP3220_PS_DATA_HIGH_TH		1023
#define AP3220_PS_THRESHOLE_LEVELS  5

#define AP3220_PS_HOR_DATA_LOW_TH		580
#define AP3220_PS_HOR_DELTA_1			25	
#define AP3220_PS_HOR_DELTA_2			5	//ok

#define AP3220_PS_VER_DATA_LOW_TH		425	// 10cm 
#define AP3220_PS_VER_DELTA_1			15	//10cm wei tiao
#define AP3220_PS_VER_DELTA_2			5	//25cm weitiao
#define STANDARD_PS_VER_VALUE                   60     //stand 25cm ps cail

/*-----add by lyq start--------*/
#define SAMPLE_PS_FACE                          80     //ps 25cm face value
#define SAMPLE_PS_25                            120	//校准箱子的采样值
#define STANDARD_PS_HOR_VALUE                   SAMPLE_PS_FACE
/*-----add by lyq end--------*/

/*OHTERS*/
#define PS_CAL_ITEMS				1

#if 1
#define DBG_AP3220(x...)   printk(KERN_INFO "AP3220: "x)
#else
#define DBG_AP3220(x...)
#endif

#define LS_LUX_65535 0X00
#define LS_LUX_16383 0X10
#define LS_LUX_4096 0X20
#define LS_LUX_1023 0X30

struct ps_adc_adjust
{
	char magic[8];
	unsigned int adckey;
	unsigned int checksum;
};

#endif


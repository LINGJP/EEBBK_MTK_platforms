/**
*alsps custom part
*
**/
#ifndef __BBK_ALSPS_H__
#define __BBK_ALSPS_H__
#include <linux/i2c.h>
#include <../inc/cust_alsps.h>

/***********************log start**********************************/
#define DBG_AP3220(x...)   printk(KERN_INFO "[lyq/AP3220]: "x)
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               pr_debug(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#define LYQ_LOG(fmt, args...)    printk("[lyq/ALSPS]""%s: "fmt, __func__, ##args)
/**********************log end ************************************/

#define DELAYED_WORK 0	/*1*/

#define AP3220_PS_DATA_HIGH_TH			1023
#define AP3220_PS_THRESHOLE_LEVELS		5
#define AP3220_PS_HOR_DATA_LOW_TH               680	//低 10cm
#define AP3220_PS_HOR_DELTA_1                   25      
#define AP3220_PS_HOR_DELTA_2                   5       //ok
#define SAMPLE_PS_FACE				150	//ps 25cm face value
#define SAMPLE_PS_25				250
#define STANDARD_PS_HOR_VALUE                   SAMPLE_PS_FACE

#define STANDARD_LIGHT_VALUE_HOR		800

#define NVRAM_IOCTL_SET_LIGHT		_IOW('C', 0x01, unsigned int[3])
#define NVRAM_IOCTL_SET_PS		_IOW('C', 0x02, unsigned int[2])

#define IOCTL_GET_LIGHT_CAIL		_IOR('C', 0x03, unsigned int[3])
#define IOCTL_GET_PS_CAIL		_IOR('C', 0x04, unsigned int[2])

struct ap3xx6_i2c_addr {        /*define a series of i2c slave address*/
        u8 write_addr;
        u8 ps_thd;      /*PS INT threshold*/
};

struct ap3xx6_priv {
	struct alsps_hw *hw;
	struct i2c_client *client;
#if DELAYED_WORK
	struct delayed_work eint_work;
#else
	struct work_struct eint_work;
#endif
	struct mutex lock;
	/*i2c address group*/
	struct ap3xx6_i2c_addr addr;
	struct device_node *irq_node;
	int irq;
	/*misc*/
	u16			als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on;	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t	trace;

	/*data*/
	u16		als;
	u16		ps;
	u8		_align;
	u16		als_level_num;
	u16		als_value_num;
	u32		als_level[C_CUST_ALS_LEVEL-1];
	u32		als_value[C_CUST_ALS_LEVEL];
	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_h;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_l;	/*the cmd value can't be read, stored in ram*/
	unsigned int	ps_cail[2];
	unsigned int	als_cail[3];

	ulong		enable;		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/

	/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend	early_drv;
#endif
};

int ap3xx6_read_ps(struct i2c_client *client, u16 *data);
int ap3xx6_set_plthres(struct i2c_client *client, int val);
int ap3xx6_set_phthres(struct i2c_client *client, int val);
int ap3220_change_ps_threshold(struct i2c_client *client_res);
int get_ps_level(struct i2c_client *client,int val);

int set_ps_cail(unsigned int *p, struct ap3xx6_priv *obj);
int set_light_cail(unsigned int *p, struct ap3xx6_priv *obj);
#endif

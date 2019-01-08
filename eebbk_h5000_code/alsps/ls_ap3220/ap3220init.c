#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include "../inc/cust_alsps.h"
#include "ap3xx6c.h"
#include "alsps.h"
#include "alsps_3220.h"

static u16 ap3220_ps_threshole_hor[AP3220_PS_THRESHOLE_LEVELS] = {
	AP3220_PS_HOR_DATA_LOW_TH + AP3220_PS_HOR_DELTA_1,
	AP3220_PS_HOR_DATA_LOW_TH - AP3220_PS_HOR_DELTA_1,
	STANDARD_PS_HOR_VALUE + AP3220_PS_HOR_DELTA_2,
	STANDARD_PS_HOR_VALUE - AP3220_PS_HOR_DELTA_2,
	0};
static u16 ap3220_ps_threshole_ver[AP3220_PS_THRESHOLE_LEVELS] = {
	AP3220_PS_VER_DATA_LOW_TH + AP3220_PS_VER_DELTA_1,
	AP3220_PS_VER_DATA_LOW_TH - AP3220_PS_VER_DELTA_1,
	STANDARD_PS_VER_VALUE + AP3220_PS_VER_DELTA_2,
	STANDARD_PS_VER_VALUE - AP3220_PS_VER_DELTA_2,
	0};

static int last_ps_index = 0xff;
int ps_threshold_hor(u16 value, struct i2c_client *client)
{
	int index;

	DBG_AP3220("lyq:entry %s\n",__func__);
	if (value >= 0) {
		for (index = 0; index < AP3220_PS_THRESHOLE_LEVELS - 1; index++) {
			if (value > ap3220_ps_threshole_hor[index]) {
				break;
			}
		}
			
		if (index > 0) {
			ap3xx6_set_plthres(client, ap3220_ps_threshole_hor[index]);
			ap3xx6_set_phthres(client, ap3220_ps_threshole_hor[index - 1]);
			DBG_AP3220("lyq: ap3220_ps_thresholde l is %d, h is %d \n",ap3220_ps_threshole_hor[index], ap3220_ps_threshole_hor[index-1]);

		}
		else {
			ap3xx6_set_plthres(client, ap3220_ps_threshole_hor[index]);
			ap3xx6_set_phthres(client, 1023);
			DBG_AP3220("lyq: ap3220_ps_thresholde l is %d, h is 1023 \n",ap3220_ps_threshole_hor[index]);
			
			}
		if ((index & 0x01) == 0) {
			index >>= 1;
			if (last_ps_index != index) {
				DBG_AP3220("lyq:report ps event1: at %s:%d %s()! %d ps adc_value is %d\n",__FILE__,__LINE__,__func__,index,value);
				last_ps_index = index;
			}
		}
		else if (last_ps_index == 0xff) {
			index >>= 1;
			DBG_AP3220("lyq:report ps event2: at %s:%d %s()! %d ps adc_value is %d\n",__FILE__,__LINE__,__func__,index,value);
			last_ps_index = index;
		}
	} 
	else {
		if (last_ps_index != 2) {
			ap3xx6_set_plthres(client, ap3220_ps_threshole_hor[4]);
			ap3xx6_set_phthres(client, ap3220_ps_threshole_hor[3]);
			index=2;
			last_ps_index = 2;
			DBG_AP3220("lyq:report ps event3: at %s:%d %s()! %d ps adc_value is %d\n",__FILE__,__LINE__,__func__,index,value);
			}
		}
	if(index == 3)
		index = last_ps_index;
	return index;

}
int ps_threshold_ver(u16 value,struct i2c_client *client)
{
	int index;

	DBG_AP3220("lyq:entry %s\n",__func__);
	if (value >= 0) {
		for (index = 0; index < AP3220_PS_THRESHOLE_LEVELS - 1; index++) {
			if (value > ap3220_ps_threshole_ver[index]) {
				break;
			}
		}
			
		if (index > 0) {
			ap3xx6_set_plthres(client, ap3220_ps_threshole_ver[index]);
			ap3xx6_set_phthres(client, ap3220_ps_threshole_ver[index - 1]);
			DBG_AP3220("lyq: ap3220_ps_thresholde l is %d, h is %d \n",ap3220_ps_threshole_ver[index], ap3220_ps_threshole_ver[index-1]);

		}
		else {
			ap3xx6_set_plthres(client, ap3220_ps_threshole_ver[index]);
			ap3xx6_set_phthres(client, 1023);
			DBG_AP3220("lyq: ap3220_ps_thresholde l is %d, h is 1023 \n",ap3220_ps_threshole_ver[index]);
			
			}
		if ((index & 0x01) == 0) {
			index >>= 1;
			if (last_ps_index != index) {
				DBG_AP3220("lyq:report ps event1: at %s:%d %s()! %d ps adc_value is %d\n",__FILE__,__LINE__,__func__,index,value);
				last_ps_index = index;
			}
		}
		else if (last_ps_index == 0xff) {
			index >>= 1;
			DBG_AP3220("lyq:report ps event2: at %s:%d %s()! %d ps adc_value is %d\n",__FILE__,__LINE__,__func__,index,value);
			last_ps_index = index;
		}
	} 
	else {
		if (last_ps_index != 2) {
			ap3xx6_set_plthres(client, ap3220_ps_threshole_ver[4]);
			ap3xx6_set_phthres(client, ap3220_ps_threshole_ver[3]);
			index=2;
			last_ps_index = 2;
			DBG_AP3220("lyq:report ps event3: at %s:%d %s()! %d ps adc_value is %d\n",__FILE__,__LINE__,__func__,index,value);
			}
		}
	if(index == 3)
		index = last_ps_index;
	return index;

}

int ap3220_change_ps_threshold(struct i2c_client *client_res)	//返回值，等级，0,1,2；返回值。
{
	struct i2c_client *client = client_res;
	u16 value = 0;

	DBG_AP3220("entry %s, the i2c client addr is %d, the name is %s\n",__func__,client->addr,client->name);
	
	ap3xx6_read_ps(client,&value);

	if(client->addr==0x1d)
		return ps_threshold_hor(value,client);
	else if(client->addr==0x1c)
		return ps_threshold_ver(value,client);
	else
		DBG_AP3220("error:the i2c client addr is %d\n",client->addr);

	return -1;

}
int get_ps_level(struct i2c_client *client,int val)
{
	int level=0;

	last_ps_index = val;
	level=ap3220_change_ps_threshold(client);

	return level;
}
int sensor_read_reg(struct i2c_client *client,int val)
{
	int temp=0;
	temp = ap3xx6_read_reg(client, val, 0xFF, 0x00);
	if(temp<0)
		return -1;
	return temp;
}
/*******H5000 hor ps cail**********/
/***********75 < p[1] < 180 *******/
/****50 < ps_adjust_data < 120*****/
int set_hor_ps_cail(unsigned int *p)
{
	unsigned int flag =0;
	unsigned int ps_adjust_data = 0;
	unsigned int temp = 0;

	flag = *p++;	//flag
	ps_adjust_data = *p;	//25cm

	DBG_AP3220("lyq: %s ioctl set ps 25cm cail is %d, flag is %d\n",__func__, ps_adjust_data,flag);
	
	if(flag == 1)
	{
		temp = (ps_adjust_data*10)/SAMPLE_PS_25;

		ps_adjust_data = (SAMPLE_PS_FACE * temp)/10;

		ap3220_ps_threshole_hor[2] = ps_adjust_data + AP3220_PS_HOR_DELTA_2;
                ap3220_ps_threshole_hor[3] = ps_adjust_data - AP3220_PS_HOR_DELTA_2;
/*	
		if (ps_adjust_data > STANDARD_PS_HOR_VALUE)
		{
			ap3220_ps_threshole_hor[0] += ((ps_adjust_data - STANDARD_PS_HOR_VALUE) << 1);
			ap3220_ps_threshole_hor[1] += ((ps_adjust_data - STANDARD_PS_HOR_VALUE) << 1);
		}*/
		DBG_AP3220("lyq:%s,line=%d, ap3220_ps_threshole_hor[0]=%d,[1]=%d,[2]=%d,[3]=%d,[4]=%d\n", __func__, __LINE__, ap3220_ps_threshole_hor[0], ap3220_ps_threshole_hor[1], ap3220_ps_threshole_hor[2], ap3220_ps_threshole_hor[3], ap3220_ps_threshole_hor[4]);
	}	
	else
		return -1;	
	
	return 1;	
}
int set_ver_ps_cail(unsigned int *p)
{
	unsigned int flag =0;
	unsigned int ps_adjust_data = 0;

	flag = *p++;	//flag
	ps_adjust_data = *p;	//25cm

	DBG_AP3220("lyq: %s ioctl set ps 25cm cail is %d, flag is %d\n",__func__, ps_adjust_data,flag);
	
	if(flag == 1)
	{
		ap3220_ps_threshole_ver[2] = ps_adjust_data + AP3220_PS_VER_DELTA_2;
		ap3220_ps_threshole_ver[3] = ps_adjust_data - AP3220_PS_VER_DELTA_2;//校准了
	
		if (ps_adjust_data > STANDARD_PS_VER_VALUE)
		{
			ap3220_ps_threshole_ver[0] += ((ps_adjust_data - STANDARD_PS_VER_VALUE) << 1);
			ap3220_ps_threshole_ver[1] += ((ps_adjust_data - STANDARD_PS_VER_VALUE) << 1);
		}
		DBG_AP3220("lyq:%s,line=%d, ap3220_ps_threshole_ver[0]=%d,[1]=%d,[2]=%d,[3]=%d,[4]=%d\n", __func__, __LINE__, ap3220_ps_threshole_ver[0], ap3220_ps_threshole_ver[1], ap3220_ps_threshole_ver[2], ap3220_ps_threshole_ver[3], ap3220_ps_threshole_ver[4]);
	}	
	else
		return -1;	
	
	return 1;	
}

/*int get_ps_cail(int *p)
{
	*p++ = ps_cali_valu.hor_res;
	*p++ = ps_cali_valu.hor_h;
	*p++ = ps_cali_valu.hor_l;
	*p++ = ps_cali_valu.hor_flag;

	*p++ = ps_cali_valu.ver_res;
	*p++ = ps_cali_valu.ver_h;
	*p++ = ps_cali_valu.ver_l;
	*p = ps_cali_valu.ver_flag;

	return 1;
}*/
/***************lightsensor**********************/
//in zhi out als_num
enum LS_RANGE{
        LS_RANGE1 = 1,//0--65535 lux	0x00
        LS_RANGE2,//0--16383 lux	0x10
        LS_RANGE3,//0--4096 lux		0x20
        LS_RANGE4,//default 0--1023 lux
};

static int range = LS_RANGE4;

int change_als_lux(struct i2c_client *client, unsigned int val)
{

	switch (range)
	{
		case LS_RANGE1:
			val = val * 64;
			if (val < 60000*16)//half next range
			{
				ap3xx6_set_ALSGain(client, LS_LUX_16383);
				range = LS_RANGE2;
			}
			DBG_AP3220("read range = %d value = %d\n", range, val);
			break;

		case LS_RANGE2:
			val = val * 16;
			if (val > 60000*16)
			{
				ap3xx6_set_ALSGain(client, LS_LUX_65535);
				range = LS_RANGE1;
			}
			if (val < 60000*4)//half next range
			{
				ap3xx6_set_ALSGain(client, LS_LUX_4096);
				range = LS_RANGE3;
			}
			DBG_AP3220("read range = %d value = %d\n", range, val);
			break;

		case LS_RANGE3:
			val = val * 4;
			if (val > 60000*4)
			{
				ap3xx6_set_ALSGain(client, LS_LUX_16383);
				range = LS_RANGE2;
			}
			if (val < 60000)
			{
				ap3xx6_set_ALSGain(client, LS_LUX_1023);
				range = LS_RANGE4;
			}
			DBG_AP3220("read range = %d value = %d\n", range, val);
			break;

		case LS_RANGE4:
			if (val > 65535)	//other case is not use
			{
				ap3xx6_set_ALSGain(client, LS_LUX_4096);
				range = LS_RANGE3;
			}
			DBG_AP3220("read range = %d value = %d\n", range, val);
			break;

		default:
			ap3xx6_set_ALSGain(client, LS_LUX_1023);
			break;
	}
	return val;
}

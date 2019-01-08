#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include "ap3xx6c.h"
#include "bbk_alsps.h"

static u16 ap3220_ps_threshole[AP3220_PS_THRESHOLE_LEVELS] = {
	AP3220_PS_HOR_DATA_LOW_TH + AP3220_PS_HOR_DELTA_1,
	AP3220_PS_HOR_DATA_LOW_TH - AP3220_PS_HOR_DELTA_1,
	STANDARD_PS_HOR_VALUE + AP3220_PS_HOR_DELTA_2,
	STANDARD_PS_HOR_VALUE - AP3220_PS_HOR_DELTA_2,
	0};

static int last_ps_index = 0xff;

int ps_threshold(u16 value, struct i2c_client *client)
{
	int index;

	DBG_AP3220("entry reset ps hight & low threshold\n");
	if (value >= 0) {
		for (index = 0; index < AP3220_PS_THRESHOLE_LEVELS - 1; index++) {
			if (value > ap3220_ps_threshole[index]) {
				break;
			}
		}
			
		if (index > 0) {
			ap3xx6_set_plthres(client, ap3220_ps_threshole[index]);
			ap3xx6_set_phthres(client, ap3220_ps_threshole[index - 1]);
			DBG_AP3220("ap3220_ps_thresholde l is %d, h is %d \n",ap3220_ps_threshole[index], ap3220_ps_threshole[index-1]);

		}
		else {
			ap3xx6_set_plthres(client, ap3220_ps_threshole[index]);
			ap3xx6_set_phthres(client, 1023);
			DBG_AP3220("ap3220_ps_thresholde l is %d, h is 1023 \n",ap3220_ps_threshole[index]);
			
			}
		if ((index & 0x01) == 0) {
			index >>= 1;
			if (last_ps_index != index) {
				DBG_AP3220("report ps event1: at %d() line! %d ps adc_value is %d\n",__LINE__,index,value);
				last_ps_index = index;
			}
		}
		else if (last_ps_index == 0xff) {
			index >>= 1;
			DBG_AP3220("report ps event2: at %d() line! %d ps adc_value is %d\n",__LINE__,index,value);
			last_ps_index = index;
		}
	} 
	else {
		if (last_ps_index != 2) {
			ap3xx6_set_plthres(client, ap3220_ps_threshole[4]);
			ap3xx6_set_phthres(client, ap3220_ps_threshole[3]);
			index=2;
			last_ps_index = 2;
			DBG_AP3220("report ps event3: at %d() line! %d ps adc_value is %d\n",__LINE__,index,value);
			}
		}
	if(index == 3)
		index = last_ps_index;
	return index;

}

int ap3220_change_ps_threshold(struct i2c_client *client_res)
{
	struct i2c_client *client = client_res;
	u16 value = 0;

	ap3xx6_read_ps(client,&value);

	return ps_threshold(value,client);
	
}
int get_ps_level(struct i2c_client *client,int val)
{
	int level=0;

	last_ps_index = val;
	level=ap3220_change_ps_threshold(client);

	return level;
}
/*int sensor_read_reg(struct i2c_client *client,int val)
{
	int temp=0;
	temp = ap3xx6_read_reg(client, val, 0xFF, 0x00);
	if(temp<0)
		return -1;
	return temp;
}*/

/***************************************
*****range of value,add by lyq**********
*****150 < P[1] <500********************
*****80 < ps_adjust_data <250***********
****************************************/
int set_ps_cail(unsigned int *p, struct ap3xx6_priv *obj)
{
	unsigned int flag =0;
	unsigned int ps_adjust_data = 0;

	flag = *p++;	//flag
	ps_adjust_data = *p;	//25cm

	DBG_AP3220("%s ioctl set ps 25cm cail is %d, flag is %d\n",__func__, ps_adjust_data,flag);
	
	if(flag == 1)
	{
		obj->ps_cail[0] = flag;
		ps_adjust_data = (ps_adjust_data * SAMPLE_PS_FACE)/SAMPLE_PS_25;
		obj->ps_cail[1] = ps_adjust_data;
	
		ap3220_ps_threshole[2] = ps_adjust_data + AP3220_PS_HOR_DELTA_2;
		ap3220_ps_threshole[3] = ps_adjust_data - AP3220_PS_HOR_DELTA_2;//Ð£×¼ÁË
	
/*		if (ps_adjust_data > STANDARD_PS_HOR_VALUE)
		{
			ap3220_ps_threshole[0] += ((ps_adjust_data - STANDARD_PS_HOR_VALUE) << 1);
			ap3220_ps_threshole[1] += ((ps_adjust_data - STANDARD_PS_HOR_VALUE) << 1);
		}*/
		DBG_AP3220("%s,line=%d, ap3220_ps_threshole[0]=%d,[1]=%d,[2]=%d,[3]=%d,[4]=%d\n", __func__, __LINE__, ap3220_ps_threshole[0], ap3220_ps_threshole[1], ap3220_ps_threshole[2], ap3220_ps_threshole[3], ap3220_ps_threshole[4]);
	}	
	else
		return -1;	
	
	return 1;	
}
/***************************************
******the range of value,add by lyq*****
************ls_dark <10*****************
**********400 < ls_normal <1500*******
****************************************/
int set_light_cail(unsigned int *p,struct ap3xx6_priv *obj)
{
        unsigned int flag =0;
        int i;
        unsigned int ls_dark,ls_normal;

        if(obj==NULL)
        {
                APS_ERR("lyq:ap3xx6_obj_horizontal is NULL\n");
                return -1;
        }

        flag = *p++;
        ls_dark = *p++;
        ls_normal = *p;

        LYQ_LOG(" %s the hor light cail flag is %d, dark is %d, cail nornal is %d\n",__func__,flag,ls_dark,ls_normal);

        if(flag ==0)
        {
                APS_ERR("lyq: %s faile\n",__func__);
                return -1;
        }
	else
	{
        	obj->als_level[1] = ls_dark+3;
		obj->als_cail[0] = flag;
		obj->als_cail[1] = ls_dark;
		obj->als_cail[2] = ls_normal;
	}

        for(i=2; i<10; i++)
        {
                if(ls_normal > ((STANDARD_LIGHT_VALUE_HOR*13)/10))
                {
                        obj->als_level[i] = (obj->als_level[i] *12)/10;
                }
                else if(ls_normal < ((STANDARD_LIGHT_VALUE_HOR*7)/10))
                {
                        obj->als_level[i] = (obj->als_level[i] *8)/10;
                }
                else
                {
                        break;
                }
        }

        LYQ_LOG(" lyq:%s obj->leve[1] is %d, [2] is %d, [3] is %d, [4] is %d, [5] is %d, [6] is %d, [7] is %d, [8] is %d, [9] is %d\n",__func__,obj->als_level[1],obj->als_level[2],obj->als_level[3],obj->als_level[4],obj->als_level[5],obj->als_level[6],obj->als_level[7],obj->als_level[8],obj->als_level[9]);

        return 1;

}

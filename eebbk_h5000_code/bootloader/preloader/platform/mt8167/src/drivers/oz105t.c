#include <typedefs.h>
#include <platform.h>
#include <i2c.h>
#include <pmic.h>
#include <oz105t.h>

/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define OZ105T_SLAVE_ADDR_WRITE   0x20
#define OZ105T_SLAVE_ADDR_READ    0x21

#define	OZ105T_PRELOADER_LOG	1

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/

int g_log_en=2;


/**********************************************************
  *
  *   [I2C Function For Read/Write oz105t] 
  *
  *********************************************************/
#define OZ105T_I2C_ID	1
static struct mt_i2c_t oz105t_i2c;

kal_uint32 oz105t_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;
    kal_uint8 val = 0;

	ret_code = oz105t_read_byte(addr, &val);
	
	if (ret_code != I2C_OK)
		return ret_code;
	else if (val == value)
		return I2C_OK;

    write_data[0]= addr;
    write_data[1] = value;

    oz105t_i2c.id = OZ105T_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set BLUEWHALE I2C address to >>1 */
    oz105t_i2c.addr = (OZ105T_SLAVE_ADDR_WRITE >> 1);
    oz105t_i2c.mode = ST_MODE;
    oz105t_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&oz105t_i2c, write_data, len);
    if(I2C_OK != ret_code)
	{
#ifdef	OZ105T_PRELOADER_LOG
        print("%s: i2c_write(%x, %x): ret_code: %d. try again\n",
				__func__, addr, value, ret_code);
#endif

		ret_code = i2c_write(&oz105t_i2c, write_data, len);

#ifdef	OZ105T_PRELOADER_LOG
		if (I2C_OK != ret_code)
			print("%s: i2c_write(%x, %x): ret_code: %d.\n", __func__, addr, value, ret_code);
#endif
	}

    return ret_code;
}

kal_uint32 oz105t_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer) 
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint16 len;
    *dataBuffer = addr;

    oz105t_i2c.id = OZ105T_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set BLUEWHALE I2C address to >>1 */
    oz105t_i2c.addr = (OZ105T_SLAVE_ADDR_READ >> 1);
    oz105t_i2c.mode = ST_MODE;
    oz105t_i2c.speed = 100;
    len = 1;

    ret_code = i2c_write_read(&oz105t_i2c, dataBuffer, len, len);
    if(I2C_OK != ret_code)
	{
#ifdef	OZ105T_PRELOADER_LOG
        print("%s: i2c_read(%x): ret_code: %d. try again\n",
				__func__, addr, ret_code);
#endif

		ret_code = i2c_write_read(&oz105t_i2c, dataBuffer, len, len);

#ifdef	OZ105T_PRELOADER_LOG
		if (I2C_OK != ret_code)
			print("%s: i2c_read(%x): ret_code: %d\n", __func__, addr, ret_code);
#endif
	}

    return ret_code;
}

static int oz105t_iic_ok_test(void)
{
	static int oz105t_iic_test_ok = 1;
	static int oz105t_iic_test_done = 0;

	kal_uint32 ret_code = I2C_OK;
	kal_uint8 val = 0;
	int i = 0;

	if (oz105t_iic_test_done) {
		return oz105t_iic_test_ok;
	}

	for (i=0; i<3; i++) {
		ret_code = oz105t_read_byte(0, &val);
		if (ret_code == I2C_OK)
			break;
	}
	
	oz105t_iic_test_done =1;
	oz105t_iic_test_ok = (ret_code == I2C_OK) ? 1:0;
#ifdef	OZ105T_PRELOADER_LOG
	print("pl oz105t iic test = %d\n", oz105t_iic_test_ok);
#endif

	return oz105t_iic_test_ok;
}


/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
void oz105t_hw_init(void)
{
#ifdef	OZ105T_PRELOADER_LOG
	print("oz105t_preload_init\n");
#endif
	if (!oz105t_iic_ok_test())
		return ;

	oz105t_write_byte(0x00, 0x0e);
	oz105t_write_byte(0x01, 0x4d);
	oz105t_write_byte(0x02, 0x21);
	oz105t_write_byte(0x03, 0x93);
	oz105t_write_byte(0x04, 0x44);
	oz105t_write_byte(0x05, 0x0a);
}

//static CHARGER_TYPE g_chr_type_num = CHARGER_UNKNOWN;
//int hw_charging_get_charger_type(void);

void oz105t_charging_enable(kal_uint32 bEnable)
{
/*
    	kal_int8 val = 0;
	
    	if (CHARGER_UNKNOWN == g_chr_type_num && KAL_TRUE == upmu_is_chr_det())
    	{
		hw_charging_get_charger_type();
		dprintf(CRITICAL, "[BATTERY] charger type: %d\n", g_chr_type_num);
	}

	printf("[BATTERY:oz105t] charger type: %d\n", g_chr_type_num);

	if(g_chr_type_num == STANDARD_CHARGER)
	{
		oz105t_write_byte(0x02, 0xd5);
	}
	else
	{
		oz105t_write_byte(0x02, 0x25);
	}
#ifdef	OZ105T_PRELOADER_LOG
	printf("[BATTERY:oz105t] charger type: %d\n", g_chr_type_num);
#endif
*/
	if (!oz105t_iic_ok_test())
		return ;

	bEnable = bEnable ;
	oz105t_write_byte(0x02, 0x63);

}

void oz105t_dump_register(void)
{
#ifdef	OZ105T_PRELOADER_LOG
	int i = 0;
	kal_uint8 val = 0;
	
	if (!oz105t_iic_ok_test())
		return ;

	printf("[oz105t_dump_register][preloader]\n");
	for(i=0; i<=0x07; i++)
	{
		oz105t_read_byte(i, &val);
		printf("[0x%x]=0x%x\n",i, val);
	}
#endif
}


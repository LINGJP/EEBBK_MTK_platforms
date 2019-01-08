#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_i2c.h>   
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/oz105t.h>
#include <printf.h>
//#include <target/cust_charging.h>

#if !defined(CONFIG_POWER_EXT)
#include <platform/upmu_common.h>
#endif


/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define OZ105T_SLAVE_ADDR_WRITE   0x20
#define OZ105T_SLAVE_ADDR_READ    0x21

#define	OZ105T_LK_LOG	1

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
#define OZ105T_I2C_ID	I2C1
static struct mt_i2c_t oz105t_i2c;

kal_uint32 oz105t_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    oz105t_i2c.id = OZ105T_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set BLUEWHALE I2C address to >>1 */
    oz105t_i2c.addr = (OZ105T_SLAVE_ADDR_WRITE >> 1);
    oz105t_i2c.mode = ST_MODE;
    oz105t_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&oz105t_i2c, write_data, len);
#ifdef	OZ105T_LK_LOG
    if(I2C_OK != ret_code)
        dprintf(INFO, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);
#endif
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

#ifdef	OZ105T_LK_LOG
    if(I2C_OK != ret_code)
        dprintf(INFO, "%s: i2c_read: ret_code: %d\n", __func__, ret_code);
#endif
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
		return oz105t_iic_test_ok ;
	}

	for (i=0; i<3; i++) {
		ret_code = oz105t_read_byte(0, &val);
		if (ret_code == I2C_OK)
			break;
	}
	
	oz105t_iic_test_done =1;
	oz105t_iic_test_ok = (ret_code == I2C_OK) ? 1:0;
	dprintf(CRITICAL, "lk oz105t iic test = %d\n", oz105t_iic_test_ok);

	return oz105t_iic_test_ok ;
}


/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
void oz105t_hw_init(void)
{
#ifdef	OZ105T_LK_LOG
	dprintf(CRITICAL, "oz105t_init\r\n");
#endif
	if (!oz105t_iic_ok_test())
		return ;

	mt6392_upmu_set_rg_bc11_bb_ctrl(1);    //BC11_BB_CTRL
	mt6392_upmu_set_rg_bc11_rst(1);        //BC11_RST

	oz105t_write_byte(0x00, 0x0e);
	oz105t_write_byte(0x01, 0x0d);
	//oz105t_write_byte(0x02, 0x62);
	oz105t_write_byte(0x03, 0x93);
	oz105t_write_byte(0x04, 0x44);
	oz105t_write_byte(0x05, 0x0a);
}


void oz105t_charging_enable(kal_uint32 bEnable)
{
	kal_int8 val = 0;
	CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
	
	if (!oz105t_iic_ok_test())
		return ;

	//set input current limit depends on connected charger type.
	CHR_Type_num  = mt_charger_type_detection();


	dprintf(CRITICAL, "[BATTERY:oz105t] charger type: %d\n", CHR_Type_num);

	if(CHR_Type_num == STANDARD_CHARGER)
	{
		oz105t_write_byte(0x02, 0x64);
	}
	else
	{
		oz105t_write_byte(0x02, 0x23);
	}

}

void oz105t_dump_register(void)
{
#ifdef	OZ105T_LK_LOG
	int i = 0;
	kal_uint8 val = 0;
	
	if (!oz105t_iic_ok_test())
		return ;

	dprintf(CRITICAL, "[oz105t_dump_register]\n");
	for(i=0; i<=0x07; i++)
	{
		oz105t_read_byte(i, &val);
		dprintf(CRITICAL, "[0x%x]=0x%x\n",i, val);
	}
#endif
}



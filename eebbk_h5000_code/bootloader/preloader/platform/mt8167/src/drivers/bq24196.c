#include "platform.h"
#include "i2c.h"
#include "pmic.h"
#include "bq24196.h"

/**********************************************************
 *
 *	[I2C Slave Setting]
 *
 *********************************************************/
#define BQ24196_SLAVE_ADDR_WRITE   0xD6
#define BQ24196_SLAVE_ADDR_READ    0xD7

#define BQ24196_BUSNUM 1

/**********************************************************
 *
 *[Global Variable]
 *
 *********************************************************/
#define bq24196_REG_NUM 11
unsigned char bq24196_reg[bq24196_REG_NUM] = {0};

/**********************************************************
 *
 *	[I2C Function For Read/Write bq24196]
 *
 *********************************************************/

static struct mt_i2c_t bq24196_i2c;

kal_uint32 bq24196_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	bq24196_i2c.id = BQ24196_BUSNUM;
	bq24196_i2c.addr = (BQ24196_SLAVE_ADDR_WRITE >> 1);
	bq24196_i2c.mode = ST_MODE;
	bq24196_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq24196_i2c, write_data, len);

	if(I2C_OK != ret_code)
	print("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

kal_uint32 bq24196_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	bq24196_i2c.id = BQ24196_BUSNUM;
	bq24196_i2c.addr = (BQ24196_SLAVE_ADDR_READ >> 1);
	bq24196_i2c.mode = ST_MODE;
	bq24196_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq24196_i2c, dataBuffer, len, len);

	if(I2C_OK != ret_code)
		print("%s: i2c_read: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
unsigned int bq24196_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK,
				  unsigned char SHIFT)
{
	unsigned char bq24196_reg = 0;
	int ret = 0;

	print("--------------------------------------------------\n");

	ret = bq24196_read_byte(RegNum, &bq24196_reg);

	print("[bq24196_read_interface] Reg[%x]=0x%x\n", RegNum, bq24196_reg);

	bq24196_reg &= (MASK << SHIFT);
	*val = (bq24196_reg >> SHIFT);

	print("[bq24196_read_interface] val=0x%x\n", *val);
	return ret;
}

unsigned int bq24196_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK,
				    unsigned char SHIFT)
{
	unsigned char bq24196_reg = 0;
	int ret = 0;

	print("--------------------------------------------------\n");

	ret = bq24196_read_byte(RegNum, &bq24196_reg);
	print("[bq24196_config_interface] Reg[%x]=0x%x\n", RegNum, bq24196_reg);

	bq24196_reg &= ~(MASK << SHIFT);
	bq24196_reg |= (val << SHIFT);

	ret = bq24196_write_byte(RegNum, bq24196_reg);
	print("[bq24196_config_interface] write Reg[%x]=0x%x\n", RegNum, bq24196_reg);

	/* Check */
	/* bq24196_read_byte(RegNum, &bq24196_reg); */
	/* print("[bq24196_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24196_reg); */

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/

/* CON1---------------------------------------------------- */

void bq24196_set_reg_rst(unsigned int val)
{
    kal_uint32 ret=0;

    ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON1),
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_REG_RST_MASK),
                                    (kal_uint8)(CON1_REG_RST_SHIFT)
                                    );
}
static int bq24196_iic_ok_test(void)
{
	static int bq24196_iic_test_ok = 1;
	static int bq24196_iic_test_done = 0;

	kal_uint32 ret_code = I2C_OK;
	kal_uint8 val = 0;
	int i = 0;

	if (bq24196_iic_test_done) {
		return bq24196_iic_test_ok;
	}

	for (i=0; i<3; i++) {
		ret_code = bq24196_read_byte(0x00, &val);
		if (ret_code == I2C_OK)
			break;
	}
	
	bq24196_iic_test_done =1;
	bq24196_iic_test_ok = (ret_code == I2C_OK) ? 1:0;
	print("pl bq24196 iic test = %d\n", bq24196_iic_test_ok);

	return bq24196_iic_test_ok;
}
int pl_bq24196_init(void)
{
	printf("pl_bq24196_init\n");
	if (bq24196_iic_ok_test() == 0)
		return;

	bq24196_write_byte(0x00,0x62);//VBUS 4.8v ilimit current 500mA
	bq24196_write_byte(0x01,0x1a);//charge en
	bq24196_write_byte(0x02,0x00);//charge curr 512mA
	bq24196_write_byte(0x03,0x01);
	bq24196_write_byte(0x04,0xb2);//charge curr 4.2v

	return 0;
}
void bq24196_close_watchdog(void)
{
	printf("plbq24196_close_watchdog\n");
	if (bq24196_iic_ok_test() == 0)
		return;

	bq24196_write_byte(0x05,0x8a);//disable watch dog timer.
}
void bq24196_dump_register(void)
{
	int i = 0;
	kal_uint8 val = 0;
	
	if (!bq24196_iic_ok_test())
		return ;

	printf("[bq24196_dump_register][preloader]\n");
	for(i=0; i<=0x07; i++)
	{
		bq24196_read_byte(i, &val);
		printf("[0x%x]=0x%x\n",i, val);
	}
}

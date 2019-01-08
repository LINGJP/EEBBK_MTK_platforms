
#ifndef	__OZ105T_H__
#define	__OZ105T_H__ __FILE__

kal_uint32 oz105t_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer);
kal_uint32 oz105t_write_byte(kal_uint8 addr, kal_uint8 value);
void oz105t_hw_init(void);
void oz105t_charging_enable(kal_uint32 bEnable);
void oz105t_dump_register(void);

#endif	// __OZ105T_H__

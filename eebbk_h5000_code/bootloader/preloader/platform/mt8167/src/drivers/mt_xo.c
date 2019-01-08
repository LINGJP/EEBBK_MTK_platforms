/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <typedefs.h>
#include <platform.h>
#include <pmic.h>
#include <timer.h>
#include "wdt.h"

/*********XO registers*******/

#define BSI_BASE          0x10210000
#define BSI_CON	          0x0000
#define BSI_WRDAT_CON     0x0004
#define BSI_WRDAT         0x0008
#define BSI_RDCON         0x0c40
#define BSI_RDADDR_CON    0x0c44
#define BSI_RDADDR        0x0c48
#define BSI_RDCS_CON      0x0c4c
#define BSI_RDDAT         0x0c50

#define BSI_WRITE_READY (1 << 31)
#define BSI_READ_READY (1 << 31)
#define BSI_READ_BIT (1 << 8)

#define BIT(n) ((UINT32) 1 << (n))
#define BITS(m,n) (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))

UINT32 bsi_flag = 0;

UINT32 BSI_read(UINT16 rdaddr)
{
       UINT16 readaddr = BSI_READ_BIT | rdaddr;
       UINT16 ret, index;
	   ulong begin = get_timer(0);
	  
	   printf("[xo] BSI_read Enter!\n");
	   for(index = 0; index < 2; index++){
	       if(bsi_flag)
	           printf("[xo] BSI_read for test1!\n");
	       WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDCON, 0x9f8b);
		   if(bsi_flag)
		       printf("[xo] BSI_read for test2!\n");
           WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDADDR_CON, 0x0902);
		   if(bsi_flag)
		       printf("[xo] BSI_read for test3!\n");
           WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDADDR, readaddr);
		   if(bsi_flag)
		       printf("[xo] BSI_read for test4!\n");
           WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDCS_CON, 0x0);
		   if(bsi_flag)
		       printf("[xo] BSI_read for test5!\n");
           WRITE_REGISTER_UINT32(BSI_BASE + BSI_RDCON, 0x89f8b);
           while (1){
		       if(bsi_flag)
			       printf("[xo] BSI_read for test6!\n");
			   if((READ_REGISTER_UINT32(BSI_BASE + BSI_RDCON) & BSI_READ_READY)){
			        if(bsi_flag)
				        printf("[xo] BSI_read for test7!\n");
				    ret = READ_REGISTER_UINT32(BSI_BASE + BSI_RDDAT) & 0x0000ffff;
					if(bsi_flag)
					    printf("BSI Read Done: value = 0x%x\n", ret);
					#ifdef TARGET_BUILD_VARIANT_ENG	
					printf("BSI Read Done: value = 0x%x\n", ret);
					#endif
					return ret;
			   }
			   if (get_timer(begin) > 1){
					printf("[xo] BSI_read time out!!!!!\n");
					break;
			   }   

       	   }
		}
		
		printf("[xo] BSI_read error!!!!!\n");
		return -1;
}

void BSI_write(UINT32 wraddr, UINT32 wrdata)
{
       UINT32 wrdat, index;
	   ulong begin = get_timer(0);
	   
	   printf("[xo] BSI_write Enter!\n");
	   for(index = 0; index < 2; index++){
	       if(bsi_flag)
	           printf("[xo] BSI_write for test1!\n");
	       WRITE_REGISTER_UINT32(BSI_BASE + BSI_WRDAT_CON, 0x1d00);
           wrdat = (wraddr << 20) + wrdata;
		   if(bsi_flag)
		       printf("BSI_write: wrdat = 0x%x\n", wrdat);
		   if(bsi_flag)
		       printf("[xo] BSI_write for test2!\n");
#ifdef TARGET_BUILD_VARIANT_ENG	
           printf("BSI_write: wrdat = 0x%x\n", wrdat);
#endif
           WRITE_REGISTER_UINT32(BSI_BASE + BSI_WRDAT, wrdat);
		   if(bsi_flag)
		       printf("[xo] BSI_write for test3!\n");
           WRITE_REGISTER_UINT32(BSI_BASE + BSI_CON, 0x80401);
           while (1){
		        if(bsi_flag)
				    printf("[xo] BSI_write for test4!\n");
			    if((READ_REGISTER_UINT32(BSI_BASE + BSI_CON) & BSI_WRITE_READY))
					return;
				if(bsi_flag)
				    printf("[xo] BSI_write for test5!\n");
				if (get_timer(begin) > 1){
					printf("[xo] BSI_write time out!!!!!\n");
					break;
				}
       	   }
	   }
	   printf("[xo] BSI_write error!!!!!\n");
       return;
}

void XO_trim_write(UINT32 cap_code)
{
       UINT32 wrdat = 0;
       // 0x09 [14:12] = cap_code[6:4]
       wrdat = BSI_read(0x09) & ~BITS(12, 14);
       wrdat |= (cap_code & BITS(4,6)) << 8;
       BSI_write(0x09, wrdat);
       // 0x09 [10:4] = cap_code[6:0]
       wrdat = BSI_read(0x09) & ~BITS(4, 10);
       wrdat |= (cap_code & BITS(0,6)) << 4;
       BSI_write(0x09, wrdat);
       // 0x01 [11:10] = 2'b11
       BSI_write(0x01, 0xC00);
       mdelay(10);
       // 0x01 [11:10] = 2'b01
       BSI_write(0x01, 0x400);
       // 0x1f [5:3] =  cap_code[6:4]
       wrdat = BSI_read(0x1f) & ~BITS(3, 5);
       wrdat |= (cap_code & BITS(4,6)) >> 1;
       BSI_write(0x1f, wrdat);
       // 0x1f [2:0] =  cap_code[6:4]
       wrdat = BSI_read(0x1f) & ~BITS(0, 2);
       wrdat |= (cap_code & BITS(4,6)) >> 4;
       BSI_write(0x1f, wrdat);
       // 0x1e [15:12] =  cap_code[3:0]
       wrdat = BSI_read(0x1e) & ~BITS(12, 15);
       wrdat |= (cap_code & BITS(0,3)) << 12;
       BSI_write(0x1e, wrdat);
       // 0x4b [5:3] =  cap_code[6:4]
       wrdat = BSI_read(0x4b) & ~BITS(3, 5);
       wrdat |= (cap_code & BITS(4,6)) >> 1;
       BSI_write(0x4b, wrdat);
       // 0x4b [2:0] =  cap_code[6:4]
       wrdat = BSI_read(0x4b) & ~BITS(0, 2);
       wrdat |= (cap_code & BITS(4,6)) >> 4;
       BSI_write(0x4b, wrdat);
       // 0x4a [15:12] =  cap_code[3:0]
       wrdat = BSI_read(0x4a) & ~BITS(12, 15);
       wrdat |= (cap_code & BITS(0,3)) << 12;
       BSI_write(0x4a, wrdat);
       //printf("set cap_code = 0x%x\n", cap_code);
       return;
}

UINT32 XO_trim_read()
{
       UINT32 cap_code = 0;
       // cap_code[4:0] = 0x00 [15:11]
       cap_code = (BSI_read(0x00) & BITS(11, 15)) >> 11;
       // cap_code[6:5] = 0x01 [1:0]
       cap_code |= (BSI_read(0x01) & BITS(0, 1)) << 5;
       //printf("get cap_code = 0x%x\n", cap_code);
       return cap_code;
}

void disable_32K_clock_to_pmic(void)
{
	UINT32 value = 0;

	//Set DIG_CR_XO_24[3:2]=2'b10.
	value = BSI_read(0x34) & ~BITS(2, 3);
	value = value | (1<<3);
	BSI_write(0x34, value);
}

void enable_26M_clock_to_pmic(void)
{
	UINT32 value = 0;

	//Set DIG_CR_XO_02[2]=1
	value = BSI_read(0x04) | 0x4;
	BSI_write(0x04, value);
	//Set DIG_CR_XO_02[1]=1
	value = BSI_read(0x04) | 0x2;
	BSI_write(0x04, value);
	//Set DIG_CR_XO_03[29]=1
	value = BSI_read(0x7) | (1<<13);
	BSI_write(0x07, value);
	//Set DIG_CR_XO_03[28]=1
	value = BSI_read(0x7) | (1<<12);
	BSI_write(0x07, value);
}

void disable_26M_clock_to_pmic(void)
{
	UINT32 value = 0;

	//Set DIG_CR_XO_02[2]=1
	value = BSI_read(0x04) | 0x4;
	BSI_write(0x04, value);
	//Set DIG_CR_XO_02[1]=0
	value = BSI_read(0x04) & 0xFFFD;
	BSI_write(0x04, value);
	//Set DIG_CR_XO_03[29]=1
	value = BSI_read(0x7) | (1<<13);
	BSI_write(0x07, value);
	//Set DIG_CR_XO_03[28]=0
	value = BSI_read(0x7) & 0xEFFF;
	BSI_write(0x07, value);
}

void enable_xo_low_power_mode(void)
{
	UINT32 value = 0;

	printf("[xo] enter low power mode!!\n");

	//RG_DA_EN_XO_BG_MANVALUE = 1
	value = BSI_read(0x03) | (1<<12);
	BSI_write(0x03, value);
	//RG_DA_EN_XO_BG_MAN = 1
	value = BSI_read(0x03) | (1<<13);
	BSI_write(0x03, value);
	//RG_DA_EN_XO_LDOH_MANVALUE = 1
	value = BSI_read(0x03) | (1<<8);
	BSI_write(0x03, value);
	//RG_DA_EN_XO_LDOH_MAN = 1
	value = BSI_read(0x03) | (1<<9);
	BSI_write(0x03, value);
	//RG_DA_EN_XO_LDOL_MANVALUE = 1
	value = BSI_read(0x03) | 0x1;
	BSI_write(0x03, value);
	//RG_DA_EN_XO_LDOL_MAN = 1
	value = BSI_read(0x03) | (1<<1);
	BSI_write(0x03, value);
	//RG_DA_EN_XO_PRENMBUF_VALUE = 1
	value = BSI_read(0x02) | (1<<6);
	BSI_write(0x02, value);
	//RG_DA_EN_XO_PRENMBUF_MAN = 1
	value = BSI_read(0x02) | (1<<7);
	BSI_write(0x02, value);
	//RG_DA_EN_XO_PLLGP_BUF_MANVALUE = 1
	value = BSI_read(0x34) | 0x1;
	BSI_write(0x34, value);
	//RG_DA_EN_XO_PLLGP_BUF_MAN = 1
	value = BSI_read(0x34) | (1<<1);
	BSI_write(0x34, value);

	//RG_DA_EN_XO_VGTIELOW_MANVALUE=0
	value = BSI_read(0x05) & 0xFEFF;
	BSI_write(0x05, value);

	//RG_DA_EN_XO_VGTIELOW_MAN=1
	value = BSI_read(0x05) | (1<<9);
	BSI_write(0x05, value);

	/* bit 10 set 0 */
	value = BSI_read(0x08) & 0xFBFF;
	BSI_write(0x08, value);

	//DIG_CR_XO_04_L[9]:RG_XO_INT32K_NOR2LPM_TRIGGER = 1
	value = BSI_read(0x08) | (1<<9);
	BSI_write(0x08, value);

}

void get_xo_status(void)
{
	UINT32 status = 0;

	status = (BSI_read(0x26) & BITS(4,9))>>4;
	printf("[xo] status: 0x%x\n", status);
}

void mt_xo_init()
{
	UINT32 xo_efuse;
	UINT32 cap_code;
	UINT32 ret, clk;
	UINT32 tmp;
	UINT32 index;

	clk = READ_REGISTER_UINT32(0x1000007C);
	printf("[xo] BSI Read : clk = 0x%x\n", clk);
	clk |= 0x0060;
	WRITE_REGISTER_UINT32(0x1000007C, clk);
	printf("[xo] BSI Read : clk = 0x%x\n", READ_REGISTER_UINT32(0x1000007C));

	printf("[xo] default cap_code: 0x%x\n", XO_trim_read());

	xo_efuse = READ_REGISTER_UINT32(0x10009264);

	if ((xo_efuse>>31) & 0x1) {

		printf("[xo] get xo efuse: %x\n", xo_efuse);
		cap_code = (xo_efuse & BITS(24, 30))>>24;

		if ((xo_efuse>>23) & 0x1) {
			if ((xo_efuse>>22) & 0x1)
				cap_code = cap_code + (xo_efuse & BITS(16, 21))>>16;
			else
				cap_code = cap_code - (xo_efuse & BITS(16, 21))>>16;
		}

		if ((xo_efuse>>15) & 0x1) {
			if ((xo_efuse>>14) & 0x1)
				cap_code = cap_code + (xo_efuse & BITS(8, 13))>>8;
			else
				cap_code = cap_code - (xo_efuse & BITS(8, 13))>>8;
		}
		XO_trim_write(cap_code);

	} else {
		printf("[xo] no efuse, apply sw default cap code!\n");
		#ifdef MTK_MT8167_EVB
		XO_trim_write(0x22);
		#else
		XO_trim_write(0x1c);
		#endif
	}

	printf("[xo] current cap_code: 0x%x\n", XO_trim_read());

	if (rtc_get_xosc_mode()) {
		/* with 32k */

        if(platform_chip_ver() != CHIP_VER_E1){
            BSI_write(0x63, BSI_read(0x63) | (1<<0));
            /* set RG_RTC32K =1 */
            WRITE_REGISTER_UINT32(0x10018000, READ_REGISTER_UINT32(0x10018000) | (1<<10));
            mdelay(1);
            BSI_write(0x63, BSI_read(0x63) & (0xFFFE));
        }

		/* set RG_DIGRSTEN=1 */
		WRITE_REGISTER_UINT32(0x10018000, READ_REGISTER_UINT32(0x10018000) | (1<<9));

		printf("[xo] disable XO to PMIC 32K\n");
		disable_32K_clock_to_pmic();

		printf("[xo] disable XO to PMIC 26M\n");
		disable_26M_clock_to_pmic();

		ret = pmic_config_interface(MT6392_ANALDO_CON1, 1, 0x1, 11);	/* [11]=1(VTCXO_ON_CTRL), */
		ret = pmic_config_interface(MT6392_ANALDO_CON1, 0, 0x1, 0);	/* [0] =0(VTCXO_LP_SEL), */

		printf("[xo] With 32K. Reg[0x%x]=0x%x\n",
			  MT6392_ANALDO_CON1, upmu_get_reg_value(MT6392_ANALDO_CON1));
	} else {
		/* 32k-less */
		printf("[xo] disable XO to PMIC 26M\n");
		disable_26M_clock_to_pmic();

		ret = pmic_config_interface(MT6392_ANALDO_CON1, 0, 0x1, 11);	/* [11]=0(VTCXO_ON_CTRL), */
		ret = pmic_config_interface(MT6392_ANALDO_CON1, 1, 0x1, 0);	/* [0] =1(VTCXO_LP_SEL), */

		printf("[xo] Without 32K. Reg[0x%x]=0x%x\n",
			  MT6392_ANALDO_CON1, upmu_get_reg_value(MT6392_ANALDO_CON1));
	}

	/*for debug*/
	print("[Preloader] BSI read before: [0x22] = 0x%x\n", BSI_read(0x22));
	print("[Preloader] BSI read before: [0x23] = 0x%x\n", BSI_read(0x23));
	print("[Preloader] BSI read before: [0x24] = 0x%x\n", BSI_read(0x24));
	print("[Preloader] BSI read before: [0x25] = 0x%x\n", BSI_read(0x25));
	print("[Preloader] BSI read before: [0x32] = 0x%x\n", BSI_read(0x32));
	print("[Preloader] BSI read before: [0x33] = 0x%x\n", BSI_read(0x33));

	/*Audio use XO path, so add the workaround setting for Audio 26M*/
	tmp = BSI_read(0x25);
	print("[Preloader] BSI read 1: [0x25] = 0x%x\n", tmp);

	if( (tmp & 0x1000) != 0)
	{
		print("[Preloader] xo set first\n");
		BSI_write(0x25, tmp & ~(1 << 12));
		print("[Preloader] after BSI write [0x25]!\n");
		tmp = BSI_read(0x25);
		print("[Preloader] BSI read 2: [0x25] = 0x%x\n", tmp);
		
		tmp = BSI_read(0x29);
		print("[Preloader] BSI read 1: [0x29] = 0x%x\n", tmp);
		BSI_write(0x29, tmp | (1 << 0));
		print("[Preloader] after BSI write [0x29]!\n");
		tmp = BSI_read(0x29);
		print("[Preloader] BSI read 2: [0x29] = 0x%x\n", tmp);
		//delay 100us
		udelay(100);
		bsi_flag = 1;
		
		tmp = BSI_read(0x29);
		print("[Preloader] BSI read 3: [0x29] = 0x%x\n", tmp);
		BSI_write(0x29, BSI_read(0x29) & ~(1 << 0));
			print("[Preloader] after BSI write [0x29][1]!\n");
		tmp = BSI_read(0x29);
		print("[Preloader] BSI read 4: [0x29] = 0x%x\n", tmp);

	}


	/*for debug*/
	print("[Preloader] BSI read after: [0x22] = 0x%x\n", BSI_read(0x22));
	print("[Preloader] BSI read after: [0x23] = 0x%x\n", BSI_read(0x23));
	print("[Preloader] BSI read after: [0x24] = 0x%x\n", BSI_read(0x24));
	print("[Preloader] BSI read after: [0x25] = 0x%x\n", BSI_read(0x25));
	print("[Preloader] BSI read after: [0x32] = 0x%x\n", BSI_read(0x32));
	print("[Preloader] BSI read after: [0x33] = 0x%x\n", BSI_read(0x33));

	get_xo_status();
}


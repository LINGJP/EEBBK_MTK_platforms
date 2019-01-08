/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2011. All rights reserved.
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
*
* The following software/firmware and/or related documentation ("MediaTek Software")
* have been modified by MediaTek Inc. All revisions are subject to any receiver's
* applicable license agreements with MediaTek Inc.
*/


#ifndef CUST_SEC_CTRL_H
#define CUST_SEC_CTRL_H

#include "typedefs.h"
#include "proj_cfg.h"
#include "keypad.h"

/**************************************************************************
 * [ROM INFO]
 **************************************************************************/
#define PROJECT_NAME                        "CUST"
#define PLATFORM_NAME                       "MT8167"


/**************************************************************************
 * [CRYPTO SEED]
 **************************************************************************/
#define CUSTOM_CRYPTO_SEED_SIZE             (16)
#define CUSTOM_CRYPTO_SEED                  "1A52A367CB12C458"

/**************************************************************************
 * [SML AES KEY CONTROL]
 **************************************************************************/
/* It can be enabled only if SUSBDL is turned on */
/* Please make sure SUSBDL is on before enabling this flag */
//#define SML_AES_KEY_ANTICLONE_EN

/**************************************************************************
 * [S-USBDL]
 **************************************************************************/
/* S-USBDL Attribute */
#define ATTR_SUSBDL_DISABLE                 0x00
#define ATTR_SUSBDL_ENABLE                  0x11
#define ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP    0x22
/* S-USBDL Control */
#define SEC_USBDL_CFG                       CUSTOM_SUSBDL_CFG

/**************************************************************************
 * [FLASHTOOL SECURE CONFIG for (for both of SLA and NON-SLA mode], 32bits
 * It's not recommended to use 32 bits (v3) mode now. Please use FLASHTOOL_SEC_CFG_64 instead
 **************************************************************************/
//#define FLASHTOOL_SEC_CFG
//#define BYPASS_CHECK_IMAGE_0_NAME           ""
//#define BYPASS_CHECK_IMAGE_0_OFFSET         0x0
//#define BYPASS_CHECK_IMAGE_0_LENGTH         0x0
//#define BYPASS_CHECK_IMAGE_1_NAME           ""
//#define BYPASS_CHECK_IMAGE_1_OFFSET         0x0
//#define BYPASS_CHECK_IMAGE_1_LENGTH         0x0
//#define BYPASS_CHECK_IMAGE_2_NAME           ""
//#define BYPASS_CHECK_IMAGE_2_OFFSET         0x0
//#define BYPASS_CHECK_IMAGE_2_LENGTH         0x0

/**************************************************************************
 * [FLASHTOOL SECURE CONFIG (for both of SLA and NON-SLA mode], 64 bits for v4 sign format
 **************************************************************************/
//#define FLASHTOOL_SEC_CFG_64
#ifdef FLASHTOOL_SEC_CFG_64
#define BYPASS_CHECK_IMAGE_0_NAME           ""
#define BYPASS_CHECK_IMAGE_0_OFFSET         0x0
#define BYPASS_CHECK_IMAGE_1_NAME           ""
#define BYPASS_CHECK_IMAGE_1_OFFSET         0x0
#define BYPASS_CHECK_IMAGE_2_NAME           ""
#define BYPASS_CHECK_IMAGE_2_OFFSET         0x0
#endif
/**************************************************************************
 * [FLASHTOOL FORBIT DOWNLOAD CONFIG (for NSLA mode only)] , 32 bits
 * It's not recommended to use 32 bits (v3) mode now. Please use FLASHTOOL_FORBID_DL_NSLA_CFG_64 instead
 **************************************************************************/
//#define FLASHTOOL_FORBID_DL_NSLA_CFG
//#define FORBID_DL_IMAGE_0_NAME              ""
//#define FORBID_DL_IMAGE_0_OFFSET            0x0
//#define FORBID_DL_IMAGE_0_LENGTH            0x0
//#define FORBID_DL_IMAGE_1_NAME              ""
//#define FORBID_DL_IMAGE_1_OFFSET            0x0
//#define FORBID_DL_IMAGE_1_LENGTH            0x0

/**************************************************************************
 * [FLASHTOOL FORBIT DOWNLOAD CONFIG (for NSLA mode only)], 64 bits for v4 sign format
 **************************************************************************/
//#define FLASHTOOL_FORBID_DL_NSLA_CFG_64
#ifdef FLASHTOOL_FORBID_DL_NSLA_CFG_64
#define FORBID_DL_IMAGE_0_NAME              ""
#define FORBID_DL_IMAGE_0_OFFSET            0x0
#define FORBID_DL_IMAGE_1_NAME              ""
#define FORBID_DL_IMAGE_1_OFFSET            0x0
#endif

#define SEC_USBDL_WITHOUT_SLA_ENABLE

#ifdef SEC_USBDL_WITHOUT_SLA_ENABLE
//#define USBDL_DETECT_VIA_KEY
/* if com port wait key is enabled, define the key*/
#ifdef USBDL_DETECT_VIA_KEY
#define COM_WAIT_KEY    KPD_DL_KEY3
#endif
//#define USBDL_DETECT_VIA_AT_COMMAND
#endif

/**************************************************************************
 * [S-BOOT]
 **************************************************************************/
/* S-BOOT Attribute */
#define ATTR_SBOOT_DISABLE                  0x00
#define ATTR_SBOOT_ENABLE                   0x11
#define ATTR_SBOOT_ONLY_ENABLE_ON_SCHIP     0x22
/* S-BOOT Control */
#define SEC_BOOT_CFG                        CUSTOM_SBOOT_CFG

/* Customized Secure Boot */
//#define CUSTOMIZED_SECURE_PARTITION_SUPPORT
#ifdef CUSTOMIZED_SECURE_PARTITION_SUPPORT
#define SBOOT_CUST_PART1    ""
#define SBOOT_CUST_PART2    ""
#endif

/* For Custom Partition Verification*/
#define VERIFY_PART_CUST                   (FALSE)
#define VERIFY_PART_CUST_NAME              ""

/* 
    RSA2048 public key for verifying mtee image
    It should be the same as AUTH_PARAM_N in alps\mediatek\custom\mt8167_evb\trustzone\TRUSTZONE_IMG_PROTECT_CFG.ini
*/
#define MTEE_IMG_VFY_PUBK_SZ 256

#define MTEE_IMG_VFY_PUBK \
    0xd0, 0x42, 0xfa, 0xd8, 0x8c, 0xa8, 0x99, 0x77, \
    0x51, 0x27, 0x55, 0xc7, 0xd6, 0xd3, 0xa3, 0xc1, \
    0xe1, 0x06, 0xe6, 0xad, 0x37, 0xba, 0xc3, 0x26, \
    0x93, 0x93, 0x7a, 0xa2, 0x15, 0xf6, 0xde, 0x63, \
    0xc8, 0x74, 0x49, 0xd4, 0x65, 0x2d, 0x16, 0xf8, \
    0xf6, 0x53, 0xb3, 0xfb, 0xbb, 0x08, 0x8d, 0xe4, \
    0x41, 0x30, 0xce, 0xe5, 0x7b, 0x18, 0x49, 0x78, \
    0xc4, 0xae, 0x98, 0x60, 0x52, 0xce, 0xf7, 0xfb, \
    0x5b, 0x44, 0xf7, 0xe6, 0x1a, 0xeb, 0x8c, 0xc8, \
    0xc3, 0x75, 0x3d, 0x34, 0xba, 0xdc, 0x70, 0xd2, \
    0x9c, 0xd8, 0xd2, 0x09, 0xdc, 0xb3, 0xde, 0x52, \
    0xa4, 0xbe, 0x68, 0x9d, 0x13, 0xb2, 0xc9, 0x08, \
    0xfc, 0xe7, 0x03, 0x7e, 0x1e, 0xdd, 0x37, 0x8d, \
    0x6e, 0xab, 0xcc, 0xdf, 0xb7, 0xaf, 0x0d, 0x8a, \
    0xa1, 0x51, 0xc2, 0x89, 0x9a, 0x24, 0xc5, 0xa9, \
    0xa6, 0x02, 0x7c, 0x93, 0xe5, 0xb6, 0x1a, 0xf4, \
    0xa0, 0x04, 0x3a, 0x71, 0x8e, 0x0e, 0x7a, 0xb2, \
    0xec, 0xca, 0x15, 0xba, 0xbd, 0x99, 0xe6, 0xf0, \
    0xba, 0x72, 0x93, 0x05, 0x8f, 0xb9, 0x69, 0x25, \
    0x34, 0xe1, 0xf3, 0x6a, 0x79, 0x30, 0xb6, 0x8c, \
    0xb2, 0xba, 0xba, 0x56, 0x3e, 0xb9, 0xf8, 0x70, \
    0x5c, 0xb8, 0x43, 0xc1, 0xe4, 0xb5, 0xf8, 0x2e, \
    0x22, 0x1d, 0xc4, 0xaa, 0xc6, 0x65, 0xe5, 0x8e, \
    0xc4, 0xa6, 0xa7, 0x35, 0xec, 0x12, 0xeb, 0x9c, \
    0x84, 0xd8, 0xd2, 0x89, 0x48, 0x76, 0xac, 0x84, \
    0x72, 0x8a, 0x7c, 0xd5, 0x34, 0x8c, 0xbe, 0xe6, \
    0x45, 0xf6, 0x07, 0x7d, 0xd5, 0x96, 0x3d, 0x35, \
    0x8c, 0x8f, 0x42, 0x38, 0xb3, 0xf0, 0x0f, 0x92, \
    0x4c, 0xc4, 0xcc, 0x6b, 0xbf, 0x51, 0x8e, 0x70, \
    0xbb, 0x84, 0x9e, 0x03, 0xaf, 0x96, 0xe9, 0xd5, \
    0x3a, 0x6c, 0x0f, 0xe9, 0x41, 0xb0, 0x71, 0xcb, \
    0xd0, 0x52, 0x87, 0xc7, 0x22, 0xd6, 0xa7, 0x0d

/**************************************************************************
 * [DEFINITION CHECK]
 **************************************************************************/
#ifdef SML_AES_KEY_ANTICLONE_EN
#ifndef SECRO_IMG_ANTICLONE_EN
#error "SML_AES_KEY_ANTICLONE_EN is defined. Should also enable SECRO_IMG_ANTICLONE_EN"
#endif
#endif

#if MTK_SECURITY_SW_SUPPORT
#ifndef SEC_USBDL_CFG
#error "MTK_SECURITY_SW_SUPPORT is NOT disabled. Should define SEC_USBDL_CFG "
#endif
#endif

#if MTK_SECURITY_SW_SUPPORT
#ifndef SEC_BOOT_CFG
#error "MTK_SECURITY_SW_SUPPORT is NOT disabled. Should define SEC_BOOT_CFG"
#endif
#endif

#if MTK_SECURITY_SW_SUPPORT
#ifndef SEC_USBDL_WITHOUT_SLA_ENABLE
#error "MTK_SECURITY_SW_SUPPORT is NOT disabled. Should define SEC_USBDL_WITHOUT_SLA_ENABLE"
#endif
#endif

#ifdef USBDL_DETECT_VIA_KEY
#ifndef SEC_USBDL_WITHOUT_SLA_ENABLE
#error "USBDL_DETECT_VIA_KEY can only be enabled when SEC_USBDL_WITHOUT_SLA_ENABLE is enabled"
#endif
#ifndef COM_WAIT_KEY
#error "COM_WAIT_KEY is not defined!!"
#endif
#endif

#ifdef USBDL_DETECT_VIA_AT_COMMAND
#ifndef SEC_USBDL_WITHOUT_SLA_ENABLE
#error "USBDL_DETECT_VIA_AT_COMMAND can only be enabled when SEC_USBDL_WITHOUT_SLA_ENABLE is enabled"
#endif
#endif


#endif   /* CUST_SEC_CTRL_H */

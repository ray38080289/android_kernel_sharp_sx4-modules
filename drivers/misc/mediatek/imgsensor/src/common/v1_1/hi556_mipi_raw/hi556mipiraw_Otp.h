/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
//otp reg
#define HI556_R_OTP_ADDR_H 0x010A
#define HI556_R_OTP_ADDR_L 0x010B
#define HI556_R_OTP_CMD 0x0102
#define HI556_R_OTP_WDATA 0x0106
#define HI556_R_OTP_RDATA 0x0108

//flag
#define HI556_MODULE_INFO_FLAG 0x0401
#define HI556_AWB_FLAG 0x0435
#define HI556_LSC_FLAG 0x0490
//#define HI556_OTP_START HI556_MODULE_INFO_FLAG

#define HI556_DATA_SIZE 5376

//GROP_1
#define HI556_START_OFFSET 0x0000

//data size
#define HI556_CALIBRATION_ID_SIZE 4
#define HI556_SENSOR_ID_SIZE 1
#define HI556_MODULE_INFO_SIZE 15
#define HI556_AWB_SIZE 20
#define HI556_AWB_SIZE_WITH_CHECKSUM 30
#define HI556_CHIP_ID_SIZE 9
#define HI556_LSC_SIZE 1868
#define HI556_LSC_SIZE_WITH_CHECKSUM 1869

//GROP_1
#define HI556_G1_MODULE_INFO_OFFSET 0x0402
#define HI556_G1_MODULE_INFO_CHECKSUM 0x0412
#define HI556_G1_AWB_OFFSET 0x0436
#define HI556_G1_AWB_CHECKSUM 0x0453
#define HI556_G1_LSC_OFFSET 0x0491
#define HI556_G1_LSC_CHECKSUM 0x0BDD
//GROP_2
#define HI556_G2_MODULE_INFO_OFFSET 0x0413
#define HI556_G2_MODULE_INFO_CHECKSUM 0x0423
#define HI556_G2_AWB_OFFSET 0x0454
#define HI556_G2_AWB_CHECKSUM 0x0471
#define HI556_G2_LSC_OFFSET 0x0BDE
#define HI556_G2_LSC_CHECKSUM 0x132A
//GROP_3
#define HI556_G3_MODULE_INFO_OFFSET 0x0424
#define HI556_G3_MODULE_INFO_CHECKSUM 0x0434
#define HI556_G3_AWB_OFFSET 0x0472
#define HI556_G3_AWB_CHECKSUM 0x048F
#define HI556_G3_LSC_OFFSET 0x132B
#define HI556_G3_LSC_CHECKSUM 0x1A77

//0x0401 ~ 0x1a77 
//#define OTP_SIZE 0x1677

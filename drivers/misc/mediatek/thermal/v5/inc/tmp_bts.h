/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __TMP_BTS_H__
#define __TMP_BTS_H__

#define APPLY_PRECISE_NTC_TABLE
#define APPLY_AUXADC_CALI_DATA
#define APPLY_PRECISE_BTS_TEMP

#define AUX_IN0_NTC (0)
#define AUX_IN1_NTC (1)
#define AUX_IN2_NTC (2)
#define AUX_IN3_NTC (3) // ChargerNTC
#define AUX_IN5_NTC (5) // Camera
#define AUX_IN6_NTC (6) // Quiet

#define BTS_RAP_PULL_UP_R		200000 /* 100K, pull up resister */

#define BTS_TAP_OVER_CRITICAL_LOW	4397119 /* base on 100K NTC temp
						 * default value -40 deg
						 */

#define BTS_RAP_PULL_UP_VOLTAGE		1800 /* 1.8V ,pull up voltage */

#define BTS_RAP_NTC_TABLE		7 /* default is NCP15WF104F03RC(100K) */

#define BTS_RAP_ADC_CHANNEL		AUX_IN0_NTC /* default is 0 */

#define BTSMDPA_RAP_PULL_UP_R		200000 /* 100K, pull up resister */

#define BTSMDPA_TAP_OVER_CRITICAL_LOW	4397119 /* base on 100K NTC temp
						 * default value -40 deg
						 */

#define BTSMDPA_RAP_PULL_UP_VOLTAGE	1800 /* 1.8V ,pull up voltage */

#define BTSMDPA_RAP_NTC_TABLE		7 /* default is NCP15WF104F03RC(100K) */

#define BTSMDPA_RAP_ADC_CHANNEL		AUX_IN1_NTC /* default is 1 */

#define CHARGER_RAP_PULL_UP_R		200000	/* 100K,pull up resister */
#define CHARGER_TAP_OVER_CRITICAL_LOW	4397119	/* base on 100K NTC temp
						 *default value -40 deg
						 */

#define CHARGER_RAP_PULL_UP_VOLTAGE	1800	/* 1.8V ,pull up voltage */
#define CHARGER_RAP_NTC_TABLE		7

#define CHARGER_RAP_ADC_CHANNEL		AUX_IN3_NTC


#define BTSNRPA_RAP_PULL_UP_R		200000	/* 100K,pull up resister */
#define BTSNRPA_TAP_OVER_CRITICAL_LOW	4397119	/* base on 100K NTC temp
						 *default value -40 deg
						 */

#define BTSNRPA_RAP_PULL_UP_VOLTAGE	1800	/* 1.8V ,pull up voltage */
#define BTSNRPA_RAP_NTC_TABLE		7

#define BTSNRPA_RAP_ADC_CHANNEL		AUX_IN2_NTC


/* Camera FPC */
#define CAMERA_RAP_PULL_UP_R		200000	/* 100K,pull up resister */
#define CAMERA_TAP_OVER_CRITICAL_LOW	4397119	/* base on 100K NTC temp
						 *default value -40 deg
						 */

#define CAMERA_RAP_PULL_UP_VOLTAGE	1800	/* 1.8V ,pull up voltage */
#define CAMERA_RAP_NTC_TABLE		7

#define CAMERA_RAP_ADC_CHANNEL		AUX_IN5_NTC

/* QUIET_THERM */
#define QUIET_RAP_PULL_UP_R		200000	/* 200K,pull up resister */
#define QUIET_TAP_OVER_CRITICAL_LOW	4397119 /* base on 100K NTC temp
						 * default value -40 deg
						 */
#define QUIET_RAP_PULL_UP_VOLTAGE	1800 /* 1.8V ,pull up voltage */
#define QUIET_RAP_NTC_TABLE		7 /* default is NCP15WF104F03RC(100K) */

#define QUIET_RAP_ADC_CHANNEL		AUX_IN6_NTC /* default is 0 */

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
extern int IMM_IsAdcInitReady(void);

#endif	/* __TMP_BTS_H__ */

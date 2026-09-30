/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     s5kjnssq33_mipiraw_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "s5kjnssq33mipiraw_Sensor.h"

/************************** Modify Following Strings for Debug **************************/
#define PFX "s5kjnssq33_camera_sensor"
#define LOG_INF(format, args...) pr_debug(PFX "[%s] " format, __func__, ##args)
/****************************   Modify end    *******************************************/

static DEFINE_SPINLOCK(imgsensor_drv_lock);

#define MULTI_WRITE 1

#if MULTI_WRITE
//static const int I2C_BUFFER_LEN = 1020; /*trans# max is 255, each 4 bytes*/
#define I2C_BUFFER_LEN (1020)
#else
//static const int I2C_BUFFER_LEN = 4;
#define I2C_BUFFER_LEN (4)
#endif

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5KJNSSQ33_SENSOR_ID,

	.checksum_value = 0xf16e8197,
	.pre = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3152,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 520000000,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3152,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 520000000,
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3152,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 2296,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 548400000,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 600000000,
		.linelength = 2400,
		.framelength = 2080,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 547200000,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 600000000,
		.linelength = 4096,
		.framelength = 4880,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2040,
		.grabwindow_height = 1536,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 796000000,
		.max_framerate = 300,
	},
	/* NightMode */
	.custom1 = {
		.pclk = 600000000,
		.linelength = 2400,
		.framelength = 4112,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 547200000,
		.max_framerate = 600,
	},

	.min_gain = 64,                  /* 1x gain */
	.max_gain = 4096,                /* 64x gain at 12.5M/UHD @ 30fps and FHD/HD @ 240fps, datasheet page51*/
	.min_gain_iso = 100,
	.margin = 4,                     /* sensor framelength & shutter margin */
	.min_shutter = 2,                /* min shutter */
	.gain_step = 2,
	.gain_type = 2,
	.max_frame_length = 0xFFFF,      /* max framelength by sensor register's limitation */
	.ae_shut_delay_frame = 0,        /* shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2 */
	.ae_sensor_gain_delay_frame = 0, /* sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2 */
	.ae_ispGain_delay_frame = 2,	 /* isp gain delay frame for AE cycle */
	.frame_time_delay_frame = 1,    //[Alex] Bokeh sw sync enable
	.ihdr_support = 1,	             /* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	         /* 1,le first ; 0, se first */
	.sensor_mode_num = 6,	         /* support sensor mode num */

	.cap_delay_frame = 3,            /* enter capture delay frame num */
	.pre_delay_frame = 3,            /* enter preview delay frame num */
	.video_delay_frame = 3,          /* enter video delay frame num */
	.hs_video_delay_frame = 3,       /* enter high speed video  delay frame num */
	.slim_video_delay_frame = 3,     /* enter slim video delay frame num */
	.custom1_delay_frame = 3,        /* enter custom1 delay frame num */

	.isp_driving_current = ISP_DRIVING_6MA,	                 /* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,     /*sensor_interface_type */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,                     /* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = 0,                             /* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr, /* sensor output first pixel color */
	.mclk = 24,                                              /* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_4_LANE,                     /* mipi lane num */
	.i2c_addr_table = {0x20, 0xff},                          /* record sensor support all write id addr, only supprt 4must end with 0xff */
	.i2c_speed = 400,                                        /* support 1MHz write */
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,	                                /* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,                     /* IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video */
	.shutter = 0x0200,	                                    /* current shutter */
	.gain = 0x100,		                                    /* current gain */
	.dummy_pixel = 0,	                                    /* current dummypixel */
	.dummy_line = 0,	                                    /* current dummyline */
	.current_fps = 300,                                     /* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.autoflicker_en = KAL_FALSE,                            /* auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker */
	.test_pattern = KAL_FALSE,                              /* test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output */
	.enable_secure = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW, /* current scenario id */
	.ihdr_en = 0,
	.i2c_write_id = 0x20,                                   /* record current sensor's i2c write id */
};

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[4] = {
	/* Preview mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	 0x00, 0x2B, 0x0FF0, 0x0C00, 0x01, 0x00, 0x0000, 0x0000,
	 0x01, 0x30, 0x027C, 0x0BF0, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	 0x00, 0x2B, 0x0FF0, 0x08F8, 0x01, 0x00, 0x0000, 0x0000,
	 0x01, 0x30, 0x027C, 0x08F0, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	 0x00, 0x2B, 0x0FF0, 0x0C00, 0x01, 0x00, 0x0000, 0x0000,
	 0x01, 0x30, 0x027C, 0x0BF0, 0x03, 0x00, 0x0000, 0x0000},
	/* Slim video mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	 0x00, 0x2B, 0x07F8, 0x0600, 0x01, 0x00, 0x0000, 0x0000,
	 0x01, 0x30, 0x027C, 0x02FC, 0x03, 0x00, 0x0000, 0x0000}
	};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] = {
	{4080, 3072,   0,   0, 4080, 3072, 4080, 3072, 0000, 0000, 4080, 3072, 0, 0, 4080, 3072},	// Preview
	{4080, 3072,   0,   0, 4080, 3072, 4080, 3072, 0000, 0000, 4080, 3072, 0, 0, 4080, 3072},	// capture
	{4080, 3072,   0, 388, 4080, 2296, 4080, 2296, 0000, 0000, 4080, 2296, 0, 0, 4080, 2296},	// video
	{4080, 3072, 120, 456, 3840, 2160, 1920, 1080, 0000, 0000, 1920, 1080, 0, 0, 1920, 1080},	// high speed video
	{4080, 3072,   0,   0, 4080, 3072, 2040, 1536, 0000, 0000, 2040, 1536, 0, 0, 2040, 1536},	// slim video
	{4080, 3072, 120, 456, 3840, 2160, 1920, 1080, 0000, 0000, 1920, 1080, 0, 0, 1920, 1080}	// Custom1
};

/* add for s5kjnssq33 pdaf */
static struct  SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 8,
	.i4OffsetY = 8,
	.i4PitchX = 8,
	.i4PitchY = 8,
	.i4PairNum =4,
	.i4SubBlkW =8,
	.i4SubBlkH =2,

	.i4PosL = { {9,8},{11,11},{15,12},{13,15} },
	.i4PosR = { {8,8},{10,11},{14,12},{12,15} },
	.i4BlockNumX = 508,
	.i4BlockNumY = 382,
	.i4LeFirst = 0,
	.i4Crop = { {0, 0}, {0, 0}, {0, 388}, {120, 456}, {0, 0}, {120, 456}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	.iMirrorFlip = 0, /* 0:IMAGE_NORMAL, 1:IMAGE_H_MIRROR, 2:IMAGE_V_MIRROR, 3:IMAGE_HV_MIRROR*/
};

static struct  SET_PD_BLOCK_INFO_T imgsensor_pd_info_video = {
	.i4OffsetX = 8,
	.i4OffsetY = 8,
	.i4PitchX = 8,
	.i4PitchY = 8,
	.i4PairNum =4,
	.i4SubBlkW =8,
	.i4SubBlkH =2,

	.i4PosL = { {9,8},{11,11},{15,12},{13,15} },
	.i4PosR = { {8,8},{10,11},{14,12},{12,15} },
	.i4BlockNumX = 508,
	.i4BlockNumY = 286,
	.i4LeFirst = 0,
	.i4Crop = { {0, 0}, {0, 0}, {0, 388}, {120, 456}, {0, 0}, {120, 456}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	.iMirrorFlip = 0, /* 0:IMAGE_NORMAL, 1:IMAGE_H_MIRROR, 2:IMAGE_V_MIRROR, 3:IMAGE_HV_MIRROR*/
};

static struct  SET_PD_BLOCK_INFO_T imgsensor_pd_info_slim_video = {
	.i4OffsetX = 4,
	.i4OffsetY = 4,
	.i4PitchX = 4,
	.i4PitchY = 4,
	.i4PairNum =1,
	.i4SubBlkW =4,
	.i4SubBlkH =4,

	.i4PosL = { {4,5} },
	.i4PosR = { {5,5} },
	.i4BlockNumX = 508,
	.i4BlockNumY = 382,
	.i4LeFirst = 0,
	.i4Crop = { {0, 0}, {0, 0}, {0, 388}, {120, 456}, {0, 0}, {120, 456}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	.iMirrorFlip = 0, /* 0:IMAGE_NORMAL, 1:IMAGE_H_MIRROR, 2:IMAGE_V_MIRROR, 3:IMAGE_HV_MIRROR*/
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *) &get_byte, 2, imgsensor.i2c_write_id);
	return ((get_byte << 8) & 0xff00) | ((get_byte >> 8) & 0x00ff);
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *) &get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[4] = {
		(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)
	};

	iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[3] = {
		(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)
	};

	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static kal_uint16 table_write_cmos_sensor(kal_uint16 *para,
					  kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	tosend = 0;
	IDX = 0;

	while (len > IDX) {
		addr = para[IDX];

		{
			puSendCmd[tosend++] = (char)(addr >> 8);
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data >> 8);
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;

		}
#if MULTI_WRITE
		/* Write when remain buffer size is less than 3 bytes
		 * or reach end of data
		 */
		if ((I2C_BUFFER_LEN - tosend) < 4
			|| IDX == len || addr != addr_last) {
			LOG_INF("zong1 burst write\n");
			iBurstWriteReg_multi(puSendCmd,
						tosend,
						imgsensor.i2c_write_id,
						4,
						imgsensor_info.i2c_speed);
			tosend = 0;
		}
#else
	    LOG_INF("zong2 burst write\n");
		iWriteRegI2C(puSendCmd, 4, imgsensor.i2c_write_id);
		tosend = 0;
#endif
	}
	return 0;
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor_8(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor_8(0x0343, imgsensor.line_length & 0xFF);
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min_framelength_en=%d\n", framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	LOG_INF("frame_length =%d\n", frame_length);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length)
		? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}				/*      set_max_framerate  */


static void write_shutter(kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;

	if (imgsensor.sensor_mode == IMGSENSOR_MODE_HIGH_SPEED_VIDEO) {
		if (shutter >
		    imgsensor.min_frame_length - imgsensor_info.margin)
			shutter = imgsensor.min_frame_length
				- imgsensor_info.margin;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.frame_length = imgsensor.min_frame_length;
		spin_unlock(&imgsensor_drv_lock);
		write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x0202, (shutter >> 8) & 0xFF);
		write_cmos_sensor_8(0x0203, shutter & 0xFF);
		LOG_INF("shutter =%d, framelength =%d\n",
			shutter, imgsensor.frame_length);
		return;
	}
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;

	spin_unlock(&imgsensor_drv_lock);
	if (shutter < imgsensor_info.min_shutter)
		shutter = imgsensor_info.min_shutter;

	if (imgsensor.autoflicker_en == KAL_TRUE) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10
			/ imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
			//set_dummy();
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
			//set_dummy();
		} else {
			write_cmos_sensor_8(0x0340,
					    imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x0341,
					    imgsensor.frame_length & 0xFF);
		}
	} else {
		// Extend frame length
		write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
	}

	// Update Shutter
	write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor_8(0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(0x0203, shutter & 0xFF);
	LOG_INF("realtime_fps =%d\n", realtime_fps);
	LOG_INF("shutter =%d, framelength =%d\n",
	shutter, imgsensor.frame_length);

	//LOG_INF("frame_length = %d ", frame_length);

}

/*************************************************************************
 * FUNCTION
 *	set_shutter
 *
 * DESCRIPTION
 *	This function set e-shutter of sensor to change exposure time.
 *
 * PARAMETERS
 *	iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	write_shutter(shutter);
}				/*      set_shutter */

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;

	reg_gain = gain / 2;
	return (kal_uint16) reg_gain;
}

/*************************************************************************
 * FUNCTION
 *	set_gain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *	iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	if (gain < imgsensor_info.min_gain || gain > imgsensor_info.max_gain) {
		LOG_INF("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > imgsensor_info.max_gain)
			gain = imgsensor_info.max_gain;
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);


	write_cmos_sensor(0x0204, (reg_gain & 0xFFFF));

	return gain;
}				/*  s5kjnssq33MIPI_SetGain  */

/*************************************************************************
 * FUNCTION
 *	night_mode
 *
 * DESCRIPTION
 *	This function night mode of sensor.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(kal_bool enable)
{
	/*No Need to implement this function*/
}				/*      night_mode      */

static void check_stream(void)
{
	unsigned int i = 0;
	int frame_count;
	int timeout = (10000 / imgsensor.current_fps) + 1;

	frame_count = read_cmos_sensor_8(0x0005);
	LOG_INF("[Stanley]frame_count: %d\n", frame_count);

	mdelay(3);
	for (i = 0; i < timeout; i++) {
		if (read_cmos_sensor_8(0x0005) != 0)
			LOG_INF("[Stanley]check_stream on!\n");
		else
		  LOG_INF("[Stanley]check_stream off!\n");
		  break;
	}
}

//[Alex] B: Bokeh sw sync enable
static void set_shutter_frame_length(kal_uint16 shutter,
				     kal_uint16 target_frame_length)
{

	//pr_debug("shutter: %d, target_frame_length: %d", shutter, target_frame_length);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.dummy_line = (target_frame_length >
				imgsensor_info.cap.framelength) ? (target_frame_length -
						imgsensor_info.cap.framelength) : 0;
	imgsensor.frame_length = imgsensor_info.cap.framelength +
				 imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	LOG_INF("shutter: %d, frame_length: %d, cap.framelength: %d dummy_line: %d\n",
		 shutter, imgsensor.frame_length,
		 imgsensor_info.cap.framelength, imgsensor.dummy_line);
	spin_unlock(&imgsensor_drv_lock);

	//set_dummy();
	// frame_length will be set in set_shutter
	set_shutter(shutter);

}
//[Alex] E: Bokeh sw sync enable

static kal_uint32 streaming_control(kal_bool enable)
{
	//pr_info("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable) {
		write_cmos_sensor(0x6028, 0x4000); // stream on
		write_cmos_sensor(0x0100, 0x0100);
		pr_info("stream on streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
		check_stream();
		mdelay(10);
	}
	else {
		write_cmos_sensor(0x0100, 0x00); // stream off
		pr_info("stream off streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
		check_stream();
	}
	return ERROR_NONE;
}

static kal_uint16 s5kjnssq33_init_setting[] = {
	0x6226,	0x0001,
	0x6028,	0x2400,
	0x602A,	0x35CC,
	0x6F12,	0x1C80,
	0x6F12,	0x0024,
	0x6F12,	0xA88C,
	0x6F12,	0x0024,
	0x602A,	0x1354,
	0x6F12,	0x0100,
	0x6F12,	0x7017,
	0x602A,	0x13B2,
	0x6F12,	0x0000,
	0x602A,	0x1236,
	0x6F12,	0x0000,
	0x602A,	0x1A0A,
	0x6F12,	0x4C0A,
	0x602A,	0x2210,
	0x6F12,	0x3401,
	0x602A,	0x2176,
	0x6F12,	0x6400,
	0x602A,	0x222E,
	0x6F12,	0x0001,
	0x602A,	0x06B6,
	0x6F12,	0x0A00,
	0x602A,	0x06BC,
	0x6F12,	0x1001,
	0x602A,	0x2140,
	0x6F12,	0x0101,
	0x602A,	0x1A0E,
	0x6F12,	0x9600,
	0x602A,	0x19FC,
	0x6F12,	0x0B00,
	0x6028,	0x4000,
	0xF44E,	0x0011,
	0xF44C,	0x0B0B,
	0xF44A,	0x0006,
	0x0118,	0x0002,
	0x011A,	0x0001,
	0x0FEA,	0x1940,
};

static kal_uint16 s5kjnssq33_preview_setting[] = {
	0x6028, 0x2400,
	0x602A, 0x1A28,
	0x6F12, 0x4C00,
	0x602A, 0x065A,
	0x6F12, 0x0000,
	0x602A, 0x139E,
	0x6F12, 0x0100,
	0x602A, 0x139C,
	0x6F12, 0x0000,
	0x602A, 0x13A0,
	0x6F12, 0x0A00,
	0x6F12, 0x0120,
	0x602A, 0x2072,
	0x6F12, 0x0000,
	0x602A, 0x1A64,
	0x6F12, 0x0301,
	0x6F12, 0xFF00,
	0x602A, 0x19E6,
	0x6F12, 0x0200,
	0x602A, 0x1A30,
	0x6F12, 0x3401,
	0x602A, 0x19F4,
	0x6F12, 0x0606,
	0x602A, 0x19F8,
	0x6F12, 0x1010,
	0x602A, 0x1B26,
	0x6F12, 0x6F80,
	0x6F12, 0xA060,
	0x602A, 0x1A3C,
	0x6F12, 0x6207,
	0x602A, 0x1A48,
	0x6F12, 0x6207,
	0x602A, 0x1444,
	0x6F12, 0x2000,
	0x6F12, 0x2000,
	0x602A, 0x144C,
	0x6F12, 0x3F00,
	0x6F12, 0x3F00,
	0x602A, 0x7F6C,
	0x6F12, 0x0100,
	0x6F12, 0x2F00,
	0x6F12, 0xFA00,
	0x6F12, 0x2400,
	0x6F12, 0xE500,
	0x602A, 0x0650,
	0x6F12, 0x0600,
	0x602A, 0x0654,
	0x6F12, 0x0000,
	0x602A, 0x1A46,
	0x6F12, 0x8A00,
	0x602A, 0x1A52,
	0x6F12, 0xBF00,
	0x602A, 0x0674,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x0668,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x602A, 0x0684,
	0x6F12, 0x4001,
	0x602A, 0x0688,
	0x6F12, 0x4001,
	0x602A, 0x147C,
	0x6F12, 0x1000,
	0x602A, 0x1480,
	0x6F12, 0x1000,
	0x602A, 0x19F6,
	0x6F12, 0x0904,
	0x602A, 0x0812,
	0x6F12, 0x0000,
	0x602A, 0x1A02,
	0x6F12, 0x1800,
	0x602A, 0x2104,
	0x6F12, 0x0018,
	0x602A, 0x2148,
	0x6F12, 0x0100,
	0x602A, 0x2042,
	0x6F12, 0x1A00,
	0x602A, 0x0874,
	0x6F12, 0x0100,
	0x602A, 0x09C0,
	0x6F12, 0x2008,
	0x602A, 0x09C4,
	0x6F12, 0x2000,
	0x602A, 0x19FE,
	0x6F12, 0x0E1C,
	0x602A, 0x4D92,
	0x6F12, 0x0100,
	0x602A, 0x8A88,
	0x6F12, 0x0100,
	0x602A, 0x4D94,
	0x6F12, 0x0005,
	0x6F12, 0x000A,
	0x6F12, 0x0010,
	0x6F12, 0x0810,
	0x6F12, 0x000A,
	0x6F12, 0x0040,
	0x6F12, 0x0810,
	0x6F12, 0x0810,
	0x6F12, 0x8002,
	0x6F12, 0xFD03,
	0x6F12, 0x0010,
	0x6F12, 0x1510,
	0x602A, 0x3570,
	0x6F12, 0x0000,
	0x602A, 0x3574,
	0x6F12, 0x1201,
	0x602A, 0x21E4,
	0x6F12, 0x0400,
	0x602A, 0x21EC,
	0x6F12, 0x1F04,
	0x602A, 0x2080,
	0x6F12, 0x0101,
	0x6F12, 0xFF00,
	0x6F12, 0x7F01,
	0x6F12, 0x0001,
	0x6F12, 0x8001,
	0x6F12, 0xD244,
	0x6F12, 0xD244,
	0x6F12, 0x14F4,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x20BA,
	0x6F12, 0x121C,
	0x6F12, 0x111C,
	0x6F12, 0x54F4,
	0x602A, 0x120E,
	0x6F12, 0x1000,
	0x602A, 0x212E,
	0x6F12, 0x0200,
	0x602A, 0x13AE,
	0x6F12, 0x0101,
	0x602A, 0x0718,
	0x6F12, 0x0001,
	0x602A, 0x0710,
	0x6F12, 0x0002,
	0x6F12, 0x0804,
	0x6F12, 0x0100,
	0x602A, 0x1B5C,
	0x6F12, 0x0000,
	0x602A, 0x0786,
	0x6F12, 0x7701,
	0x602A, 0x2022,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x1360,
	0x6F12, 0x0100,
	0x602A, 0x1376,
	0x6F12, 0x0100,
	0x6F12, 0x6038,
	0x6F12, 0x7038,
	0x6F12, 0x8038,
	0x602A, 0x1386,
	0x6F12, 0x0B00,
	0x602A, 0x06FA,
	0x6F12, 0x0000,
	0x602A, 0x4A94,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x0A76,
	0x6F12, 0x1000,
	0x602A, 0x0AEE,
	0x6F12, 0x1000,
	0x602A, 0x0B66,
	0x6F12, 0x1000,
	0x602A, 0x0BDE,
	0x6F12, 0x1000,
	0x602A, 0x0BE8,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0C56,
	0x6F12, 0x1000,
	0x602A, 0x0C60,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0CB6,
	0x6F12, 0x0100,
	0x602A, 0x0CF2,
	0x6F12, 0x0001,
	0x602A, 0x0CF0,
	0x6F12, 0x0101,
	0x602A, 0x11B8,
	0x6F12, 0x0100,
	0x602A, 0x11F6,
	0x6F12, 0x0020,
	0x602A, 0x4A74,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x218E,
	0x6F12, 0x0000,
	0x602A, 0x2268,
	0x6F12, 0xF279,
	0x602A, 0x5006,
	0x6F12, 0x0000,
	0x602A, 0x500E,
	0x6F12, 0x0100,
	0x602A, 0x4E70,
	0x6F12, 0x2062,
	0x6F12, 0x5501,
	0x602A, 0x06DC,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0xF46A, 0xAE80,
	0x0344, 0x0000,
	0x0346, 0x0000,
	0x0348, 0x1FFF,
	0x034A, 0x181F,
	0x034C, 0x0FF0,
	0x034E, 0x0C00,
	0x0350, 0x0008,
	0x0352, 0x0008,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0110, 0x1002,
	0x0114, 0x0301,
	0x0116, 0x3000,
	0x0136, 0x1800,
	0x013E, 0x0000,
	0x0300, 0x0006,
	0x0302, 0x0001,
	0x0304, 0x0004,
	0x0306, 0x008C,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x006C,
	0x0312, 0x0000,
	0x080E, 0x0000,
	0x0340, 0x0C50,
	0x0342, 0x1716,
	0x0702, 0x0000,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x0D00, 0x0101,
	0x0D02, 0x0101,
	0x0D04, 0x0102,
	0x6226, 0x0000,
};

static kal_uint16 s5kjnssq33_cap_setting[] = {
	0x6028, 0x2400,
	0x602A, 0x1A28,
	0x6F12, 0x4C00,
	0x602A, 0x065A,
	0x6F12, 0x0000,
	0x602A, 0x139E,
	0x6F12, 0x0100,
	0x602A, 0x139C,
	0x6F12, 0x0000,
	0x602A, 0x13A0,
	0x6F12, 0x0A00,
	0x6F12, 0x0120,
	0x602A, 0x2072,
	0x6F12, 0x0000,
	0x602A, 0x1A64,
	0x6F12, 0x0301,
	0x6F12, 0xFF00,
	0x602A, 0x19E6,
	0x6F12, 0x0200,
	0x602A, 0x1A30,
	0x6F12, 0x3401,
	0x602A, 0x19F4,
	0x6F12, 0x0606,
	0x602A, 0x19F8,
	0x6F12, 0x1010,
	0x602A, 0x1B26,
	0x6F12, 0x6F80,
	0x6F12, 0xA060,
	0x602A, 0x1A3C,
	0x6F12, 0x6207,
	0x602A, 0x1A48,
	0x6F12, 0x6207,
	0x602A, 0x1444,
	0x6F12, 0x2000,
	0x6F12, 0x2000,
	0x602A, 0x144C,
	0x6F12, 0x3F00,
	0x6F12, 0x3F00,
	0x602A, 0x7F6C,
	0x6F12, 0x0100,
	0x6F12, 0x2F00,
	0x6F12, 0xFA00,
	0x6F12, 0x2400,
	0x6F12, 0xE500,
	0x602A, 0x0650,
	0x6F12, 0x0600,
	0x602A, 0x0654,
	0x6F12, 0x0000,
	0x602A, 0x1A46,
	0x6F12, 0x8A00,
	0x602A, 0x1A52,
	0x6F12, 0xBF00,
	0x602A, 0x0674,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x0668,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x602A, 0x0684,
	0x6F12, 0x4001,
	0x602A, 0x0688,
	0x6F12, 0x4001,
	0x602A, 0x147C,
	0x6F12, 0x1000,
	0x602A, 0x1480,
	0x6F12, 0x1000,
	0x602A, 0x19F6,
	0x6F12, 0x0904,
	0x602A, 0x0812,
	0x6F12, 0x0000,
	0x602A, 0x1A02,
	0x6F12, 0x1800,
	0x602A, 0x2104,
	0x6F12, 0x0018,
	0x602A, 0x2148,
	0x6F12, 0x0100,
	0x602A, 0x2042,
	0x6F12, 0x1A00,
	0x602A, 0x0874,
	0x6F12, 0x0100,
	0x602A, 0x09C0,
	0x6F12, 0x2008,
	0x602A, 0x09C4,
	0x6F12, 0x2000,
	0x602A, 0x19FE,
	0x6F12, 0x0E1C,
	0x602A, 0x4D92,
	0x6F12, 0x0100,
	0x602A, 0x8A88,
	0x6F12, 0x0100,
	0x602A, 0x4D94,
	0x6F12, 0x0005,
	0x6F12, 0x000A,
	0x6F12, 0x0010,
	0x6F12, 0x0810,
	0x6F12, 0x000A,
	0x6F12, 0x0040,
	0x6F12, 0x0810,
	0x6F12, 0x0810,
	0x6F12, 0x8002,
	0x6F12, 0xFD03,
	0x6F12, 0x0010,
	0x6F12, 0x1510,
	0x602A, 0x3570,
	0x6F12, 0x0000,
	0x602A, 0x3574,
	0x6F12, 0x1201,
	0x602A, 0x21E4,
	0x6F12, 0x0400,
	0x602A, 0x21EC,
	0x6F12, 0x1F04,
	0x602A, 0x2080,
	0x6F12, 0x0101,
	0x6F12, 0xFF00,
	0x6F12, 0x7F01,
	0x6F12, 0x0001,
	0x6F12, 0x8001,
	0x6F12, 0xD244,
	0x6F12, 0xD244,
	0x6F12, 0x14F4,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x20BA,
	0x6F12, 0x121C,
	0x6F12, 0x111C,
	0x6F12, 0x54F4,
	0x602A, 0x120E,
	0x6F12, 0x1000,
	0x602A, 0x212E,
	0x6F12, 0x0200,
	0x602A, 0x13AE,
	0x6F12, 0x0101,
	0x602A, 0x0718,
	0x6F12, 0x0001,
	0x602A, 0x0710,
	0x6F12, 0x0002,
	0x6F12, 0x0804,
	0x6F12, 0x0100,
	0x602A, 0x1B5C,
	0x6F12, 0x0000,
	0x602A, 0x0786,
	0x6F12, 0x7701,
	0x602A, 0x2022,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x1360,
	0x6F12, 0x0100,
	0x602A, 0x1376,
	0x6F12, 0x0100,
	0x6F12, 0x6038,
	0x6F12, 0x7038,
	0x6F12, 0x8038,
	0x602A, 0x1386,
	0x6F12, 0x0B00,
	0x602A, 0x06FA,
	0x6F12, 0x0000,
	0x602A, 0x4A94,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x0A76,
	0x6F12, 0x1000,
	0x602A, 0x0AEE,
	0x6F12, 0x1000,
	0x602A, 0x0B66,
	0x6F12, 0x1000,
	0x602A, 0x0BDE,
	0x6F12, 0x1000,
	0x602A, 0x0BE8,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0C56,
	0x6F12, 0x1000,
	0x602A, 0x0C60,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0CB6,
	0x6F12, 0x0100,
	0x602A, 0x0CF2,
	0x6F12, 0x0001,
	0x602A, 0x0CF0,
	0x6F12, 0x0101,
	0x602A, 0x11B8,
	0x6F12, 0x0100,
	0x602A, 0x11F6,
	0x6F12, 0x0020,
	0x602A, 0x4A74,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x218E,
	0x6F12, 0x0000,
	0x602A, 0x2268,
	0x6F12, 0xF279,
	0x602A, 0x5006,
	0x6F12, 0x0000,
	0x602A, 0x500E,
	0x6F12, 0x0100,
	0x602A, 0x4E70,
	0x6F12, 0x2062,
	0x6F12, 0x5501,
	0x602A, 0x06DC,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0xF46A, 0xAE80,
	0x0344, 0x0000,
	0x0346, 0x0000,
	0x0348, 0x1FFF,
	0x034A, 0x181F,
	0x034C, 0x0FF0,
	0x034E, 0x0C00,
	0x0350, 0x0008,
	0x0352, 0x0008,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0110, 0x1002,
	0x0114, 0x0301,
	0x0116, 0x3000,
	0x0136, 0x1800,
	0x013E, 0x0000,
	0x0300, 0x0006,
	0x0302, 0x0001,
	0x0304, 0x0004,
	0x0306, 0x008C,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x006C,
	0x0312, 0x0000,
	0x080E, 0x0000,
	0x0340, 0x0C50,
	0x0342, 0x1716,
	0x0702, 0x0000,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x0D00, 0x0101,
	0x0D02, 0x0101,
	0x0D04, 0x0102,
	0x6226, 0x0000,
};

static kal_uint16 s5kjnssq33_normal_video_setting[] = {
	0x6028, 0x2400,
	0x602A, 0x1A28,
	0x6F12, 0x4C00,
	0x602A, 0x065A,
	0x6F12, 0x0000,
	0x602A, 0x139E,
	0x6F12, 0x0100,
	0x602A, 0x139C,
	0x6F12, 0x0000,
	0x602A, 0x13A0,
	0x6F12, 0x0A00,
	0x6F12, 0x0120,
	0x602A, 0x2072,
	0x6F12, 0x0000,
	0x602A, 0x1A64,
	0x6F12, 0x0301,
	0x6F12, 0xFF00,
	0x602A, 0x19E6,
	0x6F12, 0x0200,
	0x602A, 0x1A30,
	0x6F12, 0x3401,
	0x602A, 0x19F4,
	0x6F12, 0x0606,
	0x602A, 0x19F8,
	0x6F12, 0x1010,
	0x602A, 0x1B26,
	0x6F12, 0x6F80,
	0x6F12, 0xA060,
	0x602A, 0x1A3C,
	0x6F12, 0x6207,
	0x602A, 0x1A48,
	0x6F12, 0x6207,
	0x602A, 0x1444,
	0x6F12, 0x2000,
	0x6F12, 0x2000,
	0x602A, 0x144C,
	0x6F12, 0x3F00,
	0x6F12, 0x3F00,
	0x602A, 0x7F6C,
	0x6F12, 0x0100,
	0x6F12, 0x2F00,
	0x6F12, 0xFA00,
	0x6F12, 0x2400,
	0x6F12, 0xE500,
	0x602A, 0x0650,
	0x6F12, 0x0600,
	0x602A, 0x0654,
	0x6F12, 0x0000,
	0x602A, 0x1A46,
	0x6F12, 0x8A00,
	0x602A, 0x1A52,
	0x6F12, 0xBF00,
	0x602A, 0x0674,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x0668,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x602A, 0x0684,
	0x6F12, 0x4001,
	0x602A, 0x0688,
	0x6F12, 0x4001,
	0x602A, 0x147C,
	0x6F12, 0x1000,
	0x602A, 0x1480,
	0x6F12, 0x1000,
	0x602A, 0x19F6,
	0x6F12, 0x0904,
	0x602A, 0x0812,
	0x6F12, 0x0000,
	0x602A, 0x1A02,
	0x6F12, 0x1800,
	0x602A, 0x2104,
	0x6F12, 0x0018,
	0x602A, 0x2148,
	0x6F12, 0x0100,
	0x602A, 0x2042,
	0x6F12, 0x1A00,
	0x602A, 0x0874,
	0x6F12, 0x0100,
	0x602A, 0x09C0,
	0x6F12, 0x2008,
	0x602A, 0x09C4,
	0x6F12, 0x2000,
	0x602A, 0x19FE,
	0x6F12, 0x0E1C,
	0x602A, 0x4D92,
	0x6F12, 0x0100,
	0x602A, 0x8A88,
	0x6F12, 0x0100,
	0x602A, 0x4D94,
	0x6F12, 0x0005,
	0x6F12, 0x000A,
	0x6F12, 0x0010,
	0x6F12, 0x0810,
	0x6F12, 0x000A,
	0x6F12, 0x0040,
	0x6F12, 0x0810,
	0x6F12, 0x0810,
	0x6F12, 0x8002,
	0x6F12, 0xFD03,
	0x6F12, 0x0010,
	0x6F12, 0x1510,
	0x602A, 0x3570,
	0x6F12, 0x0000,
	0x602A, 0x3574,
	0x6F12, 0x0000,
	0x602A, 0x21E4,
	0x6F12, 0x0400,
	0x602A, 0x21EC,
	0x6F12, 0x8000,
	0x602A, 0x2080,
	0x6F12, 0x0101,
	0x6F12, 0xFF00,
	0x6F12, 0x7F01,
	0x6F12, 0x0001,
	0x6F12, 0x8001,
	0x6F12, 0xD244,
	0x6F12, 0xD244,
	0x6F12, 0x14F4,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x20BA,
	0x6F12, 0x121C,
	0x6F12, 0x111C,
	0x6F12, 0x54F4,
	0x602A, 0x120E,
	0x6F12, 0x1000,
	0x602A, 0x212E,
	0x6F12, 0x0200,
	0x602A, 0x13AE,
	0x6F12, 0x0101,
	0x602A, 0x0718,
	0x6F12, 0x0001,
	0x602A, 0x0710,
	0x6F12, 0x0002,
	0x6F12, 0x0804,
	0x6F12, 0x0100,
	0x602A, 0x1B5C,
	0x6F12, 0x0000,
	0x602A, 0x0786,
	0x6F12, 0x7701,
	0x602A, 0x2022,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x1360,
	0x6F12, 0x0100,
	0x602A, 0x1376,
	0x6F12, 0x0100,
	0x6F12, 0x6038,
	0x6F12, 0x7038,
	0x6F12, 0x8038,
	0x602A, 0x1386,
	0x6F12, 0x0B00,
	0x602A, 0x06FA,
	0x6F12, 0x0000,
	0x602A, 0x4A94,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x0A76,
	0x6F12, 0x1000,
	0x602A, 0x0AEE,
	0x6F12, 0x1000,
	0x602A, 0x0B66,
	0x6F12, 0x1000,
	0x602A, 0x0BDE,
	0x6F12, 0x1000,
	0x602A, 0x0BE8,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0C56,
	0x6F12, 0x1000,
	0x602A, 0x0C60,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0CB6,
	0x6F12, 0x0100,
	0x602A, 0x0CF2,
	0x6F12, 0x0001,
	0x602A, 0x0CF0,
	0x6F12, 0x0101,
	0x602A, 0x11B8,
	0x6F12, 0x0100,
	0x602A, 0x11F6,
	0x6F12, 0x0020,
	0x602A, 0x4A74,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x218E,
	0x6F12, 0x0000,
	0x602A, 0x2268,
	0x6F12, 0xF279,
	0x602A, 0x5006,
	0x6F12, 0x0000,
	0x602A, 0x500E,
	0x6F12, 0x0100,
	0x602A, 0x4E70,
	0x6F12, 0x2062,
	0x6F12, 0x5501,
	0x602A, 0x06DC,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0xF46A, 0xAE80,
	0x0344, 0x0000,
	0x0346, 0x0308,
	0x0348, 0x1FFF,
	0x034A, 0x1517,
	0x034C, 0x0FF0,
	0x034E, 0x08F8,
	0x0350, 0x0008,
	0x0352, 0x0008,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0110, 0x1002,
	0x0114, 0x0301,
	0x0116, 0x3000,
	0x0136, 0x1800,
	0x013E, 0x0000,
	0x0300, 0x0006,
	0x0302, 0x0001,
	0x0304, 0x0004,
	0x0306, 0x008C,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0072,
	0x0312, 0x0000,
	0x080E, 0x0000,
	0x0340, 0x0C50,
	0x0342, 0x1716,
	0x0702, 0x0000,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x0D00, 0x0101,
	0x0D02, 0x0101,
	0x0D04, 0x0102,
	0x6226, 0x0000,
};

static kal_uint16 s5kjnssq33_hs_video_setting[] = {
	0x6028,	0x2400,
	0x602A,	0x1A28,
	0x6F12,	0x4C00,
	0x602A,	0x065A,
	0x6F12,	0x0000,
	0x602A,	0x139E,
	0x6F12, 0x0300,
	0x602A,	0x139C,
	0x6F12,	0x0000,
	0x602A,	0x13A0,
	0x6F12,	0x0A00,
	0x6F12, 0x0020,
	0x602A,	0x2072,
	0x6F12,	0x0000,
	0x602A,	0x1A64,
	0x6F12,	0x0301,
	0x6F12, 0x3F00,
	0x602A,	0x19E6,
	0x6F12, 0x0201,
	0x602A,	0x1A30,
	0x6F12,	0x3401,
	0x602A,	0x19F4,
	0x6F12,	0x0606,
	0x602A,	0x19F8,
	0x6F12,	0x1010,
	0x602A,	0x1B26,
	0x6F12,	0x6F80,
	0x6F12, 0xA020,
	0x602A,	0x1A3C,
	0x6F12, 0x5207,
	0x602A,	0x1A48,
	0x6F12, 0x5207,
	0x602A,	0x1444,
	0x6F12, 0x2100,
	0x6F12, 0x2100,
	0x602A,	0x144C,
	0x6F12, 0x4200,
	0x6F12, 0x4200,
	0x602A,	0x7F6C,
	0x6F12,	0x0100,
	0x6F12, 0x3100,
	0x6F12, 0xF700,
	0x6F12, 0x2600,
	0x6F12, 0xE100,
	0x602A,	0x0650,
	0x6F12,	0x0600,
	0x602A,	0x0654,
	0x6F12,	0x0000,
	0x602A,	0x1A46,
	0x6F12, 0x8600,
	0x602A,	0x1A52,
	0x6F12,	0xBF00,
	0x602A,	0x0674,
	0x6F12,	0x0500,
	0x6F12,	0x0500,
	0x6F12,	0x0500,
	0x6F12,	0x0500,
	0x602A,	0x0668,
	0x6F12,	0x0800,
	0x6F12,	0x0800,
	0x6F12,	0x0800,
	0x6F12,	0x0800,
	0x602A,	0x0684,
	0x6F12,	0x4001,
	0x602A,	0x0688,
	0x6F12,	0x4001,
	0x602A,	0x147C,
	0x6F12,	0x1000,
	0x602A,	0x1480,
	0x6F12,	0x1000,
	0x602A,	0x19F6,
	0x6F12,	0x0904,
	0x602A,	0x0812,
	0x6F12,	0x0000,
	0x602A,	0x1A02,
	0x6F12, 0x0800,
	0x602A,	0x2104,
	0x6F12, 0x8E12,
	0x602A,	0x2148,
	0x6F12,	0x0100,
	0x602A,	0x2042,
	0x6F12,	0x1A00,
	0x602A,	0x0874,
	0x6F12, 0x1100,
	0x602A,	0x09C0,
	0x6F12, 0x9800,
	0x602A,	0x09C4,
	0x6F12, 0x9800,
	0x602A,	0x19FE,
	0x6F12,	0x0E1C,
	0x602A,	0x4D92,
	0x6F12,	0x0100,
	0x602A,	0x8A88,
	0x6F12,	0x0100,
	0x602A,	0x4D94,
	0x6F12, 0x4001,
	0x6F12, 0x0004,
	0x6F12,	0x0010,
	0x6F12,	0x0810,
	0x6F12, 0x0004,
	0x6F12, 0x0010,
	0x6F12,	0x0810,
	0x6F12,	0x0810,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0010,
	0x6F12,	0x0010,
	0x602A,	0x3570,
	0x6F12,	0x0000,
	0x602A,	0x3574,
	0x6F12, 0x3801,
	0x602A,	0x21E4,
	0x6F12,	0x0400,
	0x602A,	0x21EC,
	0x6F12, 0x6801,
	0x602A,	0x2080,
	0x6F12, 0x0100,
	0x6F12, 0x7F00,
	0x6F12, 0x0002,
	0x6F12, 0x8000,
	0x6F12, 0x0002,
	0x6F12, 0xC244,
	0x6F12,	0xD244,
	0x6F12,	0x14F4,
	0x6F12, 0x141C,
	0x6F12, 0x111C,
	0x6F12, 0x54F4,
	0x602A, 0x20BA,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x602A,	0x120E,
	0x6F12,	0x1000,
	0x602A,	0x212E,
	0x6F12, 0x0A00,
	0x602A,	0x13AE,
	0x6F12, 0x0102,
	0x602A,	0x0718,
	0x6F12, 0x0005,
	0x602A,	0x0710,
	0x6F12, 0x0004,
	0x6F12, 0x0401,
	0x6F12,	0x0100,
	0x602A,	0x1B5C,
	0x6F12, 0x0300,
	0x602A,	0x0786,
	0x6F12,	0x7701,
	0x602A,	0x2022,
	0x6F12, 0x0101,
	0x6F12, 0x0101,
	0x602A,	0x1360,
	0x6F12, 0x0000,
	0x602A,	0x1376,
	0x6F12, 0x0200,
	0x6F12,	0x6038,
	0x6F12,	0x7038,
	0x6F12,	0x8038,
	0x602A,	0x1386,
	0x6F12,	0x0B00,
	0x602A,	0x06FA,
	0x6F12,	0x1000,
	0x602A,	0x4A94,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x602A,	0x0A76,
	0x6F12,	0x1000,
	0x602A,	0x0AEE,
	0x6F12,	0x1000,
	0x602A,	0x0B66,
	0x6F12,	0x1000,
	0x602A,	0x0BDE,
	0x6F12,	0x1000,
	0x602A,	0x0BE8,
	0x6F12,	0x3000,
	0x6F12,	0x3000,
	0x602A,	0x0C56,
	0x6F12,	0x1000,
	0x602A,	0x0C60,
	0x6F12,	0x3000,
	0x6F12,	0x3000,
	0x602A,	0x0CB6,
	0x6F12, 0x0000,
	0x602A,	0x0CF2,
	0x6F12,	0x0001,
	0x602A,	0x0CF0,
	0x6F12,	0x0101,
	0x602A,	0x11B8,
	0x6F12, 0x0000,
	0x602A,	0x11F6,
	0x6F12, 0x0010,
	0x602A,	0x4A74,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x602A,	0x218E,
	0x6F12,	0x0000,
	0x602A,	0x2268,
	0x6F12,	0xF279,
	0x602A,	0x5006,
	0x6F12,	0x0000,
	0x602A,	0x500E,
	0x6F12,	0x0100,
	0x602A,	0x4E70,
	0x6F12,	0x2062,
	0x6F12,	0x5501,
	0x602A,	0x06DC,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6F12,	0x0000,
	0x6028,	0x4000,
	0xF46A,	0xAE80,
	0x0344, 0x00F0,
	0x0346, 0x0390,
	0x0348, 0x1F0F,
	0x034A, 0x148F,
	0x034C, 0x0780,
	0x034E, 0x0438,
	0x0350, 0x0004,
	0x0352, 0x0004,
	0x0900, 0x0144,
	0x0380,	0x0002,
	0x0382, 0x0006,
	0x0384,	0x0002,
	0x0386, 0x0006,
	0x0110,	0x1002,
	0x0114,	0x0300,
	0x0116,	0x3000,
	0x0136,	0x1800,
	0x013E,	0x0000,
	0x0300,	0x0006,
	0x0302,	0x0001,
	0x0304,	0x0004,
	0x0306, 0x0096,
	0x0308,	0x0008,
	0x030A,	0x0001,
	0x030C,	0x0000,
	0x030E, 0x0004,
	0x0310, 0x0072,
	0x0312, 0x0000,
	0x080E,	0x0000,
	0x0340, 0x0820,
	0x0342, 0x0960,
	0x0702,	0x0000,
	0x0202,	0x0100,
	0x0200,	0x0100,
	0x0D00,	0x0101,
	0x0D02,	0x0000,
	0x0D04,	0x0102,
	0x6226,	0x0000,
};

static kal_uint16 s5kjnssq33_slim_video_setting[] = {
	0x6028, 0x2400,
	0x602A, 0x1A28,
	0x6F12, 0x4C00,
	0x602A, 0x065A,
	0x6F12, 0x0000,
	0x602A, 0x139E,
	0x6F12, 0x0300,
	0x602A, 0x139C,
	0x6F12, 0x0000,
	0x602A, 0x13A0,
	0x6F12, 0x0A00,
	0x6F12, 0x0020,
	0x602A, 0x2072,
	0x6F12, 0x0000,
	0x602A, 0x1A64,
	0x6F12, 0x0301,
	0x6F12, 0x3F00,
	0x602A, 0x19E6,
	0x6F12, 0x0201,
	0x602A, 0x1A30,
	0x6F12, 0x3401,
	0x602A, 0x19F4,
	0x6F12, 0x0606,
	0x602A, 0x19F8,
	0x6F12, 0x1010,
	0x602A, 0x1B26,
	0x6F12, 0x6F80,
	0x6F12, 0xA020,
	0x602A, 0x1A3C,
	0x6F12, 0x5207,
	0x602A, 0x1A48,
	0x6F12, 0x5207,
	0x602A, 0x1444,
	0x6F12, 0x2100,
	0x6F12, 0x2100,
	0x602A, 0x144C,
	0x6F12, 0x4200,
	0x6F12, 0x4200,
	0x602A, 0x7F6C,
	0x6F12, 0x0100,
	0x6F12, 0x3100,
	0x6F12, 0xF700,
	0x6F12, 0x2600,
	0x6F12, 0xE100,
	0x602A, 0x0650,
	0x6F12, 0x0600,
	0x602A, 0x0654,
	0x6F12, 0x0000,
	0x602A, 0x1A46,
	0x6F12, 0x8600,
	0x602A, 0x1A52,
	0x6F12, 0xBF00,
	0x602A, 0x0674,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x0668,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x602A, 0x0684,
	0x6F12, 0x4001,
	0x602A, 0x0688,
	0x6F12, 0x4001,
	0x602A, 0x147C,
	0x6F12, 0x1000,
	0x602A, 0x1480,
	0x6F12, 0x1000,
	0x602A, 0x19F6,
	0x6F12, 0x0904,
	0x602A, 0x0812,
	0x6F12, 0x0000,
	0x602A, 0x1A02,
	0x6F12, 0x0800,
	0x602A, 0x2104,
	0x6F12, 0x8E12,
	0x602A, 0x2148,
	0x6F12, 0x0100,
	0x602A, 0x2042,
	0x6F12, 0x1A00,
	0x602A, 0x0874,
	0x6F12, 0x1100,
	0x602A, 0x09C0,
	0x6F12, 0x9800,
	0x602A, 0x09C4,
	0x6F12, 0x9800,
	0x602A, 0x19FE,
	0x6F12, 0x0E1C,
	0x602A, 0x4D92,
	0x6F12, 0x0100,
	0x602A, 0x8A88,
	0x6F12, 0x0100,
	0x602A, 0x4D94,
	0x6F12, 0x4001,
	0x6F12, 0x0004,
	0x6F12, 0x0010,
	0x6F12, 0x0810,
	0x6F12, 0x0004,
	0x6F12, 0x0010,
	0x6F12, 0x0810,
	0x6F12, 0x0810,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0010,
	0x6F12, 0x0010,
	0x602A, 0x3570,
	0x6F12, 0x0000,
	0x602A, 0x3574,
	0x6F12, 0x0000,
	0x602A, 0x21E4,
	0x6F12, 0x0400,
	0x602A, 0x21EC,
	0x6F12, 0x1001,
	0x602A, 0x2080,
	0x6F12, 0x0100,
	0x6F12, 0x7F00,
	0x6F12, 0x0002,
	0x6F12, 0x8000,
	0x6F12, 0x0002,
	0x6F12, 0xC244,
	0x6F12, 0xD244,
	0x6F12, 0x14F4,
	0x6F12, 0x141C,
	0x6F12, 0x111C,
	0x6F12, 0x54F4,
	0x602A, 0x20BA,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x120E,
	0x6F12, 0x1000,
	0x602A, 0x212E,
	0x6F12, 0x0A00,
	0x602A, 0x13AE,
	0x6F12, 0x0102,
	0x602A, 0x0718,
	0x6F12, 0x0005,
	0x602A, 0x0710,
	0x6F12, 0x0004,
	0x6F12, 0x0401,
	0x6F12, 0x0100,
	0x602A, 0x1B5C,
	0x6F12, 0x0300,
	0x602A, 0x0786,
	0x6F12, 0x7701,
	0x602A, 0x2022,
	0x6F12, 0x0101,
	0x6F12, 0x0101,
	0x602A, 0x1360,
	0x6F12, 0x0000,
	0x602A, 0x1376,
	0x6F12, 0x0200,
	0x6F12, 0x6038,
	0x6F12, 0x7038,
	0x6F12, 0x8038,
	0x602A, 0x1386,
	0x6F12, 0x0B00,
	0x602A, 0x06FA,
	0x6F12, 0x0000,
	0x602A, 0x4A94,
	0x6F12, 0x0C00,
	0x6F12, 0x0900,
	0x6F12, 0x0600,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x6F12, 0x0600,
	0x6F12, 0x0900,
	0x6F12, 0x0C00,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x6F12, 0x0900,
	0x602A, 0x0A76,
	0x6F12, 0x1000,
	0x602A, 0x0AEE,
	0x6F12, 0x1000,
	0x602A, 0x0B66,
	0x6F12, 0x1000,
	0x602A, 0x0BDE,
	0x6F12, 0x1000,
	0x602A, 0x0BE8,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0C56,
	0x6F12, 0x1000,
	0x602A, 0x0C60,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0CB6,
	0x6F12, 0x0000,
	0x602A, 0x0CF2,
	0x6F12, 0x0001,
	0x602A, 0x0CF0,
	0x6F12, 0x0101,
	0x602A, 0x11B8,
	0x6F12, 0x0000,
	0x602A, 0x11F6,
	0x6F12, 0x0010,
	0x602A, 0x4A74,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xD8FF,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xD8FF,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x218E,
	0x6F12, 0x0000,
	0x602A, 0x2268,
	0x6F12, 0xF279,
	0x602A, 0x5006,
	0x6F12, 0x0000,
	0x602A, 0x500E,
	0x6F12, 0x0100,
	0x602A, 0x4E70,
	0x6F12, 0x2062,
	0x6F12, 0x5501,
	0x602A, 0x06DC,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0xF46A, 0xAE80,
	0x0344, 0x0000,
	0x0346, 0x0000,
	0x0348, 0x1FFF,
	0x034A, 0x181F,
	0x034C, 0x07F8,
	0x034E, 0x0600,
	0x0350, 0x0004,
	0x0352, 0x0004,
	0x0900, 0x0144,
	0x0380, 0x0002,
	0x0382, 0x0006,
	0x0384, 0x0002,
	0x0386, 0x0006,
	0x0110, 0x1002,
	0x0114, 0x0301,
	0x0116, 0x3000,
	0x0136, 0x1800,
	0x013E, 0x0000,
	0x0300, 0x0006,
	0x0302, 0x0001,
	0x0304, 0x0004,
	0x0306, 0x0096,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x00A5,
	0x0312, 0x0000,
	0x080E, 0x0000,
	0x0340, 0x1310,
	0x0342, 0x1000,
	0x0702, 0x0000,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x0D00, 0x0101,
	0x0D02, 0x0101,
	0x0D04, 0x0102,
	0x6226, 0x0000,
};

static kal_uint16 s5kjnssq33_custom1_setting[] = {
	0x6028, 0x2400,
	0x602A, 0x1A28,
	0x6F12, 0x4C00,
	0x602A, 0x065A,
	0x6F12, 0x0000,
	0x602A, 0x139E,
	0x6F12, 0x0300,
	0x602A, 0x139C,
	0x6F12, 0x0000,
	0x602A, 0x13A0,
	0x6F12, 0x0A00,
	0x6F12, 0x0020,
	0x602A, 0x2072,
	0x6F12, 0x0000,
	0x602A, 0x1A64,
	0x6F12, 0x0301,
	0x6F12, 0x3F00,
	0x602A, 0x19E6,
	0x6F12, 0x0201,
	0x602A, 0x1A30,
	0x6F12, 0x3401,
	0x602A, 0x19F4,
	0x6F12, 0x0606,
	0x602A, 0x19F8,
	0x6F12, 0x1010,
	0x602A, 0x1B26,
	0x6F12, 0x6F80,
	0x6F12, 0xA020,
	0x602A, 0x1A3C,
	0x6F12, 0x5207,
	0x602A, 0x1A48,
	0x6F12, 0x5207,
	0x602A, 0x1444,
	0x6F12, 0x2100,
	0x6F12, 0x2100,
	0x602A, 0x144C,
	0x6F12, 0x4200,
	0x6F12, 0x4200,
	0x602A, 0x7F6C,
	0x6F12, 0x0100,
	0x6F12, 0x3100,
	0x6F12, 0xF700,
	0x6F12, 0x2600,
	0x6F12, 0xE100,
	0x602A, 0x0650,
	0x6F12, 0x0600,
	0x602A, 0x0654,
	0x6F12, 0x0000,
	0x602A, 0x1A46,
	0x6F12, 0x8600,
	0x602A, 0x1A52,
	0x6F12, 0xBF00,
	0x602A, 0x0674,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x0668,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x602A, 0x0684,
	0x6F12, 0x4001,
	0x602A, 0x0688,
	0x6F12, 0x4001,
	0x602A, 0x147C,
	0x6F12, 0x1000,
	0x602A, 0x1480,
	0x6F12, 0x1000,
	0x602A, 0x19F6,
	0x6F12, 0x0904,
	0x602A, 0x0812,
	0x6F12, 0x0000,
	0x602A, 0x1A02,
	0x6F12, 0x0800,
	0x602A, 0x2104,
	0x6F12, 0x8E12,
	0x602A, 0x2148,
	0x6F12, 0x0100,
	0x602A, 0x2042,
	0x6F12, 0x1A00,
	0x602A, 0x0874,
	0x6F12, 0x1100,
	0x602A, 0x09C0,
	0x6F12, 0x9800,
	0x602A, 0x09C4,
	0x6F12, 0x9800,
	0x602A, 0x19FE,
	0x6F12, 0x0E1C,
	0x602A, 0x4D92,
	0x6F12, 0x0100,
	0x602A, 0x8A88,
	0x6F12, 0x0100,
	0x602A, 0x4D94,
	0x6F12, 0x4001,
	0x6F12, 0x0004,
	0x6F12, 0x0010,
	0x6F12, 0x0810,
	0x6F12, 0x0004,
	0x6F12, 0x0010,
	0x6F12, 0x0810,
	0x6F12, 0x0810,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0010,
	0x6F12, 0x0010,
	0x602A, 0x3570,
	0x6F12, 0x0000,
	0x602A, 0x3574,
	0x6F12, 0x3801,
	0x602A, 0x21E4,
	0x6F12, 0x0400,
	0x602A, 0x21EC,
	0x6F12, 0x6801,
	0x602A, 0x2080,
	0x6F12, 0x0100,
	0x6F12, 0x7F00,
	0x6F12, 0x0002,
	0x6F12, 0x8000,
	0x6F12, 0x0002,
	0x6F12, 0xC244,
	0x6F12, 0xD244,
	0x6F12, 0x14F4,
	0x6F12, 0x141C,
	0x6F12, 0x111C,
	0x6F12, 0x54F4,
	0x602A, 0x20BA,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x120E,
	0x6F12, 0x1000,
	0x602A, 0x212E,
	0x6F12, 0x0A00,
	0x602A, 0x13AE,
	0x6F12, 0x0102,
	0x602A, 0x0718,
	0x6F12, 0x0005,
	0x602A, 0x0710,
	0x6F12, 0x0004,
	0x6F12, 0x0401,
	0x6F12, 0x0100,
	0x602A, 0x1B5C,
	0x6F12, 0x0300,
	0x602A, 0x0786,
	0x6F12, 0x7701,
	0x602A, 0x2022,
	0x6F12, 0x0101,
	0x6F12, 0x0101,
	0x602A, 0x1360,
	0x6F12, 0x0000,
	0x602A, 0x1376,
	0x6F12, 0x0200,
	0x6F12, 0x6038,
	0x6F12, 0x7038,
	0x6F12, 0x8038,
	0x602A, 0x1386,
	0x6F12, 0x0B00,
	0x602A, 0x06FA,
	0x6F12, 0x1000,
	0x602A, 0x4A94,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x0A76,
	0x6F12, 0x1000,
	0x602A, 0x0AEE,
	0x6F12, 0x1000,
	0x602A, 0x0B66,
	0x6F12, 0x1000,
	0x602A, 0x0BDE,
	0x6F12, 0x1000,
	0x602A, 0x0BE8,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0C56,
	0x6F12, 0x1000,
	0x602A, 0x0C60,
	0x6F12, 0x3000,
	0x6F12, 0x3000,
	0x602A, 0x0CB6,
	0x6F12, 0x0000,
	0x602A, 0x0CF2,
	0x6F12, 0x0001,
	0x602A, 0x0CF0,
	0x6F12, 0x0101,
	0x602A, 0x11B8,
	0x6F12, 0x0000,
	0x602A, 0x11F6,
	0x6F12, 0x0010,
	0x602A, 0x4A74,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x218E,
	0x6F12, 0x0000,
	0x602A, 0x2268,
	0x6F12, 0xF279,
	0x602A, 0x5006,
	0x6F12, 0x0000,
	0x602A, 0x500E,
	0x6F12, 0x0100,
	0x602A, 0x4E70,
	0x6F12, 0x2062,
	0x6F12, 0x5501,
	0x602A, 0x06DC,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0xF46A, 0xAE80,
	0x0344, 0x00F0,
	0x0346, 0x0390,
	0x0348, 0x1F0F,
	0x034A, 0x148F,
	0x034C, 0x0780,
	0x034E, 0x0438,
	0x0350, 0x0004,
	0x0352, 0x0004,
	0x0900, 0x0144,
	0x0380, 0x0002,
	0x0382, 0x0006,
	0x0384, 0x0002,
	0x0386, 0x0006,
	0x0110, 0x1002,
	0x0114, 0x0300,
	0x0116, 0x3000,
	0x0136, 0x1800,
	0x013E, 0x0000,
	0x0300, 0x0006,
	0x0302, 0x0001,
	0x0304, 0x0004,
	0x0306, 0x0096,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0072,
	0x0312, 0x0000,
	0x080E, 0x0000,
	0x0340, 0x1010,
	0x0342, 0x0960,
	0x0702, 0x0000,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x0D00, 0x0101,
	0x0D02, 0x0101,
	0x0D04, 0x0102,
	0x6226, 0x0000,
};

static void sensor_init(void)
{
	pr_info("sensor_init start");
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0000, 0x0001);
	write_cmos_sensor(0x0000, 0x38EE);
	write_cmos_sensor(0x001E, 0x000B);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6010, 0x0001);
	mdelay(13);

	table_write_cmos_sensor(s5kjnssq33_init_setting,
		sizeof(s5kjnssq33_init_setting) / sizeof(kal_uint16));
	pr_info("sensor_init end");
}

static void preview_setting(void)
{
	LOG_INF("E\n");
	table_write_cmos_sensor(s5kjnssq33_preview_setting,
		sizeof(s5kjnssq33_preview_setting) / sizeof(kal_uint16));
}

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E\n");
	table_write_cmos_sensor(s5kjnssq33_cap_setting,
		sizeof(s5kjnssq33_cap_setting) / sizeof(kal_uint16));
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E\n");
	table_write_cmos_sensor(s5kjnssq33_normal_video_setting,
		sizeof(s5kjnssq33_normal_video_setting) / sizeof(kal_uint16));
}

static void hs_video_setting(void)
{
	LOG_INF("E\n");
	table_write_cmos_sensor(s5kjnssq33_hs_video_setting,
		sizeof(s5kjnssq33_hs_video_setting) / sizeof(kal_uint16));
}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	table_write_cmos_sensor(s5kjnssq33_slim_video_setting,
		sizeof(s5kjnssq33_slim_video_setting) / sizeof(kal_uint16));
}

static void custom1_setting(void)
{
	LOG_INF("E\n");
	table_write_cmos_sensor(s5kjnssq33_custom1_setting,
		sizeof(s5kjnssq33_custom1_setting) / sizeof(kal_uint16));
}

/*************************************************************************
 * FUNCTION
 *	get_imgsensor_id
 *
 * DESCRIPTION
 *	This function get the sensor ID
 *
 * PARAMETERS
 *	*sensorID : return the sensor ID
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	pr_info("get_imgsensor_id");

	sensor_init(); // Do sensor_init before get ID, or ID will have ramdonly mis-match

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));
			pr_info("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id ) {
				pr_info("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			pr_info("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

/*************************************************************************
 * FUNCTION
 *	open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
#define i2c_fixed
static kal_uint32 open(void)
{
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;
	kal_uint8 i = 0;
	LOG_INF("%s imgsensor.enable_secure %d\n",
		__func__, imgsensor.enable_secure);

	pr_info("open");

	/* initail sequence write in  */
	sensor_init(); // Do sensor_init before get ID, or ID will have ramdonly mis-match

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));
			if (sensor_id == imgsensor_info.sensor_id) {
				pr_info("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
				break;
			}
			pr_info("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}

	if (imgsensor_info.sensor_id != sensor_id){
		pr_err("Error: imgsensor_info.sensor_id : 0x%x is not match sensor id: 0x%x got from sensor\n", imgsensor_info.sensor_id , sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}		/* open */

/*************************************************************************
 * FUNCTION
 *	close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 close(void)
{
	MUINT32 ret = ERROR_NONE;

	return ret;
}		/* close */

/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	return ERROR_NONE;
}	/* s5kjnssq33MIPIPreview */

/*************************************************************************
 * FUNCTION
 *	capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}

	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);

	return ERROR_NONE;
}	/* capture() */

static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			       MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	return ERROR_NONE;
}	/* normal_video */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	return ERROR_NONE;
}	/* hs_video */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	return ERROR_NONE;
}	/* slim_video */

static kal_uint32 custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom1_setting();
	return ERROR_NONE;
}	/* custom1 */

static kal_uint32 get_resolution(
		MSDK_SENSOR_RESOLUTION_INFO_STRUCT * sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;

	sensor_resolution->SensorHighSpeedVideoWidth = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight = imgsensor_info.slim_video.grabwindow_height;

	sensor_resolution->SensorCustom1Width = imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height = imgsensor_info.custom1.grabwindow_height;

	return ERROR_NONE;
}	/*      get_resolution  */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	// inverse with datasheet
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;	/* not use */
	sensor_info->SensorResetActiveHigh = FALSE;	/* not use */
	sensor_info->SensorResetDelayCount = 5;	/* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting sensor gain */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame; //[Alex] Bokeh sw sync enable
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

	/* Use PDAF_SUPPORT_CAMSV, snesor output PD pixels via VC */
	sensor_info->PDAF_Support = 2;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3;	/* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2;	/* not use */
	sensor_info->SensorPixelClockCount = 3;	/* not use */
	sensor_info->SensorDataLatchCount = 2;	/* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;	// 0 is default 1x
	sensor_info->SensorHightSampling = 0;	// 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

		sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

		break;
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}	/* get_info */

static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		preview(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		capture(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		normal_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		hs_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		slim_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		custom1(image_window, sensor_config_data);
		break;
	default:
		LOG_INF("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control() */

static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)	//enable auto flicker
		imgsensor.autoflicker_en = KAL_TRUE;
	else		//Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frameHeight = 0;
    
	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	if (framerate == 0)
		return ERROR_NONE;
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frameHeight = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		if (frameHeight > imgsensor_info.pre.framelength)
		{
			imgsensor.dummy_line = frameHeight - imgsensor_info.pre.framelength;
		}
		else
		{
			imgsensor.dummy_line = 0;
		}
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) //[Alex] Bokeh sw sync enable
		{
		    set_dummy();
		}
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		frameHeight = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		if (frameHeight > imgsensor_info.normal_video.framelength)
		{
			imgsensor.dummy_line = frameHeight - imgsensor_info.normal_video.framelength;
		}
		else
		{
			imgsensor.dummy_line = 0;
		}
		imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) //[Alex] Bokeh sw sync enable
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps == imgsensor_info.cap.max_framerate)
		{
			frameHeight = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
		}
		spin_lock(&imgsensor_drv_lock);
		if (frameHeight > imgsensor_info.cap.framelength)
		{
			imgsensor.dummy_line = frameHeight - imgsensor_info.cap.framelength;
		}
		else
		{
			imgsensor.dummy_line = 0;
		}
		imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) //[Alex] Bokeh sw sync enable
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frameHeight = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		if (frameHeight > imgsensor_info.hs_video.framelength)
		{
			imgsensor.dummy_line = frameHeight - imgsensor_info.hs_video.framelength;
		}
		else
		{
			imgsensor.dummy_line = 0;
		}
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) //[Alex] Bokeh sw sync enable
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frameHeight = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		if (frameHeight > imgsensor_info.slim_video.framelength)
		{
			imgsensor.dummy_line = frameHeight - imgsensor_info.slim_video.framelength;
		}
		else
		{
			imgsensor.dummy_line = 0;
		}
		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) //[Alex] Bokeh sw sync enable
		    set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		frameHeight = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		if (frameHeight > imgsensor_info.custom1.framelength)
		{
		imgsensor.dummy_line = frameHeight - imgsensor_info.custom1.framelength;
		}
		else
		{
			imgsensor.dummy_line = 0;
		}
		imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) //[Alex] Bokeh sw sync enable
		    set_dummy();
		break;
	default:	//coding with preview scenario by default
		frameHeight = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		if (frameHeight > imgsensor_info.pre.framelength)
		{
			imgsensor.dummy_line = frameHeight - imgsensor_info.pre.framelength;
		}
		else
		{
			imgsensor.dummy_line = 0;
		}
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) //[Alex] Bokeh sw sync enable
		    set_dummy();
		LOG_INF("error scenario_id = %d, we use preview scenario\n", scenario_id);
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(
		enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
		break;
	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_uint32 modes, struct SET_SENSOR_PATTERN_SOLID_COLOR *pdata)
{
	LOG_INF("set_test_pattern modes: %d,\n", modes);

	if (modes == 2){
		LOG_INF("set_test_pattern modes: %d\n", modes);
		write_cmos_sensor(0x0600, 0x0002);
	} else if (modes == 5 && (pdata != NULL)) { //Solid Color
		LOG_INF("set_test_pattern modes: %d\n", modes);
		write_cmos_sensor(0x0600, 0x0001);
		write_cmos_sensor(0x0602, 0x0000);//set black
		write_cmos_sensor(0x0604, 0x0000);
		write_cmos_sensor(0x0606, 0x0000);
		write_cmos_sensor(0x0608, 0x0000);
	} else {
		write_cmos_sensor(0x0600, 0x0000);
	}
	spin_lock(&imgsensor_drv_lock);
	//imgsensor.test_pattern = enable;
	imgsensor.test_pattern = modes;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("final set_test_pattern modes: %d\n", modes);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				  UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
	  *(feature_data + 1) = imgsensor_info.min_gain;
	  *(feature_data + 2) = imgsensor_info.max_gain;
	  break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
	  *(feature_data + 0) = imgsensor_info.min_gain_iso;
	  *(feature_data + 1) = imgsensor_info.gain_step;
	  *(feature_data + 2) = imgsensor_info.gain_type;
	  break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
	  *(feature_data + 1) = imgsensor_info.min_shutter;
	  break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
	  switch (*(feature_data + 1)) {
	  case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
	  case MSDK_SCENARIO_ID_SLIM_VIDEO:
	  case MSDK_SCENARIO_ID_CUSTOM1:
		  *feature_return_para_32 = 4;  /*BINNING_AVERAGE*/
		  break;
	  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	  case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	  case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		  *feature_return_para_32 = 1;  /*mapping ISP ISO*/
		  break;
	  default:
		  *feature_return_para_32 = 2;  /*BINNING_AVERAGE*/
		  break;
	  }
	  pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
	       *feature_return_para_32);
	  *feature_para_len = 4;

	  break;
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
	  /*
	   * 1, if driver support new sw frame sync
	   * set_shutter_frame_length() support third para auto_extend_en
	  */
	  *(feature_data + 1) = 1;
	  /* margin info bu scenario */
	  *(feature_data + 2) = imgsensor_info.margin;
	  break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom1.framelength << 16)
				+ imgsensor_info.custom1.linelength;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		LOG_INF("SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO scenarioId:%d pclk:%d\n", (UINT32)*feature_data, *(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = imgsensor.pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.pclk;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.pclk;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(*feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode((BOOL) * feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor_8(sensor_reg_data->RegAddr,
				    sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		// get the lens driver ID from EEPROM or just
		// return LENS_DRIVER_ID_DO_NOT_CARE
		// if EEPROM does not exist in camera module.
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(*feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode((BOOL) (*feature_data_16),
				      *(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM)*feature_data,
			*(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
	        set_test_pattern_mode((UINT32)*feature_data, (struct SET_SENSOR_PATTERN_SOLID_COLOR *)(feature_data+1));
		break;
	case SENSOR_FEATURE_SET_AS_SECURE_DRIVER:
		spin_lock(&imgsensor_drv_lock);
		imgsensor.enable_secure = ((kal_bool) *feature_data);
		spin_unlock(&imgsensor_drv_lock);
		LOG_INF("imgsensor.enable_secure :%d\n",
			imgsensor.enable_secure);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		//for factory mode auto testing
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n", (UINT32) *feature_data);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data;
		spin_unlock(&imgsensor_drv_lock);
		break;
		// case SENSOR_FEATURE_SET_HDR:
		// LOG_INF("ihdr enable :%d\n", *feature_data_16);
		// spin_lock(&imgsensor_drv_lock);
		//         imgsensor.ihdr_en = *feature_data_16;
		// spin_unlock(&imgsensor_drv_lock);
		//         break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		/*
		 * LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
		 * *feature_data_32);
		 * wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)
		 * (*(feature_data_32+1));
		 */
		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)
			(*(feature_data + 1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[1],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[2],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[3],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[4],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[5],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[0],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16) *feature_data,
			(UINT16) *(feature_data + 1),
			(UINT16) *(feature_data + 2));
		//ihdr_write_shutter_gain((UINT16)*feature_data,
		//(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
		break;
        //[Alex] B: Bokeh sw sync enable
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:/*lzl*/
		LOG_INF("SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME\n");
		set_shutter_frame_length((UINT16)*feature_data,
					 (UINT16) *(feature_data + 1));
		break;
        //[Alex] E: Bokeh sw sync enable
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	 /* ******************* PDAF START ******** */
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		  LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n", *feature_data);
			/* PDAF capacity enable or not, 2p8 only full size support PDAF */
			switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
			case MSDK_SCENARIO_ID_CUSTOM1:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
			default:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			}
			break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n", *feature_data);
			PDAFinfo = (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,
					       sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info_video,
					       sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info_slim_video,
					       sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			case MSDK_SCENARIO_ID_CUSTOM1:
			default:
				  LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld not get PD_Info\n", *feature_data);
					break;
			}
			break;
	case SENSOR_FEATURE_GET_VC_INFO:
		 	LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
			pvcinfo = (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		 	switch (*feature_data_32) {
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy( (void *)pvcinfo, (void *)&SENSOR_VC_INFO[3],
				        sizeof(struct SENSOR_VC_INFO_STRUCT));
					break;
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy( (void *)pvcinfo, (void *)&SENSOR_VC_INFO[2],
				        sizeof(struct SENSOR_VC_INFO_STRUCT));
					break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy( (void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				        sizeof(struct SENSOR_VC_INFO_STRUCT));
					break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					memcpy( (void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
				        sizeof(struct SENSOR_VC_INFO_STRUCT));
					break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			case MSDK_SCENARIO_ID_CUSTOM1:
			default:
				  LOG_INF("SENSOR_FEATURE_GET_VC_INFO scenarioId:%lld not get VC_Info\n", *feature_data);
					break;
			}
			break;
	 /* ******************* PDAF END ******** */
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		kal_uint32 rate;

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			rate = imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			rate = imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			rate = imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			rate = imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			rate = imgsensor_info.custom1.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			rate = imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
		LOG_INF("SENSOR_FEATURE_GET_MIPI_PIXEL_RATE scenarioId:%d rate:%d\n", (UINT32)*feature_data, rate);
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
	}
	break;

	default:
		LOG_INF("feature_id: %d is not supported!!\n", feature_id);
		break;
	}

	return ERROR_NONE;
}

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5KJNSSQ33_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}				/*      s5kjnssq33_MIPI_RAW_SensorInit      */

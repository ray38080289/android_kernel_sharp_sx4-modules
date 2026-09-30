// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 s5k3l6mipiraw_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/


#define PFX "S5K3l6_camera_sensor"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__

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

#include "s5k3l6mipiraw_Sensor.h"

#define MULTI_WRITE 1
#if MULTI_WRITE
//static const int I2C_BUFFER_LEN = 1020; /*trans# max is 255, each 4 bytes*/
#define I2C_BUFFER_LEN (1020)
#else
//static const int I2C_BUFFER_LEN = 4;
#define I2C_BUFFER_LEN (4)
#endif

static int islowlight;

/*
 * #define LOG_INF(format, args...) pr_debug(
 * PFX "[%s] " format, __func__, ##args)
 */
static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K3L6_SENSOR_ID,
	.checksum_value = 0x143d0c73,

	.pre = {
		.pclk = 480000000,               //record different mode's pclk
		.linelength  = 4896,             //record different mode's linelength
		.framelength = 3260,             //record different mode's framelength
		.startx = 0,                     //record different mode's startx of grabwindow
		.starty = 0,                     //record different mode's starty of grabwindow
		.grabwindow_width  = 4208,       //record different mode's width of grabwindow
		.grabwindow_height = 3120,       //record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85, //following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario
		.max_framerate = 300,            //following for GetDefaultFramerateByScenario()
		.mipi_pixel_rate = 480000000,
	},
	.cap = {
		.pclk = 480000000,               //record different mode's pclk
		.linelength  = 4896,             //record different mode's linelength
		.framelength = 3260,             //record different mode's framelength
		.startx = 0,                     //record different mode's startx of grabwindow
		.starty = 0,                     //record different mode's starty of grabwindow
		.grabwindow_width  = 4208,       //record different mode's width of grabwindow
		.grabwindow_height = 3120,       //record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85, //following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario
		.max_framerate = 300,            //following for GetDefaultFramerateByScenario()
		.mipi_pixel_rate = 480000000,
	},
	.normal_video = {
		.pclk = 480000000,               //record different mode's pclk
		.linelength  = 4896,             //record different mode's linelength
		.framelength = 3260,             //record different mode's framelength
		.startx = 0,                     //record different mode's startx of grabwindow
		.starty = 0,                     //record different mode's starty of grabwindow
		.grabwindow_width  = 4208,       //record different mode's width of grabwindow
		.grabwindow_height = 2368,       //record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85, //following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario
		.max_framerate = 300,            //following for GetDefaultFramerateByScenario()
		.mipi_pixel_rate = 480000000,
	},
	.hs_video = {
		.pclk = 480000000,               //record different mode's pclk
		.linelength  = 4896,             //record different mode's linelength
		.framelength = 816,              //record different mode's framelength
		.startx = 0,                     //record different mode's startx of grabwindow
		.starty = 0,                     //record different mode's starty of grabwindow
		.grabwindow_width  = 1280,       //record different mode's width of grabwindow
		.grabwindow_height = 720,        //record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85, //following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario
		.max_framerate = 1200,           //following for GetDefaultFramerateByScenario()
		.mipi_pixel_rate = 14240000,
	},
	.slim_video = {
		.pclk = 480000000,               //record different mode's pclk
		.linelength  = 4896,             //record different mode's linelength
		.framelength = 3260,             //record different mode's framelength
		.startx = 0,                     //record different mode's startx of grabwindow
		.starty = 0,                     //record different mode's starty of grabwindow
		.grabwindow_width  = 1920,       //record different mode's width of grabwindow
		.grabwindow_height = 1080,       //record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85, //following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario
		.max_framerate = 300,            //following for GetDefaultFramerateByScenario()
		.mipi_pixel_rate = 208000000,
	},

	.custom1 = {
		.pclk = 480000000,               //record different mode's pclk
		.linelength  = 4896,             //record different mode's linelength
		.framelength = 3260,             //record different mode's framelength
		.startx = 0,                     //record different mode's startx of grabwindow
		.starty = 0,                     //record different mode's starty of grabwindow
		.grabwindow_width  = 4208,       //record different mode's width of grabwindow
		.grabwindow_height = 3120,       //record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85, //following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario
		.max_framerate = 300,            //following for GetDefaultFramerateByScenario()
		.mipi_pixel_rate = 480000000,
	},

	.min_gain = 64,                  // 1x gain
	.max_gain = 1024,                // 16x gain
	.min_gain_iso = 100,
	.margin = 4,                     //sensor framelength & shutter margin
	.gain_step = 1,
	.gain_type = 2,
	.min_shutter = 4,                //min shutter
	.max_frame_length = 0xffff,      //max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,        //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0, //sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,     //isp gain delay frame for AE cycle
	.frame_time_delay_frame = 1,     //The delay frame of setting frame length
	.ihdr_support = 0,               //1, support; 0,not support
	.ihdr_le_firstline = 0,          //1,le first ; 0, se first
	.sensor_mode_num = 6,            //support sensor mode num

	.cap_delay_frame = 2,            //enter capture delay frame num
	.pre_delay_frame = 1,            //enter preview delay frame num
	.video_delay_frame = 2,          //enter video delay frame num
	.hs_video_delay_frame = 3,       //enter high speed video  delay frame num
	.slim_video_delay_frame = 3,     //enter slim video delay frame num

#ifdef CONFIG_IMPROVE_CAMERA_PERFORMANCE
	.custom1_delay_frame = 1,
#else
	.custom1_delay_frame = 3,
#endif

	.isp_driving_current = ISP_DRIVING_6MA,                  //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,     //sensor_interface_type
	.mipi_sensor_type = MIPI_OPHY_NCSI2,                     //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = 0,                             //0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb, //sensor output first pixel color
	.mclk = 24,                                              //mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,                     //mipi lane num
	.i2c_speed = 400,                                        //support 1MHz write
	.i2c_addr_table = {0x20, 0xff},                          //record sensor support all write id addr, only supprt 4must end with 0xff
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_HV_MIRROR,                               //mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT,                      //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x200,                                        //current shutter
	.gain = 0x100,                                           //current gain
	.dummy_pixel = 0,                                        //current dummypixel
	.dummy_line = 0,                                         //current dummyline
	.current_fps = 300,                                      //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,                             //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,                               //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,  //current scenario id
	.ihdr_mode = KAL_FALSE,                                  //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x20,                                    //record current sensor's i2c write id
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT
	imgsensor_winsize_info[6] = {
	{ 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // Preview
	{ 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // capture
	{ 4208, 3120,	  0,  376, 4208, 2368, 4208, 2368,   0,	0, 4208, 2368, 	 0, 0, 4208, 2368}, // video
	{ 4208, 3120,	184,  480, 3840, 2160, 1280,  720,   0,	0, 1280,  720, 	 0, 0, 1280,  720}, // hight speed video
	{ 4208, 3120,	184,  480, 3840, 2160, 1920, 1080,   0,	0, 1920, 1080, 	 0, 0, 1920, 1080}, // slim video
	{ 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // custom1
};

/* add for s5k3l6 pdaf */
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX = 24,
	.i4OffsetY = 24,
	.i4PitchX = 64,
	.i4PitchY = 64,
	.i4PairNum = 16,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,

	// MTK recommend that PD info coordinates should be configured according to the module house ini file
	.i4PosL = { {31, 28}, {47, 32}, {67, 32}, {83, 28}, {35, 40}, {51, 44}, {63, 44}, {79, 40}, {35, 64}, {51, 60}, {63, 60}, {79, 64}, {31, 76}, {47, 72}, {67, 72}, {83, 76} },
	.i4PosR = { {31, 24}, {47, 28}, {67, 28}, {83, 24}, {35, 44}, {51, 48}, {63, 48}, {79, 44}, {35, 60}, {51, 56}, {63, 56}, {79, 60}, {31, 80}, {47, 76}, {67, 76}, {83, 80} },

	.i4BlockNumX = 65,
	.i4BlockNumY = 48,
	.i4LeFirst = 0,
	.i4Crop = { {0, 0}, {0, 0}, {0, 376}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	.iMirrorFlip = 0, //0:IMAGE_NORMAL, 1:IMAGE_H_MIRROR, 2:IMAGE_V_MIRROR, 3:IMAGE_HV_MIRROR
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_video =
{
	.i4OffsetX = 24,
	.i4OffsetY = 16,
	.i4PitchX = 64,
	.i4PitchY = 64,
	.i4PairNum = 16,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,

	// The coordinates of the PD point will be re-transformed after mirror flip, just fill in directly according to the INI file provided by the sensor vendor
	.i4PosL = { {28, 31}, {44, 35}, {64, 35}, {80, 31}, {32, 51}, {48, 55}, {60, 55}, {76, 51}, {32, 67}, {48, 63}, {60, 63}, {76, 67}, {28, 87}, {44, 83}, {64, 83}, {80, 87} },
	.i4PosR = { {28, 35}, {44, 39}, {64, 39}, {80, 35}, {32, 47}, {48, 51}, {60, 51}, {76, 47}, {32, 71}, {48, 67}, {60, 67}, {76, 71}, {28, 83}, {44, 79}, {64, 79}, {80, 83} },

	.i4BlockNumX = 65,
	.i4BlockNumY = 36,
	.i4LeFirst = 0,
	.i4Crop = { {0, 0}, {0, 0}, {0, 376}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	.iMirrorFlip = 3, //0:IMAGE_NORMAL, 1:IMAGE_H_MIRROR, 2:IMAGE_V_MIRROR, 3:IMAGE_HV_MIRROR
};

#if 0
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *) &get_byte, 2,
		    imgsensor.i2c_write_id);
	return ((get_byte << 8) & 0xff00) | ((get_byte >> 8) & 0x00ff);
}
#endif

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {
		(char)(addr >> 8), (char)(addr & 0xFF),
		(char)(para >> 8), (char)(para & 0xFF)
	};
	iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *) &get_byte, 1,
		    imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[3] = {
		(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)
	};

	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void read_otp_chip_id(void)
{
	kal_uint16 check_byte = 0;
	unsigned char chip_id[8];
	int i=0;

	write_cmos_sensor_8(0x0100, 0x01);  // Streaming ON
	mdelay(50);                         // Delay 50ms
	write_cmos_sensor_8(0x0A02, 0x00);  // Page 0 = chip id
	write_cmos_sensor_8(0x0A00, 0x01);  // enable
	mdelay(50);                         // Delay 50ms

	check_byte = read_cmos_sensor_8(0x0A01);
	if((check_byte & 0x01)==1){
	    for(i = 0; i < 8; i++) {
	        chip_id[i] = read_cmos_sensor_8(0xA04+i);
	        pr_err("[S5K3L6]chip_id[%d] 0x%02X\n", i, chip_id[i]);
	    }
	}
}

static void set_dummy(void)
{
	pr_debug("dummyline = %d, dummypixels = %d\n",
		 imgsensor.dummy_line, imgsensor.dummy_pixel);

	/* return; //for test */
	write_cmos_sensor(0x0340, imgsensor.frame_length);
	write_cmos_sensor(0x0342, imgsensor.line_length);
}				/*      set_dummy  */

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

		if ((I2C_BUFFER_LEN - tosend) < 4 || IDX == len
		    || addr != addr_last) {
			iBurstWriteReg_multi(
				puSendCmd, tosend, imgsensor.i2c_write_id, 4,
				imgsensor_info.i2c_speed);
			tosend = 0;
		}
#else
		iWriteRegI2CTiming(puSendCmd, 4,
				   imgsensor.i2c_write_id, imgsensor_info.i2c_speed);

		tosend = 0;

#endif
	}
	return 0;
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	pr_debug("framerate = %d, min framelength should enable %d\n", framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length)
		imgsensor.frame_length = frame_length;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

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

static void check_streamoff(void)
{
	unsigned int i = 0;
	int timeout = (10000 / imgsensor.current_fps) + 1;

	mdelay(3);
	for (i = 0; i < timeout; i++) {
		if (read_cmos_sensor_8(0x0005) != 0xFF)
			mdelay(1);
		else
			break;
	}
	pr_debug("%s exit!\n", __func__);
}

static kal_uint32 streaming_control(kal_bool enable)
{
	pr_debug("streaming_enable(0=Sw Standby,1=streaming): %d\n",
		 enable);

	if (enable) {
		//write_cmos_sensor(0x6214, 0x7970);
		write_cmos_sensor(0x0100, 0x0100);
	} else {
		//write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x0100, 0x0000);
		check_streamoff();
	}
	return ERROR_NONE;
}

static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;

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

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x0340, imgsensor.frame_length);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length);
		pr_debug("(else)imgsensor.frame_length = %d\n", imgsensor.frame_length);
	}
	/* Update Shutter*/
	write_cmos_sensor(0x0202, shutter);
	pr_debug("shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);
}				/*      write_shutter  */

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
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}				/*      set_shutter */

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 target_frame_length)
{

	//pr_debug("shutter: %d, target_frame_length: %d", shutter, target_frame_length);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.dummy_line = (target_frame_length >
				imgsensor_info.cap.framelength) ? (target_frame_length -
						imgsensor_info.cap.framelength) : 0;
	imgsensor.frame_length = imgsensor_info.cap.framelength +
				 imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	pr_debug("shutter: %d, frame_length: %d, cap.framelength: %d dummy_line: %d\n",
		 shutter, imgsensor.frame_length,
		 imgsensor_info.cap.framelength, imgsensor.dummy_line);
	spin_unlock(&imgsensor_drv_lock);

	//set_dummy();
	// frame_length will be set in set_shutter
	set_shutter(shutter);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;

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
	kal_uint16 reg_gain, max_gain = imgsensor_info.max_gain;

	if (gain == 1024)
		islowlight = 1;
	else
		islowlight = 0;

	if (gain < imgsensor_info.min_gain || gain > max_gain) {
		pr_debug("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > max_gain)
			gain = max_gain;
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	pr_debug("gain = %d , reg_gain = 0x%x, max_gain = 0x%x\n", gain, reg_gain, max_gain);

	write_cmos_sensor(0x0204, reg_gain);
	return gain;
}				/*      set_gain  */

static void set_mirror_flip(kal_uint8 image_mirror)
{
	kal_uint8 itemp;
	itemp = read_cmos_sensor_8(0x0101);
	pr_err("[FIH]Before*******image_mirror itemp= %d*******\n", itemp);
	if((image_mirror == IMAGE_HV_MIRROR) && (itemp == 0x03))
	{
		return;
	}

	switch (image_mirror) {

		case IMAGE_NORMAL:
			write_cmos_sensor(0x0100, 0x0100);
			break;

		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0100, 0x0102);
			break;

		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0100, 0x0101);
			break;

		case IMAGE_HV_MIRROR:
		{
			write_cmos_sensor(0x0100, 0x0103);
			//write_cmos_sensor(0x0101, 0x0300);
			itemp=66;
			itemp = read_cmos_sensor_8(0x0101);
			pr_err("[FIH]After*******image_mirror itemp= %d*******\n", itemp);
		}
			break;
	}
}

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
#if 0
static void night_mode(kal_bool enable)
{
	/*No Need to implement this function*/
}	/*	night_mode	*/
#endif

static kal_uint16 addr_data_pair_init_3l6[] = {
	0x3084, 0x1314,
	0x3266, 0x0001,
	0x3242, 0x2020,
	0x306A, 0x2F4C,
	0x306C, 0xCA01,
	0x307A, 0x0D20,
	0x309E, 0x002D,
	0x3072, 0x0013,
	0x3074, 0x0977,
	0x3076, 0x9411,
	0x3024, 0x0016,
	0x3070, 0x3D00,
	0x3002, 0x0E00,
	0x3006, 0x1000,
	0x300A, 0x0C00,
	0x3010, 0x0400,
	0x3018, 0xC500,
	0x303A, 0x0204,
	0x3452, 0x0001,
	0x3454, 0x0001,
	0x3456, 0x0001,
	0x3458, 0x0001,
	0x345a, 0x0002,
	0x345C, 0x0014,
	0x345E, 0x0002,
	0x3460, 0x0014,
	0x3464, 0x0006,
	0x3466, 0x0012,
	0x3468, 0x0012,
	0x346A, 0x0012,
	0x346C, 0x0012,
	0x346E, 0x0012,
	0x3470, 0x0012,
	0x3472, 0x0008,
	0x3474, 0x0004,
	0x3476, 0x0044,
	0x3478, 0x0004,
	0x347A, 0x0044,
	0x347E, 0x0006,
	0x3480, 0x0010,
	0x3482, 0x0010,
	0x3484, 0x0010,
	0x3486, 0x0010,
	0x3488, 0x0010,
	0x348A, 0x0010,
	0x348E, 0x000C,
	0x3490, 0x004C,
	0x3492, 0x000C,
	0x3494, 0x004C,
	0x3496, 0x0020,
	0x3498, 0x0006,
	0x349A, 0x0008,
	0x349C, 0x0008,
	0x349E, 0x0008,
	0x34A0, 0x0008,
	0x34A2, 0x0008,
	0x34A4, 0x0008,
	0x34A8, 0x001A,
	0x34AA, 0x002A,
	0x34AC, 0x001A,
	0x34AE, 0x002A,
	0x34B0, 0x0080,
	0x34B2, 0x0006,
	0x32A2, 0x0000,
	0x32A4, 0x0000,
	0x32A6, 0x0000,
	0x32A8, 0x0000,
	0x3066, 0x7E00,
	0x3004, 0x0800,
	0x3C08, 0xFFFF,
};

static void sensor_init(void)
{
	pr_debug("sensor_init() E\n");

	write_cmos_sensor(0x0100, 0x0000);
	write_cmos_sensor(0x0000, 0x0060);  //setfile version
	write_cmos_sensor(0x0000, 0x30C6);
	mdelay(3);
	write_cmos_sensor(0x0A02, 0x3400); //OTP page select(def.00h)

	table_write_cmos_sensor(addr_data_pair_init_3l6,
				sizeof(addr_data_pair_init_3l6) / sizeof(kal_uint16));

}				/*      sensor_init  */

static kal_uint16 addr_data_pair_pre_3l6[] = {
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x1070,
	0x034E, 0x0C30,
	0x0900, 0x0000,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0114, 0x0330,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0004,
	0x0306, 0x0078,
	0x3C1E, 0x0000,
	0x030C, 0x0003,
	0x030E, 0x004B,
	0x3C16, 0x0000,
	0x0300, 0x0006,
	0x0342, 0x1320,
	0x0340, 0x0CBC,
	0x38C4, 0x0009,
	0x38D8, 0x002A,
	0x38DA, 0x000A,
	0x38DC, 0x000B,
	0x38C2, 0x000A,
	0x38C0, 0x000F,
	0x38D6, 0x000A,
	0x38D4, 0x0009,
	0x38B0, 0x000F,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x04B0,
	0x380C, 0x0090,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8800,
	0x3238, 0x000C,
	0x314A, 0x5F00,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E42,
	0x32B2, 0x0006,
	0x32B4, 0x0006,
	0x32B6, 0x0006,
	0x32B8, 0x0006,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x303A, 0x0204,
	0x3034, 0x4B01,
	0x3036, 0x0029,
	0x3032, 0x4800,
	0x320E, 0x049E,
	0x3C1E, 0x0100,  //STREAM ON
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

static void preview_setting(void)
{
	pr_debug("preview_setting() E\n");

	table_write_cmos_sensor(addr_data_pair_pre_3l6,
				sizeof(addr_data_pair_pre_3l6) / sizeof(kal_uint16));
}				/*      preview_setting  */

static kal_uint16 addr_data_pair_cap_3l6[] = {
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x1070,
	0x034E, 0x0C30,
	0x0900, 0x0000,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0114, 0x0330,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0004,
	0x0306, 0x0078,
	0x3C1E, 0x0000,
	0x030C, 0x0003,
	0x030E, 0x004B,
	0x3C16, 0x0000,
	0x0300, 0x0006,
	0x0342, 0x1320,
	0x0340, 0x0CBC,
	0x38C4, 0x0009,
	0x38D8, 0x002A,
	0x38DA, 0x000A,
	0x38DC, 0x000B,
	0x38C2, 0x000A,
	0x38C0, 0x000F,
	0x38D6, 0x000A,
	0x38D4, 0x0009,
	0x38B0, 0x000F,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x04B0,
	0x380C, 0x0090,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8800,
	0x3238, 0x000C,
	0x314A, 0x5F00,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E42,
	0x32B2, 0x0006,
	0x32B4, 0x0006,
	0x32B6, 0x0006,
	0x32B8, 0x0006,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x303A, 0x0204,
	0x3034, 0x4B01,
	0x3036, 0x0029,
	0x3032, 0x4800,
	0x320E, 0x049E,
	0x3C1E, 0x0100,  //STREAM ON
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

static void capture_setting(kal_uint16 currefps)
{
	pr_debug("capture_setting() E! currefps:%d\n", currefps);

	table_write_cmos_sensor(addr_data_pair_cap_3l6,
				sizeof(addr_data_pair_cap_3l6) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_video_3l6[] = {
	0x0100, 0x0000,
	0x0344, 0x0008,
	0x0346, 0x0180,
	0x0348, 0x1077,
	0x034A, 0x0ABF,
	0x034C, 0x1070,
	0x034E, 0x0940,
	0x0900, 0x0000,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0114, 0x0330,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0004,
	0x0306, 0x0078,
	0x3C1E, 0x0000,
	0x030C, 0x0003,
	0x030E, 0x004B,
	0x3C16, 0x0000,
	0x0300, 0x0006,
	0x0342, 0x1320,
	0x0340, 0x0CBC,
	0x38C4, 0x0009,
	0x38D8, 0x002A,
	0x38DA, 0x000A,
	0x38DC, 0x000B,
	0x38C2, 0x000A,
	0x38C0, 0x000F,
	0x38D6, 0x000A,
	0x38D4, 0x0009,
	0x38B0, 0x000F,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x04B0,
	0x380C, 0x0090,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8800,
	0x3238, 0x000C,
	0x314A, 0x5F00,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E42,
	0x32B2, 0x0006,
	0x32B4, 0x0006,
	0x32B6, 0x0006,
	0x32B8, 0x0006,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x3C1E, 0x0100,
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

static void normal_video_setting(kal_uint16 currefps)
{
	pr_debug("normal_video_setting() E! currefps:%d\n", currefps);

	table_write_cmos_sensor(addr_data_pair_video_3l6,
				sizeof(addr_data_pair_video_3l6) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_hs_3l6[] = {
	0x0344, 0x0340,
	0x0346, 0x0350,
	0x0348, 0x0D3F,
	0x034A, 0x08EF,
	0x034C, 0x0500,
	0x034E, 0x02D0,
	0x0900, 0x0122,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0003,
	0x0114, 0x0330,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0004,
	0x0306, 0x0078,
	0x3C1E, 0x0000,
	0x030C, 0x0003,
	0x030E, 0x0059,
	0x3C16, 0x0002,
	0x0300, 0x0006,
	0x0342, 0x1320,
	0x0340, 0x0330,
	0x38C4, 0x0002,
	0x38D8, 0x0008,
	0x38DA, 0x0003,
	0x38DC, 0x0004,
	0x38C2, 0x0003,
	0x38C0, 0x0001,
	0x38D6, 0x0003,
	0x38D4, 0x0002,
	0x38B0, 0x0004,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x0162,
	0x380C, 0x003D,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8000,
	0x3238, 0x000B,
	0x314A, 0x5F02,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46,
	0x32B2, 0x0008,
	0x32B4, 0x0008,
	0x32B6, 0x0008,
	0x32B8, 0x0008,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0024,
	0x393E, 0x4000,
	0x303A, 0x0204,
	0x3034, 0x4B01,
	0x3036, 0x0029,
	0x3032, 0x4800,
	0x320E, 0x049E,
};

static void hs_video_setting(void)
{
	pr_debug("hs_video_setting() E\n");


	table_write_cmos_sensor(addr_data_pair_hs_3l6,
				sizeof(addr_data_pair_hs_3l6) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_slim_3l6[] = {
	0x0344, 0x00C0,
	0x0346, 0x01E8,
	0x0348, 0x0FBF,
	0x034A, 0x0A57,
	0x034C, 0x0780,
	0x034E, 0x0438,
	0x0900, 0x0122,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0003,
	0x0114, 0x0330,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0004,
	0x0306, 0x0078,
	0x3C1E, 0x0000,
	0x030C, 0x0003,
	0x030E, 0x0082,
	0x3C16, 0x0002,
	0x0300, 0x0006,
	0x0342, 0x1320,
	0x0340, 0x0CBC,
	0x38C4, 0x0004,
	0x38D8, 0x000F,
	0x38DA, 0x0005,
	0x38DC, 0x0005,
	0x38C2, 0x0004,
	0x38C0, 0x0003,
	0x38D6, 0x0004,
	0x38D4, 0x0003,
	0x38B0, 0x0006,
	0x3932, 0x2000,
	0x3938, 0x000C,
	0x0820, 0x0208,
	0x380C, 0x0049,
	0x3064, 0xEFCF,
	0x309C, 0x0640,
	0x3090, 0x8000,
	0x3238, 0x000B,
	0x314A, 0x5F02,
	0x32B2, 0x0003,
	0x32B4, 0x0003,
	0x32B6, 0x0003,
	0x32B8, 0x0003,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46,
	0x32B2, 0x0008,
	0x32B4, 0x0008,
	0x32B6, 0x0008,
	0x32B8, 0x0008,
	0x3C34, 0x0008,
	0x3C36, 0x0000,
	0x3C38, 0x0000,
	0x393E, 0x4000,
};

static void slim_video_setting(void)
{
	pr_debug("slim_video_setting() E\n");

	table_write_cmos_sensor(addr_data_pair_slim_3l6,
				sizeof(addr_data_pair_slim_3l6) / sizeof(kal_uint16));
}

static kal_uint16 addr_data_pair_custom1_3l6[] = {
	0x0344, 0x0008,
	0x0346, 0x0008,
	0x0348, 0x1077,
	0x034A, 0x0C37,
	0x034C, 0x1070,
	0x034E, 0x0C30,
	0x0900, 0x0000,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0001,
	0x0114, 0x0330,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0004,
	0x0306, 0x0078,
	0x3C1E, 0x0000,
	0x030C, 0x0003,
	0x030E, 0x004B,
	0x3C16, 0x0000,
	0x0300, 0x0006,
	0x0342, 0x1320,
	0x0340, 0x0CBC,
	0x38C4, 0x0009,
	0x38D8, 0x002A,
	0x38DA, 0x000A,
	0x38DC, 0x000B,
	0x38C2, 0x000A,
	0x38C0, 0x000F,
	0x38D6, 0x000A,
	0x38D4, 0x0009,
	0x38B0, 0x000F,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x04B0,
	0x380C, 0x0090,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8800,
	0x3238, 0x000C,
	0x314A, 0x5F00,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E42,
	0x32B2, 0x0006,
	0x32B4, 0x0006,
	0x32B6, 0x0006,
	0x32B8, 0x0006,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x303A, 0x0204,
	0x3034, 0x4B01,
	0x3036, 0x0029,
	0x3032, 0x4800,
	0x320E, 0x049E,
	0x3C1E, 0x0100,  //STREAM ON
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

static void custom1_setting(void)
{
	pr_debug("custom1_setting() use custom1\n");

	table_write_cmos_sensor(addr_data_pair_custom1_3l6,
				sizeof(addr_data_pair_custom1_3l6) / sizeof(kal_uint16));
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

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));
			if (*sensor_id == imgsensor_info.sensor_id) {
				pr_debug("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			pr_debug("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
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
static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;

	pr_debug("%s", __func__);

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = (
					    (read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));

			if (sensor_id == imgsensor_info.sensor_id) {
				pr_debug("i2c write id: 0x%x, sensor id: 0x%x\n",
					 imgsensor.i2c_write_id, sensor_id);
				break;
			}

			pr_debug("Read sensor id fail, id: 0x%x\n",
				 imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in  */
	sensor_init();

	/* get otp chip id */
	read_otp_chip_id();

	//fih mirror flip NG
	//set_mirror_flip(imgsensor.mirror);

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
	imgsensor.ihdr_mode = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}				/*      open  */

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
	pr_debug("E\n");

	/*No Need to implement this function */

	return ERROR_NONE;
}				/*      close  */

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
	pr_debug("preview E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	preview_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}				/*      preview   */

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
	pr_debug("capture E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) {
		/* PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M */
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}				/* capture() */

static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("normal_video E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length =
		imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}				/*      normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("hs_video E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}				/*      hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("slim_video E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length =
		imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}				/*      slim_video       */

static kal_uint32 custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("E cur fps: %d\n", imgsensor.current_fps);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom1_setting();
	set_mirror_flip(imgsensor.mirror);
	//mdelay(10);

	return ERROR_NONE;
}   /*  custom1   */

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT(*sensor_resolution))
{
	pr_debug("get_resolution E\n");
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
}				/*      get_resolution  */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("get_info -> scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */

	/* inverse with datasheet */
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

	/* The frame of setting sensor gain*/
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;

	/*  The delay frame of setting frame length   */
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame;

	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

	/* 0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode(Full), 3:PDAF VC mode(Binning) */
	sensor_info->PDAF_Support = PDAF_SUPPORT_RAW;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3;	/* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2;	/* not use */
	sensor_info->SensorPixelClockCount = 3;	/* not use */
	sensor_info->SensorDataLatchCount = 2;	/* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

		sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

		break;

	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

		break;

	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}				/*      get_info  */

static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("scenario_id = %d\n", scenario_id);
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
		custom1(image_window, sensor_config_data); // custom1
		break;
	default:
		pr_debug("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control() */

static kal_uint32 set_video_mode(UINT16 framerate)
{
	/* //pr_debug("framerate = %d\n ", framerate); */
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150)
		 && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(
	kal_bool enable, UINT16 framerate)
{
	pr_debug("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)		/* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else			/* Cancel Auto flick */
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id,	MUINT32 framerate)
{
	kal_uint32 frame_length;

	pr_debug("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);

		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength) : 0;

		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.normal_video.framelength)
			? (frame_length - imgsensor_info.normal_video.  framelength) : 0;

		imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);

		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap.framelength)
			? (frame_length - imgsensor_info.cap.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength)
			? (frame_length - imgsensor_info.hs_video.  framelength) : 0;

		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength)
			? (frame_length - imgsensor_info.slim_video.  framelength) : 0;

		imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom1.framelength)
			? (frame_length - imgsensor_info.custom1.framelength) : 0;
		if (imgsensor.dummy_line < 0)
			imgsensor.dummy_line = 0;
		imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	default:		/* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength) : 0;

		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		pr_debug("error scenario_id = %d, we use preview scenario\n",
			 scenario_id);
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	pr_debug("scenario_id = %d\n", scenario_id);

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
		pr_debug("custom1 *framerate = %d\n", *framerate);
		break;

	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_uint32 modes,
	struct SET_SENSOR_PATTERN_SOLID_COLOR *pdata)
{
	kal_uint16 Color_R, Color_Gr, Color_Gb, Color_B;

	pr_debug("set_test_pattern enum: %d\n", modes);

	if (modes == 1 && (pdata != NULL)) { //Solid Color
		write_cmos_sensor(0x3202, 0x0080);
		write_cmos_sensor(0x3204, 0x0080);
		write_cmos_sensor(0x3206, 0x0080);
		write_cmos_sensor(0x3208, 0x0080);
		write_cmos_sensor(0x3232, 0x0000);
		write_cmos_sensor(0x3234, 0x0000);
		write_cmos_sensor(0x32A0, 0x0100);
		write_cmos_sensor(0x3300, 0x0001);
		write_cmos_sensor(0x3400, 0x0001);
		write_cmos_sensor(0x3402, 0x4E00);
		write_cmos_sensor(0x3268, 0x0000);

		write_cmos_sensor_8(0x0601, modes); //1:solid color from 3L6 datasheet
		pr_debug("Solid color : R=0x%x,Gr=0x%x,B=0x%x,Gb=0x%x",
			pdata->COLOR_R, pdata->COLOR_Gr, pdata->COLOR_B, pdata->COLOR_Gb);
		Color_R = (pdata->COLOR_R >> 22) & 0x3FF; //10bits depth color
		Color_Gr = (pdata->COLOR_Gr >> 22) & 0x3FF;
		Color_B = (pdata->COLOR_B >> 22) & 0x3FF;
		Color_Gb = (pdata->COLOR_Gb >> 22) & 0x3FF;
		//write_cmos_sensor(0x0602, (Color_R >> 8) & 0x2);
		pr_debug("write : R=0x%x,Gr=0x%x,B=0x%x,Gb=0x%x",Color_R, Color_Gr, Color_B, Color_Gb);
		//due to pass ITS,we change R and B
		write_cmos_sensor_8(0x0602, (Color_B >> 8) & 0x3);
		pr_debug("write 03: R=0x%x",((Color_B >> 8) & 0x3));
		write_cmos_sensor_8(0x0603, Color_B & 0xFF);
		pr_debug("write ff: R=0x%x",(Color_B & 0xFF));
		write_cmos_sensor_8(0x0604, (Color_Gr >> 8) & 0x3);
		pr_debug("write 03: Gr=0x%x",((Color_Gr >> 8) & 0x3));
		write_cmos_sensor_8(0x0605, Color_Gr & 0xFF);
		pr_debug("write ff: Gr=0x%x",(Color_Gr & 0xFF));
		write_cmos_sensor_8(0x0606, (Color_R >> 8) & 0x3);
		pr_debug("write 03: B=0x%x",((Color_R >> 8) & 0x3));
		write_cmos_sensor_8(0x0607, Color_R & 0xFF);
		pr_debug("write ff: B=0x%x",(Color_R & 0xFF));
		write_cmos_sensor_8(0x0608, (Color_Gb >> 8) & 0x3);
		pr_debug("write 03: Gb=0x%x",((Color_Gb >> 8) & 0x3));
		write_cmos_sensor_8(0x0609, Color_Gb & 0xFF);
		pr_debug("write ff: Gb=0x%x",(Color_Gb & 0xFF));
	} else if (modes == 2) { //Color bars
		pr_debug("Color bars");
		write_cmos_sensor_8(0x0601, modes); //2:color bars from 3L6 datasheet*/
	} else if ( modes == 5) { //Black color
		write_cmos_sensor_8(0x0601, 0x01); //1:solid color from 3L6 datasheet
		//ITS send 0xffff(white) in RGB, but ITS check 0x00(black)
		//this why to change RGB to 0
		pr_debug("BLACK pdata : R=0x%x,Gr=0x%x,B=0x%x,Gb=0x%x",
			pdata->COLOR_R, pdata->COLOR_Gr, pdata->COLOR_B, pdata->COLOR_Gb);
		Color_R = 0;
		Color_Gr = 0;
		Color_B = 0;
		Color_Gb = 0;
		pr_debug("Force set BLACK pattern: R=0x%x,Gr=0x%x,B=0x%x,Gb=0x%x",Color_R, Color_Gr, Color_B, Color_Gb);
		write_cmos_sensor_8(0x0602, Color_R);
		write_cmos_sensor_8(0x0603, Color_R);
		write_cmos_sensor_8(0x0604, Color_Gr);
		write_cmos_sensor_8(0x0605, Color_Gr);
		write_cmos_sensor_8(0x0606, Color_B);
		write_cmos_sensor_8(0x0607, Color_B);
		write_cmos_sensor_8(0x0608, Color_Gb);
		write_cmos_sensor_8(0x0609, Color_Gb);
	} else {
		pr_debug("No pattern");
		write_cmos_sensor(0x3202, 0x0081);
		write_cmos_sensor(0x3204, 0x0081);
		write_cmos_sensor(0x3206, 0x0081);
		write_cmos_sensor(0x3208, 0x0081);
		write_cmos_sensor(0x3232, 0x0040);
		write_cmos_sensor(0x3234, 0x0100);
		write_cmos_sensor(0x32A0, 0x0000);
		write_cmos_sensor(0x3300, 0x0000);
		write_cmos_sensor(0x3400, 0x0000);
		write_cmos_sensor(0x3402, 0x4E42);
		write_cmos_sensor(0x3268, 0x0012);
		write_cmos_sensor_8(0x0601, 0x0000); /*No pattern*/
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = modes;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(void)
{
	UINT8 temperature;
	INT32 temperature_convert;

	temperature = read_cmos_sensor_8(0x013a);

	if (temperature >= 0x0 && temperature <= 0x78)
		temperature_convert = temperature;
	else
		temperature_convert = -1;

	/*pr_info("temp_c(%d), read_reg(%d), enable %d\n",
	 *	temperature_convert, temperature, read_cmos_sensor_8(0x0138));
	 */

	return temperature_convert;
}

static void get_af_max_num_focus_areas(UINT32 *pFeatureReturnPara32)
{
	*pFeatureReturnPara32 = 1;
	/* 0 is means it is not support touch af,1 is means it is support touch af */
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				  UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	pr_debug("feature_id = %d\n", feature_id);
	//move this to check mirror flip again. mirror flip can't set successfully
	//	set_mirror_flip(imgsensor.mirror);
	if(feature_id == SENSOR_FEATURE_GET_CROP_INFO){
		pr_debug("[FIH_set_mirror]feature_id = SENSOR_FEATURE_GET_CROP_INFO\n");
		set_mirror_flip(imgsensor.mirror);
	}

	switch (feature_id) {
	case SENSOR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
	    *(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 1100000;
	break;
	case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
		get_af_max_num_focus_areas(feature_data_32);
		*feature_para_len = 4;
		break;
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
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CUSTOM1:
			*feature_return_para_32 = 1;  /*BINNING_NONE*/
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		default:
			*feature_return_para_32 = 2;  /*BINNING_AVERAGE*/
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n", *feature_return_para_32);
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
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.pre.framelength << 16)
					+ imgsensor_info.pre.linelength;
				break;
			}
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
		/* night_mode((BOOL) *feature_data); no need to implement this mode */
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
			read_cmos_sensor_8(sensor_reg_data->RegAddr);
		break;

	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM or
		 * just return LENS_DRIVER_ID_DO_NOT_CARE
		 */
		/* if EEPROM does not exist in camera module. */
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
			(enum MSDK_SCENARIO_ID_ENUM) *feature_data, *(feature_data + 1));
		break;

	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM) *(feature_data),
			(MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((UINT32)*feature_data,
		(struct SET_SENSOR_PATTERN_SOLID_COLOR *)(uintptr_t)(*(feature_data + 1)));
		break;

	/* for factory mode auto testing */
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		pr_debug("current fps :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = (UINT16) *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		pr_debug("ihdr enable :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_mode = (UINT8) *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;

	case SENSOR_FEATURE_GET_CROP_INFO:
		pr_debug("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			 (UINT32) *feature_data);

		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *) (uintptr_t)
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
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[5],
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
		pr_debug("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			 (UINT16) *feature_data,
			 (UINT16) *(feature_data + 1),
			 (UINT16) *(feature_data + 2));

		/* ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),
		 * (UINT16)*(feature_data+2));
		 */
		break;
	case SENSOR_FEATURE_SET_AWB_GAIN:
		break;
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		pr_debug("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
			 (UINT16) *feature_data,
			 (UINT16) *(feature_data + 1));
		/* ihdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1)); */
		break;

	/******************** PDAF START >>> *********/
	case SENSOR_FEATURE_GET_PDAF_INFO:
		pr_debug("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n", (UINT16)*feature_data);
		PDAFinfo = (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data + 1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			memcpy( (void *)PDAFinfo, (void *)&imgsensor_pd_info,
					sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy( (void *)PDAFinfo, (void *)&imgsensor_pd_info_video,
					sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		pr_debug("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n", (UINT16)*feature_data);
		/* PDAF capacity enable or not */
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 1;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 1;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:	/*get cal data from eeprom*/
		pr_debug("SENSOR_FEATURE_GET_PDAF_DATA\n");
		//read_3L6_eeprom((kal_uint16)(*feature_data),(char *)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
		//pr_debug("SENSOR_FEATURE_GET_PDAF_DATA success\n");
		break;
	case SENSOR_FEATURE_SET_PDAF:
		pr_debug("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
		break;

	/******************** PDAF END   <<< *********/

	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:/*lzl*/
		pr_debug("SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME\n");
		set_shutter_frame_length((UINT16)*feature_data,
					 (UINT16) *(feature_data + 1));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		pr_debug("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		pr_debug("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			 *feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature();
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_GET_PIXEL_RATE:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				(imgsensor_info.cap.pclk /
				 (imgsensor_info.cap.linelength - 80)) *
				imgsensor_info.cap.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				(imgsensor_info.normal_video.pclk /
				 (imgsensor_info.normal_video.linelength - 80)) *
				imgsensor_info.normal_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				(imgsensor_info.hs_video.pclk /
				 (imgsensor_info.hs_video.linelength - 80)) *
				imgsensor_info.hs_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				(imgsensor_info.slim_video.pclk /
				 (imgsensor_info.slim_video.linelength - 80)) *
				imgsensor_info.slim_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				(imgsensor_info.custom1.pclk /
				 (imgsensor_info.custom1.linelength - 80)) *
				imgsensor_info.custom1.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				(imgsensor_info.pre.pclk /
				 (imgsensor_info.pre.linelength - 80)) *
				imgsensor_info.pre.grabwindow_width;
			break;
		}
		break;

	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.custom1.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_Y_AVERAGE:
		*feature_return_para_32 = islowlight;
		break;
	default:
		break;
	}

	return ERROR_NONE;
}				/*      feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K3L6_MIPI_RAW_SensorInit(
	struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}

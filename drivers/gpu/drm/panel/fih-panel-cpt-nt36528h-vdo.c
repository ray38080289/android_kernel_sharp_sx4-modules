// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/backlight.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_modes.h>
#include <linux/delay.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
#include "../mediatek/mediatek_v2/mtk_corner_pattern/mtk_data_hw_roundedpattern.h"
#endif

#include <linux/proc_fs.h>
#include <linux/poll.h>
#define FIH_PROC_DIR_LCM0 "AllHWList/LCM0"
#define FIH_PROC_PATH_BRIDGE_PING "ping"

static DECLARE_WAIT_QUEUE_HEAD(log_wait);
int bl_onoff = 1;
int old_bl_onoff = 0;
static __poll_t fih_ping_poll(struct file *file, poll_table *wait)
{
    pr_debug("[Kernel/LCM]%s, %d: <-- START\n", __func__, __LINE__);

    poll_wait(file, &log_wait, wait);

    if (old_bl_onoff != bl_onoff)
    {
        pr_debug("[Kernel/LCM]%s, %d: old_bl_onoff(%d) != bl_onoff(%d), return POLLPRI\n", __func__, __LINE__, old_bl_onoff, bl_onoff);
        old_bl_onoff = bl_onoff;
        return POLLPRI;
    }
    else
    {
        //pr_err("[HL]%s, %d: old_bl_onoff == bl_onoff, return POLLIN\n", __func__, __LINE__);
        return POLLIN;
    }
}

static ssize_t fih_ping_write_proc(struct file *file, const char __user *buffer,
                    size_t count, loff_t *offp)
{
    pr_debug("[Kernel/LCM]%s, %d\n", __func__, __LINE__);

    return 1;
}

static int fih_ping_read_proc(struct seq_file *m, void *v)
{
    pr_debug("[Kernel/LCM]%s, %d: bl_onoff = %d <-- START\n", __func__, __LINE__, bl_onoff);

    seq_printf(m, "%d\n", bl_onoff);

    pr_debug("[Kernel/LCM]%s, %d: <-- END\n", __func__, __LINE__);

    return 0;
}

static int fih_ping_proc_open(struct inode *inode, struct file *file)
{
  return single_open(file, fih_ping_read_proc, NULL);
}

static const struct proc_ops ping_file_ops = {
  .proc_open     = fih_ping_proc_open,
  .proc_read     = seq_read,
  .proc_write    = fih_ping_write_proc,
  .proc_lseek   = seq_lseek,
  .proc_release  = seq_release,
  .proc_poll     = fih_ping_poll,
};
extern int fih_aw37501_write_bytes(unsigned char addr, unsigned char value);
extern void aw99703_init_reg(void);

/* ----------------------------------------------------------------- */
/* LCM */
/* ----------------------------------------------------------------- */
struct lcm {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bias_pos, *bias_neg;

	bool prepared;
	bool enabled;

	int error;

	struct gpio_desc *lcm_enable_gpio;
	struct gpio_desc *pm_enable_gpio;
};

static int current_fps = 60;

#define lcm_dcs_write_seq(ctx, seq...) \
({\
	const u8 d[] = { seq };\
	BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64, "DCS sequence too big for stack");\
	lcm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

#define lcm_dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	lcm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static inline struct lcm *panel_to_lcm(struct drm_panel *panel)
{
	return container_of(panel, struct lcm, panel);
}

static void lcm_dcs_write(struct lcm *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;
	char *addr;

	if (ctx->error < 0)
		return;

	addr = (char *)data;
	if ((int)*addr < 0xB0)
		ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	else
		ret = mipi_dsi_generic_write(dsi, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %zd writing seq: %ph\n", ret, data);
		ctx->error = ret;
	}
}

#ifdef PANEL_SUPPORT_READBACK
static int lcm_dcs_read(struct lcm *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return 0;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

static void lcm_panel_get_data(struct lcm *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = lcm_dcs_read(ctx,  0x0A, buffer, 1);
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void lcm_panel_init(struct lcm *ctx)
{
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	lcm_dcs_write_seq_static(ctx, 0xFF,0x20);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x01,0x55);
	lcm_dcs_write_seq_static(ctx, 0x03,0x55);
	lcm_dcs_write_seq_static(ctx, 0x05,0xAB);
	lcm_dcs_write_seq_static(ctx, 0x06,0xC0);
	lcm_dcs_write_seq_static(ctx, 0x07,0xC8);
	lcm_dcs_write_seq_static(ctx, 0x08,0x78);
	lcm_dcs_write_seq_static(ctx, 0x0D,0x80);
	lcm_dcs_write_seq_static(ctx, 0x1F,0x55);
	lcm_dcs_write_seq_static(ctx, 0x94,0x00);
	lcm_dcs_write_seq_static(ctx, 0x95,0xD7);
	lcm_dcs_write_seq_static(ctx, 0x96,0xD7);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x23);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x11,0x00);
	lcm_dcs_write_seq_static(ctx, 0x12,0xB5);
	lcm_dcs_write_seq_static(ctx, 0x15,0xE7);
	lcm_dcs_write_seq_static(ctx, 0x16,0x14);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x24);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x00,0x00);
	lcm_dcs_write_seq_static(ctx, 0x01,0x08);
	lcm_dcs_write_seq_static(ctx, 0x02,0x09);
	lcm_dcs_write_seq_static(ctx, 0x03,0x00);
	lcm_dcs_write_seq_static(ctx, 0x04,0x00);
	lcm_dcs_write_seq_static(ctx, 0x05,0x00);
	lcm_dcs_write_seq_static(ctx, 0x06,0x00);
	lcm_dcs_write_seq_static(ctx, 0x07,0x00);
	lcm_dcs_write_seq_static(ctx, 0x08,0x22);
	lcm_dcs_write_seq_static(ctx, 0x09,0x23);
	lcm_dcs_write_seq_static(ctx, 0x0A,0x26);
	lcm_dcs_write_seq_static(ctx, 0x0B,0x06);
	lcm_dcs_write_seq_static(ctx, 0x0C,0x13);
	lcm_dcs_write_seq_static(ctx, 0x0D,0x12);
	lcm_dcs_write_seq_static(ctx, 0x0E,0x11);
	lcm_dcs_write_seq_static(ctx, 0x0F,0x10);
	lcm_dcs_write_seq_static(ctx, 0x10,0x0F);
	lcm_dcs_write_seq_static(ctx, 0x11,0x0E);
	lcm_dcs_write_seq_static(ctx, 0x12,0x0D);
	lcm_dcs_write_seq_static(ctx, 0x13,0x0C);
	lcm_dcs_write_seq_static(ctx, 0x14,0x05);
	lcm_dcs_write_seq_static(ctx, 0x15,0x04);
	lcm_dcs_write_seq_static(ctx, 0x16,0x00);
	lcm_dcs_write_seq_static(ctx, 0x17,0x08);
	lcm_dcs_write_seq_static(ctx, 0x18,0x09);
	lcm_dcs_write_seq_static(ctx, 0x19,0x00);
	lcm_dcs_write_seq_static(ctx, 0x1A,0x00);
	lcm_dcs_write_seq_static(ctx, 0x1B,0x00);
	lcm_dcs_write_seq_static(ctx, 0x1C,0x00);
	lcm_dcs_write_seq_static(ctx, 0x1D,0x00);
	lcm_dcs_write_seq_static(ctx, 0x1E,0x22);
	lcm_dcs_write_seq_static(ctx, 0x1F,0x23);
	lcm_dcs_write_seq_static(ctx, 0x20,0x26);
	lcm_dcs_write_seq_static(ctx, 0x21,0x06);
	lcm_dcs_write_seq_static(ctx, 0x22,0x13);
	lcm_dcs_write_seq_static(ctx, 0x23,0x12);
	lcm_dcs_write_seq_static(ctx, 0x24,0x11);
	lcm_dcs_write_seq_static(ctx, 0x25,0x10);
	lcm_dcs_write_seq_static(ctx, 0x26,0x0F);
	lcm_dcs_write_seq_static(ctx, 0x27,0x0E);
	lcm_dcs_write_seq_static(ctx, 0x28,0x0D);
	lcm_dcs_write_seq_static(ctx, 0x29,0x0C);
	lcm_dcs_write_seq_static(ctx, 0x2A,0x05);
	lcm_dcs_write_seq_static(ctx, 0x2B,0x04);
	lcm_dcs_write_seq_static(ctx, 0x2F,0x0A);
	lcm_dcs_write_seq_static(ctx, 0x30,0x06);
	lcm_dcs_write_seq_static(ctx, 0x31,0x4C);
	lcm_dcs_write_seq_static(ctx, 0x33,0x30);
	lcm_dcs_write_seq_static(ctx, 0x34,0x32);
	lcm_dcs_write_seq_static(ctx, 0x35,0x44);
	lcm_dcs_write_seq_static(ctx, 0x37,0xAA);
	lcm_dcs_write_seq_static(ctx, 0x38,0xA4);
	lcm_dcs_write_seq_static(ctx, 0x39,0x00);
	lcm_dcs_write_seq_static(ctx, 0x3A,0xB2);
	lcm_dcs_write_seq_static(ctx, 0x3B,0xB0);
	lcm_dcs_write_seq_static(ctx, 0x3D,0x53);
	lcm_dcs_write_seq_static(ctx, 0x3F,0x45);
	lcm_dcs_write_seq_static(ctx, 0x40,0x47);
	lcm_dcs_write_seq_static(ctx, 0x43,0x0E);
	lcm_dcs_write_seq_static(ctx, 0x44,0x12);
	lcm_dcs_write_seq_static(ctx, 0x47,0x55);
	lcm_dcs_write_seq_static(ctx, 0x49,0x00);
	lcm_dcs_write_seq_static(ctx, 0x4A,0xB2);
	lcm_dcs_write_seq_static(ctx, 0x4B,0xB0);
	lcm_dcs_write_seq_static(ctx, 0x4C,0x52);
	lcm_dcs_write_seq_static(ctx, 0x4D,0x21);
	lcm_dcs_write_seq_static(ctx, 0x4E,0x43);
	lcm_dcs_write_seq_static(ctx, 0x4F,0x65);
	lcm_dcs_write_seq_static(ctx, 0x50,0x87);
	lcm_dcs_write_seq_static(ctx, 0x51,0x12);
	lcm_dcs_write_seq_static(ctx, 0x52,0x78);
	lcm_dcs_write_seq_static(ctx, 0x53,0x56);
	lcm_dcs_write_seq_static(ctx, 0x54,0x34);
	lcm_dcs_write_seq_static(ctx, 0x55,0x42,0x0A);
	lcm_dcs_write_seq_static(ctx, 0x56,0x08);
	lcm_dcs_write_seq_static(ctx, 0x58,0x21);
	lcm_dcs_write_seq_static(ctx, 0x59,0x30);
	lcm_dcs_write_seq_static(ctx, 0x5A,0xB2);
	lcm_dcs_write_seq_static(ctx, 0x5B,0xB0);
	lcm_dcs_write_seq_static(ctx, 0x5E,0x00,0x14);
	lcm_dcs_write_seq_static(ctx, 0x61,0x4C);
	lcm_dcs_write_seq_static(ctx, 0x63,0x60);
	lcm_dcs_write_seq_static(ctx, 0x91,0x00);
	lcm_dcs_write_seq_static(ctx, 0x92,0xCE);
	lcm_dcs_write_seq_static(ctx, 0x93,0xC3,0x00);
	lcm_dcs_write_seq_static(ctx, 0x94,0x18);
	lcm_dcs_write_seq_static(ctx, 0xAB,0x55);
	lcm_dcs_write_seq_static(ctx, 0xAC,0xA4);
	lcm_dcs_write_seq_static(ctx, 0xC5,0x00);
	lcm_dcs_write_seq_static(ctx, 0xC7,0x17);
	lcm_dcs_write_seq_static(ctx, 0xC8,0x14);
	lcm_dcs_write_seq_static(ctx, 0xC9,0x17);
	lcm_dcs_write_seq_static(ctx, 0xCA,0x14);
	lcm_dcs_write_seq_static(ctx, 0xDB,0x05);
	lcm_dcs_write_seq_static(ctx, 0xDC,0xCA);
	lcm_dcs_write_seq_static(ctx, 0xDF,0x05);
	lcm_dcs_write_seq_static(ctx, 0xE0,0xCA);
	lcm_dcs_write_seq_static(ctx, 0xE1,0x05);
	lcm_dcs_write_seq_static(ctx, 0xE2,0xCA);
	lcm_dcs_write_seq_static(ctx, 0xEF,0x05);
	lcm_dcs_write_seq_static(ctx, 0xF0,0xCA);
	lcm_dcs_write_seq_static(ctx, 0xF7,0xAA);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x25);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x13,0x02);
	lcm_dcs_write_seq_static(ctx, 0x14,0x72);
	lcm_dcs_write_seq_static(ctx, 0x15,0x01);
	lcm_dcs_write_seq_static(ctx, 0x16,0x8A);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x26);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x00,0x80);
	lcm_dcs_write_seq_static(ctx, 0x02,0x0D);
	lcm_dcs_write_seq_static(ctx, 0x04,0x2F);
	lcm_dcs_write_seq_static(ctx, 0x0A,0x05);
	lcm_dcs_write_seq_static(ctx, 0x1D,0x00);
	lcm_dcs_write_seq_static(ctx, 0x1E,0xCE);
	lcm_dcs_write_seq_static(ctx, 0x1F,0xCE);
	lcm_dcs_write_seq_static(ctx, 0x24,0x00);
	lcm_dcs_write_seq_static(ctx, 0x2F,0x00);
	lcm_dcs_write_seq_static(ctx, 0x39,0x10);
	lcm_dcs_write_seq_static(ctx, 0x3A,0xCE);
	lcm_dcs_write_seq_static(ctx, 0x3B,0x00);
	lcm_dcs_write_seq_static(ctx, 0x40,0x82);
	lcm_dcs_write_seq_static(ctx, 0x41,0x82);
	lcm_dcs_write_seq_static(ctx, 0x42,0x82);
	lcm_dcs_write_seq_static(ctx, 0x4A,0x82);
	lcm_dcs_write_seq_static(ctx, 0x4D,0x6B);
	lcm_dcs_write_seq_static(ctx, 0x4E,0x6F);
	lcm_dcs_write_seq_static(ctx, 0x4F,0x6B);
	lcm_dcs_write_seq_static(ctx, 0x50,0x6F);
	lcm_dcs_write_seq_static(ctx, 0x51,0x6B);
	lcm_dcs_write_seq_static(ctx, 0x52,0x6F);
	lcm_dcs_write_seq_static(ctx, 0x56,0x6B);
	lcm_dcs_write_seq_static(ctx, 0x58,0x6F);
	lcm_dcs_write_seq_static(ctx, 0x5B,0x6B);
	lcm_dcs_write_seq_static(ctx, 0x5C,0x6F);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x27);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x58,0xAC);
	lcm_dcs_write_seq_static(ctx, 0x59,0x00,0x16);
	lcm_dcs_write_seq_static(ctx, 0x5A,0x00,0xC2);
	lcm_dcs_write_seq_static(ctx, 0x5D,0x00,0x01);
	lcm_dcs_write_seq_static(ctx, 0x5E,0x00,0xB0);
	lcm_dcs_write_seq_static(ctx, 0x5F,0x02);
	lcm_dcs_write_seq_static(ctx, 0x60,0x06,0x78);
	lcm_dcs_write_seq_static(ctx, 0xC0,0x08);
	lcm_dcs_write_seq_static(ctx, 0xE2,0x39);
	lcm_dcs_write_seq_static(ctx, 0xE3,0x01);
	lcm_dcs_write_seq_static(ctx, 0xE4,0x00);
	lcm_dcs_write_seq_static(ctx, 0xE5,0x9D);
	lcm_dcs_write_seq_static(ctx, 0xE6,0x00);
	lcm_dcs_write_seq_static(ctx, 0xEF,0x00);
	lcm_dcs_write_seq_static(ctx, 0xF0,0x77);
	lcm_dcs_write_seq_static(ctx, 0xF1,0x00);
	lcm_dcs_write_seq_static(ctx, 0xF2,0x00);
	lcm_dcs_write_seq_static(ctx, 0xF3,0x40);
	lcm_dcs_write_seq_static(ctx, 0xF4,0x00);
	lcm_dcs_write_seq_static(ctx, 0xF5,0x77);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x2A);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x14,0x7E);
	lcm_dcs_write_seq_static(ctx, 0x15,0x7E);
	lcm_dcs_write_seq_static(ctx, 0x16,0x7E);
	lcm_dcs_write_seq_static(ctx, 0x23,0x08);
	lcm_dcs_write_seq_static(ctx, 0x27,0x00);
	lcm_dcs_write_seq_static(ctx, 0x28,0x18);
	lcm_dcs_write_seq_static(ctx, 0x2B,0x00);
	lcm_dcs_write_seq_static(ctx, 0x2D,0x18);
	lcm_dcs_write_seq_static(ctx, 0x32,0x00);
	lcm_dcs_write_seq_static(ctx, 0x33,0x18);
	lcm_dcs_write_seq_static(ctx, 0x36,0x00);
	lcm_dcs_write_seq_static(ctx, 0x37,0x18);
	lcm_dcs_write_seq_static(ctx, 0x99,0x95);
	lcm_dcs_write_seq_static(ctx, 0x9A,0x0B);
	lcm_dcs_write_seq_static(ctx, 0xA2,0x3F);
	lcm_dcs_write_seq_static(ctx, 0xA3,0xF0);
	lcm_dcs_write_seq_static(ctx, 0xA4,0x03);
	lcm_dcs_write_seq_static(ctx, 0xAC,0x00);
	lcm_dcs_write_seq_static(ctx, 0xAD,0x78);
	lcm_dcs_write_seq_static(ctx, 0xAE,0xB4);
	lcm_dcs_write_seq_static(ctx, 0xAF,0x43);
	lcm_dcs_write_seq_static(ctx, 0xB0,0x90);
	lcm_dcs_write_seq_static(ctx, 0xB1,0x93);
	lcm_dcs_write_seq_static(ctx, 0xB3,0x66);
	lcm_dcs_write_seq_static(ctx, 0xC4,0x80);
	lcm_dcs_write_seq_static(ctx, 0xC5,0x16);
	lcm_dcs_write_seq_static(ctx, 0xC6,0x01);
	lcm_dcs_write_seq_static(ctx, 0xC7,0x29);
	lcm_dcs_write_seq_static(ctx, 0xC8,0x10);
	lcm_dcs_write_seq_static(ctx, 0xC9,0x02);
	lcm_dcs_write_seq_static(ctx, 0xCA,0x16);
	lcm_dcs_write_seq_static(ctx, 0xCB,0x26);
	lcm_dcs_write_seq_static(ctx, 0xCC,0x78);

	//LT CK 4H width - 20221222
	lcm_dcs_write_seq_static(ctx, 0xFF,0x27);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x78,0x85);
	lcm_dcs_write_seq_static(ctx, 0x79,0x04);
	lcm_dcs_write_seq_static(ctx, 0x7A,0x0C);
	lcm_dcs_write_seq_static(ctx, 0x7C,0xB2);
	lcm_dcs_write_seq_static(ctx, 0x7D,0xB0);
	lcm_dcs_write_seq_static(ctx, 0x83,0x0A);
	lcm_dcs_write_seq_static(ctx, 0x84,0x06);
	lcm_dcs_write_seq_static(ctx, 0x85,0x4C);
	lcm_dcs_write_seq_static(ctx, 0x87,0x30);
	lcm_dcs_write_seq_static(ctx, 0x88,0x32);
	lcm_dcs_write_seq_static(ctx, 0x89,0x44);
	lcm_dcs_write_seq_static(ctx, 0x8B,0x45);
	lcm_dcs_write_seq_static(ctx, 0x8C,0x47);
	lcm_dcs_write_seq_static(ctx, 0x8F,0x0E);
	lcm_dcs_write_seq_static(ctx, 0x90,0x12);
	lcm_dcs_write_seq_static(ctx, 0x93,0xA5);
	lcm_dcs_write_seq_static(ctx, 0x94,0x5A);
	lcm_dcs_write_seq_static(ctx, 0x96,0x0B);
	lcm_dcs_write_seq_static(ctx, 0xC4,0xB2);
	lcm_dcs_write_seq_static(ctx, 0xC5,0xB0);
	lcm_dcs_write_seq_static(ctx, 0xC8,0xB2);
	lcm_dcs_write_seq_static(ctx, 0xC9,0xB0);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x10);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x3B,0x03,0xC3);

	//switch to table B3
	lcm_dcs_write_seq_static(ctx, 0xFF,0x24);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x55,0x43,0x0B);
	lcm_dcs_write_seq_static(ctx, 0x59,0x40);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x25);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0xBC,0x24);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x26);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x09,0x05);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x2A);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x23,0x0C);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x10);
	lcm_dcs_write_seq_static(ctx, 0x3B,0x13,0x2A);

	//update vfp = 298, vbp = 24
	lcm_dcs_write_seq_static(ctx, 0xFF,0x27);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x58,0xA4);
	lcm_dcs_write_seq_static(ctx, 0xFF,0x2A);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x32,0x01);
	lcm_dcs_write_seq_static(ctx, 0x33,0x2A);
	lcm_dcs_write_seq_static(ctx, 0x36,0x01);
	lcm_dcs_write_seq_static(ctx, 0x37,0x2A);

	// VGH : 17V
	// VGL : -12V
	lcm_dcs_write_seq_static(ctx, 0xFF,0x20);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x05,0xBB);
	lcm_dcs_write_seq_static(ctx, 0x07,0xF0);
	lcm_dcs_write_seq_static(ctx, 0x08,0x8C);

	//PG 03
	//CMD2_Page0
	lcm_dcs_write_seq_static(ctx, 0xFF,0x20);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	//R(+)
	lcm_dcs_write_seq_static(ctx, 0xB0 ,0x00 ,0x08 ,0x00 ,0x2E ,0x00 ,0x5A ,0x00 ,0x80 ,0x00 ,0x9A ,0x00 ,0xB2 ,0x00 ,0xC7 ,0x00 ,0xDB);
	lcm_dcs_write_seq_static(ctx, 0xB1 ,0x00 ,0xEA ,0x01 ,0x1E ,0x01 ,0x47 ,0x01 ,0x82 ,0x01 ,0xAC ,0x01 ,0xEF ,0x02 ,0x22 ,0x02 ,0x24);
	lcm_dcs_write_seq_static(ctx, 0xB2 ,0x02 ,0x55 ,0x02 ,0x8E ,0x02 ,0xB5 ,0x02 ,0xE9 ,0x03 ,0x0B ,0x03 ,0x38 ,0x03 ,0x48 ,0x03 ,0x57);
	lcm_dcs_write_seq_static(ctx, 0xB3 ,0x03 ,0x66 ,0x03 ,0x7B ,0x03 ,0x94 ,0x03 ,0xAD ,0x03 ,0xC9 ,0x03 ,0xD8);
	//G(+)
	lcm_dcs_write_seq_static(ctx, 0xB4 ,0x00 ,0x08 ,0x00 ,0x2C ,0x00 ,0x5C ,0x00 ,0x7E ,0x00 ,0x9B ,0x00 ,0xB3 ,0x00 ,0xC8 ,0x00 ,0xDA);
	lcm_dcs_write_seq_static(ctx, 0xB5 ,0x00 ,0xEB ,0x01 ,0x20 ,0x01 ,0x47 ,0x01 ,0x82 ,0x01 ,0xAF ,0x01 ,0xF0 ,0x02 ,0x24 ,0x02 ,0x26);
	lcm_dcs_write_seq_static(ctx, 0xB6 ,0x02 ,0x58 ,0x02 ,0x91 ,0x02 ,0xB7 ,0x02 ,0xEB ,0x03 ,0x0D ,0x03 ,0x37 ,0x03 ,0x47 ,0x03 ,0x56);
	lcm_dcs_write_seq_static(ctx, 0xB7 ,0x03 ,0x66 ,0x03 ,0x7B ,0x03 ,0x94 ,0x03 ,0xAD ,0x03 ,0xCC ,0x03 ,0xD8);
	//B(+)
	lcm_dcs_write_seq_static(ctx, 0xB8 ,0x00 ,0x08 ,0x00 ,0x31 ,0x00 ,0x5A ,0x00 ,0x7E ,0x00 ,0x9E ,0x00 ,0xBA ,0x00 ,0xD0 ,0x00 ,0xE4);
	lcm_dcs_write_seq_static(ctx, 0xB9 ,0x00 ,0xF4 ,0x01 ,0x29 ,0x01 ,0x50 ,0x01 ,0x8A ,0x01 ,0xB4 ,0x01 ,0xF4 ,0x02 ,0x26 ,0x02 ,0x28);
	lcm_dcs_write_seq_static(ctx, 0xBA ,0x02 ,0x59 ,0x02 ,0x92 ,0x02 ,0xB9 ,0x02 ,0xEE ,0x03 ,0x12 ,0x03 ,0x38 ,0x03 ,0x4B ,0x03 ,0x5F);
	lcm_dcs_write_seq_static(ctx, 0xBB ,0x03 ,0x66 ,0x03 ,0x7B ,0x03 ,0x94 ,0x03 ,0xC1 ,0x03 ,0xC6 ,0x03 ,0xD8);
	//R+C1
	lcm_dcs_write_seq_static(ctx, 0xC6 ,0x42);
	lcm_dcs_write_seq_static(ctx, 0xC7 ,0x30);
	lcm_dcs_write_seq_static(ctx, 0xC8 ,0x31);
	lcm_dcs_write_seq_static(ctx, 0xC9 ,0x20);
	lcm_dcs_write_seq_static(ctx, 0xCA ,0x10);
	//R-C1
	lcm_dcs_write_seq_static(ctx, 0xCB ,0x42);
	lcm_dcs_write_seq_static(ctx, 0xCC ,0x30);
	lcm_dcs_write_seq_static(ctx, 0xCD ,0x31);
	lcm_dcs_write_seq_static(ctx, 0xCE ,0x20);
	lcm_dcs_write_seq_static(ctx, 0xCF ,0x10);
	//G+C1
	lcm_dcs_write_seq_static(ctx, 0xD0 ,0x52);
	lcm_dcs_write_seq_static(ctx, 0xD1 ,0x30);
	lcm_dcs_write_seq_static(ctx, 0xD2 ,0x31);
	lcm_dcs_write_seq_static(ctx, 0xD3 ,0x20);
	lcm_dcs_write_seq_static(ctx, 0xD4 ,0x10);
	//G-C1
	lcm_dcs_write_seq_static(ctx, 0xD5 ,0x52);
	lcm_dcs_write_seq_static(ctx, 0xD6 ,0x30);
	lcm_dcs_write_seq_static(ctx, 0xD7 ,0x31);
	lcm_dcs_write_seq_static(ctx, 0xD8 ,0x20);
	lcm_dcs_write_seq_static(ctx, 0xD9 ,0x10);
	//B+C1
	lcm_dcs_write_seq_static(ctx, 0xDA ,0x52);
	lcm_dcs_write_seq_static(ctx, 0xDB ,0x30);
	lcm_dcs_write_seq_static(ctx, 0xDC ,0x31);
	lcm_dcs_write_seq_static(ctx, 0xDD ,0x20);
	lcm_dcs_write_seq_static(ctx, 0xDE ,0x10);
	//B-C1
	lcm_dcs_write_seq_static(ctx, 0xDF ,0x52);
	lcm_dcs_write_seq_static(ctx, 0xE0 ,0x30);
	lcm_dcs_write_seq_static(ctx, 0xE1 ,0x31);
	lcm_dcs_write_seq_static(ctx, 0xE2 ,0x20);
	lcm_dcs_write_seq_static(ctx, 0xE3 ,0x10);
	//CMD2_Page1
	lcm_dcs_write_seq_static(ctx, 0xFF,0x21);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	//R(-)
	lcm_dcs_write_seq_static(ctx, 0xB0 ,0x00 ,0x00 ,0x00 ,0x26 ,0x00 ,0x52 ,0x00 ,0x78 ,0x00 ,0x92 ,0x00 ,0xAA ,0x00 ,0xBF ,0x00 ,0xD3);
	lcm_dcs_write_seq_static(ctx, 0xB1 ,0x00 ,0xE2 ,0x01 ,0x16 ,0x01 ,0x3F ,0x01 ,0x7A ,0x01 ,0xA4 ,0x01 ,0xE7 ,0x02 ,0x1A ,0x02 ,0x1C);
	lcm_dcs_write_seq_static(ctx, 0xB2 ,0x02 ,0x4D ,0x02 ,0x86 ,0x02 ,0xAD ,0x02 ,0xE1 ,0x03 ,0x03 ,0x03 ,0x30 ,0x03 ,0x40 ,0x03 ,0x4F);
	lcm_dcs_write_seq_static(ctx, 0xB3 ,0x03 ,0x5E ,0x03 ,0x73 ,0x03 ,0x8C ,0x03 ,0xA5 ,0x03 ,0xC1 ,0x03 ,0xD0);
	//G(-)
	lcm_dcs_write_seq_static(ctx, 0xB4 ,0x00 ,0x00 ,0x00 ,0x24 ,0x00 ,0x54 ,0x00 ,0x76 ,0x00 ,0x93 ,0x00 ,0xAB ,0x00 ,0xC0 ,0x00 ,0xD2);
	lcm_dcs_write_seq_static(ctx, 0xB5 ,0x00 ,0xE3 ,0x01 ,0x18 ,0x01 ,0x3F ,0x01 ,0x7A ,0x01 ,0xA7 ,0x01 ,0xE8 ,0x02 ,0x1C ,0x02 ,0x1E);
	lcm_dcs_write_seq_static(ctx, 0xB6 ,0x02 ,0x50 ,0x02 ,0x89 ,0x02 ,0xAF ,0x02 ,0xE3 ,0x03 ,0x05 ,0x03 ,0x2F ,0x03 ,0x3F ,0x03 ,0x4E);
	lcm_dcs_write_seq_static(ctx, 0xB7 ,0x03 ,0x5E ,0x03 ,0x73 ,0x03 ,0x8C ,0x03 ,0xA5 ,0x03 ,0xC4 ,0x03 ,0xD0);
	//B(-)
	lcm_dcs_write_seq_static(ctx, 0xB8 ,0x00 ,0x00 ,0x00 ,0x29 ,0x00 ,0x52 ,0x00 ,0x76 ,0x00 ,0x96 ,0x00 ,0xB2 ,0x00 ,0xC8 ,0x00 ,0xDC);
	lcm_dcs_write_seq_static(ctx, 0xB9 ,0x00 ,0xEC ,0x01 ,0x21 ,0x01 ,0x48 ,0x01 ,0x82 ,0x01 ,0xAC ,0x01 ,0xEC ,0x02 ,0x1E ,0x02 ,0x20);
	lcm_dcs_write_seq_static(ctx, 0xBA ,0x02 ,0x51 ,0x02 ,0x8A ,0x02 ,0xB1 ,0x02 ,0xE6 ,0x03 ,0x0A ,0x03 ,0x30 ,0x03 ,0x43 ,0x03 ,0x57);
	lcm_dcs_write_seq_static(ctx, 0xBB ,0x03 ,0x5E ,0x03 ,0x73 ,0x03 ,0x8C ,0x03 ,0xB9 ,0x03 ,0xBE ,0x03 ,0xD0);

	lcm_dcs_write_seq_static(ctx, 0xFF,0x10);
	lcm_dcs_write_seq_static(ctx, 0xFB,0x01);
	lcm_dcs_write_seq_static(ctx, 0x3B,0x03,0x18);
	lcm_dcs_write_seq_static(ctx, 0x35,0x00);
	// end 

	//Sleep-Out
	lcm_dcs_write_seq_static(ctx, 0x11);
	msleep(120);
	//Display On
	lcm_dcs_write_seq_static(ctx, 0x29);
	msleep(20);
}

static int lcm_disable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->pm_enable_gpio =
		devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->pm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get pm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->pm_enable_gpio));
	} else {
		gpiod_set_value(ctx->pm_enable_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->pm_enable_gpio);
	}

	ctx->enabled = false;
	bl_onoff = 0;
	pr_err("[Kernel/LCM]  bl_onoff, %d \n", bl_onoff);

	return 0;
}

static int lcm_unprepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (!ctx->prepared)
		return 0;

	lcm_dcs_write_seq_static(ctx, 0x28);
	msleep(20);
	lcm_dcs_write_seq_static(ctx, 0x10);
	msleep(60);

	ctx->error = 0;
	ctx->prepared = false;

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ctx->bias_neg = devm_gpiod_get_index(ctx->dev,
		"bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	gpiod_set_value(ctx->bias_neg, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);

	ctx->bias_pos = devm_gpiod_get_index(ctx->dev,
		"bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	gpiod_set_value(ctx->bias_pos, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);

	ctx->lcm_enable_gpio =
		devm_gpiod_get(ctx->dev, "lcm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->lcm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get lcm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->lcm_enable_gpio));
		return PTR_ERR(ctx->lcm_enable_gpio);
	}
	gpiod_set_value(ctx->lcm_enable_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->lcm_enable_gpio);

	return 0;
}

static int lcm_prepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret;
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (ctx->prepared)
		return 0;

	ctx->lcm_enable_gpio =
		devm_gpiod_get(ctx->dev, "lcm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->lcm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get lcm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->lcm_enable_gpio));
		return PTR_ERR(ctx->lcm_enable_gpio);
	}
	gpiod_set_value(ctx->lcm_enable_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->lcm_enable_gpio);
	msleep(1);

	// VPOS
	ctx->bias_pos = devm_gpiod_get_index(ctx->dev,
		"bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	gpiod_set_value(ctx->bias_pos, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);

	// VNEG
	ctx->bias_neg = devm_gpiod_get_index(ctx->dev,
		"bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	gpiod_set_value(ctx->bias_neg, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);

	// Set VPOS voltage to 6v
	ret = fih_aw37501_write_bytes(0x00, 0x14);
	if (ret < 0) {
		pr_err("[Kernel/LCM] panel AW37501 VPOS i2c write error\n");
	}

	// Set VNEG voltage to -6v
	ret = fih_aw37501_write_bytes(0x01, 0x14);
	if (ret < 0) {
		pr_err("[Kernel/LCM] panel AW37501 VNEG i2c write error\n");
	}

	msleep(10);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	msleep(1);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	udelay(10);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	msleep(10);	

	lcm_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0)
		lcm_unprepare(panel);

	ctx->prepared = true;

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif
#ifdef PANEL_SUPPORT_READBACK
	lcm_panel_get_data(ctx);
#endif

	return ret;
}

static int lcm_enable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	pr_err("[Kernel/LCM] %s enter\n", __func__);

	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->pm_enable_gpio =
		devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->pm_enable_gpio)) {
		dev_err(ctx->dev, "%s: cannot get pm_enable_gpio %ld\n",
			__func__, PTR_ERR(ctx->pm_enable_gpio));
	} else {
		gpiod_set_value(ctx->pm_enable_gpio, 1);
		devm_gpiod_put(ctx->dev, ctx->pm_enable_gpio);
	}
	aw99703_init_reg();

	ctx->enabled = true;
	bl_onoff = 1;
	pr_err("[Kernel/LCM]  bl_onoff, %d \n", bl_onoff);

	return 0;
}

#define HFP (100)
#define HSA (20)
#define HBP (200)
#define VFP (298)
#define VFP_60HZ (1265)
#define VSA (2)
#define VBP (22)
#define VAC (1612)
#define HAC (720)
static u32 fake_heigh = 1612;
static u32 fake_width = 720;
#define PHYSICAL_WIDTH_UM  (73230)
#define PHYSICAL_HEIGHT_UM (163450)

static bool need_fake_resolution;

static struct drm_display_mode default_mode = {
	.clock = 181020,
	.hdisplay = HAC,
	.hsync_start = HAC + HFP,
	.hsync_end = HAC + HFP + HSA,
	.htotal = HAC + HFP + HSA + HBP,
	.vdisplay = VAC,
	.vsync_start = VAC + VFP_60HZ,
	.vsync_end = VAC + VFP_60HZ + VSA,
	.vtotal = VAC + VFP_60HZ + VSA + VBP,
	//.vrefresh = 60,
};

static struct drm_display_mode performance_mode = {
	.clock = 181020,
	.hdisplay = HAC,
	.hsync_start = HAC + HFP,
	.hsync_end = HAC + HFP + HSA,
	.htotal = HAC + HFP + HSA + HBP,
	.vdisplay = VAC,
	.vsync_start = VAC + VFP,
	.vsync_end = VAC + VFP + VSA,
	.vtotal = VAC + VFP + VSA + VBP,
	//.vrefresh = 90,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct lcm *ctx = panel_to_lcm(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_ata_check(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	unsigned char data[3] = {0x00, 0x00, 0x00};
	unsigned char id[3] = {0x00, 0x00, 0x00};
	ssize_t ret;

	ret = mipi_dsi_dcs_read(dsi, 0x4, data, 3);
	if (ret < 0) {
		pr_err("%s error\n", __func__);
		return 0;
	}

	pr_info("ATA read data %x %x %x\n", data[0], data[1], data[2]);

	if (data[0] == id[0] &&
			data[1] == id[1] &&
			data[2] == id[2])
		return 1;

	pr_info("ATA expect read data is %x %x %x\n",
			id[0], id[1], id[2]);

	return 0;
}

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	char bl_tb0[] = {0x51, 0xFF};

	bl_tb0[1] = level;

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));

	return 0;
}

static int lcm_get_virtual_heigh(void)
{
	return VAC;
}

static int lcm_get_virtual_width(void)
{
	return HAC;
}

static struct mtk_panel_params ext_params = {
	.pll_clk = 584,
	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = PHYSICAL_WIDTH_UM,
	.physical_height_um = PHYSICAL_HEIGHT_UM,
	.phy_timcon = {
		.clk_hs_post = 36,
	},
	.dyn_fps = {
		.switch_en = 1, .vact_timing_fps = 90,
	},
};

static struct mtk_panel_params ext_params_90hz = {
	.pll_clk = 584,
	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = PHYSICAL_WIDTH_UM,
	.physical_height_um = PHYSICAL_HEIGHT_UM,
	.phy_timcon = {
		.clk_hs_post = 36,
	},
	.dyn_fps = {
		.switch_en = 1, .vact_timing_fps = 90,
	},
};

static struct drm_display_mode *get_mode_by_id(struct drm_connector *connector,
	unsigned int mode)
{
	struct drm_display_mode *m;
	unsigned int i = 0;

	list_for_each_entry(m, &connector->modes, head) {
		if (i == mode)
			return m;
		i++;
	}
	return NULL;
}
static int mtk_panel_ext_param_set(struct drm_panel *panel,
			 struct drm_connector *connector, unsigned int mode)
{
	struct mtk_panel_ext *ext = find_panel_ext(panel);
	int ret = 0;
	struct drm_display_mode *m = get_mode_by_id(connector, mode);

	if (drm_mode_vrefresh(m) == 60)
		ext->params = &ext_params;
	else if (drm_mode_vrefresh(m) == 90)
		ext->params = &ext_params_90hz;
	else
		ret = 1;
	if (!ret)
		current_fps = drm_mode_vrefresh(m);
	return ret;
}

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = panel_ata_check,
	.ext_param_set = mtk_panel_ext_param_set,
	.get_virtual_heigh = lcm_get_virtual_heigh,
	.get_virtual_width = lcm_get_virtual_width,
};
#endif

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;

	unsigned int bpc;

	struct {
		unsigned int width;
		unsigned int height;
	} size;

	struct {
		unsigned int prepare;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
	} delay;
};

static void change_drm_disp_mode_params(struct drm_display_mode *mode)
{
	if (fake_heigh > 0 && fake_heigh < VAC) {
		mode->vsync_start = mode->vsync_start - mode->vdisplay
					+ fake_heigh;
		mode->vsync_end = mode->vsync_end - mode->vdisplay + fake_heigh;
		mode->vtotal = mode->vtotal - mode->vdisplay + fake_heigh;
		mode->vdisplay = fake_heigh;
	}
	if (fake_width > 0 && fake_width < HAC) {
		mode->hsync_start = mode->hsync_start - mode->hdisplay
					+ fake_width;
		mode->hsync_end = mode->hsync_end - mode->hdisplay + fake_width;
		mode->htotal = mode->htotal - mode->hdisplay + fake_width;
		mode->hdisplay = fake_width;
	}
}

static int lcm_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode2;

	if (need_fake_resolution){
		change_drm_disp_mode_params(&default_mode);
		change_drm_disp_mode_params(&performance_mode);
	}
	mode = drm_mode_duplicate(connector->dev, &default_mode);
	if (!mode) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay,
			default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	mode2 = drm_mode_duplicate(connector->dev, &performance_mode);
	if (!mode2) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode.hdisplay,
			performance_mode.vdisplay,
			drm_mode_vrefresh(&performance_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode2);
	mode2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode2);
	connector->display_info.width_mm = PHYSICAL_WIDTH_UM/1000;
	connector->display_info.height_mm = PHYSICAL_HEIGHT_UM/1000;

	return 1;
}

static const struct drm_panel_funcs lcm_drm_funcs = {
	.disable = lcm_disable,
	.unprepare = lcm_unprepare,
	.prepare = lcm_prepare,
	.enable = lcm_enable,
	.get_modes = lcm_get_modes,
};

static void check_is_need_fake_resolution(struct device *dev)
{
	unsigned int ret = 0;

	ret = of_property_read_u32(dev->of_node, "fake_heigh", &fake_heigh);
	if (ret)
		need_fake_resolution = false;
	ret = of_property_read_u32(dev->of_node, "fake_width", &fake_width);
	if (ret)
		need_fake_resolution = false;
	if (fake_heigh > 0 && fake_heigh < VAC)
		need_fake_resolution = true;
	if (fake_width > 0 && fake_width < HAC)
		need_fake_resolution = true;
}

static int lcm_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct lcm *ctx;
	struct device_node *backlight;
	int ret;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	pr_err("[Kernel/LCM] nt36528h_hdplus_vdo %s enter\n", __func__);

	dsi_node = of_get_parent(dev->of_node);
	if (dsi_node) {
		endpoint = of_graph_get_next_endpoint(dsi_node, NULL);
		if (endpoint) {
			remote_node = of_graph_get_remote_port_parent(endpoint);
			if (!remote_node) {
				pr_err("[Kernel/LCM] No panel connected,skip probe lcm\n");
				return -ENODEV;
			}
			pr_err("[Kernel/LCM] device node name:%s\n", remote_node->name);
		}
	}
	if (remote_node != dev->of_node) {
		pr_err("[Kernel/LCM] %s+ skip probe due to not current lcm\n", __func__);
		return -ENODEV;
	}

	struct proc_dir_entry *lcm0_dir;
	lcm0_dir = proc_mkdir (FIH_PROC_DIR_LCM0, NULL);

	pr_err("[Kernel/LCM] start to create proc/AllHWList/LCM0/%s\n", FIH_PROC_PATH_BRIDGE_PING);
	if (proc_create(FIH_PROC_PATH_BRIDGE_PING, 0, lcm0_dir, &ping_file_ops) == NULL)
	{
		pr_err("[Kernel/LCM] fail to create /proc/AllHWList/LCM0/%s\n", FIH_PROC_PATH_BRIDGE_PING);
	}

	ctx = devm_kzalloc(dev, sizeof(struct lcm), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE
			 | MIPI_DSI_MODE_LPM;

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get reset-gpios %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);

	ctx->bias_pos = devm_gpiod_get_index(dev, "bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get bias-pos 0 %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	devm_gpiod_put(dev, ctx->bias_pos);

	ctx->bias_neg = devm_gpiod_get_index(dev, "bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get bias-neg 1 %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	devm_gpiod_put(dev, ctx->bias_neg);

	ctx->pm_enable_gpio = devm_gpiod_get(dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->pm_enable_gpio)) {
		dev_err(dev, "[Kernel/LCM] %s: cannot get pm-enable-gpios %ld\n",
			__func__, PTR_ERR(ctx->pm_enable_gpio));
	} else {
		devm_gpiod_put(dev, ctx->pm_enable_gpio);
	}

	ctx->prepared = true;
	ctx->enabled = true;

	drm_panel_init(&ctx->panel, dev, &lcm_drm_funcs, DRM_MODE_CONNECTOR_DSI);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &lcm_drm_funcs;

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif
	check_is_need_fake_resolution(dev);
	pr_err("[Kernel/LCM] %s success\n", __func__);

	return ret;
}

static void lcm_remove(struct mipi_dsi_device *dsi)
{
	struct lcm *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
	remove_proc_entry(FIH_PROC_PATH_BRIDGE_PING, NULL);
	pr_err("[Kernel/LCM] %s success\n", __func__);

}

static const struct of_device_id lcm_of_match[] = {
	{ .compatible = "cpt,nt36528h_hdplus_vdo", },
	{ }
};

MODULE_DEVICE_TABLE(of, lcm_of_match);

static struct mipi_dsi_driver cpt_driver = {
	.probe = lcm_probe,
	.remove = lcm_remove,
	.driver = {
		.name = "panel-cpt-nt36528h-vdo",
		.owner = THIS_MODULE,
		.of_match_table = lcm_of_match,
	},
	// .shutdown = lcm_shutdown,
};

// module_mipi_dsi_driver(lcm_driver);
static int __init cpt_drv_init(void)
{
	int ret = 0;

	pr_notice("%s+\n", __func__);
	mtk_panel_lock();
	ret = mipi_dsi_driver_register(&cpt_driver);
	if (ret < 0)
	pr_notice("%s, Failed to register cpt driver: %d\n", __func__, ret);

	mtk_panel_unlock();
	pr_notice("%s- ret:%d\n", __func__, ret);
	return 0;
}

static void __exit cpt_drv_exit(void)
{
	pr_notice("%s+\n", __func__);
	mtk_panel_lock();
	mipi_dsi_driver_unregister(&cpt_driver);
	mtk_panel_unlock();
	pr_notice("%s-\n", __func__);
}
module_init(cpt_drv_init);
module_exit(cpt_drv_exit);

MODULE_AUTHOR("FIH");
MODULE_DESCRIPTION("CPT NT36528H VDO LCD Panel Driver");
MODULE_LICENSE("GPL v2");
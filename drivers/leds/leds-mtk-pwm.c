// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc.
 *
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>

#include <leds-mtk.h>

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME " %s(%d) :" fmt, __func__, __LINE__

struct led_pwm {
	const char	*name;
	u8		active_low;
	unsigned int	max_brightness;
};

struct led_pwm_data {
	struct mt_led_data m_led;
	struct pwm_device	*pwm;
	struct pwm_state	pwmstate;
	unsigned int		active_low;
};

struct mt_leds_pwm {
	int num_leds;
	struct led_pwm_data leds[];
};

static int __maybe_unused led_pwm_get_conn_id(struct mt_led_data *mdev,
		       int flag)
{
	mdev->conf.connector_id = mtk_drm_get_conn_obj_id_from_idx(mdev->desp.index, flag);
	pr_info("disp_id: %d, get connector id %d", mdev->desp.index, mdev->conf.connector_id);
	return 0;
}

typedef struct {
    int brightness;
    int last_brightness;
} brightness_level_table_t;

#ifdef CONFIG_FIH_SX4
brightness_level_table_t brightness_level[256] = {
    { 255, 250},{ 254, 250},{ 253, 249},{ 252, 249},{ 251, 248},{ 250, 248},{ 249, 247},
	{ 248, 247},{ 247, 247},{ 246, 246},{ 245, 246},{ 244, 245},{ 243, 245},{ 242, 244},
	{ 241, 244},{ 240, 243},{ 239, 243},{ 238, 243},{ 237, 242},{ 236, 242},{ 235, 241},
	{ 234, 241},{ 233, 240},{ 232, 240},{ 231, 240},{ 230, 239},{ 229, 239},{ 228, 238},
	{ 227, 238},{ 226, 237},{ 225, 237},{ 224, 236},{ 223, 235},{ 222, 234},{ 221, 233},
	{ 220, 231},{ 219, 230},{ 218, 229},{ 217, 228},{ 216, 227},{ 215, 226},{ 214, 225},
	{ 213, 224},{ 212, 222},{ 211, 221},{ 210, 220},{ 209, 219},{ 208, 218},{ 207, 217},
	{ 206, 216},{ 205, 215},{ 204, 213},{ 203, 212},{ 202, 211},{ 201, 210},{ 200, 209},
	{ 199, 208},{ 198, 207},{ 197, 206},{ 196, 204},{ 195, 203},{ 194, 202},{ 193, 201},
	{ 192, 200},{ 191, 199},{ 190, 198},{ 189, 197},{ 188, 196},{ 187, 196},{ 186, 195},
	{ 185, 194},{ 184, 193},{ 183, 192},{ 182, 191},{ 181, 190},{ 180, 189},{ 179, 188},
	{ 178, 188},{ 177, 187},{ 176, 186},{ 175, 185},{ 174, 184},{ 173, 183},{ 172, 182},
	{ 171, 181},{ 170, 180},{ 169, 179},{ 168, 179},{ 167, 178},{ 166, 177},{ 165, 176},
	{ 164, 175},{ 163, 174},{ 162, 173},{ 161, 172},{ 160, 172},{ 159, 171},{ 158, 170},
	{ 157, 169},{ 156, 168},{ 155, 167},{ 154, 166},{ 153, 165},{ 152, 165},{ 151, 164},
	{ 150, 163},{ 149, 162},{ 148, 161},{ 147, 160},{ 146, 159},{ 145, 159},{ 144, 158},
	{ 143, 157},{ 142, 156},{ 141, 155},{ 140, 154},{ 139, 153},{ 138, 152},{ 137, 152},
	{ 136, 151},{ 135, 150},{ 134, 149},{ 133, 148},{ 132, 147},{ 131, 146},{ 130, 146},
	{ 129, 145},{ 128, 144},{ 127, 143},{ 126, 142},{ 125, 141},{ 124, 141},{ 123, 140},
	{ 122, 139},{ 121, 138},{ 120, 137},{ 119, 136},{ 118, 136},{ 117, 135},{ 116, 134},
	{ 115, 133},{ 114, 132},{ 113, 131},{ 112, 131},{ 111, 130},{ 110, 129},{ 109, 128},
	{ 108, 127},{ 107, 126},{ 106, 126},{ 105, 125},{ 104, 124},{ 103, 123},{ 102, 122},
	{ 101, 122},{ 100, 121},{ 99, 120},{ 98, 119},{ 97, 118},{ 96, 118},{ 95, 117},{ 94, 116},
	{ 93, 115},{ 92, 114},{ 91, 114},{ 90, 113},{ 89, 112},{ 88, 111},{ 87, 110},{ 86, 109},
	{ 85, 108},{ 84, 107},{ 83, 107},{ 82, 106},{ 81, 105},{ 80, 104},{ 79, 103},{ 78, 102},
	{ 77, 101},{ 76, 100},{ 75, 99},{ 74, 98},{ 73, 97},{ 72, 96},{ 71, 96},{ 70, 95},
	{ 69, 94},{ 68, 93},{ 67, 92},{ 66, 91},{ 65, 90},{ 64, 89},{ 63, 88},{ 62, 87},{ 61, 86},
	{ 60, 85},{ 59, 84},{ 58, 83},{ 57, 82},{ 56, 81},{ 55, 80},{ 54, 79},{ 53, 78},{ 52, 77},
	{ 51, 76},{ 50, 75},{ 49, 74},{ 48, 72},{ 47, 71},{ 46, 70},{ 45, 68},{ 44, 67},{ 43, 66},
	{ 42, 64},{ 41, 63},{ 40, 62},{ 39, 60},{ 38, 59},{ 37, 58},{ 36, 56},{ 35, 55},{ 34, 54},
	{ 33, 52},{ 32, 51},{ 31, 49},{ 30, 48},{ 29, 46},{ 28, 44},{ 27, 43},{ 26, 41},{ 25, 39},
	{ 24, 38},{ 23, 36},{ 22, 35},{ 21, 33},{ 20, 32},{ 19, 30},{ 18, 29},{ 17, 27},{ 16, 26},
	{ 15, 24},{ 14, 23},{ 13, 21},{ 12, 20},{ 11, 18},{ 10, 17},{ 9, 15},{ 8, 13},{ 7, 11},
	{ 6, 9},{ 5, 7},{ 4, 6},{ 3, 4},{ 2, 3},{ 1, 1},{ 0, 0}
};
#else
brightness_level_table_t brightness_level[256] = {
    { 255, 251},{ 254, 250},{ 253, 250},{ 252, 249},{ 251, 249},{ 250, 248},{ 249, 248},
	{ 248, 247},{ 247, 247},{ 246, 246},{ 245, 246},{ 244, 245},{ 243, 245},{ 242, 244},
	{ 241, 244},{ 240, 243},{ 239, 242},{ 238, 242},{ 237, 241},{ 236, 241},{ 235, 240},
	{ 234, 240},{ 233, 239},{ 232, 239},{ 231, 238},{ 230, 238},{ 229, 237},{ 228, 237},
	{ 227, 236},{ 226, 236},{ 225, 235},{ 224, 234},{ 223, 233},{ 222, 232},{ 221, 231},
	{ 220, 230},{ 219, 228},{ 218, 227},{ 217, 226},{ 216, 225},{ 215, 224},{ 214, 223},
	{ 213, 222},{ 212, 221},{ 211, 220},{ 210, 219},{ 209, 218},{ 208, 216},{ 207, 215},
	{ 206, 214},{ 205, 213},{ 204, 212},{ 203, 211},{ 202, 210},{ 201, 209},{ 200, 208},
	{ 199, 207},{ 198, 206},{ 197, 204},{ 196, 203},{ 195, 202},{ 194, 201},{ 193, 200},
	{ 192, 199},{ 191, 198},{ 190, 197},{ 189, 196},{ 188, 195},{ 187, 195},{ 186, 194},
	{ 185, 193},{ 184, 192},{ 183, 191},{ 182, 190},{ 181, 189},{ 180, 188},{ 179, 187},
	{ 178, 187},{ 177, 186},{ 176, 185},{ 175, 184},{ 174, 183},{ 173, 182},{ 172, 181},
	{ 171, 180},{ 170, 179},{ 169, 178},{ 168, 178},{ 167, 177},{ 166, 176},{ 165, 175},
	{ 164, 174},{ 163, 173},{ 162, 172},{ 161, 172},{ 160, 171},{ 159, 170},{ 158, 169},
	{ 157, 168},{ 156, 167},{ 155, 167},{ 154, 166},{ 153, 165},{ 152, 164},{ 151, 163},
	{ 150, 162},{ 149, 162},{ 148, 161},{ 147, 160},{ 146, 159},{ 145, 158},{ 144, 157},
	{ 143, 157},{ 142, 156},{ 141, 155},{ 140, 154},{ 139, 153},{ 138, 152},{ 137, 152},
	{ 136, 151},{ 135, 150},{ 134, 149},{ 133, 148},{ 132, 147},{ 131, 146},{ 130, 146},
	{ 129, 145},{ 128, 144},{ 127, 143},{ 126, 142},{ 125, 141},{ 124, 141},{ 123, 140},
	{ 122, 139},{ 121, 138},{ 120, 137},{ 119, 136},{ 118, 136},{ 117, 135},{ 116, 134},
	{ 115, 133},{ 114, 132},{ 113, 131},{ 112, 131},{ 111, 130},{ 110, 129},{ 109, 128},
	{ 108, 127},{ 107, 126},{ 106, 126},{ 105, 125},{ 104, 124},{ 103, 123},{ 102, 122},
	{ 101, 122},{ 100, 121},{ 99, 120},{ 98, 119},{ 97, 118},{ 96, 118},{ 95, 117},{ 94, 116},
	{ 93, 115},{ 92, 114},{ 91, 114},{ 90, 113},{ 89, 112},{ 88, 111},{ 87, 110},{ 86, 109},
	{ 85, 108},{ 84, 107},{ 83, 107},{ 82, 106},{ 81, 105},{ 80, 104},{ 79, 103},{ 78, 102},
	{ 77, 101},{ 76, 100},{ 75, 99},{ 74, 98},{ 73, 97},{ 72, 96},{ 71, 96},{ 70, 95},
	{ 69, 94},{ 68, 93},{ 67, 92},{ 66, 91},{ 65, 90},{ 64, 88},{ 63, 87},{ 62, 85},{ 61, 84},
	{ 60, 82},{ 59, 81},{ 58, 79},{ 57, 78},{ 56, 76},{ 55, 75},{ 54, 73},{ 53, 71},{ 52, 69},
	{ 51, 67},{ 50, 65},{ 49, 63},{ 48, 61},{ 47, 59},{ 46, 57},{ 45, 56},{ 44, 54},{ 43, 52},
	{ 42, 50},{ 41, 48},{ 40, 46},{ 39, 44},{ 38, 42},{ 37, 40},{ 36, 38},{ 35, 36},{ 34, 35},
	{ 33, 34},{ 32, 33},{ 31, 32},{ 30, 31},{ 29, 30},{ 28, 28},{ 27, 27},{ 26, 26},{ 25, 25},
	{ 24, 24},{ 23, 23},{ 22, 22},{ 21, 21},{ 20, 20},{ 19, 19},{ 18, 18},{ 17, 17},{ 16, 16},
	{ 15, 15},{ 14, 14},{ 13, 13},{ 12, 12},{ 11, 11},{ 10, 10},{ 9, 9},{ 8, 8},{ 7, 7},
	{ 6, 6},{ 5, 5},{ 4, 4},{ 3, 3},{ 2, 2},{ 1, 1},{ 0, 0}
};
#endif

static int led_pwm_set(struct mt_led_data *mdev,
		int brightness, unsigned int params, unsigned int params_flag)
{
	struct led_pwm_data *led_dat =
		container_of(mdev, struct led_pwm_data, m_led);
	unsigned int max = mdev->conf.max_hw_brightness;
	unsigned long long duty = led_dat->pwmstate.period;

	int i = 0;

	for(i = 0; i <= 255; i++)
	{
	    if( brightness == brightness_level[i].brightness){
			brightness = brightness_level[i].last_brightness;
#ifdef CONFIG_FIH_SX4
			pr_notice("SX4 brightness_level[%d].brightness:%d  new brightness state:%d!\n", i, brightness_level[i].brightness, brightness);
#else
			pr_notice("SX3 brightness_level[%d].brightness:%d  new brightness state:%d!\n", i, brightness_level[i].brightness, brightness);
#endif
		    break;
	    }
	}

	duty *= brightness;
	do_div(duty, max);

	if (led_dat->active_low)
		duty = led_dat->pwmstate.period - duty;

	led_dat->pwmstate.duty_cycle = duty;
	led_dat->pwmstate.enabled = duty > 0;
	return pwm_apply_state(led_dat->pwm, &led_dat->pwmstate);
}

__attribute__((nonnull))
static int led_pwm_add(struct device *dev, struct mt_leds_pwm *priv,
		       struct led_pwm *led, struct fwnode_handle *fwnode)
{

	int ret;
	struct led_pwm_data *led_dat = &priv->leds[priv->num_leds];

	led_dat->pwm = devm_fwnode_pwm_get(dev, fwnode, NULL);
	if (IS_ERR(led_dat->pwm)) {
		ret = PTR_ERR(led_dat->pwm);
		dev_notice(dev,
			"unable to request PWM for %s: %d\n",
			led->name, ret);
		return ret;
	}

	pwm_init_state(led_dat->pwm, &led_dat->pwmstate);

	ret = mt_leds_classdev_register(dev, &led_dat->m_led);
	if (ret < 0) {
		dev_notice(dev, "failed to register PWM led for %s: %d\n",
			led->name, ret);
		return ret;
	}

	return 0;
}

static int led_pwm_create_fwnode(struct device *dev, struct mt_leds_pwm *priv)
{
	struct fwnode_handle *fwnode;
	struct led_pwm led;
	struct led_pwm_data *led_data;
	int ret = 0;

	pr_info("create fwnode begain +++");

	memset(&led, 0, sizeof(led));

	device_for_each_child_node(dev, fwnode) {
		led_data = &priv->leds[priv->num_leds];
		ret = mt_leds_parse_dt(&led_data->m_led, fwnode);
		if (ret < 0) {
			fwnode_handle_put(fwnode);
			return -EINVAL;
		}
		led.name = led_data->m_led.conf.cdev.name;
		led.max_brightness = led_data->m_led.conf.cdev.max_brightness;
		led_data->m_led.mtk_hw_brightness_set = led_pwm_set;
		led_data->m_led.mtk_conn_id_get = led_pwm_get_conn_id;
		led_data->m_led.desp.connector_id = mtk_drm_get_conn_obj_id_from_idx(led_data->m_led.desp.index, 0);

		ret = led_pwm_add(dev, priv, &led, fwnode);
		priv->num_leds++;
		if (ret) {
			fwnode_handle_put(fwnode);
			break;
		}
		pr_info("parse led: %s, num: %d, max: %d",
			led.name, priv->num_leds, led.max_brightness);
	}

	return ret;
}

static int led_pwm_probe(struct platform_device *pdev)
{
	struct mt_leds_pwm *priv;
	int ret = 0;
	int count;

	pr_info("probe begain +++");

	count = device_get_child_node_count(&pdev->dev);

	if (!count) {
		ret = -EINVAL;
		goto err;
	}

	priv = devm_kzalloc(&pdev->dev, struct_size(priv, leds, count),
			    GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		goto err;
	}

	ret = led_pwm_create_fwnode(&pdev->dev, priv);

	if (ret < 0)
		goto err;

	platform_set_drvdata(pdev, priv);

	pr_info("probe end +++");

	return 0;
err:
	pr_notice("Failed to probe: %d, %d!\n", ret, count);
	return ret;

}

static void __maybe_unused led_pwm_shutdown(struct platform_device *pdev)
{
	int i;
	struct mt_leds_pwm *m_leds = dev_get_platdata(&pdev->dev);

	pr_info("Turn off backlight\n");

	for (i = 0; m_leds && i < m_leds->num_leds; i++) {
		if (!&(m_leds->leds[i]))
			continue;

		led_pwm_set(&(m_leds->leds[i].m_led), 0, 0, 0);
		mt_leds_call_notifier(LED_STATUS_SHUTDOWN, &(m_leds->leds[i].m_led.conf));
	}
}

static const struct of_device_id of_pwm_leds_match[] = {
	{ .compatible = "mediatek,pwm-leds", },
	{},
};
MODULE_DEVICE_TABLE(of, of_pwm_leds_match);

static struct platform_driver led_pwm_driver = {
	.probe		= led_pwm_probe,
	.driver		= {
		.name	= "mtk_leds_pwm",
		.of_match_table = of_pwm_leds_match,
	},
	.shutdown = led_pwm_shutdown,
};

module_platform_driver(led_pwm_driver);

MODULE_AUTHOR("Mediatek Corporation");
MODULE_DESCRIPTION("MTK Disp PWM Backlight Driver");
MODULE_LICENSE("GPL");


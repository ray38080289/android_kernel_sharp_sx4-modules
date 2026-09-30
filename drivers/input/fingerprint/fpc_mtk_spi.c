/*
 * FPC Fingerprint sensor device driver
 *
 * This driver will control the platform resources that the FPC fingerprint
 * sensor needs to operate. The major things are probing the sensor to check
 * that it is actually connected and let the Kernel know this and with that also
 * enabling and disabling of regulators, enabling and disabling of platform
 * clocks.
 * *
 * The driver will expose most of its available functionality in sysfs which
 * enables dynamic control of these features from eg. a user space process.
 *
 * The sensor's IRQ events will be pushed to Kernel's event handling system and
 * are exposed in the drivers event node. This makes it possible for a user
 * space process to poll the input node and receive IRQ events easily. Usually
 * this node is available under /dev/input/eventX where 'X' is a number given by
 * the event system. A user space process will need to traverse all the event
 * nodes and ask for its parent's name (through EVIOCGNAME) which should match
 * the value in device tree named input-device-name.
 *
 *
 * Copyright (c) 2018 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/pm_wakeup.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>

#ifdef FPC_DRIVER_FOR_ISEE
#include "teei_fp.h"
#include "tee_client_api.h"
#endif

#ifdef FPC_DRIVER_FOR_ISEE
struct TEEC_UUID uuid_ta_fpc = { 0x7778c03f, 0xc30c, 0x4dd0,
	{0xa3, 0x19, 0xea, 0x29, 0x64, 0x3d, 0x4d, 0x4b}
};
#endif

#define FPC_RESET_LOW_US 5000
#define FPC_RESET_HIGH1_US 100
#define FPC_RESET_HIGH2_US 5000
#define FPC_TTW_HOLD_TIME 1000
#define FPC_PWR_ON_US 1000

static const char * const pctl_names[] = {
	"fpsensor_fpc_rst_low",
	"fpsensor_fpc_rst_high",
	"fpsensor_fpc_pwren_low",
	"fpsensor_fpc_pwren_high",
	"fpsensor_spi5_active",
	"fpsensor_spi5_disable",
};

struct fpc_data {
	struct device *dev;
	struct spi_device *spidev;
	struct pinctrl *pinctrl_fpc;
	struct pinctrl_state *pinctrl_state[ARRAY_SIZE(pctl_names)];
	int irq_gpio;
	int rst_gpio;
	bool wakeup_enabled;
	struct wakeup_source *ttw_wl;
	bool clocks_enabled;
};

static DEFINE_MUTEX(spidev_set_gpio_mutex);

extern void mt_spi_disable_master_clk(struct spi_device *spidev);
extern void mt_spi_enable_master_clk(struct spi_device *spidev);
extern struct wakeup_source *wakeup_source_register(struct device *dev, const char *name);
extern void wakeup_source_unregister(struct wakeup_source *ws);

static int select_pin_ctl(struct fpc_data *fpc, const char *name)
{
	size_t i;
	int rc;
	struct device *dev = fpc->dev;
	for (i = 0; i < ARRAY_SIZE(fpc->pinctrl_state); i++) {
		const char *n = pctl_names[i];
		if (!strncmp(n, name, strlen(n))) {
			mutex_lock(&spidev_set_gpio_mutex);
			rc = pinctrl_select_state(fpc->pinctrl_fpc, fpc->pinctrl_state[i]);
			mutex_unlock(&spidev_set_gpio_mutex);
			if (rc)
				dev_err(dev, "cannot select '%s'\n", name);
			else
				dev_dbg(dev, "Selected '%s'\n", name);
			goto exit;
		}
	}
	rc = -EINVAL;
	dev_err(dev, "%s:'%s' not found\n", __func__, name);
exit:
	return rc;
}

static int set_clks(struct fpc_data *fpc, bool enable)
{
	int rc = 0;
	if(fpc->clocks_enabled == enable)
		return rc;
	if (enable) {
		mt_spi_enable_master_clk(fpc->spidev);
		fpc->clocks_enabled = true;
		rc = 1;
#if 1
	} else {
		mt_spi_disable_master_clk(fpc->spidev);
		fpc->clocks_enabled = false;
		rc = 0;
#endif
	}

	return rc;
}

static int hw_enable(struct fpc_data *fpc, bool enable)
{
	if (enable) {
		select_pin_ctl(fpc, "fpsensor_fpc_pwren_high");
		usleep_range(FPC_PWR_ON_US, FPC_PWR_ON_US + 100);
	} else {
		select_pin_ctl(fpc, "fpsensor_fpc_pwren_low");
	}

	return 0;
}

static int hw_reset(struct  fpc_data *fpc)
{
	int irq_gpio;
	struct device *dev = fpc->dev;

	select_pin_ctl(fpc, "fpsensor_fpc_rst_high");
	usleep_range(FPC_RESET_HIGH1_US, FPC_RESET_HIGH1_US + 100);

	select_pin_ctl(fpc, "fpsensor_fpc_rst_low");
	usleep_range(FPC_RESET_LOW_US, FPC_RESET_LOW_US + 100);

	select_pin_ctl(fpc, "fpsensor_fpc_rst_high");
	usleep_range(FPC_RESET_HIGH2_US, FPC_RESET_HIGH2_US + 100);

	irq_gpio = gpio_get_value(fpc->irq_gpio);
	dev_info(dev, "IRQ after reset %d\n", irq_gpio);

	dev_info( dev, "Using GPIO#%d as IRQ.\n", fpc->irq_gpio );
	dev_info( dev, "Using GPIO#%d as RST.\n", fpc->rst_gpio );

	return (irq_gpio) ? (0) : (-1);
}

static ssize_t hw_reset_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct  fpc_data *fpc = dev_get_drvdata(dev);

	if (!strncmp(buf, "reset", strlen("reset"))) {
		rc = hw_reset(fpc);
		return rc ? rc : count;
	}
	else
		return -EINVAL;
}
static DEVICE_ATTR(hw_reset, S_IWUSR, NULL, hw_reset_set);

static int power_enable(struct fpc_data *fpc, bool enable)
{
	int rc = 0;
	if (enable) {
		(void)hw_enable(fpc, true);

		select_pin_ctl(fpc, "fpsensor_spi5_active");
		usleep_range(300, 500);

		rc = hw_reset(fpc);
	} else {
		select_pin_ctl(fpc, "fpsensor_fpc_rst_low");
		usleep_range(100, 500);

		select_pin_ctl(fpc, "fpsensor_spi5_disable");
		usleep_range(100, 500);

		(void)hw_enable(fpc, false);
	}

	return rc;
}

static ssize_t power_enable_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int rc = 0;
	struct  fpc_data *fpc = dev_get_drvdata(dev);

	if (!strncmp(buf, "enable", strlen("enable"))) {
		rc = power_enable(fpc, true);
	} else if (!strncmp(buf, "disable", strlen("disable"))) {
		(void)power_enable(fpc, false);
	}

	return rc ? rc : count;
}
static DEVICE_ATTR(power_enable, S_IWUSR, NULL, power_enable_set);
/**
 * sysfs node for controlling whether the driver is allowed
 * to wake up the platform on interrupt.
 */
static ssize_t wakeup_enable_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct  fpc_data *fpc = dev_get_drvdata(dev);
	ssize_t ret = count;

	if (!strncmp(buf, "enable", strlen("enable"))) {
		fpc->wakeup_enabled = true;
		smp_wmb();
	} else if (!strncmp(buf, "disable", strlen("disable"))) {
		fpc->wakeup_enabled = false;
		smp_wmb();
	}
	else
		ret = -EINVAL;

	return ret;
}
static DEVICE_ATTR(wakeup_enable, S_IWUSR, NULL, wakeup_enable_set);

/**
 * sysf node to check the interrupt status of the sensor, the interrupt
 * handler should perform sysf_notify to allow userland to poll the node.
 */
static ssize_t irq_get(struct device *device,
			struct device_attribute *attribute,
			char* buffer)
{
	struct fpc_data *fpc = dev_get_drvdata(device);
	int irq = gpio_get_value(fpc->irq_gpio);

	return scnprintf(buffer, PAGE_SIZE, "%i\n", irq);
}

/**
 * writing to the irq node will just drop a printk message
 * and return success, used for latency measurement.
 */
static ssize_t irq_ack(struct device *device,
			struct device_attribute *attribute,
			const char *buffer, size_t count)
{
	struct fpc_data *fpc = dev_get_drvdata(device);
	dev_dbg(fpc->dev, "%s\n", __func__);

	return count;
}
static DEVICE_ATTR(irq, S_IRUSR | S_IWUSR, irq_get, irq_ack);

static ssize_t clk_enable_set(struct device *device,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct fpc_data *fpc = dev_get_drvdata(device);
	return set_clks(fpc, (*buf == '1')) ? : count;
}
static DEVICE_ATTR(clk_enable, S_IWUSR, NULL, clk_enable_set);

static struct attribute *fpc_attributes[] = {
	&dev_attr_hw_reset.attr,
	&dev_attr_wakeup_enable.attr,
	&dev_attr_clk_enable.attr,
	&dev_attr_irq.attr,
	&dev_attr_power_enable.attr,
	NULL
};

static const struct attribute_group fpc_attribute_group = {
	.attrs = fpc_attributes,
};

static irqreturn_t fpc_irq_handler(int irq, void *handle)
{
	struct fpc_data *fpc = handle;
	struct device *dev = fpc->dev;
	static int current_level = 0; // We assume low level from start
	current_level = !current_level;

	if (current_level) {
		dev_dbg(dev, "Reconfigure irq to trigger in low level\n");
		irq_set_irq_type(irq, IRQF_TRIGGER_LOW);
	} else {
		dev_dbg(dev, "Reconfigure irq to trigger in high level\n");
		irq_set_irq_type(irq, IRQF_TRIGGER_HIGH);
	}

	/* Make sure 'wakeup_enabled' is updated before using it
	** since this is interrupt context (other thread...) */
	smp_rmb();
	if (fpc->wakeup_enabled) {
		__pm_wakeup_event(fpc->ttw_wl, FPC_TTW_HOLD_TIME);
	}

	sysfs_notify(&fpc->dev->kobj, NULL, dev_attr_irq.attr.name);

	return IRQ_HANDLED;
}

static int mtk6797_probe(struct spi_device *spidev)
{
	struct device *dev = &spidev->dev;
	struct fpc_data *fpc;
	int irqf = 0;
	int irq_num = 0;
	int rc = 0;
	size_t i;

	dev_dbg(dev, "%s\n", __func__);

	fpc = devm_kzalloc(dev, sizeof(*fpc), GFP_KERNEL);
	if (!fpc) {
		dev_err(dev, "failed to allocate memory for struct fpc_data\n");
		rc = -ENOMEM;
		goto exit;
	}

	fpc->dev = dev;
	dev_set_drvdata(dev, fpc);
	fpc->spidev = spidev;
	fpc->spidev->irq = 0; /*SPI_MODE_0*/

	fpc->pinctrl_fpc = devm_pinctrl_get(&spidev->dev);
	if (IS_ERR(fpc->pinctrl_fpc)) {
		rc = PTR_ERR(fpc->pinctrl_fpc);
		dev_err(fpc->dev, "Cannot find pinctrl_fpc rc = %d.\n", rc);
		goto exit;
	}

	for (i = 0; i < ARRAY_SIZE(fpc->pinctrl_state); i++) {
		const char *n = pctl_names[i];
		struct pinctrl_state *state = pinctrl_lookup_state(fpc->pinctrl_fpc, n);
		if (IS_ERR(state)) {
			dev_err(dev, "cannot find '%s'\n", n);
			rc = -EINVAL;
			goto exit;
		}
		dev_info(dev, "found pin control %s\n", n);
		fpc->pinctrl_state[i] = state;
	}

	fpc->irq_gpio = of_get_named_gpio(spidev->dev.of_node, "int-gpios", 0);
	irq_num = gpio_to_irq(fpc->irq_gpio);
	if (!irq_num) {
		rc = -EINVAL;
		dev_err(fpc->dev, "get irq_num error rc = %d.\n", rc);
		goto exit;
	}

	fpc->clocks_enabled = false;
	dev_dbg(dev, "Using GPIO#%d as IRQ.\n", fpc->irq_gpio);
	dev_dbg(dev, "Using GPIO#%d as RST.\n", fpc->rst_gpio);

	fpc->wakeup_enabled = false;

	irqf = IRQF_TRIGGER_HIGH | IRQF_ONESHOT;

	rc = devm_request_threaded_irq(dev, irq_num,
		NULL, fpc_irq_handler, irqf,
		dev_name(dev), fpc);
	if (rc) {
		dev_err(dev, "could not request irq %d\n", irq_num);
		goto exit;
	}
	dev_dbg(dev, "requested irq %d\n", irq_num);

	/* Request that the interrupt should be wakeable */
	enable_irq_wake(irq_num);
	fpc->ttw_wl = wakeup_source_register(dev, "fpc_ttw_wl");

	rc = sysfs_create_group(&dev->kobj, &fpc_attribute_group);
	if (rc) {
		dev_err(dev, "could not create sysfs\n");
		goto exit;
	}

	if (power_enable(fpc, true)) {
		dev_err(dev, "IRQ after reset is low, power off sensor\n");
		power_enable(fpc, false);
	}

	dev_info(dev, "%s: ok\n", __func__);
exit:
	return rc;
}

static void mtk6797_remove(struct spi_device *spidev)
{
	struct  fpc_data *fpc = dev_get_drvdata(&spidev->dev);

	sysfs_remove_group(&spidev->dev.kobj, &fpc_attribute_group);
	wakeup_source_unregister(fpc->ttw_wl);
	(void)power_enable(fpc, false);
	dev_info(&spidev->dev, "%s\n", __func__);
}

static struct of_device_id mt6797_of_match[] = {
	{ .compatible = "fpc,fpc_spi", },
	{}
};
MODULE_DEVICE_TABLE(of, mt6797_of_match);

static struct spi_driver mtk6797_driver = {
	.driver = {
		.name	= "fpc_spi",
		.bus = &spi_bus_type,
		.owner	= THIS_MODULE,
		.of_match_table = mt6797_of_match,
	},
	.probe	= mtk6797_probe,
	.remove	= mtk6797_remove
};

static int __init fpc_sensor_init(void)
{
	int status;

	status = spi_register_driver(&mtk6797_driver);
	if (status < 0) {
		printk("%s, fpc_sensor_init failed.\n", __func__);
	}

#ifdef FPC_DRIVER_FOR_ISEE
	memcpy(&uuid_fp, &uuid_ta_fpc, sizeof(struct TEEC_UUID));
#endif

	return status;
}
module_init(fpc_sensor_init);

static void __exit fpc_sensor_exit(void)
{
	spi_unregister_driver(&mtk6797_driver);
}
module_exit(fpc_sensor_exit);

MODULE_LICENSE("GPL");

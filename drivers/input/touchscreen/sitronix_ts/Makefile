# Linux driver folder
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/touchscreen/sitronix_ts/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/touchscreen/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(DEVICE_MODULES_PATH)/drivers/gpu/drm/mediatek/mediatek_v2/

obj-$(CONFIG_TOUCHSCREEN_SITRONIX_TS)  += sitronix_touch.o
sitronix_touch-objs := sitronix_ts.o sitronix_ts_monitor.o sitronix_ts_swu_gesture.o sitronix_ts_upgrade_fw.o sitronix_ts_i2c.o sitronix_ts_rawtest.o sitronix_ts_sysfs.o sitronix_ts_utility.o \
		tpd_control.o \
		tpd_button.o

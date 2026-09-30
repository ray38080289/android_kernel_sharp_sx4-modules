# Makefile for the focaltech touchscreen drivers.

ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/touchscreen/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/touchscreen/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/touchscreen/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)/focaltech_test/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/touchscreen/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)/focaltech_flash/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/input/touchscreen/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)/focaltech_test/supported_ic/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/base
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/alsps/inc/
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/sensor/1.0/hwmon/include
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/sensor/1.0/alsps/inc/
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(DEVICE_MODULES_PATH)/drivers/gpu/drm/mediatek/mediatek_v2/
subdir-ccflags-y += -DANDROID_GKI_VFS_EXPORT_ONLY=VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver

obj-$(CONFIG_TOUCHSCREEN_FT5446U) += ft5446u.o

ft5446u-y :=  focaltech_core.o \
              focaltech_ex_fun.o \
              focaltech_ex_mode.o \
              focaltech_gesture.o \
              focaltech_esdcheck.o \
              focaltech_point_report_check.o \
              focaltech_proximity.o \
              focaltech_flash.o \
              focaltech_i2c.o \
              tpd_control.o \
              tpd_debug.o	\
              tpd_default.o \
              tpd_setting.o \
              tpd_button.o \
			  focaltech_flash/focaltech_upgrade_ft5446u.o \
			  focaltech_test/focaltech_test.o \
			  focaltech_test/focaltech_test_ini.o \
			  focaltech_test/supported_ic/focaltech_test_ft5446u.o
			  #$(wildcard focaltech_flash/*.o) \
			  #$(wildcard focaltech_test/*.o) \
			  #$(wildcard focaltech_test/supported_ic/*.o)
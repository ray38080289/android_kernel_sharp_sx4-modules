#
# Makefile for the Novatek NT36xxx touchscreen driver.
#

# Each configuration option enables a list of files.
ccflags-$(CONFIG_DRM_MEDIATEK) += -I$(DEVICE_MODULES_PATH)/drivers/gpu/drm/mediatek/
ccflags-$(CONFIG_DRM_MEDIATEK_V2) += -I$(DEVICE_MODULES_PATH)/drivers/gpu/drm/mediatek/mediatek_v2/
obj-$(CONFIG_TOUCHSCREEN_NT36528_HOSTDL_SPI) += nt36528.o
nt36528-y += nt36xxx.o
nt36528-y += nt36xxx_fw_update.o
nt36528-y += nt36xxx_ext_proc.o
nt36528-y += nt36xxx_mp_ctrlram.o

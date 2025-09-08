#
# Makefile for the Synaptics TCM touchscreen driver.
#

# Each configuration option enables a list of files.
ccflags-y += -I$(srctree)/drivers/input/touchscreen/synaptics_tcm_zte/firmware_config/$(CONFIG_TS_FIRMWARE)/
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_SPI_ZTE) += synaptics_tcm_spi.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_I2C_ZTE) += synaptics_tcm_i2c.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_CORE_ZTE) += synaptics_tcm_core.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_TOUCH_ZTE) += synaptics_tcm_touch.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_DEVICE_ZTE) += synaptics_tcm_device.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_TESTING_ZTE) += synaptics_common_interface.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_TESTING_ZTE) += synaptics_tcm_testing.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_REFLASH_ZTE) += synaptics_tcm_reflash.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_RECOVERY_ZTE) += synaptics_tcm_recovery.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_ZEROFLASH_ZTE) += synaptics_tcm_zeroflash.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_DIAGNOSTICS_ZTE) += synaptics_tcm_diagnostics.o
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_TCM_ROMBOOT_ZTE) += synaptics_tcm_romboot.o
obj-$(CONFIG_TP_COMMON_INTERFACE) += synaptics_common_interface.o

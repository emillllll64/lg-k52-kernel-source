#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

#
# Makefile for misc devices that really don't fit anywhere else.
#

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
IMGSENSOR_DRIVER_PATH  := $(srctree)/drivers/misc/mediatek/imgsensor/src
COMMON_VERSION := v1

ifeq (,$(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor))
subdir-ccflags-y +=  -I$(IMGSENSOR_DRIVER_PATH)/common/$(COMMON_VERSION)
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_hw.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_i2c.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_legacy.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_proc.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_sensor_list.o
else
subdir-ccflags-y +=  -I$(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor
obj-y				+= ../$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor/imgsensor.o
obj-y				+= ../$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor/imgsensor.o
obj-y				+= ../$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor/imgsensor_hw.o
obj-y				+= ../$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor/imgsensor_i2c.o
obj-y				+= ../$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor/imgsensor_legacy.o
obj-y				+= ../$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor/imgsensor_proc.o
obj-y				+= ../$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor/imgsensor_sensor_list.o
endif

define FILTER_DRV
ifeq (,$(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/imgsensor))
ifeq ($(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/$(1)),)
common_drv += ../common/$(COMMON_VERSION)/$(1)/
else
platform_drv += $(1)/
endif
else
project_drv += ./camera_project/$(ARCH_MTK_PROJECT)/imgsensor/$(1)/
endif
endef


$(foreach drv,$(subst $\",,$(CONFIG_CUSTOM_KERNEL_IMGSENSOR)),$(eval $(call FILTER_DRV,$(drv))))

ifneq ($(common_drv),)
$(info imgsensor drv by common $(common_drv))
endif
obj-y				+= $(common_drv)


ifneq ($(platform_drv),)
$(info imgsensor drv by platform $(platform_drv))
endif
obj-y				+= $(platform_drv)
obj-y				+= imgsensor_clk.o

ifeq (,$(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/camera_hw))
subdir-ccflags-y		+= -I$(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_hw
obj-y				+= camera_hw/
ifneq ($(project_drv),)
obj-y				+= camera_project/$(ARCH_MTK_PROJECT)/
endif
else
obj-y				+= camera_project/$(ARCH_MTK_PROJECT)/camera_hw/
subdir-ccflags-y		+= -I$(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/camera_hw
ifneq ($(project_drv),)
obj-y				+= $(project_drv)
endif
endif

#subdir-ccflags-y +=  -DNO_CLK_METER

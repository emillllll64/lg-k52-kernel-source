# SPDX-License-Identifier: GPL-2.0
#
# USB peripheral controller drivers
#
subdir-ccflags-$(CONFIG_USB_GADGET_DEBUG)	:= -DDEBUG
subdir-ccflags-$(CONFIG_USB_GADGET_VERBOSE)	+= -DVERBOSE_DEBUG
ccflags-y				+= -I$(srctree)/drivers/usb/gadget/udc

obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o configfs.o u_f.o
libcomposite-$(CONFIG_USB_CONFIGFS_MTK_FASTMETA)	+= meta.o

obj-$(CONFIG_USB_GADGET)	+= udc/ function/ legacy/

obj-$(CONFIG_LGE_USB_GADGET) += u_lgeusb.o

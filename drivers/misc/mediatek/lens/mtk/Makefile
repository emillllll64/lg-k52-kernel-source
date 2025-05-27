# SPDX-License-Identifier: GPL-2.0
# Mediatek lens drivers (common MTK folder)

MTK_LENS_SUBDIR := sub2          # add more when you have them

# -----------------------------------------------------------------------------
# Add global include paths that every sub-driver needs
# (you already had this line in sub2/Makefile)
# -----------------------------------------------------------------------------
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lens/mtk/sub2/inc

# -----------------------------------------------------------------------------
# Tell kbuild to enter the sub-directory if the menu-config selects the block
# -----------------------------------------------------------------------------
obj-$(CONFIG_MTK_LENS) += $(MTK_LENS_SUBDIR)/

# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the EYS3D eCV5546 SoC framebuffer driver
#
obj-$(CONFIG_FB_eCV5546) += ecv5546fb.o
ecv5546fb-objs := ecv5546fb_main.o
ifdef CONFIG_FB_eCV5546_DEBUG
ecv5546fb-objs += ecv5546fb_debug.o
endif

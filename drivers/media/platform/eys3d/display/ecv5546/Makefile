# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the eys3d ecv5546 video device drivers.
#
obj-$(CONFIG_VIDEO_eCV5546_DISP) += display_ecv5546.o
display_ecv5546-objs := ecv5546_display.o
display_ecv5546-objs += ecv5546_disp_tcon.o
ifndef CONFIG_DRM_eCV5546
display_ecv5546-objs += ecv5546_disp_mipitx.o
endif
display_ecv5546-objs += ecv5546_disp_dmix.o
display_ecv5546-objs += ecv5546_disp_tgen.o
display_ecv5546-objs += ecv5546_disp_osd.o
display_ecv5546-objs += ecv5546_disp_vpp.o
display_ecv5546-objs += ecv5546_disp_v4l2.o
ifdef CONFIG_VIDEO_eCV5546_DISP_DEBUG
display_ecv5546-objs += ecv5546_disp_debug.o
endif
ifdef CONFIG_VIDEO_eCV5546_DISP_PI_PANEL
display_ecv5546-objs += ecv5546_disp_pi_dsi_panel.o
endif
ifdef CONFIG_VIDEO_eCV5546_DISP_LT8912B
display_ecv5546-objs += ecv5546_disp_lt8912b.o
endif

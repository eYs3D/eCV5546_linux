# SPDX-License-Identifier: GPL-2.0-only
ecv5546-drm-y := \
	ecv5546_drm_plane.o \
	ecv5546_drm_kms.o \
	ecv5546_drm_crtc.o \
	ecv5546_drm_dsi.o  \
	ecv5546_drm_drv.o

#ecv5546-drm-$(CONFIG_DRM_FBDEV_EMULATION) += ecv5546_drm_fbdev.o
ecv5546-drm-$(CONFIG_DRM_PANEL_eCV5546_SIMPLE_DSI) += ecv5546_drm_dsi_panel_simple.o

ecv5546-drm-y += ecv5546_drm_debugfs.o

obj-$(CONFIG_DRM_eCV5546) += ecv5546-drm.o

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * eYs3D eCV5546 SoC DRM CRTCs
 *
 * Author: dx.jiang<eys3d@eys3d.com>
 *         hammer.hsieh<eys3d@eys3d.com>
 */

#ifndef __EYS3D_eCV5546_DRM_CRTC_H__
#define __EYS3D_eCV5546_DRM_CRTC_H__

#include <drm/drm_atomic.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_device.h>
#include <drm/drm_encoder.h>

#include "ecv5546_drm_plane.h"

struct drm_pending_vblank_event;

enum ecv5546_encoder_type {
	eCV5546_DRM_ENCODER_TYPE_NONE,
	/* Notes: C3V soc display controller only support one MIPI DSI interface!!!
	 *     but connected to two socket, can not be used simultaneously.
	 *     so, only one DSI inetface and one encoder actually.
	 */
	eCV5546_DRM_ENCODER_TYPE_DSI0,  /* DSI interface compatible with Raspberry Pi */
	//eCV5546_DRM_ENCODER_TYPE_DSI1,  /* DSI to HDMI bridge */
	//eCV5546_DRM_ENCODER_TYPE_HDMI0,  /* NO any HDMI Controller onchip or onboard */
	//eCV5546_DRM_ENCODER_TYPE_HDMI1, /* NO  any HDMI Controller onchip or onboard*/
	eCV5546_DRM_ENCODER_TYPE_MAX
};

struct ecv5546_encoder {
	struct drm_encoder base;
	enum ecv5546_encoder_type type;
	u32 clock_select;

	void (*pre_crtc_configure)(struct drm_encoder *encoder);
	void (*pre_crtc_enable)(struct drm_encoder *encoder);
	void (*post_crtc_enable)(struct drm_encoder *encoder);

	void (*post_crtc_disable)(struct drm_encoder *encoder);
	void (*post_crtc_powerdown)(struct drm_encoder *encoder);
};

#define to_ecv5546_encoder(encoder) \
	container_of(encoder, struct ecv5546_encoder, base)

#endif /* __EYS3D_eCV5546_DRM_CRTC_H__ */

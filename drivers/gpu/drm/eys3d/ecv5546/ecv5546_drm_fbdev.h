/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * eYs3D eCV5546 SoC DRM fbdev
 *
 * Author: dx.jiang<eys3d@eys3d.com>
 */

#ifndef _eCV5546_DRM_FBDEV_H_
#define _eCV5546_DRM_FBDEV_H_

#ifdef CONFIG_DRM_FBDEV_EMULATION

int ecv5546_drm_fbdev_init(struct drm_device *dev,
			  unsigned int preferred_bpp);
//void ecv5546_drm_fbdev_fini(struct drm_device *dev);

#else

static inline int ecv5546_drm_fbdev_init(struct drm_device *dev,
					unsigned int preferred_bpp)
{
	return 0;
}

//static inline void ecv5546_drm_fbdev_fini(struct drm_device *dev)
//{
//}

#endif

#endif

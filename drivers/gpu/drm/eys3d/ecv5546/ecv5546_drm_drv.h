/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * eYs3D eCV5546 SoC DRM driver
 *
 * Author: dx.jiang<eys3d@eys3d.com>
 *         hammer.hsieh<eys3d@eys3d.com>
 */

#ifndef __EYS3D_eCV5546_DRM_DRV_H__
#define __EYS3D_eCV5546_DRM_DRV_H__

//#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#include <drm/drm.h>
#include <drm/drm_gem.h>

//#include <drm/drm_atomic.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_device.h>
//#include <drm/drm_encoder.h>
//#include <drm/drm_managed.h>
//#include <drm/drm_mm.h>
//#include <drm/drm_modeset_lock.h>

//#if (((LINUX_VERSION_CODE >> 16) & 0xFF) >= 6 && ((LINUX_VERSION_CODE >> 8) & 0xFF) >= 6)
/* enable it for kernel 6.6.x and higher */
#define DRM_GEM_DMA_AVAILABLE  1
//#else
/* disable it for kernel 6.6.x and higher */
//#define DSI_BRIDGE_OPERATION_MANUALLY  1
//#endif

#define XRES_MIN    16
#define YRES_MIN    16

/* [TODO]:
 *  For VPP, Max Resolution 3840x2880.
 *  For OSD, Max Resolution 1920x1080.
 */
#define XRES_MAX  1920
#define YRES_MAX  1280

#define	SP_DISP_MAX_OSD_LAYER	4
#define	SP_DISP_MAX_VPP_LAYER	1

struct ecv5546_dev {
	struct drm_device base;
	struct device *dev;

	void __iomem *crtc_regs;
	//void __iomem *dsi_regs;
	//void __iomem *ao3_regs;

	void *osd_hdr[SP_DISP_MAX_OSD_LAYER];
	dma_addr_t osd_hdr_phy[SP_DISP_MAX_OSD_LAYER];

	/*
	 * Set to true when the debug test is active.
	 * (reserved for future use)
	 */
	bool debug_test_enabled;

	struct list_head debugfs_list;

};

#define eCV5546_DRM_REG32(reg) { .name = #reg, .offset = reg }

/* ecv5546_drm_debugfs.c */
void ecv5546_debugfs_init(struct drm_minor *minor);
#ifdef CONFIG_DEBUG_FS
void ecv5546_debugfs_add_file(struct drm_device *drm,
			  const char *filename,
			  int (*show)(struct seq_file*, void*),
			  void *data);
void ecv5546_debugfs_add_regset32(struct drm_device *drm,
			      const char *filename,
			      struct debugfs_regset32 *regset);
#else
static inline void ecv5546_debugfs_add_file(struct drm_device *drm,
					const char *filename,
					int (*show)(struct seq_file*, void*),
					void *data)
{
}

static inline void ecv5546_debugfs_add_regset32(struct drm_device *drm,
					    const char *filename,
					    struct debugfs_regset32 *regset)
{
}
#endif

#define to_ecv5546_dev(dev)\
	container_of(dev, struct ecv5546_dev, base)

/* ecv5546_drm_dsi.c */
extern struct platform_driver ecv5546_dsi_driver;
/* ecv5546_drm_crtc.c */
extern struct platform_driver ecv5546_crtc_driver;

#endif /* __EYS3D_eCV5546_DRM_DRV_H__ */

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * eYs3D eCV5546 SoC DRM Planes
 *
 * Author: dx.jiang<eys3d@eys3d.com>
 *         hammer.hsieh<eys3d@eys3d.com>
 */

#ifndef __EYS3D_eCV5546_DRM_PLANE_H__
#define __EYS3D_eCV5546_DRM_PLANE_H__
#include <drm/drm.h>

#include "ecv5546_drm_crtc.h"

#define eCV5546_MAX_PLANE  5

/* for layer_mode */
#define eCV5546_DMIX_BG	0x0 //BG
#define eCV5546_DMIX_L1	0x1 //VPP0
#define eCV5546_DMIX_L2	0x2 //VPP1 (unsupported)
#define eCV5546_DMIX_L3	0x3 //OSD3
#define eCV5546_DMIX_L4	0x4 //OSD2
#define eCV5546_DMIX_L5	0x5 //OSD1
#define eCV5546_DMIX_L6	0x6 //OSD0
#define eCV5546_DMIX_MAX_LAYER	7

/* for pattern_sel */
#define eCV5546_DMIX_BIST_BGC		0x0
#define eCV5546_DMIX_BIST_COLORBAR_ROT0	0x1
#define eCV5546_DMIX_BIST_COLORBAR_ROT90	0x2
#define eCV5546_DMIX_BIST_BORDER_NONE	0x3
#define eCV5546_DMIX_BIST_BORDER_ONE	0x4
#define eCV5546_DMIX_BIST_BORDER		0x5
#define eCV5546_DMIX_BIST_SNOW		0x6
#define eCV5546_DMIX_BIST_SNOW_MAX	0x7
#define eCV5546_DMIX_BIST_SNOW_HALF	0x8
#define eCV5546_DMIX_BIST_REGION		0x9

/*
 * OSD Header config[0]
 */
#define eCV5546_OSD_HDR_CULT				BIT(31) /* En Color Look Up Table */
#define eCV5546_OSD_HDR_BS				BIT(12) /* BYTE SWAP */
/*
 *   BL =1 define HDR_ALPHA as fix value
 *   BL2=1 define HDR_ALPHA as factor value
 */
#define eCV5546_OSD_HDR_BL2				BIT(10)
#define eCV5546_OSD_HDR_BL				BIT(8)
#define eCV5546_OSD_HDR_ALPHA			GENMASK(7, 0)
#define eCV5546_OSD_HDR_KEY				BIT(11)

/*
 * OSD Header config[5]
 */
#define eCV5546_OSD_HDR_CSM				GENMASK(19, 16) /* Color Space Mode */
#define eCV5546_OSD_HDR_CSM_SET(sel)		FIELD_PREP(GENMASK(19, 16), sel)
#define eCV5546_OSD_CSM_RGB_BT601		0x1 /* RGB to BT601 */
#define eCV5546_OSD_CSM_BYPASS			0x4 /* Bypass */

/*
 * OSD region dirty flag for SW latch
 */
#define REGION_ADDR_DIRTY				BIT(0) //(1 << 0)
#define REGION_GSCL_DIRTY				BIT(1) //(1 << 1)

/* for ecv5546_osd_header*/
#define eCV5546_OSD_COLOR_MODE_8BPP			0x2
#define eCV5546_OSD_COLOR_MODE_YUY2			0x4
#define eCV5546_OSD_COLOR_MODE_RGB565		0x8
#define eCV5546_OSD_COLOR_MODE_ARGB1555		0x9
#define eCV5546_OSD_COLOR_MODE_RGBA4444		0xa
#define eCV5546_OSD_COLOR_MODE_ARGB4444		0xb
#define eCV5546_OSD_COLOR_MODE_RGBA8888		0xd
#define eCV5546_OSD_COLOR_MODE_ARGB8888		0xe

#define ALIGNED(x, n)		((x) & (~(n - 1)))
#define EXTENDED_ALIGNED(x, n)	(((x) + ((n) - 1)) & (~(n - 1)))

#define SWAP32(x)	((((unsigned int)(x)) & 0x000000ff) << 24 \
			| (((unsigned int)(x)) & 0x0000ff00) << 8 \
			| (((unsigned int)(x)) & 0x00ff0000) >> 8 \
			| (((unsigned int)(x)) & 0xff000000) >> 24)
#define SWAP16(x)	(((x) & 0x00ff) << 8 | ((x) >> 8))

/*
 * DRM PLANE Setting
 *
 */
#define eCV5546_DRM_PLANE_CAP_SCALE      (1 << 0)
#define eCV5546_DRM_PLANE_CAP_ZPOS       (1 << 1)
#define eCV5546_DRM_PLANE_CAP_ROTATION   (1 << 2)
#define eCV5546_DRM_PLANE_CAP_PIX_BLEND           (1 << 3)
#define eCV5546_DRM_PLANE_CAP_ALPHA_BLEND         (1 << 4)
#define eCV5546_DRM_PLANE_CAP_WIN_BLEND           (1 << 5)
#define eCV5546_DRM_PLANE_CAP_REGION_COLOR_KEYING (1 << 6)
#define eCV5546_DRM_PLANE_CAP_COLOR_KEYING        (1 << 7)
#define eCV5546_DRM_PLANE_CAP_BG_BLEND            (1 << 8)
#define eCV5546_DRM_PLANE_CAP_BG_FORMAT           (1 << 9)
#define eCV5546_DRM_PLANE_CAP_BG_COLOR            (1 << 10)
#define eCV5546_DRM_PLANE_CAP_BRIGHTNESS          (1 << 11)
#define eCV5546_DRM_PLANE_CAP_CONTRAST            (1 << 12)

enum ecv5546_plane_type {
	eCV5546_PLANE_TYPE_VPP0,
	eCV5546_PLANE_TYPE_OSD0,
	eCV5546_PLANE_TYPE_OSD1,
	eCV5546_PLANE_TYPE_OSD2,
	eCV5546_PLANE_TYPE_OSD3,
};

struct ecv5546_plane_region_alpha_info {
	int regionid;
	int alpha;
};

struct ecv5546_plane_region_color_keying_info {
	int regionid;
	int keying;
};

/*
 *  ecv5546_osd_header
 */
struct ecv5546_osd_header {
	u32 osd_header[8];
	u32 osd_rsv[24];
};

struct ecv5546_osd_region_info {
	u32 buf_width;
	u32 buf_height;
	u32 start_x;
	u32 start_y;
	u32 act_width;
	u32 act_height;
	u32 act_x;
	u32 act_y;
};

struct ecv5546_osd_alpha_info {
	u32 region_alpha_en;
	u32 region_alpha;
	u32 color_key_en;
	u32 color_key;
};

/*
 *  ecv5546 osd region include
 *  ecv5546_osd_header + ecv5546_osd_palette + bitmap
 */
struct ecv5546_osd_region {
	struct ecv5546_osd_region_info region_info;
	struct ecv5546_osd_alpha_info alpha_info;
	u32	color_mode;	/* osd color mode */
	u32	buf_num;	/* fix 2 */
	u32	buf_align;	/* fix 4096 */

	u32	buf_addr_phy;	/* buffer address physical */
	u8	*buf_addr_vir;	/* buffer address virtual */
	//palette addr in osd header
	u8	*hdr_pal;	/* palette address virtual */
	u32	buf_size;	/* buffer size */
	u32	buf_cur_id;	/* buffer current id */

	// SW latch
	u32	dirty_flag;	/* dirty flag */
	//other side palette addr, Gearing with swap buffer.
	u8	*pal;		/* palette address virtual, for swap buffer */

	struct ecv5546_osd_header *hdr;
	//structure size should be 32 alignment.
	u32	reserved[4];
};

struct ecv5546_plane_state {
	struct drm_plane_state base;
	bool scaling_adjustment_enable;
	unsigned int win_alpha;
	//struct ecv5546_plane_region_alpha_info region_alpha;
	struct ecv5546_plane_region_color_keying_info region_color_keying;
	unsigned int color_keying;
	unsigned int bg_color;
	unsigned int bg_format;
	unsigned int bg_alpha;
	int brightness;
	int contrast;
	struct ecv5546_osd_region info;
	/* updated by struct drm_display_mode adjusted_mode */
	int hdisplay;
	int vdisplay;
};

struct ecv5546_plane {
	struct drm_plane base;
	enum drm_plane_type type;
	u32 *pixel_formats;
	unsigned int num_pixel_formats;
	const struct drm_plane_helper_funcs *funcs;
	uint32_t capabilities;
	uint32_t zpos;
	uint32_t src_h_max, src_w_max;
	uint32_t scl_h_max, scl_w_max;
	uint32_t out_h_max, out_w_max;
	bool is_media_plane;
	struct drm_property *scaling_adjustment_property;
	struct drm_property *win_alpha_property;
	struct drm_property *region_color_keying_property;
	struct drm_property *color_keying_property;
	struct drm_property *background_format_property;
	struct drm_property *background_color_property;
	struct drm_property *background_alpha_property;
	struct drm_property *brightness_property;
	struct drm_property *contrast_property;
	struct drm_property_blob *region_color_keying_blob;
	u16 updated_alpha;
	unsigned int updated_win_alpha;
	//struct ecv5546_plane_region_alpha_info updated_region_alpha;
	struct ecv5546_plane_region_color_keying_info updated_region_color_keying;
	unsigned int updated_color_keying;

	struct ecv5546_plane_state *state;
	int osd_layer_sel; /* internal use, one of eCV5546_LAYER_XXX, ex.eCV5546_LAYER_OSD1 */

	int dmix_fg_sel; /* one of eCV5546_DMIX_XXX_SEL, ex.eCV5546_DMIX_OSD1_SEL */
	int dmix_layer;  /* one of eCV5546_DMIX_XX, ex.eCV5546_DMIX_L1 */
	int dtg_adjust;  /* tgen dtg adjust value, default value ex.eCV5546_DTG_ADJ_DMIX_L1 */
};

#define to_ecv5546_plane(plane) \
	container_of(plane, struct ecv5546_plane, base)

#define to_ecv5546_plane_state(state) \
		container_of(state, struct ecv5546_plane_state, base)

struct drm_plane *ecv5546_plane_init(struct drm_device *drm,
	enum drm_plane_type type, enum ecv5546_plane_type sptype, int init_zpos);
int ecv5546_plane_release(struct drm_device *drm, struct drm_plane *plane);
int ecv5546_plane_dev_suspend(struct device *dev, struct drm_plane *plane);
int ecv5546_plane_dev_resume(struct device *dev, struct drm_plane *plane);

#endif /* __EYS3D_eCV5546_DRM_PLANE_H__ */

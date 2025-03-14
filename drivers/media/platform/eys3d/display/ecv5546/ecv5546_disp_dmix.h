/* SPDX-License-Identifier: GPL-2.0 */
/*
 * eYs3D eCV5546 SoC Display driver for DMIX block
 *
 * Author: Hammer Hsieh <eys3d@eys3d.com>
 */
#ifndef __eCV5546_DISP_DMIX_H__
#define __eCV5546_DISP_DMIX_H__

/*DMIX_LAYER_CONFIG_0*/
#define eCV5546_DMIX_L6_FG_SEL(sel)	FIELD_PREP(GENMASK(31, 28), sel)
#define eCV5546_DMIX_L5_FG_SEL(sel)	FIELD_PREP(GENMASK(27, 24), sel)
#define eCV5546_DMIX_L4_FG_SEL(sel)	FIELD_PREP(GENMASK(23, 20), sel)
#define eCV5546_DMIX_L3_FG_SEL(sel)	FIELD_PREP(GENMASK(19, 16), sel)
#define eCV5546_DMIX_L2_FG_SEL(sel)	FIELD_PREP(GENMASK(15, 12), sel)
#define eCV5546_DMIX_L1_FG_SEL(sel)	FIELD_PREP(GENMASK(11, 8), sel)
#define eCV5546_DMIX_BG_FG_SEL(sel)	FIELD_PREP(GENMASK(7, 4), sel)
#define eCV5546_DMIX_VPP0_SEL		0x0
#define eCV5546_DMIX_VPP1_SEL		0x1
#define eCV5546_DMIX_VPP2_SEL		0x2
#define eCV5546_DMIX_OSD0_SEL		0x3
#define eCV5546_DMIX_OSD1_SEL		0x4
#define eCV5546_DMIX_OSD2_SEL		0x5
#define eCV5546_DMIX_OSD3_SEL		0x6
#define eCV5546_DMIX_PTG_SEL		0x7
/*DMIX_LAYER_CONFIG_1*/
#define eCV5546_DMIX_L6_MODE_SEL(sel)	FIELD_PREP(GENMASK(11, 10), sel)
#define eCV5546_DMIX_L5_MODE_SEL(sel)	FIELD_PREP(GENMASK(9, 8), sel)
#define eCV5546_DMIX_L4_MODE_SEL(sel)	FIELD_PREP(GENMASK(7, 6), sel)
#define eCV5546_DMIX_L3_MODE_SEL(sel)	FIELD_PREP(GENMASK(5, 4), sel)
#define eCV5546_DMIX_L2_MODE_SEL(sel)	FIELD_PREP(GENMASK(3, 2), sel)
#define eCV5546_DMIX_L1_MODE_SEL(sel)	FIELD_PREP(GENMASK(1, 0), sel)
#define eCV5546_DMIX_BLENDING		0x0
#define eCV5546_DMIX_TRANSPARENT		0x1
#define eCV5546_DMIX_OPACITY		0x2

/*DMIX_PTG_CONFIG_0*/
#define eCV5546_DMIX_PTG_COLOR_BAR_ROTATE	BIT(15)
#define eCV5546_DMIX_PTG_COLOR_BAR_SNOW		BIT(14)
#define eCV5546_DMIX_PTG_BORDER_PATTERN(sel)	FIELD_PREP(GENMASK(13, 12), sel)
#define eCV5546_DMIX_PTG_COLOR_BAR	0x0
#define eCV5546_DMIX_PTG_BORDER		0x2
#define eCV5546_DMIX_PTG_REGION		0x3
#define eCV5546_DMIX_PTG_BORDER_PIX(sel)		FIELD_PREP(GENMASK(2, 0), sel)
#define eCV5546_DMIX_PTG_BORDER_PIX_00	0x0
#define eCV5546_DMIX_PTG_BORDER_PIX_01	0x1
#define eCV5546_DMIX_PTG_BORDER_PIX_02	0x2
#define eCV5546_DMIX_PTG_BORDER_PIX_03	0x3
#define eCV5546_DMIX_PTG_BORDER_PIX_04	0x4
#define eCV5546_DMIX_PTG_BORDER_PIX_05	0x5
#define eCV5546_DMIX_PTG_BORDER_PIX_06	0x6
#define eCV5546_DMIX_PTG_BORDER_PIX_07	0x7

/*DMIX_PTG_CONFIG_1*/
#define eCV5546_DMIX_PTG_V_DOT_SET(sel)		FIELD_PREP(GENMASK(7, 4), sel)
#define eCV5546_DMIX_PTG_H_DOT_SET(sel)		FIELD_PREP(GENMASK(3, 0), sel)
#define eCV5546_DMIX_PTG_DOT_00	0x0
#define eCV5546_DMIX_PTG_DOT_01	0x1
#define eCV5546_DMIX_PTG_DOT_02	0x2
#define eCV5546_DMIX_PTG_DOT_03	0x3
#define eCV5546_DMIX_PTG_DOT_04	0x4
#define eCV5546_DMIX_PTG_DOT_05	0x5
#define eCV5546_DMIX_PTG_DOT_06	0x6
#define eCV5546_DMIX_PTG_DOT_07	0x7
#define eCV5546_DMIX_PTG_DOT_08	0x8
#define eCV5546_DMIX_PTG_DOT_09	0x9
#define eCV5546_DMIX_PTG_DOT_10	0xa
#define eCV5546_DMIX_PTG_DOT_11	0xb
#define eCV5546_DMIX_PTG_DOT_12	0xc
#define eCV5546_DMIX_PTG_DOT_13	0xd
#define eCV5546_DMIX_PTG_DOT_14	0xe
#define eCV5546_DMIX_PTG_DOT_15	0xf

/*DMIX_PTG_CONFIG_2*/
#define eCV5546_DMIX_PTG_BLACK		0x108080
#define eCV5546_DMIX_PTG_RED		0x4040f0
#define eCV5546_DMIX_PTG_GREEN		0x101010
#define eCV5546_DMIX_PTG_BLUE		0x29f06e

/*DMIX_SOURCE_SEL*/
#define eCV5546_DMIX_SOURCE_SEL(sel)		FIELD_PREP(GENMASK(2, 0), sel)
#define eCV5546_DMIX_TCON0_DMIX		0x2

/*DMIX_ADJUST_CONFIG_0*/
#define eCV5546_DMIX_LUMA_ADJ_EN		BIT(17)
#define eCV5546_DMIX_CRMA_ADJ_EN		BIT(16)

#define DMIX_LUMA_OFFSET_MIN	(-50)
#define DMIX_LUMA_OFFSET_MAX	(50)
#define DMIX_LUMA_SLOPE_MIN	(0.60)
#define DMIX_LUMA_SLOPE_MAX	(1.40)

/* for fg_sel */
#define eCV5546_DMIX_VPP0	0x0
#define eCV5546_DMIX_VPP1	0x1 //unsupported
#define eCV5546_DMIX_VPP2	0x2 //unsupported
#define eCV5546_DMIX_OSD0	0x3
#define eCV5546_DMIX_OSD1	0x4
#define eCV5546_DMIX_OSD2	0x5
#define eCV5546_DMIX_OSD3	0x6
#define eCV5546_DMIX_PTG		0x7

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
 * Init eCV5546 DMIX Setting
 */
void ecv5546_dmix_init(void);

/*
 * Show eCV5546 DMIX Info
 */
void ecv5546_dmix_decrypt_info(void);
void ecv5546_dmix_all_layer_info(void);
void ecv5546_dmix_layer_cfg_set(int layer_id);
void ecv5546_dmix_layer_cfg_store(void);
void ecv5546_dmix_layer_cfg_restore(void);

/*
 * eCV5546 DMIX PTG Settings
 */
void ecv5546_dmix_bist_info(void);
void ecv5546_dmix_ptg_set(int pattern_sel, int bg_color_yuv);

/*
 * eCV5546 DMIX Layer Settings
 */
void ecv5546_dmix_layer_init(int layer, int fg_sel, int layer_mode);
void ecv5546_dmix_layer_set(int fg_sel, int layer_mode);
void ecv5546_dmix_layer_info(int layer);

/*
 * eCV5546 DMIX Layer Alpha Setting
 */
void ecv5546_dmix_plane_alpha_config(int layer, int enable, int fix_alpha, int alpha_value);

/*
 * eCV5546 DMIX LUMA/CRMA Adjustment
 */
void ecv5546_dmix_color_adj_onoff(int enable);
int ecv5546_dmix_color_adj_luma_cp_set(const u8 *cp_src, const u8 *cp_sdt, u32 cp_size);
int ecv5546_dmix_color_adj_luma_slope_set(const u16 *slope, u32 slope_size);
int ecv5546_dmix_color_adj_luma_cp_get(u8 *cp_src, u8 *cp_sdt, u32 cp_size);
int ecv5546_dmix_color_adj_luma_slope_get(u16 *slope, u32 slope_size);
void ecv5546_dmix_color_adj_crma_set(u16 satcos, u16 satsin);
void ecv5546_dmix_color_adj_crma_get(u16 *satcos, u16 *satsin);

#endif	/* __eCV5546_DISP_DMIX_H__ */


/* SPDX-License-Identifier: GPL-2.0 */
/*
 * eYs3D eCV5546 SoC Display driver for TGEN block
 *
 * Author: Hammer Hsieh <eys3d@eys3d.com>
 */
#ifndef __eCV5546_DISP_TGEN_H__
#define __eCV5546_DISP_TGEN_H__

/*TGEN_RESET*/
#define eCV5546_TGEN_RESET		BIT(0)

/*TGEN_USER_INT1_CONFIG*/
#define eCV5546_TGEN_USER_INT1_CONFIG	GENMASK(11, 0)
/*TGEN_USER_INT2_CONFIG*/
#define eCV5546_TGEN_USER_INT2_CONFIG	GENMASK(11, 0)

/*TGEN_DTG_CONFIG*/
#define eCV5546_TGEN_FORMAT		GENMASK(10, 8)
#define eCV5546_TGEN_FORMAT_SET(fmt)	FIELD_PREP(GENMASK(10, 8), fmt)
#define eCV5546_TGEN_FORMAT_480P			0x0
#define eCV5546_TGEN_FORMAT_576P			0x1
#define eCV5546_TGEN_FORMAT_720P			0x2
#define eCV5546_TGEN_FORMAT_1080P		0x3
#define eCV5546_TGEN_FORMAT_64X64_360X100	0x6
#define eCV5546_TGEN_FORMAT_64X64_144X100	0x7
#define eCV5546_TGEN_FPS			GENMASK(5, 4)
#define eCV5546_TGEN_FPS_SET(fps)	FIELD_PREP(GENMASK(5, 4), fps)
#define eCV5546_TGEN_FPS_59P94HZ		0x0
#define eCV5546_TGEN_FPS_50HZ		0x1
#define eCV5546_TGEN_FPS_24HZ		0x2
#define eCV5546_TGEN_USER_MODE		BIT(0)
#define eCV5546_TGEN_INTERNAL		0x0
#define eCV5546_TGEN_USER_DEF		0x1

/*TGEN_DTG_ADJUST1*/
#define eCV5546_TGEN_DTG_ADJ_MASKA	GENMASK(13, 8)
#define eCV5546_TGEN_DTG_ADJ_MASKB	GENMASK(5, 0)
#define eCV5546_TGEN_DTG_ADJ1_VPP0(adj)	FIELD_PREP(GENMASK(13, 8), adj)
#define eCV5546_TGEN_DTG_ADJ1_VPP1(adj)	FIELD_PREP(GENMASK(5, 0), adj)
/*TGEN_DTG_ADJUST2*/
#define eCV5546_TGEN_DTG_ADJ2_OSD3(adj)	FIELD_PREP(GENMASK(13, 8), adj)
#define eCV5546_TGEN_DTG_ADJ2_OSD2(adj)	FIELD_PREP(GENMASK(5, 0), adj)
/*TGEN_DTG_ADJUST3*/
#define eCV5546_TGEN_DTG_ADJ3_OSD1(adj)	FIELD_PREP(GENMASK(13, 8), adj)
#define eCV5546_TGEN_DTG_ADJ3_OSD0(adj)	FIELD_PREP(GENMASK(5, 0), adj)
/*TGEN_DTG_ADJUST4*/
#define eCV5546_TGEN_DTG_ADJ4_PTG(adj)	FIELD_PREP(GENMASK(13, 8), adj)
#define eCV5546_TGEN_DTG_ADJ4_VPP2(adj)	FIELD_PREP(GENMASK(5, 0), adj)

/* for tgen_input_adj */
#define eCV5546_TGEN_DTG_ADJ_VPP0	0x0
#define eCV5546_TGEN_DTG_ADJ_VPP1	0x1
#define eCV5546_TGEN_DTG_ADJ_VPP2	0x2
#define eCV5546_TGEN_DTG_ADJ_OSD0	0x3
#define eCV5546_TGEN_DTG_ADJ_OSD1	0x4
#define eCV5546_TGEN_DTG_ADJ_OSD2	0x5
#define eCV5546_TGEN_DTG_ADJ_OSD3	0x6
#define eCV5546_TGEN_DTG_ADJ_PTG		0x7
#define eCV5546_TGEN_DTG_ADJ_ALL		0x8

struct ecv5546_tgen_timing {
	int usr;
	int fps;
	int fmt;
	u16 htt;
	u16 vtt;
	u16 hact;
	u16 vact;
	u16 vbp;
};

/*
 * Init eCV5546 TGEN Setting
 */
void ecv5546_tgen_init(void);

/*
 * Show eCV5546 TGEN Info
 */
void ecv5546_tgen_decrypt_info(void);
void ecv5546_tgen_resolution_chk(void);


/*
 * eCV5546 TGEN Timing Generator Settings
 */
void ecv5546_tgen_reset(void);
void ecv5546_tgen_set_user_int1(u32 count);
void ecv5546_tgen_set_user_int2(u32 count);
u32 ecv5546_tgen_get_current_line_count(void);

void ecv5546_tgen_timing_set_dsi(void);
void ecv5546_tgen_timing_set_csi(void);
void ecv5546_tgen_timing_get(void);

/*
 * eCV5546 TGEN Timing Adjust Settings
 */
void ecv5546_tgen_input_adjust(int tgen_input_adj, u32 adj_value);

/*
 * eCV5546 TGEN register store/restore
 */
void ecv5546_tgen_store(void);
void ecv5546_tgen_restore(void);

#endif	//__eCV5546_DISP_TGEN_H__


/* SPDX-License-Identifier: GPL-2.0 */
/*
 * eYs3D eCV5546 SoC Display driver for TCON block
 *
 * Author: Hammer Hsieh <eys3d@eys3d.com>
 */
#ifndef __eCV5546_DISP_TCON_H__
#define __eCV5546_DISP_TCON_H__

/*TCON_TCON0*/
#define eCV5546_TCON_DOT_RGB888_MASK		BIT(15)
#define eCV5546_TCON_DOT_RGB666	                0x0
#define eCV5546_TCON_DOT_RGB888	                0x1
#define eCV5546_TCON_DOT3_BITREV		        BIT(14)
#define eCV5546_TCON_DOT2_BITREV		        BIT(13)
#define eCV5546_TCON_DOT1_BITREV		        BIT(12)
#define eCV5546_TCON_DOT_ORDER_MASK		GENMASK(11, 9)
#define eCV5546_TCON_DOT_ORDER_SET(ord)	        FIELD_PREP(GENMASK(11, 9), ord)
#define eCV5546_TCON_DOT_ORDER_RGB		0x0
#define eCV5546_TCON_DOT_ORDER_RBG		0x1
#define eCV5546_TCON_DOT_ORDER_GBR		0x2
#define eCV5546_TCON_DOT_ORDER_GRB		0x3
#define eCV5546_TCON_DOT_ORDER_BRG		0x4
#define eCV5546_TCON_DOT_ORDER_BGR		0x5
#define eCV5546_TCON_DITHER_YUV_EN		BIT(6)
#define eCV5546_TCON_OUT_PACKAGE_MASK		GENMASK(5, 3)
#define eCV5546_TCON_OUT_PACKAGE_SET(fmt)	FIELD_PREP(GENMASK(5, 3), fmt)
#define eCV5546_TCON_OUT_PACKAGE_DSI_RGB565	0x0
#define eCV5546_TCON_OUT_PACKAGE_CSI_RGB565	0x1
#define eCV5546_TCON_OUT_PACKAGE_CSI_YUY2	0x2
#define eCV5546_TCON_OUT_PACKAGE_DSI_RGB666_18	0x3
#define eCV5546_TCON_OUT_PACKAGE_DSI_RGB888	0x4
#define eCV5546_TCON_OUT_PACKAGE_DSI_RGB666_24	0x5
#define eCV5546_TCON_OUT_PACKAGE_CSI_RGB888	0x6
#define eCV5546_TCON_HVIF_EN		        BIT(1)
#define eCV5546_TCON_YU_SWAP		        BIT(0)

/*TCON_TCON1*/
#define eCV5546_TCON_EN		                BIT(15)
#define eCV5546_TCON_INV_POL		        BIT(14)
#define eCV5546_TCON_CSC_MODE		        BIT(11)
#define eCV5546_TCON_CSC_MODE_RGB_TO_BT601	0x0
#define eCV5546_TCON_CSC_MODE_RGB_TO_BT709	0x1
#define eCV5546_TCON_DE_VACT_EN		        BIT(10)
#define eCV5546_TCON_DT_MODE		        GENMASK(9, 8)
#define eCV5546_TCON_VMODE_EN		        BIT(7)
#define eCV5546_TCON_ALINE_MNL_EN		BIT(6)
#define eCV5546_TCON_STVU_MNL_EN		        BIT(5)
#define eCV5546_TCON_YUV_UV_SWAP		        BIT(4)
#define eCV5546_TCON_TB_VCOM_POL		        BIT(3)
#define eCV5546_TCON_TB_VCOM_EN		        BIT(2)
#define eCV5546_TCON_STHLR_DLY_MASK		GENMASK(1, 0)
#define eCV5546_TCON_STHLR_DLY_SET(dly)	        FIELD_PREP(GENMASK(1, 0), dly)
#define eCV5546_TCON_STHLR_DLY_0T	        0x0
#define eCV5546_TCON_STHLR_DLY_1T	        0x1
#define eCV5546_TCON_STHLR_DLY_2T	        0x2
#define eCV5546_TCON_STHLR_DLY_3T	        0x3

/*TCON_TCON2*/
#define eCV5546_TCON_CPH1_POL		        BIT(2)
#define eCV5546_TCON_HDS_FILTER		        BIT(0)

/*TCON_TCON3*/
#define eCV5546_TCON_DP_RGB_OFF		        BIT(11)
#define eCV5546_TCON_STHLR_POL		        BIT(5)
#define eCV5546_TCON_STVU_POL		        BIT(4)
#define eCV5546_TCON_CKV_POL		        BIT(3)
#define eCV5546_TCON_OEV_POL		        BIT(2)
#define eCV5546_TCON_OEH_POL		        BIT(1)
#define eCV5546_TCON_VCOM_POL		        BIT(0)

/*TCON_TCON4*/
#define eCV5546_TCON_PIX_EN_SEL_MASK		GENMASK(3, 0)
#define eCV5546_TCON_PIX_EN_SEL_SET(sel)	        FIELD_PREP(GENMASK(3, 0), sel)
#define eCV5546_TCON_PIX_EN_DIV_1_CLK_TCON	0x0
#define eCV5546_TCON_PIX_EN_DIV_2_CLK_TCON	0x1
#define eCV5546_TCON_PIX_EN_DIV_3_CLK_TCON	0x2
#define eCV5546_TCON_PIX_EN_DIV_4_CLK_TCON	0x3
#define eCV5546_TCON_PIX_EN_DIV_6_CLK_TCON	0x4
#define eCV5546_TCON_PIX_EN_DIV_8_CLK_TCON	0x5
#define eCV5546_TCON_PIX_EN_DIV_12_CLK_TCON	0x6
#define eCV5546_TCON_PIX_EN_DIV_16_CLK_TCON	0x7
#define eCV5546_TCON_PIX_EN_DIV_24_CLK_TCON	0x8
#define eCV5546_TCON_PIX_EN_DIV_32_CLK_TCON	0x9

/*TCON_TCON5*/
#define eCV5546_TCON_CHK_SUM_ACTIVE_EN		BIT(5)
#define eCV5546_TCON_SUR_SEL		        BIT(4)
#define eCV5546_TCON_AFIFO_DIS		        BIT(3)
#define eCV5546_TCON_CHK_SUM_EN		        BIT(2)
#define eCV5546_TCON_COLOR_SPACE_DIS		BIT(1)
#define eCV5546_TCON_COLOR_SPACE_SEL		BIT(0)
#define eCV5546_TCON_COLOR_SPACE_BT601	        0x0
#define eCV5546_TCON_COLOR_SPACE_BT709	        0x1

/*TCON_TPG_CTRL*/
#define eCV5546_TCON_TPG_PATTERN			GENMASK(5, 2)
#define eCV5546_TCON_TPG_PATTERN_SET(pat)	FIELD_PREP(GENMASK(5, 2), pat)
#define eCV5546_TCON_TPG_PATTERN_H_1_BAR		0x0
#define eCV5546_TCON_TPG_PATTERN_H_2_RAMP	0x1
#define eCV5546_TCON_TPG_PATTERN_H_3_ODD		0x2
#define eCV5546_TCON_TPG_PATTERN_V_1_BAR		0x3
#define eCV5546_TCON_TPG_PATTERN_V_2_RAMP	0x4
#define eCV5546_TCON_TPG_PATTERN_V_3_ODD		0x5
#define eCV5546_TCON_TPG_PATTERN_HV_1_CHK	0x6
#define eCV5546_TCON_TPG_PATTERN_HV_2_FRAME	0x7
#define eCV5546_TCON_TPG_PATTERN_HV_3_MOI_A	0x8
#define eCV5546_TCON_TPG_PATTERN_HV_4_MOI_B	0x9
#define eCV5546_TCON_TPG_PATTERN_HV_5_CONTR	0xa
#define eCV5546_TCON_TPG_MODE			GENMASK(1, 0)
#define eCV5546_TCON_TPG_MODE_SET(mod)		FIELD_PREP(GENMASK(1, 0), mod)
#define eCV5546_TCON_TPG_MODE_NORMAL		0x0
#define eCV5546_TCON_TPG_MODE_INTERNAL		0x1
#define eCV5546_TCON_TPG_MODE_EXTERNAL		0x2

/* TCON_GAMMA0_CTRL && TCON_GAMMA1_CTRL  && TCON_GAMMA2_CTRL */
#define eCV5546_TCON_GM_UPD_SCHEME                  BIT(15)
#define eCV5546_TCON_GM_S2D_ODD_ORDER_MASK          GENMASK(14, 12)
#define eCV5546_TCON_GM_S2D_ODD_ORDER_SET(order)    FIELD_PREP(GENMASK(14, 12), order)
#define eCV5546_TCON_GM_S2D_EVEN_ORDER_MASK         GENMASK(11, 9)
#define eCV5546_TCON_GM_S2D_EVEN_ORDER_SET(order)   FIELD_PREP(GENMASK(11, 9), order)
#define eCV5546_TCON_GM_S2D_EN                      BIT(8)
#define eCV5546_TCON_GM_BYPASS                      BIT(5)
#define eCV5546_TCON_GM_UPDWE                       BIT(4)
#define eCV5546_TCON_GM_UPDDEL_RGB_MASK             GENMASK(3, 2)
#define eCV5546_TCON_GM_UPDDEL_RGB_SET(sel)         FIELD_PREP(GENMASK(3, 2), sel)
#define eCV5546_TCON_GM_UPDDEL_RGB_ALL              0x0
#define eCV5546_TCON_GM_UPDDEL_RGB_R                0x1
#define eCV5546_TCON_GM_UPDDEL_RGB_G                0x2
#define eCV5546_TCON_GM_UPDDEL_RGB_B                0x3
#define eCV5546_TCON_GM_UPDEN                       BIT(1)
#define eCV5546_TCON_GM_EN                          BIT(0)

/* TCON_RGB_ADJ_CTRL */
#define eCV5546_TCON_RGB_ADJ_B_EN                  BIT(2)
#define eCV5546_TCON_RGB_ADJ_G_EN                  BIT(1)
#define eCV5546_TCON_RGB_ADJ_R_EN                  BIT(0)
#define eCV5546_TCON_RGB_ADJ_CHANNEL_R_EN          1
#define eCV5546_TCON_RGB_ADJ_CHANNEL_G_EN          2
#define eCV5546_TCON_RGB_ADJ_CHANNEL_B_EN          4
/* TCON_RGB_ADJ_CONTROL_POINT */
#define eCV5546_TCON_RGB_ADJ_CP_SDT_MASK          GENMASK(15, 8)
#define eCV5546_TCON_RGB_ADJ_CP_SDT_SET(val)      FIELD_PREP(GENMASK(15, 8), val)
#define eCV5546_TCON_RGB_ADJ_CP_SRC_MASK          GENMASK(7, 0)
#define eCV5546_TCON_RGB_ADJ_CP_SRC_SET(val)      FIELD_PREP(GENMASK(7, 0), val)
#define eCV5546_TCON_RGB_ADJ_CP_SIZE              3
/* TCON_RGB_ADJ_SLOPE */
#define eCV5546_TCON_RGB_ADJ_SLOPE_SIZE           4

/* TCON_TC_DITHER_TVOUT */
#define eCV5546_TCON_DITHER_PANEL_DOT_MODE_MASK         GENMASK(9, 8)
#define eCV5546_TCON_DITHER_PANEL_DOT_MODE_SET(mode)    FIELD_PREP(GENMASK(9, 8), mode)
#define eCV5546_TCON_DITHER_PANEL_DOT_MODE_1DOT         0
#define eCV5546_TCON_DITHER_PANEL_DOT_MODE_H2DOT        1
#define eCV5546_TCON_DITHER_PANEL_DOT_MODE_V2DOT        2
#define eCV5546_TCON_DITHER_PANEL_DOT_MODE_2DOT         3
#define eCV5546_TCON_DITHER_565_EN                      BIT(7)
#define eCV5546_TCON_DITHER_6BIT_MODE                   BIT(6)
#define eCV5546_TCON_DITHER_6BIT_MODE_MATCHED           0
#define eCV5546_TCON_DITHER_6BIT_MODE_ROBUST            1
#define eCV5546_TCON_DITHER_6BIT_TABLE_V_SHIFT_EN       BIT(5)
#define eCV5546_TCON_DITHER_6BIT_TABLE_H_SHIFT_EN       BIT(4)
#define eCV5546_TCON_DITHER_NEW_EN                      BIT(3)
#define eCV5546_TCON_DITHER_TEMP_EN                     BIT(2)
#define eCV5546_TCON_DITHER_INIT_MODE                   BIT(1)
#define eCV5546_TCON_DITHER_INIT_MODE_METHOD2           0
#define eCV5546_TCON_DITHER_INIT_MODE_METHOD1           1
#define eCV5546_TCON_DITHER_RGBC_SEL                    BIT(0)
#define eCV5546_TCON_DITHER_RGBC_SEL_R                  0
#define eCV5546_TCON_DITHER_RGBC_SEL_RGB                1

/* TCON_BIT_SWAP_CTRL */
#define eCV5546_TCON_BIT_SW_ON                  BIT(0)
/* bit mode */
#define eCV5546_TCON_BIT_SW_BIT_MSB                  0
#define eCV5546_TCON_BIT_SW_BIT_LSB                  1
/* channel mode */
#define eCV5546_TCON_BIT_SW_CHNL_RGB                 0
#define eCV5546_TCON_BIT_SW_CHNL_RBG                 1
#define eCV5546_TCON_BIT_SW_CHNL_GBR                 2
#define eCV5546_TCON_BIT_SW_CHNL_GRB                 3
#define eCV5546_TCON_BIT_SW_CHNL_BRG                 4
#define eCV5546_TCON_BIT_SW_CHNL_BGR                 5


/*
 * Init eCV5546 TCON Setting
 */
void ecv5546_tcon_init(void);

/*
 * Show eCV5546 TCON Info
 */
void ecv5546_tcon_decrypt_info(void);
void ecv5546_tcon_resolution_chk(void);

/*
 * eCV5546 TCON BIST Settings
 */
void ecv5546_tcon_bist_info(void);
void ecv5546_tcon_bist_set(int bist_mode, int tcon_bist_pat);

/*
 * eCV5546 TCON GEN Settings
 */
void ecv5546_tcon_gen_pix_set(int enable);

/*
 * eCV5546 TCON Timing Control Settings
 */
void ecv5546_tcon_timing_set_dsi(void);
void ecv5546_tcon_timing_set_csi(void);
void ecv5546_tcon_timing_get(void);

/*
 * eCV5546 TCON GAMMA Settings
 */
void ecv5546_tcon_gamma_table_set(u32 updsel_rgb, const u16 *table, u32 tablesize);
void ecv5546_tcon_gamma_table_get(u32 updsel_rgb, u16 *table, u32 tablesize);
void ecv5546_tcon_gamma_table_enable(int enable);

/*
 * eCV5546 TCON RGB Adjust Settings
 */
int ecv5546_tcon_rgb_adjust_cp_set(u32 channel_sel, const u8 *cp_src, const u8 *cp_sdt, u32 cp_size);
int ecv5546_tcon_rgb_adjust_slope_set(u32 channel_sel, const u16 *slope, u32 slope_size);
int ecv5546_tcon_rgb_adjust_cp_get(u32 channel_sel, u8 *cp_src, u8 *cp_sdt, u32 cp_size);
int ecv5546_tcon_rgb_adjust_slope_get(u32 channel_sel, u16 *slope, u32 slope_size);
void ecv5546_tcon_rgb_adjust_enable(u32 channel_sel);

/*
 * eCV5546 Dither Settings
 */
void ecv5546_tcon_enhanced_dither_6bit_set(u32 mode, u32 table_v_shift_en, u32 table_h_shift_en);
void ecv5546_tcon_enhanced_dither_8bit_set(void);
void ecv5546_tcon_enhanced_dither_set(u32 rgbc_sel, u32 method, u32 temporal_mode_en, u32 dot_mode);
void ecv5546_tcon_enhanced_dither_enable(u32 enable);

/*
 * eCV5546 TCON bitswap Settings
 */
int ecv5546_tcon_bitswap_set(int bit_mode, int channel_mode);
void ecv5546_tcon_bitswap_enable(int enable);


/*
 * eCV5546 TCON register store/restore
 */
void ecv5546_tcon_store(void);
void ecv5546_tcon_restore(void);

#endif	//__eCV5546_DISP_TCON_H__

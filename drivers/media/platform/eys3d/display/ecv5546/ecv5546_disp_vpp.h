/* SPDX-License-Identifier: GPL-2.0 */
/*
 * eYs3D eCV5546 SoC Display driver for VPP layer
 *
 * Author: Hammer Hsieh <eys3d@eys3d.com>
 */
#ifndef __eCV5546_DISP_VPP_H__
#define __eCV5546_DISP_VPP_H__

/*IMGREAD_GLOBAL_CONTROL*/
#define eCV5546_VPP_IMGREAD_FETCH_EN             BIT(31)

/*IMGREAD_CONFIG*/
#define eCV5546_VPP_IMGREAD_HDS_LPF              BIT(24)
#define eCV5546_VPP_IMGREAD_CRMA_REPEAT          BIT(20)
#define eCV5546_VPP_IMGREAD_DATA_FMT             GENMASK(18, 16)
#define eCV5546_VPP_IMGREAD_DATA_FMT_SEL(fmt)    FIELD_PREP(GENMASK(18, 16), fmt)
#define eCV5546_VPP_IMGREAD_DATA_FMT_UYVY        0x1
#define eCV5546_VPP_IMGREAD_DATA_FMT_YUY2        0x2
#define eCV5546_VPP_IMGREAD_DATA_FMT_NV16	0x3
#define eCV5546_VPP_IMGREAD_DATA_FMT_NV24	0x6
#define eCV5546_VPP_IMGREAD_DATA_FMT_NV12	0x7
#define eCV5546_VPP_IMGREAD_YC_SWAP              BIT(12)
#define eCV5546_VPP_IMGREAD_UV_SWAP              BIT(11)
#define eCV5546_VPP_IMGREAD_UPDN_FLIP            BIT(7)
#define eCV5546_VPP_IMGREAD_BIST_MASK            GENMASK(5, 4)
#define eCV5546_VPP_IMGREAD_BIST_EN              BIT(5)
#define eCV5546_VPP_IMGREAD_BIST_MODE            BIT(4)
#define eCV5546_VPP_IMGREAD_FM_MODE              BIT(1)
#define eCV5546_VPP_IMGREAD_FIELD_ID             BIT(0)

/*VSCL_CONFIG2*/
#define eCV5546_VPP_VSCL_BUFR_BW_LIMIT	        BIT(9)

#define eCV5546_VPP_VSCL_BIST_MASK	        GENMASK(8, 7)
#define eCV5546_VPP_VSCL_BIST_MODE	        BIT(8)
#define eCV5546_VPP_VSCL_BIST_EN		        BIT(7)

#define eCV5546_VPP_VSCL_CHKSUM_EN	        BIT(6)
#define eCV5546_VPP_VSCL_BUFR_EN		        BIT(4)

#define eCV5546_VPP_VSCL_VINT_EN		        BIT(3)
#define eCV5546_VPP_VSCL_HINT_EN		        BIT(2)
#define eCV5546_VPP_VSCL_DCTRL_EN	        BIT(1)
#define eCV5546_VPP_VSCL_ACTRL_EN	        BIT(0)

/*VSCL_HINT_CTRL*/
#define eCV5546_VPP_VSCL_HINT_FLT_EN	        BIT(1)
/*VSCL_HINT_HFACTOR_HIGH*/
#define eCV5546_VPP_VSCL_HINT_HFACTOR_HIGH	GENMASK(8, 0)
/*VSCL_HINT_HFACTOR_LOW*/
#define eCV5546_VPP_VSCL_HINT_HFACTOR_LOW	GENMASK(15, 0)
/*VSCL_HINT_INITF_HIGH*/
#define eCV5546_VPP_VSCL_HINT_INITF_PN	        BIT(6)
#define eCV5546_VPP_VSCL_HINT_INITF_HIGH	        GENMASK(5, 0)
/*VSCL_HINT_INITF_LOW*/
#define eCV5546_VPP_VSCL_HINT_INITF_LOW	        GENMASK(15, 0)

/*VSCL_VINT_CTRL*/
#define eCV5546_VPP_VSCL_VINT_FLT_EN	        BIT(1)
/*VSCL_VINT_VFACTOR_HIGH*/
#define eCV5546_VPP_VSCL_VINT_VFACTOR_HIGH	GENMASK(8, 0)
/*VSCL_VINT_VFACTOR_LOW*/
#define eCV5546_VPP_VSCL_VINT_VFACTOR_LOW	GENMASK(15, 0)
/*VSCL_VINT_INITF_HIGH*/
#define eCV5546_VPP_VSCL_VINT_INITF_PN	        BIT(6)
#define eCV5546_VPP_VSCL_VINT_INITF_HIGH	        GENMASK(5, 0)
/*VSCL_VINT_INITF_LOW*/
#define eCV5546_VPP_VSCL_VINT_INITF_LOW	        GENMASK(15, 0)

/*VPOST_CONFIG*/
#define eCV5546_VPP_VPOST_ADJ_EN	                BIT(1)
#define eCV5546_VPP_VPOST_CSPACE_EN	        BIT(2)
#define eCV5546_VPP_VPOST_OPIF_EN	        BIT(3)
/*VPOST_CSPACE_CONFIG*/
#define eCV5546_VPP_VPOST_CSPACE_MASK            GENMASK(2, 0)
#define eCV5546_VPP_VPOST_CSPACE_BYPASS          0x0
#define eCV5546_VPP_VPOST_CSPACE_601_TO_709      0x1
#define eCV5546_VPP_VPOST_CSPACE_709_TO_601      0x2
#define eCV5546_VPP_VPOST_CSPACE_JPG_TO_709      0x3
#define eCV5546_VPP_VPOST_CSPACE_JPG_TO_601      0x4
/*VPOST_OPIF_CONFIG*/
#define eCV5546_VPP_VPOST_MODE_MASK              GENMASK(5, 4)
#define eCV5546_VPP_VPOST_MODE_COLORBAR          0x0
#define eCV5546_VPP_VPOST_MODE_BORDER            0x1
#define eCV5546_VPP_VPOST_MODE_BGCOLOR           0x2
#define eCV5546_VPP_VPOST_MODE_GRAY_FADING       0x3
#define eCV5546_VPP_VPOST_WIN_ALPHA_EN	        BIT(1)
#define eCV5546_VPP_VPOST_WIN_YUV_EN	        BIT(0)
/*VPOST_OPIF_ALPHA*/
#define eCV5546_VPP_VPOST_WIN_ALPHA_MASK         GENMASK(15, 8)
#define eCV5546_VPP_VPOST_WIN_ALPHA_SET(val)     FIELD_PREP(GENMASK(15, 8), val)
#define eCV5546_VPP_VPOST_VPP_ALPHA_MASK         GENMASK(7, 0)
#define eCV5546_VPP_VPOST_VPP_ALPHA_SET(val)     FIELD_PREP(GENMASK(7, 0), val)
/*VPOST_OPIF_MSKTOP*/
#define eCV5546_VPP_VPOST_OPIF_TOP_MASK          GENMASK(11, 0)
#define eCV5546_VPP_VPOST_OPIF_TOP_SET(val)      FIELD_PREP(GENMASK(11, 0), val)
/*VPOST_OPIF_MSKBOT*/
#define eCV5546_VPP_VPOST_OPIF_BOT_MASK          GENMASK(11, 0)
#define eCV5546_VPP_VPOST_OPIF_BOT_SET(val)      FIELD_PREP(GENMASK(11, 0), val)
/*VPOST_OPIF_MSKLEFT*/
#define eCV5546_VPP_VPOST_OPIF_LEFT_MASK         GENMASK(12, 0)
#define eCV5546_VPP_VPOST_OPIF_LEFT_SET(val)     FIELD_PREP(GENMASK(12, 0), val)
/*VPOST_OPIF_MSKRIGHT*/
#define eCV5546_VPP_VPOST_OPIF_RIGHT_MASK        GENMASK(12, 0)
#define eCV5546_VPP_VPOST_OPIF_RIGHT_SET(val)    FIELD_PREP(GENMASK(12, 0), val)

/*
 * Init eCV5546 VPP Setting
 */
void ecv5546_vpp_init(void);

/*
 * Show eCV5546 VPP Info
 */
void ecv5546_vpp_decrypt_info(void);
void ecv5546_vpp_imgread_resolution_chk(void);
void ecv5546_vpp_vscl_resolution_chk(void);

/*
 * eCV5546 VPP BIST Settings
 */
void ecv5546_vpp_bist_info(void);
void ecv5546_vpp_bist_set(int img_vscl_sel, int bist_en, int vpp_bist_type);

/*
 * eCV5546 VPP Layer Settings
 */
void ecv5546_vpp_layer_onoff(int onoff);
#if 1//def SP_DISP_VPP_SCALE_NEW
int ecv5546_vpp_imgread_set(u32 data_addr1, int x, int y, int img_src_w, int img_src_h, int input_w, int input_h, int yuv_fmt);
int ecv5546_vpp_vscl_set(int x, int y, int img_src_w, int img_src_h, int img_dest_x,int img_dest_y, int img_dest_w, int img_dest_h, int output_w, int output_h);

#else
int ecv5546_vpp_imgread_set(u32 data_addr1, int x, int y, int w, int h, int yuv_fmt);
int ecv5546_vpp_vscl_set(int x, int y, int xlen, int ylen, int img_dest_x,int img_dest_y, int input_w, int input_h, int output_w, int output_h);
#endif
int ecv5546_vpp_vpost_set(int x, int y, int input_w, int input_h, int output_w, int output_h);
int ecv5546_vpp_vpost_opif_set(int act_x, int act_y, int act_w, int act_h, int output_w, int output_h);
int ecv5546_vpp_vpost_opif_alpha_set(int alpha, int mask_alpha);

/*
 * eCV5546 VPP register store/restore
 */
void ecv5546_vpp0_store(void);
void ecv5546_vpp0_restore(void);

#endif	//__eCV5546_DISP_VPP_H__


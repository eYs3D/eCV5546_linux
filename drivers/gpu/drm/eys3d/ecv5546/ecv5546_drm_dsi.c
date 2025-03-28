// SPDX-License-Identifier: GPL-2.0+
/*
 * eYs3D eCV5546 SoC DRM CRTCs and Encoder/Connecter
 *
 * Author: dx.jiang<eys3d@eys3d.com>
 *         hammer.hsieh<eys3d@eys3d.com>
 */

#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/completion.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_edid.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>
#include <drm/drm_print.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include "ecv5546_drm_drv.h"
#include "ecv5546_drm_crtc.h"
#include "ecv5546_drm_dsi.h"
#include "ecv5546_drm_regs.h"

#define DSI_PFORMAT_RGB565          0
#define DSI_PFORMAT_RGB666_PACKED   1
#define DSI_PFORMAT_RGB666          2
#define DSI_PFORMAT_RGB888          3

static const char * const ecv5546_dsi_fmt[] = {
	"DSI_PFORMAT_RGB565", "DSI_PFORMAT_RGB666_PACKED",
	"DSI_PFORMAT_RGB666", "DSI_PFORMAT_RGB888",
};

#define MIPITX_CMD_FIFO_FULL   0x00000001
#define MIPITX_CMD_FIFO_EMPTY  0x00000010
#define MIPITX_DATA_FIFO_FULL  0x00000100
#define MIPITX_DATA_FIFO_EMPTY 0x00001000

static const struct debugfs_reg32 sp_dsi_host_g0_regs[] = {
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_00),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_01),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_02),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_03),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_04),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_05),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_06),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_07),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_08),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_09),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_10),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_11),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_12),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_13),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_14),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_15),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_16),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_17),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_18),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_19),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_20),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_21),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_22),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_23),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_24),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_25),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_26),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_27),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_28),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_29),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_30),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G0_G204_31),
};

static const struct debugfs_reg32 sp_dsi_host_g1_regs[] = {
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_00),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_01),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_02),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_03),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_04),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_05),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_06),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_07),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_08),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_09),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_10),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_11),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_12),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_13),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_14),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_15),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_16),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_17),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_18),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_19),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_20),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_21),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_22),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_23),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_24),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_25),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_26),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_27),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_28),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_29),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_30),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_G1_G205_31),
};

static const struct debugfs_reg32 sp_dsi_ao_moon3_regs[] = {
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_00),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_01),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_02),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_03),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_04),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_05),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_06),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_07),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_08),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_09),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_10),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_11),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_12),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_13),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_14),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_15),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_16),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_17),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_18),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_19),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_20),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_21),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_22),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_23),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_24),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_25),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_26),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_27),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_28),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_29),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_30),
	eCV5546_DRM_REG32(eCV5546_DISP_MIPITX_AO_MOON3_31),
};

/* display MIPITX Sync-Timing Parameters Setting.
 * Formula:
 *   hsa = hsync_end-hsync_start
 *   hbp = htotal-hsync_end
 *   hact = hdisplay
 *   vsa = vsync_end-vsync_start
 *   vfp = vsync_start-vdisplay
 *   vbp = vtotal-vsync_end
 *   vact = vdisplay
 *
 * Notes: hsync_end,hsync_start,htotal,vsync_end,vsync_start,vtotal,vdisplay
 *   are derived from drm_display_mode.
 */
struct ecv5546_mpitx_sync_timing_param {
	u32 hsa;
	u32 hbp;
	u32 hact;
	u32 vsa;
	u32 vfp;
	u32 vbp;
	u32 vact;
};

/* display MIPITX Lane Clock and TXPLL Parameters Setting.
 * MIPI Lane Clock Formula:
 *                   pixel_clock * pixel_bits
 *   lane_clock = -----------------------------
 *                         data_lanes
 *
 * TXPLL Clock Formula:
 *                    25 * prescal * fbkdiv
 *   lane_clock = -----------------------------
 *                 prediv * postdiv * 5^en_div5
 *  Fvco = (25 * prescal * fbkdiv) / prediv
 *  lane_clock = Fvco / (postdiv * 5^en_div5)
 *  lane_divider = pixel_bits / data_lanes
 *==>
 *  lane_clock = pixel_clock * pixel_bits / data_lanes
 *             = pixel_clock * lane_divider
 *  Fvco = lane_clock * txpll_postdiv * 5^txpll_endiv5
 *  txpll_fbkdiv = Fvco * txpll_prediv / (25 * txpll_prescal)
 *
 * PreSetting-Rule:
 *   txpll_prescal=1
 *   txpll_prediv =1
 *   lane_clock = [80, 150)MHz, txpll_endiv5=1, txpll_postdiv=2
 *   lane_clock = [150,375)MHz, txpll_endiv5=0, txpll_postdiv=4
 *   lane_clock = [375,1500)MHz, txpll_endiv5=0, txpll_postdiv=1
 *   Fvco = [ 320,  640]MHz, txpll_bnksel=0
 *   Fvco = [ 640, 1000]MHz, txpll_bnksel=1
 *   Fvco = [1000, 1200]MHz, txpll_bnksel=2
 *   Fvco = [1200, 1500]MHz, txpll_bnksel=3
 *
 * Register Setting Formula:
 *   txpll_prescal= PRESCAL[4]+1 = {1, 2}
 *   txpll_prediv = map{PREDIV[1:0]} = {1, 2, 5, 8}
 *   txpll_postdiv= map{POSTDIV[18:16]} = {1, 2, 4, 8, 16}
 *   txpll_endiv5 = EN_DIV5[20] = {0, 1}
 *   txpll_fbkdiv = FBK_DIV[13:8] = [3, 63];
 *   txpll_bnksel = BNKSEL[2:0] = [0, 3]
 *==>
 *   PRESCAL[4]  = txpll_prescal-1
 *   PREDIV[1:0] = 3 for txpll_prediv = 8
 *   PREDIV[1:0] = txpll_prediv / 2
 *   POSTDIV[18:16] = log2(txpll_postdiv)
 *   EN_DIV5[20]   = txpll_endiv5
 *   FBKDIV[13:8] = fbkdiv;
 *   BNKSEL[2:0]   = txpll_bnksel
 *
 * Notes: pixel_clock, from drm_display_mode. lane_divider from dsi driver.
 */
struct ecv5546_mpitx_lane_clock {
	/**
	 * @clock:
	 *
	 * mipitx lane clock in kHz.
	 */
	int clock;
	u32 txpll_prescal;
	u32 txpll_fbkdiv;
	u32 txpll_prediv;
	u32 txpll_postdiv;
	u32 txpll_endiv5;
	u32 txpll_bnksel;
};

/* display MIPITX Pixel Clock and PLLH Parameters Setting.
 * MIPI Pixel Clock PLLH Formula:
 *                   25M * prescal * fbkdiv
 *   pixel_clock = -----------------------------
 *                 prediv * postdiv * seldiv
 *  Fvco = (25 * prescal * fbkdiv) / prediv
 *  pixel_clock = Fvco / (postdiv * seldiv)
 *  postdiv_10x = postdiv * 10
 *==>
 *  Fvco = pixel_clock * postdiv_10x * seldiv / 10
 *  fbkdiv = Fvco * prediv / (25 * prescal)
 *
 * PreSetting-Rule:
 *   prescal=1
 *   pixel_clock = [  5,   8)MHz, prediv =2, postdiv_10x=125, seldiv=16
 *   pixel_clock = [  8,  14)MHz, prediv =1, postdiv_10x=125, seldiv=16
 *   pixel_clock = [ 14,  20)MHz, prediv =1, postdiv_10x=90,  seldiv=16
 *   pixel_clock = [ 20,  29)MHz, prediv =1, postdiv_10x=125, seldiv=8
 *   pixel_clock = [ 29,  40)MHz, prediv =1, postdiv_10x=90,  seldiv=8
 *   pixel_clock = [ 40,  70)MHz, prediv =1, postdiv_10x=25,  seldiv=16
 *   pixel_clock = [ 70, 112)MHz, prediv =1, postdiv_10x=125, seldiv=2
 *   pixel_clock = [112, 160)MHz, prediv =1, postdiv_10x=90,  seldiv=2
 *   pixel_clock = [160, 230)MHz, prediv =1, postdiv_10x=125, seldiv=1
 *   pixel_clock = [230, 320)MHz, prediv =1, postdiv_10x=90,  seldiv=1
 *   pixel_clock = [320, 540)MHz, prediv =1, postdiv_10x=25,  seldiv=2
 *   pixel_clock = [540, 900)MHz, prediv =1, postdiv_10x=30,  seldiv=1
 *   pixel_clock = [900,1200)MHz, prediv =1, postdiv_10x=25,  seldiv=1
 *   Fvco = [1000, 1500]MHz, bnksel=0
 *   Fvco = [1500, 2000]MHz, bnksel=1
 *   Fvco = [2000, 2500]MHz, bnksel=2
 *   Fvco = [2500, 3000]MHz, bnksel=3
 *
 * Register Setting Formula:
 *   prescal= PRESCAL_H[15]+1 = {1, 2}
 *   prediv = PREDIV_H[2:1]+1 = {1, 2}
 *   postdiv= map{PSTDIV_H[6:3]} = {2.5, 3, 3.5, 4, 5, 5.5, 6, 7, 7.5, 8, 9, 10, 10.5, 11, 12, 12.5}
 *   postdiv_10x = postdiv * 10
 *   fbkdiv = FBKDIV_H[14:7] + 64 = [64, 127];
 *   seldiv = bitmap{MIPITX_SELDIV_H[11:7]} = {1, 2, 4, 8, 16}.
 *   bnksel = BNKSEL_H[1:0] = [0, 3]
 *==>
 *   PRESCAL_H[15] = prescal-1
 *   PREDIV_H[2:1] = prediv-1
 *   FBKDIV_H[14:7] = fbkdiv-64
 *   MIPITX_SELDIV_H[11:7] = log2(seldiv)
 *   PSTDIV_H[6:3]   = (postdiv_10x-25)/5   for postdiv_10x<=40
 *   PSTDIV_H[6:3]   = (postdiv_10x-50)/5+4 for postdiv_10x<=60
 *   PSTDIV_H[6:3]   = (postdiv_10x-70)/5+7 for postdiv_10x<=80
 *   PSTDIV_H[6:3]   = 10 for postdiv_10x=90
 *   PSTDIV_H[6:3]   = (postdiv_10x-100)/5+11 for postdiv_10x<=110
 *   PSTDIV_H[6:3]   = (postdiv_10x-120)/5+14 for postdiv_10x<=125
 *   BNKSEL_H[1:0]   = bnksel
 *
 * Notes: pixel_clock, from drm_display_mode.
 */
struct ecv5546_mpitx_pixel_clock {
	/**
	 * @clock:
	 *
	 * mipitx pixel clock in kHz.
	 */
	int clock;
	u32 prescal;
	u32 fbkdiv;
	u32 prediv;
	u32 postdiv_10x;
	u32 seldiv;
	u32 bnksel;
};

/* General DSI hardware state. */
struct ecv5546_dsi_host {
	struct mipi_dsi_host dsi_host;

	struct platform_device *pdev;
	struct drm_encoder *encoder;
	struct drm_bridge *bridge;
	struct drm_panel *panel;
	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	struct list_head bridge_chain;
	#endif

	void __iomem *regs;
	void __iomem *ao_moon3;

	/* clock */
	struct clk		*disp_clk[16];
	/* reset */
	struct reset_control	*disp_rstc[16];

	struct dma_chan *reg_dma_chan;
	dma_addr_t reg_dma_paddr;
	u32 *reg_dma_mem;
	dma_addr_t reg_paddr;

	/* DSI0 Fixed. */
	unsigned int port;

	/* DSI channel for the panel we're connected to. */
	u32 channel;
	u32 lanes;
	u32 format;
	u32 divider;
	u32 mode_flags;

	/* Input clock from CPRMAN to the digital PHY, for the DSI
	 * escape clock.
	 */
	//struct clk *escape_clock;

	/* Input clock to the analog PHY, used to generate the DSI bit
	 * clock.
	 */
	//struct clk *pll_phy_clock;

	/* HS Clocks generated within the DSI analog PHY. */
	//struct clk_fixed_factor phy_clocks[3];

	//struct clk_hw_onecell_data *clk_onecell;

	/* Pixel clock output to the pixelvalve, generated from the HS
	 * clock.
	 */
	//struct clk *pixel_clock;
	struct ecv5546_mpitx_lane_clock lane_clock;
	struct ecv5546_mpitx_pixel_clock pixel_clock;

	struct completion xfer_completion;
	int xfer_result;

	/* define for debugfs */
	struct debugfs_regset32 regset_g204;
	struct debugfs_regset32 regset_g205;
	struct debugfs_regset32 regset_ao_moon3;
	struct ecv5546_mpitx_sync_timing_param mipitx_sync_timing;
};

#define eCV5546_DSI_HOST_READ(offset) readl(sp_dsi_host->regs + (offset))
#define eCV5546_DSI_HOST_WRITE(offset, val) writel(val, sp_dsi_host->regs + (offset))

#define eCV5546_DSI_TXPLL_READ(offset) readl(sp_dsi_host->regs + (offset))
#define eCV5546_DSI_TXPLL_WRITE(offset, val) writel(val, sp_dsi_host->regs + (offset))

#define eCV5546_DSI_PLLH_READ(offset) readl(sp_dsi_host->ao_moon3 + (offset))
#define eCV5546_DSI_PLLH_WRITE(offset, val) writel(val, sp_dsi_host->ao_moon3 + (offset))

#define ecv5546_host_to_dsi(host) \
	container_of(host, struct ecv5546_dsi_host, dsi_host)

/* DSI encoder KMS struct */
struct ecv5546_dsi_encoder {
	struct ecv5546_encoder base;
	struct ecv5546_dsi_host *sp_dsi_host;
	bool is_enabled;
};

#define to_ecv5546_dsi_encoder(encoder) \
	container_of(encoder, struct ecv5546_dsi_encoder, base.base)

/*
 * ecv5546_mipitx_phy_timing[x]
 * x = 0-10, MIPITX phy
 *   T_HS-EXIT / T_LPX
 *   T_CLK-PREPARE / T_CLK-ZERO
 *   T_CLK-TRAIL / T_CLK-PRE / T_CLK-POST
 *   T_HS-TRAIL / T_HS-PREPARE / T_HS-ZERO
 */
static const u32 ecv5546_mipitx_phy_timing[10] = {
	0x10,  /* T_HS-EXIT */
	0x08,  /* T_LPX */
	0x10,  /* T_CLK-PREPARE */
	0x10,  /* T_CLK-ZERO */
	0x05,  /* T_CLK-TRAIL */
	0x12,  /* T_CLK-PRE */
	0x20,  /* T_CLK-POST */
	0x05,  /* T_HS-TRAIL */
	0x05,  /* T_HS-PREPARE */
	0x10,  /* T_HS-ZERO */
};

static void ecv5546_mipitx_phy_init(struct ecv5546_dsi_host *sp_dsi_host)
{
	u32 value;

	DRM_DEBUG_DRIVER("lanes=%d flags=0x%08x format=%s\n",
		sp_dsi_host->lanes,
		sp_dsi_host->mode_flags,
		ecv5546_dsi_fmt[sp_dsi_host->format]);

	//PHY Reset(under reset)
	value = eCV5546_DSI_HOST_READ(MIPITX_ANALOG_CTRL2);
	value &= ~eCV5546_MIPITX_NORMAL;
	eCV5546_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL2, value);

	/*
	 * Setting T_HS-EXIT & T_LPX for Clock/Data Lane
	 */
	value = 0;
	value |= (eCV5546_MIPITX_T_HS_EXIT_SET(ecv5546_mipitx_phy_timing[0]) |
			eCV5546_MIPITX_T_LPX_SET(ecv5546_mipitx_phy_timing[1]));
	eCV5546_DSI_HOST_WRITE(MIPITX_LANE_TIME_CTRL, value);

	/*
	 * Setting T_CLK-PREPARE & T_CLK-ZERO for Clock Lane
	 */
	value = 0;
	value |= (eCV5546_MIPITX_T_CLK_PREPARE_SET(ecv5546_mipitx_phy_timing[2]) |
			eCV5546_MIPITX_T_CLK_ZERO_SET(ecv5546_mipitx_phy_timing[3]));
	eCV5546_DSI_HOST_WRITE(MIPITX_CLK_TIME_CTRL0, value);

	/*
	 * Setting T_CLK-TRAIL & T_CLK-PRE & T_CLK-POST for Clock Lane
	 */
	value = 0;
	value |= (eCV5546_MIPITX_T_CLK_TRAIL_SET(ecv5546_mipitx_phy_timing[4]) |
			eCV5546_MIPITX_T_CLK_PRE_SET(ecv5546_mipitx_phy_timing[5]) |
			eCV5546_MIPITX_T_CLK_POST_SET(ecv5546_mipitx_phy_timing[6]));
	eCV5546_DSI_HOST_WRITE(MIPITX_CLK_TIME_CTRL1, value);

	/*
	 * Enable HSA & HBP for Blanking Mode
	 */
	value = 0;
	value |= (eCV5546_MIPITX_T_HS_TRAIL_SET(ecv5546_mipitx_phy_timing[7]) |
			eCV5546_MIPITX_T_HS_PREPARE_SET(ecv5546_mipitx_phy_timing[8]) |
			eCV5546_MIPITX_T_HS_ZERO_SET(ecv5546_mipitx_phy_timing[9]));
	eCV5546_DSI_HOST_WRITE(MIPITX_DATA_TIME_CTRL0, value);

	/*
	 * Enable HSA & HBP for Blanking Mode
	 */
	value = 0;
	value |= (eCV5546_MIPITX_BLANK_POWER_HSA | eCV5546_MIPITX_BLANK_POWER_HBP);
	eCV5546_DSI_HOST_WRITE(MIPITX_BLANK_POWER_CTRL, value);

	value = 0;
	value |= (eCV5546_MIPITX_CORE_CTRL_INPUT_EN |
			eCV5546_MIPITX_CORE_CTRL_ANALOG_EN |
			eCV5546_MIPITX_CORE_CTRL_DSI_EN);
	if ((sp_dsi_host->lanes == 1) || (sp_dsi_host->lanes == 2) ||
		(sp_dsi_host->lanes == 4)) {
		value |= eCV5546_MIPITX_CORE_CTRL_LANE_NUM_SET(sp_dsi_host->lanes - 1);
		eCV5546_DSI_HOST_WRITE(MIPITX_CORE_CTRL, value);
	} else
		pr_err("unsupported %d lanes\n", sp_dsi_host->lanes);

	value = 0x00000000;
	if (!(sp_dsi_host->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE))
		value |= eCV5546_MIPITX_FORMAT_VTF_SET(eCV5546_MIPITX_VTF_SYNC_EVENT);

	if (sp_dsi_host->format == DSI_PFORMAT_RGB565)
		value |= eCV5546_MIPITX_FORMAT_VPF_SET(eCV5546_MIPITX_VPF_DSI_RGB565);
	else if (sp_dsi_host->format == DSI_PFORMAT_RGB666)
		value |= eCV5546_MIPITX_FORMAT_VPF_SET(eCV5546_MIPITX_VPF_DSI_RGB666_18BITS);
	else if (sp_dsi_host->format == DSI_PFORMAT_RGB666_PACKED)
		value |= eCV5546_MIPITX_FORMAT_VPF_SET(eCV5546_MIPITX_VPF_DSI_RGB666_24BITS);
	else if (sp_dsi_host->format == DSI_PFORMAT_RGB888)
		value |= eCV5546_MIPITX_FORMAT_VPF_SET(eCV5546_MIPITX_VPF_DSI_RGB888);
	else
		value |= eCV5546_MIPITX_FORMAT_VPF_SET(eCV5546_MIPITX_VPF_DSI_RGB888);
	eCV5546_DSI_HOST_WRITE(MIPITX_FORMAT_CTRL, value);

	//PHY Reset(back to normal mode)
	value = eCV5546_DSI_HOST_READ(MIPITX_ANALOG_CTRL2);
	value |= eCV5546_MIPITX_NORMAL;
	eCV5546_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL2, value);

}

static const u32 ecv5546_pllh_pstdiv_int[] = {
	25, 30, 35, 40, 50, 55, 60, 70, 75, 80, 90, 100, 105, 110, 120, 125
};

static const u32 ecv5546_pllh_mipitx_sel_int[] = {
	1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static void ecv5546_mipitx_pllclk_get(struct ecv5546_dsi_host *sp_dsi_host)
{
	u32 tmp_value1, tmp_value2, tmp_value3;
	u32 value1, value2;

	value1 = eCV5546_DSI_PLLH_READ(MIPITX_AO_MOON3_14);
	value2 = eCV5546_DSI_PLLH_READ(MIPITX_AO_MOON3_25);

	tmp_value1 = 25 * ((FIELD_GET(GENMASK(15, 15), value1)?2:1) *
		(FIELD_GET(GENMASK(14, 7), value1) + 64)) /
		(FIELD_GET(GENMASK(2, 1), value1)?2:1);
	tmp_value2 = (tmp_value1 * 10) / ((ecv5546_pllh_pstdiv_int[FIELD_GET(GENMASK(6, 3), value1)]) *
		(ecv5546_pllh_mipitx_sel_int[FIELD_GET(GENMASK(11, 7), value2)]));
	tmp_value3 = (tmp_value1 * 1000) / ((ecv5546_pllh_pstdiv_int[FIELD_GET(GENMASK(6, 3), value1)]) *
		(ecv5546_pllh_mipitx_sel_int[FIELD_GET(GENMASK(11, 7), value2)]));

	DRM_DEBUG_DRIVER("     PLLH FVCO %04d MHz , pix_clk %03d.%02d MHz\n",
		tmp_value1, tmp_value2, (tmp_value3 - tmp_value2*100));
}

static const char * const ecv5546_txpll_prediv[] = {
	"DIV1", "DIV2", "DIV5", "DIV8",
};
static const u32 ecv5546_txpll_prediv_int[] = {
	1, 2, 5, 8
};

static const char * const ecv5546_txpll_pstdiv[] = {
	"DIV1", "DIV2", "DIV4", "DIV8", "DIV16"
};
static const u32 ecv5546_txpll_pstdiv_int[] = {
	1, 2, 4, 8, 16
};

static const char * const ecv5546_txpll_endiv5[] = {
	"DIV1", "DIV5"
};
static const u32 ecv5546_txpll_endiv5_int[] = {
	1, 5
};

static void ecv5546_mipitx_txpll_get(struct ecv5546_dsi_host *sp_dsi_host)
{
	u32 tmp_value1, tmp_value2, tmp_value3;
	u32 value1, value2;

	value1 = eCV5546_DSI_TXPLL_READ(MIPITX_ANALOG_CTRL6);

	tmp_value1 = 25 * ((FIELD_GET(GENMASK(4, 4), value1)?2:1) *
		(FIELD_GET(GENMASK(13, 8), value1))) /
		(ecv5546_txpll_prediv_int[FIELD_GET(GENMASK(1, 0), value1)]);

	tmp_value2 = (tmp_value1) / ((ecv5546_txpll_pstdiv_int[FIELD_GET(GENMASK(18, 16), value1)]) *
		(ecv5546_txpll_endiv5_int[FIELD_GET(GENMASK(20, 20), value1)]));
	tmp_value3 = (tmp_value1 * 100) / ((ecv5546_txpll_pstdiv_int[FIELD_GET(GENMASK(18, 16), value1)]) *
		(ecv5546_txpll_endiv5_int[FIELD_GET(GENMASK(20, 20), value1)]));
	DRM_DEBUG_DRIVER("    TXPLL FVCO %04d MHz , bit_clk %03d.%02d MHz\n",
		tmp_value1, tmp_value2, (tmp_value3 - (tmp_value2*100)));

	tmp_value2 = (tmp_value1) / ((ecv5546_txpll_pstdiv_int[FIELD_GET(GENMASK(18, 16), value1)]) *
		(ecv5546_txpll_endiv5_int[FIELD_GET(GENMASK(20, 20), value1)])*8);
	tmp_value3 = (tmp_value1 * 100) / ((ecv5546_txpll_pstdiv_int[FIELD_GET(GENMASK(18, 16), value1)]) *
		(ecv5546_txpll_endiv5_int[FIELD_GET(GENMASK(20, 20), value1)])*8);
	DRM_DEBUG_DRIVER("    TXPLL ---- ---- --- , byteclk %03d.%02d MHz\n",
		tmp_value2, (tmp_value3 - (tmp_value2*100)));

	value2 = eCV5546_DSI_TXPLL_READ(MIPITX_LP_CK);
	tmp_value2 = (tmp_value1) / ((ecv5546_txpll_pstdiv_int[FIELD_GET(GENMASK(18, 16), value1)]) *
		(ecv5546_txpll_endiv5_int[FIELD_GET(GENMASK(20, 20), value1)])*8*(FIELD_GET(GENMASK(5, 0), value2)+1));
	tmp_value3 = (tmp_value1 * 100) / ((ecv5546_txpll_pstdiv_int[FIELD_GET(GENMASK(18, 16), value1)]) *
		(ecv5546_txpll_endiv5_int[FIELD_GET(GENMASK(20, 20), value1)])*8*(FIELD_GET(GENMASK(5, 0), value2)+1));
	DRM_DEBUG_DRIVER("    TXPLL ---- ---- --- , LPCDclk %03d.%02d MHz\n",
		tmp_value2, (tmp_value3 - (tmp_value2*100)));
}

static void ecv5546_mipitx_clock_init(struct ecv5546_dsi_host *sp_dsi_host)
{
	DRM_DEBUG_DRIVER("clock init");

	eCV5546_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL9, 0x80000000); //init clock

	/*
	 * PLLH Fvco = 2150MHz (fixed)
	 *                             2150
	 * MIPITX pixel CLK = ----------------------- = 59.72MHz
	 *                     PST_DIV * MIPITX_SEL
	 */
	eCV5546_DSI_PLLH_WRITE(MIPITX_AO_MOON3_15, 0xffff40be);
	eCV5546_DSI_PLLH_WRITE(MIPITX_AO_MOON3_16, 0xffff0009);
	eCV5546_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, 0xffff0b50); //PST_DIV = div9
	eCV5546_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, 0x07800180); //MIPITX_SEL = div4

	/*
	 * TXPLL
	 * PRESCAL = 1, FBKDIV = 48, PRE_DIV = 1, EN_DIV5 = 0, PRE_DIV = 2, POST_DIV = 1
	 *                    25 * PRESCAL * FBKDIV            25 * 48
	 * MIPITX bit CLK = ------------------------------ = ----------- = 600MHz
	 *                   PRE_DIV * POST_DIV * 5^EN_DIV5       2
	 */
	eCV5546_DSI_TXPLL_WRITE(MIPITX_ANALOG_CTRL5, 0x00000003); //enable and reset
	eCV5546_DSI_TXPLL_WRITE(MIPITX_ANALOG_CTRL6, 0x00003001); //MIPITX CLK = 600MHz
	eCV5546_DSI_TXPLL_WRITE(MIPITX_ANALOG_CTRL7, 0x00000140); //BNKSEL = 0x0 (320~640MHz)

	/*
	 *                      600
	 * MIPITX LP CLK = ------------ = 8.3MHz
	 *                   8 * div9
	 */
	eCV5546_DSI_TXPLL_WRITE(MIPITX_LP_CK, 0x00000008); //(600/8/div9)=8.3MHz

	eCV5546_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL9, 0x00000000); //init clock done

	/*
	 * check pll clock setting (debug only)
	 */
	ecv5546_mipitx_pllclk_get(sp_dsi_host);
	ecv5546_mipitx_txpll_get(sp_dsi_host);
}

static void ecv5546_mipitx_lane_timing_init(struct ecv5546_dsi_host *sp_dsi_host)
{
	u32 value = 0;

	/*
	 * Enable clock lane at High Speed Mode
	 */
	value = 0;
	value |= eCV5546_MIPITX_CLK_CTRL_CKHS_EN;
	eCV5546_DSI_HOST_WRITE(MIPITX_CLK_CTRL, value);
}

static void ecv5546_mipitx_dsi_cmd_mode_start(struct ecv5546_dsi_host *sp_dsi_host)
{
	u32 value;


	value = 0;
	value |= eCV5546_MIPITX_OP_CTRL_TXLDPT;
	eCV5546_DSI_HOST_WRITE(MIPITX_OP_CTRL, value);

	value = 0;
	value |= (eCV5546_MIPITX_CORE_CTRL_INPUT_EN |
			eCV5546_MIPITX_CORE_CTRL_ANALOG_EN |
			eCV5546_MIPITX_CORE_CTRL_CMD_TRANS_TIME |
			eCV5546_MIPITX_CORE_CTRL_DSI_EN);
	if ((sp_dsi_host->lanes == 1) || (sp_dsi_host->lanes == 2) ||
		(sp_dsi_host->lanes == 4)) {
		value |= eCV5546_MIPITX_CORE_CTRL_LANE_NUM_SET(sp_dsi_host->lanes - 1);
		eCV5546_DSI_HOST_WRITE(MIPITX_CORE_CTRL, value);
	} else
		pr_err("unsupported %d lanes\n", sp_dsi_host->lanes);

	eCV5546_DSI_HOST_WRITE(MIPITX_BTA_CTRL, 0x00520004);
	eCV5546_DSI_HOST_WRITE(MIPITX_ULPS_DELAY, 0x00000aff);
}

static void ecv5546_mipitx_dsi_video_mode_on(struct ecv5546_dsi_host *sp_dsi_host)
{
	u32 value;


	value = 0;
	value |= (eCV5546_MIPITX_CORE_CTRL_INPUT_EN |
			eCV5546_MIPITX_CORE_CTRL_ANALOG_EN |
			eCV5546_MIPITX_CORE_CTRL_DSI_EN);

	if ((sp_dsi_host->lanes == 1) || (sp_dsi_host->lanes == 2) ||
		(sp_dsi_host->lanes == 4)) {
		value |= eCV5546_MIPITX_CORE_CTRL_LANE_NUM_SET(sp_dsi_host->lanes - 1);
		eCV5546_DSI_HOST_WRITE(MIPITX_CORE_CTRL, value);
	} else
		pr_err("unsupported %d lanes\n", sp_dsi_host->lanes);

}

static void check_dsi_cmd_fifo_full(struct ecv5546_dsi_host *sp_dsi_host)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = eCV5546_DSI_HOST_READ(MIPITX_CMD_FIFO);
	while ((value & MIPITX_CMD_FIFO_FULL) == MIPITX_CMD_FIFO_FULL) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("cmd fifo full timeout\n");
			break;
		}
		value = eCV5546_DSI_HOST_READ(MIPITX_CMD_FIFO);
		++mipitx_fifo_timeout;
		udelay(100);
	}
}

static void check_dsi_data_fifo_full(struct ecv5546_dsi_host *sp_dsi_host)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = eCV5546_DSI_HOST_READ(MIPITX_CMD_FIFO);
	while ((value & MIPITX_DATA_FIFO_FULL) == MIPITX_DATA_FIFO_FULL) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("data fifo full timeout\n");
			break;
		}
		value = eCV5546_DSI_HOST_READ(MIPITX_CMD_FIFO);
		++mipitx_fifo_timeout;
		udelay(100);
	}
}

/* display MIPITX Lane Clock and TXPLL Parameters Setting. */
static void ecv5546_mipitx_dsi_lane_clock_setting(struct ecv5546_dsi_host *sp_dsi_host, struct drm_display_mode *mode)
{
	u32 value;
	u32 reg_prediv;
	u32 reg_postdiv;
	struct ecv5546_mpitx_lane_clock *lane_clock = &sp_dsi_host->lane_clock;

	if (mode) {
		int fvco; /* KHz */
		/* MIPI Lane Clock Formula:
		 *  lane_divider = pixel_bits / data_lanes
		 *  lane_clock = pixel_clock * pixel_bits / data_lanes
		 *             = pixel_clock * lane_divider
		 *
		 *  Fvco = (25 * prescal * fbkdiv) / prediv
		 *  lane_clock = Fvco / (postdiv * 5^en_div5)
		 * ==>
		 *  Fvco = lane_clock * postdiv * 5^en_div5
		 *  fbkdiv = Fvco * prediv / (25 * prescal)
		 *
		 * PreSetting-Rule:
		 *   txpll_prescal=1
		 *   txpll_prediv =1
		 *   lane_clock = [80, 150)MHz, txpll_endiv5=1, txpll_postdiv=2
		 *   lane_clock = [150MHz,375)MHz, txpll_endiv5=0, txpll_postdiv=4
		 *   lane_clock = [375MHz,1500)MHz, txpll_endiv5=0, txpll_postdiv=1
		 *   Fvco = [ 320,  640]MHz, txpll_bnksel=0
		 *   Fvco = [ 640, 1000]MHz, txpll_bnksel=1
		 *   Fvco = [1000, 1200]MHz, txpll_bnksel=2
		 *   Fvco = [1200, 1500]MHz, txpll_bnksel=3
		 */
		lane_clock->txpll_prescal = 1;
		lane_clock->txpll_prediv  = 1;

		lane_clock->clock    = mode->clock * sp_dsi_host->divider;
		if (lane_clock->clock < 150000) {
			lane_clock->txpll_postdiv = 2;
			lane_clock->txpll_endiv5  = 1;
		} else if (lane_clock->clock < 375000) {
			lane_clock->txpll_postdiv = 4;
			lane_clock->txpll_endiv5  = 0;
		} else {
			lane_clock->txpll_postdiv = 1;
			lane_clock->txpll_endiv5  = 0;
		}
		fvco = lane_clock->clock * lane_clock->txpll_postdiv * (lane_clock->txpll_endiv5 ? 5 : 1);
		if (fvco < 640000)
			lane_clock->txpll_bnksel = 0;
		else if (fvco < 1000000)
			lane_clock->txpll_bnksel = 1;
		else if (fvco < 1200000)
			lane_clock->txpll_bnksel = 2;
		else
			lane_clock->txpll_bnksel = 3;

		lane_clock->txpll_fbkdiv =  fvco * lane_clock->txpll_prediv / (25000 * lane_clock->txpll_prescal);
		if ((fvco * lane_clock->txpll_prediv) % (25000 * lane_clock->txpll_prescal))
			lane_clock->txpll_fbkdiv += 1;

		DRM_DEBUG_DRIVER("\nMIPITX Lane Clock Info:\n"
							"   %dKHz(pixel:%dKHz), Fvco=%dKHz\n"
							"   txpll_prescal=%d, txpll_prediv=%d\n"
							"   txpll_postdiv=%d, txpll_endiv5=%d\n"
							"   txpll_fbkdiv=%d, txpll_bnksel=%d\n",
			lane_clock->clock, mode->clock, fvco,
			lane_clock->txpll_prescal, lane_clock->txpll_prediv,
			lane_clock->txpll_postdiv, lane_clock->txpll_endiv5,
			lane_clock->txpll_fbkdiv, lane_clock->txpll_bnksel);
	}
	if (!lane_clock->clock) {
		/* do nothing */
		return;
	}

	/* Register Setting Formula:
	 *   txpll_prescal= PRESCAL[4]+1 = {1, 2}
	 *   txpll_prediv = map{PREDIV[1:0]} = {1, 2, 5, 8}
	 *   txpll_postdiv= map{POSTDIV[18:16]} = {1, 2, 4, 8, 16}
	 *   txpll_endiv5 = EN_DIV5[20] = {0, 1}
	 *   txpll_fbkdiv = FBK_DIV[13:8] = [3, 63];
	 *   txpll_bnksel = BNKSEL[2:0] = [0, 3]
	 *==>
	 *   PRESCAL[4]  = txpll_prescal-1
	 *   PREDIV[1:0] = 3 for txpll_prediv = 8
	 *   PREDIV[1:0] = txpll_prediv / 2
	 *   POSTDIV[18:16] = log2(txpll_postdiv)
	 *   EN_DIV5[20]   = txpll_endiv5
	 *   FBK_DIV[13:8] = fbkdiv;
	 *   BNKSEL[2:0]   = txpll_bnksel
	 */
	reg_prediv = lane_clock->txpll_prediv / 2;
	if (reg_prediv > 3)
		reg_prediv = 3;

	reg_postdiv = 0;
	value = lane_clock->txpll_postdiv / 2;
	while (value) {
		value /= 2;
		reg_postdiv++;
	}
	DRM_DEBUG_DRIVER("\nMIPITX PLL Setting:\n"
						"   %dKHz\n"
						"   PRESCAL[4]=%d, PREDIV[1:0]=%d\n"
						"   POSTDIV[18:16]=%d, EN_DIV5[20]=%d\n"
						"   FBK_DIV[13:8]=%d, BNKSEL[2:0]=%d\n",
		lane_clock->clock,
		lane_clock->txpll_prescal-1, reg_prediv,
		reg_postdiv, lane_clock->txpll_endiv5,
		lane_clock->txpll_fbkdiv, lane_clock->txpll_bnksel);

	value = 0x00000000;
	value |= (eCV5546_MIPITX_MIPI_PHY_EN_DIV5(lane_clock->txpll_endiv5) |
			eCV5546_MIPITX_MIPI_PHY_POSTDIV(reg_postdiv) |
			eCV5546_MIPITX_MIPI_PHY_FBKDIV(lane_clock->txpll_fbkdiv) |
			eCV5546_MIPITX_MIPI_PHY_PRESCALE(lane_clock->txpll_prescal-1) |
			eCV5546_MIPITX_MIPI_PHY_PREDIV(reg_prediv));
	eCV5546_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL6, value);

	value = eCV5546_DSI_HOST_READ(MIPITX_ANALOG_CTRL7);
	value &= ~(eCV5546_MIPITX_MIPI_PHY_BNKSEL_MASK);
	value |= eCV5546_MIPITX_MIPI_PHY_BNKSEL(lane_clock->txpll_bnksel);
	eCV5546_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL7, value);

	/*
	 * check pll clock setting (debug only)
	 */
	ecv5546_mipitx_txpll_get(sp_dsi_host);

}

/* display MIPITX Pixel Clock and TXPLL Parameters Setting. */
static void ecv5546_mipitx_dsi_pixel_clock_setting(struct ecv5546_dsi_host *sp_dsi_host, struct drm_display_mode *mode)
{
	u32 value;
	u32 reg_postdiv;
	struct ecv5546_mpitx_pixel_clock *pixel_clock = &sp_dsi_host->pixel_clock;

	if (mode) {
		int fvco; /* KHz */
		/* MIPI Pixel Clock Formula:
		 *                   25M * prescal * fbkdiv
		 *   pixel_clock = -----------------------------
		 *                 prediv * postdiv * seldiv
		 *  Fvco = (25 * prescal * fbkdiv) / prediv
		 *  pixel_clock = Fvco / (postdiv * seldiv)
		 *  postdiv_10x = postdiv * 10
		 *==>
		 *  Fvco = pixel_clock * postdiv_10x * seldiv / 10
		 *  fbkdiv = Fvco * prediv / (25 * prescal)
		 *
		 * PreSetting-Rule:
		 *   prescal=1
		 *   pixel_clock = [  5,   8)MHz, prediv =2, postdiv_10x=125, seldiv=16
		 *   pixel_clock = [  8,  14)MHz, prediv =1, postdiv_10x=125, seldiv=16
		 *   pixel_clock = [ 14,  20)MHz, prediv =1, postdiv_10x=90,  seldiv=16
		 *   pixel_clock = [ 20,  29)MHz, prediv =1, postdiv_10x=125, seldiv=8
		 *   pixel_clock = [ 29,  40)MHz, prediv =1, postdiv_10x=90,  seldiv=8
		 *   pixel_clock = [ 40,  70)MHz, prediv =1, postdiv_10x=25,  seldiv=16
		 *   pixel_clock = [ 70, 112)MHz, prediv =1, postdiv_10x=125, seldiv=2
		 *   pixel_clock = [112, 160)MHz, prediv =1, postdiv_10x=90,  seldiv=2
		 *   pixel_clock = [160, 230)MHz, prediv =1, postdiv_10x=125, seldiv=1
		 *   pixel_clock = [230, 320)MHz, prediv =1, postdiv_10x=90,  seldiv=1
		 *   pixel_clock = [320, 540)MHz, prediv =1, postdiv_10x=25,  seldiv=2
		 *   pixel_clock = [540, 900)MHz, prediv =1, postdiv_10x=30,  seldiv=1
		 *   pixel_clock = [900,1200)MHz, prediv =1, postdiv_10x=25,  seldiv=1
		 *   Fvco = [1000, 1500]MHz, bnksel=0
		 *   Fvco = [1500, 2000]MHz, bnksel=1
		 *   Fvco = [2000, 2500]MHz, bnksel=2
		 *   Fvco = [2500, 3000]MHz, bnksel=3
		 */
		pixel_clock->prescal = 1;
		pixel_clock->clock   = mode->clock;
		if (pixel_clock->clock < 8000) {
			pixel_clock->prediv = 2;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 16;
		} else if (pixel_clock->clock < 14000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 16;
		} else if (pixel_clock->clock < 20000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 16;
		} else if (pixel_clock->clock < 29000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 8;
		} else if (pixel_clock->clock < 40000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 8;
		} else if (pixel_clock->clock < 70000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 25;
			pixel_clock->seldiv  = 16;
		} else if (pixel_clock->clock < 112000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 2;
		} else if (pixel_clock->clock < 160000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 2;
		} else if (pixel_clock->clock < 230000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 1;
		} else if (pixel_clock->clock < 320000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 1;
		} else if (pixel_clock->clock < 540000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 25;
			pixel_clock->seldiv  = 2;
		} else if (pixel_clock->clock < 900000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 30;
			pixel_clock->seldiv  = 1;
		} else {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 25;
			pixel_clock->seldiv  = 1;
		}
		fvco = pixel_clock->clock * pixel_clock->postdiv_10x * pixel_clock->seldiv / 10;
		if (fvco < 1500000)
			pixel_clock->bnksel = 0;
		else if (fvco < 2000000)
			pixel_clock->bnksel = 1;
		else if (fvco < 2500000)
			pixel_clock->bnksel = 2;
		else
			pixel_clock->bnksel = 3;

		pixel_clock->fbkdiv =  fvco * pixel_clock->prediv / (25000 * pixel_clock->prescal);
		if ((fvco * pixel_clock->prediv) % (25000 * pixel_clock->prescal))
			pixel_clock->fbkdiv += 1;

		DRM_DEBUG_DRIVER("\nMIPITX Pixel Clock Info:\n"
							"   %dKHz(pixel:%dKHz), Fvco=%dKHz\n"
							"   prescal=%d, prediv=%d\n"
							"   postdiv_10x=%d, fbkdiv=%d\n"
							"   bnksel=%d, seldiv=%d\n",
			pixel_clock->clock, mode->clock, fvco,
			pixel_clock->prescal, pixel_clock->prediv,
			pixel_clock->postdiv_10x, pixel_clock->fbkdiv,
			pixel_clock->bnksel, pixel_clock->seldiv);
	}
	if (!pixel_clock->clock) {
		/* do nothing */
		return;
	}

	/* Register Setting Formula:
	 *   prescal= PRESCAL_H[15]+1 = {1, 2}
	 *   prediv = PREDIV_H[2:1]+1 = {1, 2}
	 *   postdiv= map{PSTDIV_H[6:3]} = {2.5, 3, 3.5, 4, 5, 5.5, 6, 7, 7.5, 8, 9, 10, 10.5, 11, 12, 12.5}
	 *   postdiv_10x = postdiv * 10
	 *   fbkdiv = FBKDIV_H[14:7] + 64 = [64, 127];
	 *   seldiv = MIPITX_SELDIV_H[11:7]+1 = {1, 2, 4, 8, 16}.
	 *   bnksel = BNKSEL_H[1:0] = [0, 3]
	 *==>
	 *   PRESCAL_H[15]  = prescal-1
	 *   PREDIV_H[2:1]  = prediv-1
	 *   FBKDIV_H[14:7] = fbkdiv-64
	 *   MIPITX_SELDIV_H[11:7] = seldiv-1
	 *   PSTDIV_H[6:3]   = (postdiv_10x-25)/5   for postdiv_10x<=40
	 *   PSTDIV_H[6:3]   = (postdiv_10x-50)/5+4 for postdiv_10x<=60
	 *   PSTDIV_H[6:3]   = (postdiv_10x-70)/5+7 for postdiv_10x<=80
	 *   PSTDIV_H[6:3]   = 10 for postdiv_10x=90
	 *   PSTDIV_H[6:3]   = (postdiv_10x-100)/5+11 for postdiv_10x<=110
	 *   PSTDIV_H[6:3]   = (postdiv_10x-120)/5+14 for postdiv_10x<=125
	 *   BNKSEL_H[1:0]   = bnksel
	 */
	if (pixel_clock->postdiv_10x == 90)
		reg_postdiv = 10;
	else if (pixel_clock->postdiv_10x <= 40)
		reg_postdiv = (pixel_clock->postdiv_10x - 25) / 5;
	else if (pixel_clock->postdiv_10x <= 60)
		reg_postdiv = (pixel_clock->postdiv_10x - 50) / 5 + 4;
	else if (pixel_clock->postdiv_10x <= 80)
		reg_postdiv = (pixel_clock->postdiv_10x - 70) / 5 + 7;
	else if (pixel_clock->postdiv_10x <= 110)
		reg_postdiv = (pixel_clock->postdiv_10x - 100) / 5 + 11;
	else
		reg_postdiv = (pixel_clock->postdiv_10x - 120) / 5 + 14;

	DRM_DEBUG_DRIVER("\nAO MOON3 PLLH Setting:\n"
						"   %dKHz\n"
						"   PRESCAL[15]=%d, PREDIV[2:1]=%d\n"
						"   POSTDIV[6:3]=%d, FBK_DIV[14:7]=%d\n"
						"   BNKSEL[1:0]=%d, MIPITX_SELDIV_H[11:7]=%d\n",
		pixel_clock->clock,
		pixel_clock->prescal - 1, pixel_clock->prediv - 1,
		reg_postdiv, pixel_clock->fbkdiv-64,
		pixel_clock->bnksel, pixel_clock->seldiv - 1);

	value = 0;
	/* Update PRESCAL_H[15] */
	value |= (0x80000000 | ((pixel_clock->prescal - 1) << 15));
	/* Update FBKDIV_H[14:7] */
	value |= (0x7f800000 | ((pixel_clock->fbkdiv - 64) << 7));
	/* Update PSTDIV_H[6:3] */
	value |= (0x00780000 | (reg_postdiv << 3));
	/* Update PREDIV_H[2:1] */
	value |= (0x00060000 | ((pixel_clock->prediv - 1) << 1));
	eCV5546_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, value);

	value = 0;
	/* Update BNKSEL_H[1:0] */
	value = 0x00030000 | (pixel_clock->bnksel);
	eCV5546_DSI_PLLH_WRITE(MIPITX_AO_MOON3_15, value);

	value = 0;
	/* Update MIPITX_SELDIV_H[11:7] */
	value |= (0x0f800000 | ((pixel_clock->seldiv - 1) << 7));
	eCV5546_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, value);

	ecv5546_mipitx_pllclk_get(sp_dsi_host);
}

/* mipitx dsi sync timing setting */
static void ecv5546_mipitx_dsi_video_mode_setting(struct ecv5546_dsi_host *sp_dsi_host, struct drm_display_mode *mode)
{
	u32 value;
	u32 word_cnt;
	struct ecv5546_mpitx_sync_timing_param *sync_timing = &sp_dsi_host->mipitx_sync_timing;

	if (mode) {
		/* display MIPITX Sync-Timing Formula:
		 *   hsa = hsync_end-hsync_start
		 *   hbp = htotal-hsync_end
		 *   hact= hdisplay
		 *   vsa = vsync_end-vsync_start
		 *   vfp = vsync_start-vdisplay
		 *   vbp = vtotal-vsync_end
		 *   vact= vdisplay
		 */
		sync_timing->hsa  = mode->hsync_end - mode->hsync_start;
		sync_timing->hbp  = mode->htotal - mode->hsync_end;
		sync_timing->hact = mode->hdisplay;
		sync_timing->vsa  = mode->vsync_end - mode->vsync_start;
		sync_timing->vfp  = mode->vsync_start - mode->vdisplay;
		sync_timing->vbp  = mode->vtotal - mode->vsync_end;
		sync_timing->vact = mode->vdisplay;
	}
	if (!sync_timing->hact || !sync_timing->vact) {
		/* do nothing */
		return;
	}
	/* Register Setting Formula:
	 *   hsa = HSA[31:24]
	 *   hbp = HBP[11:0]
	 *   hact= HACT[31:16] = WORD_CNT[31:16]
	 *   vsa = VSA[23:16]
	 *   vfp = VFP[15:8]
	 *   vbp = VBP[7:0]
	 *   vact= VACT[15:0]
	 *   word_cnt = hact * pixel_bits / 8
	 *            = hact * lane_divider * data_lanes / 8
	 *            = WORD_CNT[15:0]
	 *==>
	 *   HSA[31:24]  = hsa
	 *   HBP[11:0]   = hbp
	 *   HACT[31:16] = hact
	 *   VSA[23:16]  = vsa
	 *   VFP[15:8]   = vfp
	 *   VBP[7:0]    = vbp
	 *   VACT[15:0]  = vact
	 *   WORD_CNT[15:0] = word_cnt
	 */
	word_cnt = sync_timing->hact * sp_dsi_host->divider * sp_dsi_host->lanes / 8;
	DRM_DEBUG_DRIVER("\nMIPITX Timing Setting:\n"
					"   HSA[31:24]=%d, HBP[11:0]=%d, HACT[31:16]=%d\n"
					"   VSA[23:16]=%d, VFP[15:8]=%d, VBP[7:0]=%d, VACT[15:0]=%d\n"
					"   WORD_CNT[15:0]=%d\n",
		sync_timing->hsa, sync_timing->hbp, sync_timing->hact,
		sync_timing->vsa, sync_timing->vfp, sync_timing->vbp, sync_timing->vact,
		word_cnt);

	value = 0;
	value |= eCV5546_MIPITX_HSA_SET(sync_timing->hsa) |
		eCV5546_MIPITX_HBP_SET(sync_timing->hbp);
	eCV5546_DSI_HOST_WRITE(MIPITX_VM_HT_CTRL, value);

	value = 0;
	value |= eCV5546_MIPITX_VSA_SET(sync_timing->vsa) |
		eCV5546_MIPITX_VFP_SET(sync_timing->vfp) |
		eCV5546_MIPITX_VBP_SET(sync_timing->vbp);
	eCV5546_DSI_HOST_WRITE(MIPITX_VM_VT0_CTRL, value);

	value = 0;
	value |= eCV5546_MIPITX_VACT_SET(sync_timing->vact);
	eCV5546_DSI_HOST_WRITE(MIPITX_VM_VT1_CTRL, value);

	//MIPITX  Video Mode WordCount Setting
	value = 0;
	value |= ((sync_timing->hact << 16) | word_cnt);
	eCV5546_DSI_HOST_WRITE(MIPITX_WORD_CNT, value);
}

static enum drm_mode_status _ecv5546_dsi_encoder_phy_mode_valid(
					struct drm_encoder *encoder,
					const struct drm_display_mode *mode)
{
	/* Any display HW(TCON/MIPITX DSI/TXPLL/PLLH...) Limit??? */
	if (mode->clock > 375000)
		return MODE_CLOCK_HIGH;
	if (mode->clock < 5000)
		return MODE_CLOCK_LOW;

	if (mode->hdisplay > 1920)
		return MODE_BAD_HVALUE;

	if (mode->vdisplay > 1920)
		return MODE_BAD_VVALUE;

	return MODE_OK;
}

static enum drm_mode_status ecv5546_dsi_encoder_mode_valid(struct drm_encoder *encoder,
							  const struct drm_display_mode *mode)
{
	const struct drm_crtc_helper_funcs *crtc_funcs = NULL;
	struct drm_crtc *crtc = NULL;
	struct drm_display_mode adj_mode;
	enum drm_mode_status ret;


	/*
	 * The crtc might adjust the mode, so go through the
	 * possible crtcs (technically just one) and call
	 * mode_fixup to figure out the adjusted mode before we
	 * validate it.
	 */
	drm_for_each_crtc(crtc, encoder->dev) {
		/*
		 * reset adj_mode to the mode value each time,
		 * so we don't adjust the mode twice
		 */
		drm_mode_copy(&adj_mode, mode);

		crtc_funcs = crtc->helper_private;
		if (crtc_funcs && crtc_funcs->mode_fixup)
			if (!crtc_funcs->mode_fixup(crtc, mode, &adj_mode))
				return MODE_BAD;

		ret = _ecv5546_dsi_encoder_phy_mode_valid(encoder, &adj_mode);
		if (ret != MODE_OK)
			return ret;
	}

	return MODE_OK;
}

static int ecv5546_dsi_encoder_atomic_check(struct drm_encoder *encoder,
					   struct drm_crtc_state *crtc_state,
				    struct drm_connector_state *conn_state)
{
	/* do nothing */
	DRM_DEBUG_DRIVER("[do nothing]\n");
	return 0;
}

static void ecv5546_dsi_encoder_atomic_mode_set(struct drm_encoder *encoder,
				struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	struct ecv5546_dsi_encoder *sp_dsi_encoder = to_ecv5546_dsi_encoder(encoder);
	struct ecv5546_dsi_host *sp_dsi_host = sp_dsi_encoder->sp_dsi_host;
	struct drm_display_mode *adj_mode = &crtc_state->adjusted_mode;
	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	struct drm_bridge *iter;
	#endif

	/* TODO reference to dsi_encoder_mode_set */
	//DRM_DEBUG_DRIVER("[TODO]\n");
	DRM_DEBUG_DRIVER("\nSET DSI mode(%s):\n"
		 "   hdisplay=%d, hsync_start=%d, hsync_end=%d, htotal=%d\n"
		 "   vdisplay=%d, vsync_start=%d, vsync_end=%d, vtotal=%d\n",
		 adj_mode->name,
		 adj_mode->hdisplay, adj_mode->hsync_start, adj_mode->hsync_end, adj_mode->htotal,
		 adj_mode->vdisplay, adj_mode->vsync_start, adj_mode->vsync_end, adj_mode->vtotal);

	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	list_for_each_entry_reverse(iter, &sp_dsi_host->bridge_chain, chain_node) {
		if (iter->funcs->mode_set)
			iter->funcs->mode_set(iter, &crtc_state->mode, adj_mode);
	}
	#endif

	ecv5546_mipitx_dsi_pixel_clock_setting(sp_dsi_host, adj_mode);
	ecv5546_mipitx_dsi_lane_clock_setting(sp_dsi_host, adj_mode);
	ecv5546_mipitx_dsi_video_mode_setting(sp_dsi_host, adj_mode);
}

static void ecv5546_dsi_encoder_atomic_disable(struct drm_encoder *encoder,
			       struct drm_atomic_state *state)
{
	struct ecv5546_dsi_encoder *sp_dsi_encoder = to_ecv5546_dsi_encoder(encoder);
	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	struct ecv5546_dsi_host *sp_dsi_host = sp_dsi_encoder->sp_dsi_host;
	struct drm_bridge *iter;

	if (!sp_dsi_host || !sp_dsi_host->bridge || !encoder->name)
		return;

	DRM_DEBUG_DRIVER("%s\n", encoder->name);

	list_for_each_entry_reverse(iter, &sp_dsi_host->bridge_chain, chain_node) {
		if (iter->funcs->disable)
			iter->funcs->disable(iter);

		if (iter->funcs->post_disable)
			iter->funcs->post_disable(iter);
	}
	#else
	DRM_DEBUG_DRIVER("[nothing]%s\n", encoder->name);
	#endif
	sp_dsi_encoder->is_enabled = false;
}

static void ecv5546_dsi_encoder_atomic_enable(struct drm_encoder *encoder,
			      struct drm_atomic_state *state)
{
	struct ecv5546_dsi_encoder *sp_dsi_encoder = to_ecv5546_dsi_encoder(encoder);
	struct ecv5546_dsi_host *sp_dsi_host = sp_dsi_encoder->sp_dsi_host;
	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	struct drm_bridge *iter;
	#endif

	if (!sp_dsi_host || !sp_dsi_host->bridge || !encoder->name)
		return;

	DRM_DEBUG_DRIVER("%s\n", encoder->name);

	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	list_for_each_entry_reverse(iter, &sp_dsi_host->bridge_chain, chain_node) {
		if (iter->funcs->pre_enable)
			iter->funcs->pre_enable(iter);
	}
	#endif

	ecv5546_mipitx_dsi_video_mode_on(sp_dsi_host);

	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	list_for_each_entry_reverse(iter, &sp_dsi_host->bridge_chain, chain_node) {
		if (iter->funcs->enable)
			iter->funcs->enable(iter);
	}
	#endif
	sp_dsi_encoder->is_enabled = true;
}

/*
 * MIPI DSI (Display Command Set) for eCV5546
 */
static ssize_t ecv5546_dsi_host_transfer(struct mipi_dsi_host *host,
					const struct mipi_dsi_msg *msg)
{
	struct ecv5546_dsi_host *sp_dsi_host = ecv5546_host_to_dsi(host);
	u32 value, data_cnt;
	u8 *data1;
	int i;

	DRM_DEBUG_DRIVER("len %ld\n", msg->tx_len);

	data1 = (u8 *)msg->tx_buf;

	udelay(100);
	if (msg->tx_len == 0) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000003;
		eCV5546_DSI_HOST_WRITE(MIPITX_SPKT_HEAD, value);
	} else if (msg->tx_len == 1) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000013 | (data1[0] << 8);
		eCV5546_DSI_HOST_WRITE(MIPITX_SPKT_HEAD, value);
	} else if (msg->tx_len == 2) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000023 | (data1[0] << 8) | (data1[1] << 16);
		eCV5546_DSI_HOST_WRITE(MIPITX_SPKT_HEAD, value);
	} else if ((msg->tx_len >= 3) && (msg->tx_len <= 64)) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000029 | ((u32)msg->tx_len << 8);
		eCV5546_DSI_HOST_WRITE(MIPITX_LPKT_HEAD, value);

		if (msg->tx_len % 4)
			data_cnt = ((u32)msg->tx_len / 4) + 1;
		else
			data_cnt = ((u32)msg->tx_len / 4);

		for (i = 0; i < data_cnt; i++) {
			check_dsi_data_fifo_full(sp_dsi_host);
			value = 0x00000000;
			if (i * 4 + 0 < msg->tx_len)
				value |= (data1[i * 4 + 0] << 0);
			if (i * 4 + 1 < msg->tx_len)
				value |= (data1[i * 4 + 1] << 8);
			if (i * 4 + 2 < msg->tx_len)
				value |= (data1[i * 4 + 2] << 16);
			if (i * 4 + 3 < msg->tx_len)
				value |= (data1[i * 4 + 3] << 24);
			eCV5546_DSI_HOST_WRITE(MIPITX_LPKT_PAYLOAD, value);
		}
	} else {
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev, "data length over %ld\n", msg->tx_len);
		return -1;
	}

	return 0;
}

static int ecv5546_dsi_host_attach(struct mipi_dsi_host *host,
				  struct mipi_dsi_device *device)
{
	struct ecv5546_dsi_host *sp_dsi_host = ecv5546_host_to_dsi(host);

	if (!sp_dsi_host->regs || !sp_dsi_host->ao_moon3) {
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev, "dsi host probe fail!.\n");
		return -1;
	}

	DRM_DEBUG_DRIVER("channel %d lanes=%d flags=0x%08lx format=%s\n",
		device->channel,
		device->lanes,
		device->mode_flags,
		ecv5546_dsi_fmt[device->format]);

	sp_dsi_host->lanes = device->lanes;
	sp_dsi_host->channel = device->channel;
	sp_dsi_host->mode_flags = device->mode_flags;

	switch (device->format) {
	case MIPI_DSI_FMT_RGB888:
		sp_dsi_host->format = DSI_PFORMAT_RGB888;
		sp_dsi_host->divider = 24 / sp_dsi_host->lanes;
		break;
	case MIPI_DSI_FMT_RGB666:
		sp_dsi_host->format = DSI_PFORMAT_RGB666;
		sp_dsi_host->divider = 24 / sp_dsi_host->lanes;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		sp_dsi_host->format = DSI_PFORMAT_RGB666_PACKED;
		sp_dsi_host->divider = 18 / sp_dsi_host->lanes;
		break;
	case MIPI_DSI_FMT_RGB565:
		sp_dsi_host->format = DSI_PFORMAT_RGB565;
		sp_dsi_host->divider = 16 / sp_dsi_host->lanes;
		break;
	default:
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev, "Unknown DSI format: %d.\n",
			      sp_dsi_host->format);
		return -1;
	}

	if (!(sp_dsi_host->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev,
			      "Only VIDEO mode panels supported currently.\n");
		return -1;
	}

	ecv5546_mipitx_phy_init(sp_dsi_host);
	ecv5546_mipitx_clock_init(sp_dsi_host);
	ecv5546_mipitx_lane_timing_init(sp_dsi_host);
	//ecv5546_dsi_tcon_init(sp_dsi_host);
	ecv5546_mipitx_dsi_cmd_mode_start(sp_dsi_host);

	return 0;
}

static int ecv5546_dsi_host_detach(struct mipi_dsi_host *host,
				  struct mipi_dsi_device *device)
{
	DRM_DEBUG_DRIVER("[TODO]\n");
	return 0;
}

static const struct mipi_dsi_host_ops ecv5546_dsi_host_ops = {
	.attach = ecv5546_dsi_host_attach,
	.detach = ecv5546_dsi_host_detach,
	.transfer = ecv5546_dsi_host_transfer,
};

static const struct drm_encoder_helper_funcs ecv5546_dsi_encoder_helper_funcs = {
	.atomic_check	= ecv5546_dsi_encoder_atomic_check,
	.mode_valid	= ecv5546_dsi_encoder_mode_valid,
	//.mode_set	= ecv5546_dsi_encoder_mode_set,
	//.disable = ecv5546_dsi_encoder_disable,
	//.enable = ecv5546_dsi_encoder_enable,
	.atomic_mode_set = ecv5546_dsi_encoder_atomic_mode_set,
	.atomic_disable = ecv5546_dsi_encoder_atomic_disable,
	.atomic_enable = ecv5546_dsi_encoder_atomic_enable,
};

static const struct of_device_id ecv5546_dsi_dt_match[] = {
	{ .compatible = "eys3d,ecv5546-dsi0" },
	{}
};

static int ecv5546_encoder_init(struct device *dev,
				   struct drm_device *drm_dev,
				   struct drm_encoder *encoder)
{
	u32 crtc_mask = drm_of_find_possible_crtcs(drm_dev, dev->of_node);
	int ret;


	if (!crtc_mask) {
		DRM_DEV_ERROR(dev, "failed to find crtc mask\n");
		return -EINVAL;
	}

	encoder->possible_crtcs = crtc_mask;
	DRM_DEV_DEBUG_DRIVER(dev, "crtc_mask:0x%X\n", crtc_mask);
	ret = drm_simple_encoder_init(drm_dev, encoder, DRM_MODE_ENCODER_DSI);
	if (ret) {
		DRM_DEV_ERROR(dev, "failed to init dsi encoder\n");
		return ret;
	}

	drm_encoder_helper_add(encoder, &ecv5546_dsi_encoder_helper_funcs);

	return 0;
}

static int ecv5546_dsi_bind(struct device *dev, struct device *master, void *data)
{
	//struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm = dev_get_drvdata(master);
	struct ecv5546_dsi_host *sp_dsi_host = dev_get_drvdata(dev);
	struct ecv5546_dsi_encoder *sp_dsi_encoder;
	//const struct of_device_id *match;
	//dma_cap_mask_t dma_mask;
	int ret;
	int child_count = 0;
	u32 endpoint_id = 0;
	struct device_node  *port, *endpoint;

	DRM_DEV_DEBUG_DRIVER(dev, "start.\n");

	//match = of_match_device(ecv5546_dsi_dt_match, dev);
	//if (!match)
	//	return -ENODEV;

	sp_dsi_host->port = 0;

	sp_dsi_encoder = devm_kzalloc(dev, sizeof(*sp_dsi_encoder),
					  GFP_KERNEL);
	if (!sp_dsi_encoder)
		return -ENOMEM;

	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	INIT_LIST_HEAD(&sp_dsi_host->bridge_chain);
	#endif
	sp_dsi_encoder->base.type = eCV5546_DRM_ENCODER_TYPE_DSI0;
	sp_dsi_encoder->sp_dsi_host = sp_dsi_host;

	sp_dsi_host->regset_g204.base = sp_dsi_host->regs + (eCV5546_REG_OFFSET_MIPITX_G204 << 7);
	sp_dsi_host->regset_g204.regs = sp_dsi_host_g0_regs;
	sp_dsi_host->regset_g204.nregs = ARRAY_SIZE(sp_dsi_host_g0_regs);

	sp_dsi_host->regset_g205.base = sp_dsi_host->regs + (eCV5546_REG_OFFSET_MIPITX_G205 << 7);
	sp_dsi_host->regset_g205.regs = sp_dsi_host_g1_regs;
	sp_dsi_host->regset_g205.nregs = ARRAY_SIZE(sp_dsi_host_g1_regs);

	sp_dsi_host->regset_ao_moon3.base = sp_dsi_host->ao_moon3;
	sp_dsi_host->regset_ao_moon3.regs = sp_dsi_ao_moon3_regs;
	sp_dsi_host->regset_ao_moon3.nregs = ARRAY_SIZE(sp_dsi_ao_moon3_regs);

	init_completion(&sp_dsi_host->xfer_completion);

	/*
	 * Get the endpoint node. In our case, dsi has one output port1
	 * to which the internal panel or external HDMI bridge connected.
	 * Cannot support both at the same time, internal panel first.
	 */
	//ret = drm_of_find_panel_or_bridge(dev->of_node, 1, 0, &panel, &dsi->bridge);
	port = of_graph_get_port_by_id(dev->of_node, 1);
	if (!port) {
		DRM_DEV_ERROR(dev,
			      "can't found port point, please init lvds panel port!\n");
		return -EINVAL;
	}
	for_each_child_of_node(port, endpoint) {
		child_count++;
		of_property_read_u32(endpoint, "reg", &endpoint_id);
		DRM_DEV_DEBUG(dev, "endpoint_id:%d\n", endpoint_id);
		ret = drm_of_find_panel_or_bridge(dev->of_node, 1, endpoint_id,
						  &sp_dsi_host->panel, &sp_dsi_host->bridge);
		of_node_put(endpoint);
		if (!ret)
			break;
	}
	of_node_put(port);
	if (!child_count) {
		DRM_DEV_ERROR(dev, "dsi0 port does not have any children\n");
		return -EINVAL;
	}
	if (ret) {
		DRM_DEV_DEBUG(dev, "drm_of_find_panel_or_bridge failed -%d\n", -ret);
		/* If the bridge or panel pointed by dev->of_node is not
		 * enabled, just return 0 here so that we don't prevent the DRM
		 * dev from being registered. Of course that means the DSI
		 * encoder won't be exposed, but that's not a problem since
		 * nothing is connected to it.
		 */
		if (ret == -ENODEV)
			return 0;

		return ret;
	}

	DRM_DEV_DEBUG_DRIVER(dev, "devm_drm_panel_bridge_add_typed\n");
	if (sp_dsi_host->panel && !sp_dsi_host->bridge) {
		sp_dsi_host->bridge = devm_drm_panel_bridge_add_typed(dev, sp_dsi_host->panel,
							      DRM_MODE_CONNECTOR_DSI);
		if (IS_ERR(sp_dsi_host->bridge))
			return PTR_ERR(sp_dsi_host->bridge);
	}

	sp_dsi_host->encoder = &sp_dsi_encoder->base.base;

	DRM_DEV_DEBUG_DRIVER(dev, "ecv5546_encoder_init\n");
	ret = ecv5546_encoder_init(dev, drm, sp_dsi_host->encoder);
	if (ret) {
		sp_dsi_host->encoder = NULL;
		return ret;
	}


	ret = drm_bridge_attach(sp_dsi_host->encoder, sp_dsi_host->bridge, NULL, 0);
	if (ret) {
		DRM_DEV_ERROR(dev, "bridge attach failed: %d\n", ret);
		sp_dsi_host->encoder = NULL;
		return ret;
	}
	DRM_DEV_DEBUG_DRIVER(dev, "Add [encoder:%d:%s] encoder:%p\n",
			sp_dsi_host->encoder->base.id, sp_dsi_host->encoder->name, sp_dsi_host->encoder);

	/* FIXME, use firmware EDID for lt8912b */
	#if IS_ENABLED(CONFIG_DRM_LOAD_EDID_FIRMWARE) && IS_ENABLED(CONFIG_DRM_LONTIUM_LT8912B)
	{
		DRM_DEV_DEBUG_DRIVER(dev, "Use firmware EDID edid/1920x1080.bin for lt8912b output\n");
		__drm_set_edid_firmware_path("edid/1920x1080.bin");
	}
	#endif

	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	/* Disable the atomic helper calls into the bridge.  We
	 * manually call the bridge pre_enable / enable / etc. calls
	 * from our driver, since we need to sequence them within the
	 * encoder's enable/disable paths.
	 */
	list_splice_init(&sp_dsi_host->encoder->bridge_chain, &sp_dsi_host->bridge_chain);
	#endif

	//ecv5546_debugfs_add_regset32(drm, dsi->variant->debugfs_name, &sp_dsi_host->regset);
	ecv5546_debugfs_add_regset32(drm, "regs_g204", &sp_dsi_host->regset_g204);
	ecv5546_debugfs_add_regset32(drm, "regs_g205", &sp_dsi_host->regset_g205);
	ecv5546_debugfs_add_regset32(drm, "regs_ao_moon3", &sp_dsi_host->regset_ao_moon3);

	pm_runtime_enable(dev);

	DRM_DEV_DEBUG_DRIVER(dev, "finish.\n");

	return 0;
}

static void ecv5546_dsi_unbind(struct device *dev, struct device *master,
			      void *data)
{
	struct ecv5546_dsi_host *sp_dsi_host = dev_get_drvdata(dev);


	if (sp_dsi_host->bridge)
		pm_runtime_disable(dev);

	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	/*
	 * Restore the bridge_chain so the bridge detach procedure can happen
	 * normally.
	 */
	list_splice_init(&sp_dsi_host->bridge_chain, &sp_dsi_host->encoder->bridge_chain);
	#endif
	drm_encoder_cleanup(sp_dsi_host->encoder);

	/* sp_dsi_encoder alloc by devm_kzalloc, not care it. */
	sp_dsi_host->encoder = NULL;
}

static const struct component_ops ecv5546_dsi_ops = {
	.bind   = ecv5546_dsi_bind,
	.unbind = ecv5546_dsi_unbind,
};

static const char * const ecv5546_disp_clkc[] = {
	"clkc_dispsys", "clkc_dmix",  "clkc_tgen", "clkc_tcon", "clkc_mipitx",
	"clkc_gpost0", "clkc_gpost1", "clkc_gpost2", "clkc_gpost3",
	"clkc_osd0", "clkc_osd1", "clkc_osd2", "clkc_osd3",
	"clkc_imgread0", "clkc_vscl0", "clkc_vpost0"
};

static const char * const ecv5546_disp_rtsc[] = {
	"rstc_dispsys", "rstc_dmix", "rstc_tgen", "rstc_tcon", "rstc_mipitx",
	"rstc_gpost0", "rstc_gpost1", "rstc_gpost2", "rstc_gpost3",
	"rstc_osd0", "rstc_osd1", "rstc_osd2", "rstc_osd3",
	"rstc_imgread0", "rstc_vscl0", "rstc_vpost0"
};

static int ecv5546_dsi_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ecv5546_dsi_host *sp_dsi_host;
	int ret, i;

	DRM_DEV_DEBUG_DRIVER(dev, "start\n");

	sp_dsi_host = devm_kzalloc(dev, sizeof(*sp_dsi_host), GFP_KERNEL);
	if (!sp_dsi_host)
		return -ENOMEM;
	dev_set_drvdata(dev, sp_dsi_host);

	sp_dsi_host->pdev = pdev;

	/*
	 * init clk & reset
	 */
	DRM_DEV_DEBUG_DRIVER(dev, "init clken & reset\n");
	for (i = 0; i < 16; i++) {
		sp_dsi_host->disp_clk[i] = devm_clk_get(dev, ecv5546_disp_clkc[i]);
		if (IS_ERR(sp_dsi_host->disp_clk[i]))
			return PTR_ERR(sp_dsi_host->disp_clk[i]);

		sp_dsi_host->disp_rstc[i] = devm_reset_control_get_exclusive(dev, ecv5546_disp_rtsc[i]);
		if (IS_ERR(sp_dsi_host->disp_rstc[i]))
			return dev_err_probe(dev, PTR_ERR(sp_dsi_host->disp_rstc[i]), "err get reset\n");

		ret = reset_control_deassert(sp_dsi_host->disp_rstc[i]);
		if (ret)
			return dev_err_probe(dev, ret, "failed to deassert reset\n");

		ret = clk_prepare_enable(sp_dsi_host->disp_clk[i]);
		if (ret)
			return ret;
	}

	/*
	 * get disp reg base (G204 - G205)
	 */
	DRM_DEV_DEBUG_DRIVER(dev, "init mipitx regs\n");
	sp_dsi_host->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sp_dsi_host->regs))
		return dev_err_probe(&pdev->dev, PTR_ERR(sp_dsi_host->regs), "dsi reg not found\n");
	/*
	 * get pllh reg base (G03_AO)
	 */
	DRM_DEV_DEBUG_DRIVER(dev, "init pllh regs\n");
	sp_dsi_host->ao_moon3 = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(sp_dsi_host->ao_moon3))
		return dev_err_probe(&pdev->dev, PTR_ERR(sp_dsi_host->ao_moon3), "dsi reg ao_moon3 not found\n");

	/* Note, the initialization sequence for DSI and panels is
	 * tricky.  The component bind above won't get past its
	 * -EPROBE_DEFER until the panel/bridge probes.  The
	 * panel/bridge will return -EPROBE_DEFER until it has a
	 * mipi_dsi_host to register its device to.  So, we register
	 * the host during pdev probe time, so ecv5546_drm as a whole can then
	 * -EPROBE_DEFER its component bind process until the panel
	 * successfully attaches.
	 */
	sp_dsi_host->dsi_host.ops = &ecv5546_dsi_host_ops;
	sp_dsi_host->dsi_host.dev = dev;
	mipi_dsi_host_register(&sp_dsi_host->dsi_host);

	ret = component_add(&pdev->dev, &ecv5546_dsi_ops);
	if (ret) {
		mipi_dsi_host_unregister(&sp_dsi_host->dsi_host);
		return ret;
	}

	DRM_DEV_DEBUG_DRIVER(dev, "finish\n");

	return ret;
}

static int ecv5546_dsi_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ecv5546_dsi_host *sp_dsi_host = dev_get_drvdata(dev);

	DRM_DEV_DEBUG_DRIVER(dev, "dsi driver remove.\n");

	component_del(&pdev->dev, &ecv5546_dsi_ops);
	mipi_dsi_host_unregister(&sp_dsi_host->dsi_host);

	return 0;
}

static int ecv5546_dsi_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ecv5546_dsi_host *sp_dsi_host = dev_get_drvdata(&pdev->dev);

	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "dsi driver suspend.\n");

	if (sp_dsi_host->bridge)
		pm_runtime_put(&pdev->dev);

	/*
	 * phy mipitx registers store...
	 */

	/*
	 * TODO
	 * phy power off, disable clock, disable irq...
	 */

	#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
	if (sp_dsi_host->encoder) {
		struct ecv5546_dsi_encoder *sp_dsi_encoder = to_ecv5546_dsi_encoder(sp_dsi_host->encoder);

		if (sp_dsi_encoder->is_enabled) {
			if (sp_dsi_host->bridge->funcs->disable)
				sp_dsi_host->bridge->funcs->disable(sp_dsi_host->bridge);
			if (sp_dsi_host->bridge->funcs->post_disable)
				sp_dsi_host->bridge->funcs->post_disable(sp_dsi_host->bridge);
		}
	}
	#endif

	return 0;
}

static int ecv5546_dsi_dev_resume(struct platform_device *pdev)
{
	struct ecv5546_dsi_host *sp_dsi_host = dev_get_drvdata(&pdev->dev);

	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "dsi driver resume.\n");

	if (sp_dsi_host->bridge)
		pm_runtime_get(&pdev->dev);

	/*
	 * TODO
	 * phy power on, enable clock, enable irq...
	 */

	/*
	 * phy mipitx restore...
	 */

	if (sp_dsi_host->encoder) {
		struct ecv5546_dsi_encoder *sp_dsi_encoder = to_ecv5546_dsi_encoder(sp_dsi_host->encoder);

		ecv5546_mipitx_phy_init(sp_dsi_host);
		ecv5546_mipitx_clock_init(sp_dsi_host);
		ecv5546_mipitx_lane_timing_init(sp_dsi_host);
		ecv5546_mipitx_dsi_cmd_mode_start(sp_dsi_host);
		if (sp_dsi_encoder->is_enabled) {
			ecv5546_mipitx_dsi_pixel_clock_setting(sp_dsi_host, NULL);
			ecv5546_mipitx_dsi_lane_clock_setting(sp_dsi_host, NULL);
			ecv5546_mipitx_dsi_video_mode_setting(sp_dsi_host, NULL);
			#if defined(DSI_BRIDGE_OPERATION_MANUALLY)
			if (sp_dsi_host->bridge->funcs->pre_enable)
				sp_dsi_host->bridge->funcs->pre_enable(sp_dsi_host->bridge);
			ecv5546_mipitx_dsi_video_mode_on(sp_dsi_host);
			if (sp_dsi_host->bridge->funcs->enable)
				sp_dsi_host->bridge->funcs->enable(sp_dsi_host->bridge);
			#else
			ecv5546_mipitx_dsi_video_mode_on(sp_dsi_host);
			#endif
		}
	}

	return 0;
}

struct platform_driver ecv5546_dsi_driver = {
	.probe   = ecv5546_dsi_dev_probe,
	.remove  = ecv5546_dsi_dev_remove,
	.suspend = ecv5546_dsi_dev_suspend,
	.resume  = ecv5546_dsi_dev_resume,
	.driver  = {
		.name = "ecv5546_dsi_host",
		.of_match_table = ecv5546_dsi_dt_match,
	},
};


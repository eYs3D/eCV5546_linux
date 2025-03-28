// SPDX-License-Identifier: GPL-2.0
/*
 * Parallel NAND Controller driver
 *
 * Derived from:
 *	Copyright (C) 2019-2021 Faraday Technology Corp.
 *
 * TODO:
 *	1.Convert the driver to exec_op() to have one less driver
 *	relying on the legacy interface.
 *	2.Get the device parameter from sp_paranand_ids.c rather
 *	than nand_attr[].
 *
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/partitions.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/ktime.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/mtd/bbm.h>

#include "sp_paranand.h"

static int startchn;
module_param(startchn, int, 0644);

/* Note: The unit of tWPST/tRPST/tWPRE/tRPRE field of sp_nfc_chip_timing is ns.
 *
 * tWH, tCH, tCLH, tALH, tCALH, tWP, tREH, tCR, tRSTO, tREAID,
 * tREA, tRP, tWB, tRB, tWHR, tWHR2, tRHW, tRR, tAR, tRC
 * tADL, tRHZ, tCCS, tCS, tCS2, tCLS, tCLR, tALS, tCALS, tCAL2, tCRES, tCDQSS, tDBS, tCWAW, tWPRE,
 * tRPRE, tWPST, tRPST, tWPSTH, tRPSTH, tDQSHZ, tDQSCK, tCAD, tDSL
 * tDSH, tDQSL, tDQSH, tDQSD, tCKWR, tWRCK, tCK, tCALS2, tDQSRE, tWPRE2, tRPRE2, tCEH
 */
static struct sp_nfc_chip_timing chip_timing[] = {
	{			//SAMSUNG_K9F2G08U0A_ZEBU
	 10, 5, 5, 5, 0, 12, 10, 0, 0, 0,
	 20, 12, 100, 0, 60, 0, 100, 20, 10, 0,
	 70, 100, 0, 20, 0, 12, 10, 12, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//GD9FS2G8F2A 256MiB 1.8V 8-bit
	 10, 5, 5, 5, 0, 15, 10, 0, 0, 0,
	 15, 12, 100, 0, 60, 0, 100, 20, 10, 25,
	 300, 100, 0, 15, 0, 12, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//GD9AU4G8F3A 512MiB 3.3V 8-bit
	 7, 5, 5, 5, 0, 12, 7, 0, 0, 0,
	 18, 10, 100, 0, 80, 0, 100, 20, 10, 20,
	 100, 100, 0, 15, 0, 12, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//GD9FU4G8F4B 512MiB 3.3V 8-bit
	 10, 5, 5, 5, 0, 12, 10, 0, 0, 0,
	 20, 12, 100, 0, 60, 0, 100, 20, 10, 0,
	 150, 100, 0, 15, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//W29N08GZSIBA 1GMiB 1.8V 8-bit
	 10, 5, 5, 5, 0, 12, 10, 0, 0, 0,
	 25, 12, 100, 0, 80, 0, 100, 20, 10, 0,
	 150, 100, 0, 15, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//K9GBG08U0B 4GiB 3.3V 8-bit
	 11, 5, 5, 5, 0, 11, 11, 0, 0, 0,
	 20, 11, 100, 0, 120, 300, 100, 20, 10, 25,
	 70, 100, 0, 20, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 0 10MHz
	 40, 20, 20, 20, 0, 60, 50, 0, 0, 0,
	 50, 50, 200, 0, 120, 0, 200, 40, 25, 100,
	 200, 200, 0, 70, 0, 50, 20, 50, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 1 20MHz
	 15, 10, 10, 10, 0, 30, 15, 0, 0, 0,
	 30, 25, 100, 0, 80, 0, 100, 20, 10, 50,
	 100, 100, 0, 35, 0, 25, 10, 25, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 2 28MHz
	 15, 10, 10, 10, 0, 20, 15, 0, 0, 0,
	 20, 17, 100, 0, 80, 0, 100, 20, 10, 35,
	 100, 100, 0, 25, 0, 15, 10, 15, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 3 33MHz
	 12, 5, 5, 5, 0, 17, 12, 0, 0, 0,
	 17, 15, 100, 0, 60, 0, 100, 20, 10, 30,
	 100, 100, 0, 25, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 4 40MHz
	 10, 5, 5, 5, 0, 15, 10, 0, 0, 0,
	 15, 12, 100, 0, 60, 0, 100, 20, 10, 25,
	 70, 100, 0, 20, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 5 50MHz
	 7, 5, 5, 5, 0, 12, 7, 0, 0, 0,
	 12, 12, 100, 0, 60, 0, 100, 20, 10, 20,
	 70, 100, 0, 15, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static struct sp_nfc_chip_timing sync_timing[] = {
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 0 20MHz
	 0, 10, 0, 0, 10, 0, 0, 0, 0, 0,
	 0, 0, 100, 0, 80, 0, 100, 20, 0, 0,
	 100, 0, 200, 35, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 20, 20, 25, 0,
	 0, 0, 0, 18, 2, 20, 50, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 1 33MHz
	 0, 5, 0, 0, 5, 0, 0, 0, 0, 0,
	 0, 0, 100, 0, 80, 0, 100, 20, 0, 0,
	 100, 0, 200, 25, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 20, 20, 25, 0,
	 0, 0, 0, 18, 2, 20, 30, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 2 50MHz
	 0, 4, 0, 0, 4, 0, 0, 0, 0, 0,
	 0, 0, 100, 0, 80, 0, 100, 20, 0, 0,
	 70, 0, 200, 15, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 20, 20, 25, 0,
	 0, 0, 0, 18, 2, 20, 20, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 3 67MHz
	 0, 3, 0, 0, 3, 0, 0, 0, 0, 0,
	 0, 0, 100, 0, 80, 0, 100, 20, 0, 0,
	 70, 0, 200, 15, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 20, 20, 25, 0,
	 0, 0, 0, 18, 3, 20, 15, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 4 83MHz
	 0, 2.5, 0, 0, 2.5, 0, 0, 0, 0, 0,
	 0, 0, 100, 0, 80, 0, 100, 20, 0, 0,
	 70, 0, 200, 15, 0, 0, 0, 0, 2.5, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 20, 20, 25, 0,
	 0, 0, 0, 18, 3, 20, 12, 0, 0, 0, 0, 0},
	{			//MT29F32G08ABXXX 4GiB 8-bit mode 5 100MHz
	 0, 2, 0, 0, 2, 0, 0, 0, 0, 0,
	 0, 0, 100, 0, 80, 0, 100, 20, 0, 0,
	 70, 0, 200, 15, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 20, 20, 25, 0,
	 0, 0, 0, 18, 3, 20, 10, 0, 0, 0, 0, 0}
};

static int sp_nfc_ooblayout_ecc(struct mtd_info *mtd, int section,
				struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section >= chip->ecc.steps)
		return -ERANGE;

	oobregion->offset = 0;
	oobregion->length = 1;

	return 0;
}

static int sp_nfc_ooblayout_free(struct mtd_info *mtd, int section,
				 struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section >= chip->ecc.steps)
		return -ERANGE;

	oobregion->offset = 0;
	oobregion->length = mtd->oobsize;

	return 0;
}

static const struct mtd_ooblayout_ops sp_nfc_ooblayout_ops = {
	.ecc = sp_nfc_ooblayout_ecc,
	.free = sp_nfc_ooblayout_free,
};

static void sp_nfc_set_warmup_cycle(struct nand_chip *chip,
				    u8 wr_cyc, u8 rd_cyc)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	int val;

	val = readl(nfc->regs + MEM_ATTR_SET2);
	val &= ~(0xFF);
	val |= (((rd_cyc & 0x3) << 4) | (wr_cyc & 0x3));
	writel(val, nfc->regs + MEM_ATTR_SET2);
}

static struct sp_nfc_chip_timing *sp_nfc_scan_timing(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);

	DBGLEVEL2(sp_nfc_dbg("nfc->name %s\n", nfc->name));
	if (strcmp(nfc->name, "K9F2G08XXX 256MiB ZEBU 8-bit") == 0) {
		return &chip_timing[0];
	} else if (strcmp(nfc->name, "9FS2G8F2A 256MiB 1.8V 8-bit") == 0) {
		return &chip_timing[1];
	} else if (strcmp(nfc->name, "9AU4G8F3A 512MiB 3.3V 8-bit") == 0) {
		return &chip_timing[2];
	} else if (strcmp(nfc->name, "9FU4G8F4B 512MiB 3.3V 8-bit") == 0) {
		return &chip_timing[3];
	} else if (strcmp(nfc->name, "W29N08GZSIBA 1GiB 1.8V 8-bit") == 0) {
		return &chip_timing[4];
	} else if (strcmp(nfc->name, "K9GBG08U0B 4GiB 3.3V 8-bit") == 0) {
		return &chip_timing[5];
	} else if (strcmp(nfc->name, "MT29F32G08ABXXX 4GiB 8-bit") == 0) {
		if (nfc->flash_type == ONFI2)
			return &sync_timing[nfc->timing_mode];
		else
			return &chip_timing[6 + nfc->timing_mode];
	} else {
		return NULL;
	}
}

/* The unit of Hclk is MHz, and the unit of Time is ns.
 * We desire to calculate N to satisfy N*(1/Hclk) > Time given Hclk and Time
 * ==> N > Time * Hclk
 * ==> N > Time * 10e(-9) * Hclk *10e(6)        --> take the order out
 * ==> N > Time * Hclk * 10e(-3)
 * ==> N > Time * Hclk / 1000
 * ==> N = (Time * Hclk + 999) / 1000
 */
static void sp_nfc_calc_timing(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	int twh = 0, twp = 0, treh = 0, tres = 0;
	int trlat = 0, tbsy = 0, t1 = 0;
	int tbuf1 = 0, tbuf2 = 0, tbuf3 = 0, tbuf4 = 0;
	int tpre = 0, tpst = 0, tpsth = 0, twrck = 0;
	int i, toggle_offset = 0;
	struct sp_nfc_chip_timing *p;
	u32 CLK, ftck, timing[4];

	CLK = nfc->clkfreq / 1000000;

	p = sp_nfc_scan_timing(chip);
	if (!p)
		DBGLEVEL1(sp_nfc_dbg("Failed to get AC timing!\n"));

	if (nfc->flash_type == LEGACY_FLASH) {
		// twh = max(twh, tch, tclh, talh)
		twh = max_4(p->twh, p->tch, p->tclh, p->talh);
		twh = (twh * CLK) / 1000;
		// twp = twp
		twp = (p->twp * CLK) / 1000;
		// treh = treh
		treh = (p->treh * CLK) / 1000;
		// tres = max(trea, trsto, treaid)
		tres = max_3(p->trea, p->trsto, p->treaid);
		tres = (tres * CLK) / 1000;
		// trlat < (tres + treh) + 2
		trlat = 3;
		// t1 = max(tcs, tcls, tals) - twp
		t1 = max_3(p->tcs, p->tcls, p->tals) - p->twp;
		if (t1 < 0)
			t1 = 0;
		else
			t1 = (t1 * CLK) / 1000;
		// tPSTH(EBI setup time) = max(tCS, tCLS, tALS)
		tpsth = max_3(p->tcs, p->tcls, p->tals);
		tpsth = (tpsth * CLK) / 1000;
		// tWRCK(EBI hold time) = max(tRHZ, tREH)
		twrck = max_2(p->trhz, p->treh);
		twrck = (twrck * CLK) / 1000;
	}

	else if (nfc->flash_type == ONFI2) {
		if (nfc->clkfreq == 400000000) { //unit = 2.5ns
			//mode 0 20MHz ---> 50ns
			if (nfc->timing_mode == 0)
				tres = 10;
			else if (nfc->timing_mode == 1)	// mode 1 33MHz ---> 30ns
				tres = 6;
			else if (nfc->timing_mode == 2)	// mode 2 50MHz ---> 20ns
				tres = 4;
			else if (nfc->timing_mode == 3)	// mode 3 67MHz ---> 15ns
				tres = 3;
			else if (nfc->timing_mode == 4)	// mode 4 83MHz ---> 12ns
				tres = 3;	// 2*2.5 < 6 < 3*2.5
			else if (nfc->timing_mode == 5)	// mode 5 100MHz ---> 10ns
				tres = 2;
		}
		// tWP = tCAD
		twp = (p->tcad * CLK) / 1000;

		// Fill this field with value N, FTck = mem_clk/2(N + 1)
		// Note:mem_clk is same as core_clk. Here, we'd like to
		// assign 30MHz to FTck.
		//tRES = 0;
		//FtCK = CLK / ( 2 * (tRES + 1));
		ftck = CLK / (2 * tres);

		// Increase p->tCK by one, is for the fraction which
		// cannot store in the variable, Integer type.
		p->tck = 1000 / ftck + 1;

		p->twpre = 2 * p->tck;
		p->twpst = 2 * p->tck;
		p->tdqsl = 1 * p->tck;
		p->tdqsh = 1 * p->tck;

		p->tckwr = (p->tdqsck + p->tck) / p->tck;
		if (p->tdqsck % p->tck != 0)
			p->tckwr += 1;

		t1 = (p->tcs * CLK) / 1000;

		tpre = 2;	// Assign 2 due to p->tWPRE is 1.5*p->tCK
		tpst = 2;	// Assign 2 due to p->tWPST is 1.5*p->tCK
		tpsth = ((p->tdqshz * ftck) / 1000) + 1;
		twrck = (p->twrck * CLK) / 1000;
	}
	// tBSY = max(tWB, tRB), min value = 1
	tbsy = max_2(p->twb, p->trb);
	tbsy = (tbsy * CLK) / 1000;
	if (tbsy < 1)
		tbsy = 1;
	// tBUF1 = max(tADL, tCCS)
	tbuf1 = max_2(p->tadl, p->tccs);
	tbuf1 = (tbuf1 * CLK) / 1000;
	// tBUF2 = max(tAR, tRR, tCLR, tCDQSS, tCRES, tCALS, tCALS2, tDBS)
	tbuf2 = max_2(max_4(p->tar, p->trr, p->tclr, p->tcdqss),
		      max_4(p->tcres, p->tcals, p->tcals2, p->tdbs));
	tbuf2 = (tbuf2 * CLK) / 1000;
	// tBUF3 = max(tRHW, tRHZ, tDQSHZ)
	tbuf3 = max_3(p->trhw, p->trhz, p->tdqshz);
	tbuf3 = (tbuf3 * CLK) / 1000;
	// tBUF4 = max(twhR, twhR2)
	tbuf4 = max_2(p->twhr, p->twhr2);
	if (nfc->flash_type == ONFI3)
		tbuf4 = max_2(tbuf4, p->tccs);
	tbuf4 = (tbuf4 * CLK) / 1000;

	// For FPGA, we use the looser AC timing
	if (nfc->flash_type == TOGGLE1 || nfc->flash_type == TOGGLE2) {
		toggle_offset = 3;
		treh += toggle_offset;
		tres += toggle_offset;
		twh += toggle_offset;
		twp += toggle_offset;
		t1 += toggle_offset;
		tbsy += toggle_offset;
		tbuf1 += toggle_offset;
		tbuf2 += toggle_offset;
		tbuf3 += toggle_offset;
		tbuf4 += toggle_offset;
		twrck += toggle_offset;
		tpsth += toggle_offset;
		tpst += toggle_offset;
		tpre += toggle_offset;
	}

	/* The value written to the register is incremented by 1 when actually used */
	twh -= 1;
	twp -= 1;
	treh -= 1;
	if (tres != 0)
		tres -= 1;
	tpre -= 1;
	tpst -= 1;
	tpsth -= 1;
	twrck -= 1;

	timing[0] = (twh << 24) | (twp << 16) | (treh << 8) | tres;
	timing[1] = (trlat << 16) | (tbsy << 8) | t1;
	timing[2] = (tbuf4 << 24) | (tbuf3 << 16) | (tbuf2 << 8) | tbuf1;
	timing[3] = (tpre << 28) | (tpst << 24) | (tpsth << 16) | twrck;

	for (i = 0; i < MAX_CHANNEL; i++) {
		writel(timing[0], nfc->regs + FL_AC_TIMING0(i));
		writel(timing[1], nfc->regs + FL_AC_TIMING1(i));
		writel(timing[2], nfc->regs + FL_AC_TIMING2(i));
		writel(timing[3], nfc->regs + FL_AC_TIMING3(i));

		/* A380: Illegal data latch occur at setting "rlat" field
		 * of ac timing register from 0 to 1.
		 * read command failed on A380 Linux
		 * Workaround: Set Software Reset(0x184) after
		 * "Trlat" field of AC Timing Register changing.
		 * Fixed in IP version 2.2.0
		 */
		if (trlat) {
			if (readl(nfc->regs + REVISION_NUM) < 0x020200) {
				writel((1 << i), nfc->regs + NANDC_SW_RESET);
				// Wait for the NANDC024 reset is complete
				do {
				} while (readl(nfc->regs + NANDC_SW_RESET) & (1 << i));
			}
		}
	}

	DBGLEVEL1(sp_nfc_dbg("AC Timing 0:0x%08x\n", timing[0]));
	DBGLEVEL1(sp_nfc_dbg("AC Timing 1:0x%08x\n", timing[1]));
	DBGLEVEL1(sp_nfc_dbg("AC Timing 2:0x%08x\n", timing[2]));
	DBGLEVEL1(sp_nfc_dbg("AC Timing 3:0x%08x\n", timing[3]));
}

static void sp_nfc_onfi_set_feature(struct nand_chip *chip, int val, int mode)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;

	/* val is sub-feature Parameter P1 (P2~P4 = 0)
	 * b[5:4] means Data interface: 0x0(SDR); 0x1(NV-DDR); 0x2(NV-DDR2)
	 * b[3:0] means Timing mode number
	 */
	writel(val, nfc->regs + SPARE_SRAM +
	       (nfc->cur_chan << nfc->spare_ch_offset));

	/* 0x1 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0x1;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(mode) |
	    CMD_START_CE(nfc->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(4) |
	    CMD_INDEX(ONFI_FIXFLOW_SETFEATURE);

	sp_nfc_issue_cmd(chip, &cmd_f);

	sp_nfc_wait(chip);
}

static u32 sp_nfc_onfi_get_feature(struct nand_chip *chip, int type)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u32 val;

	writel(0xffff,
	       nfc->regs + SPARE_SRAM +
	       (nfc->cur_chan << nfc->spare_ch_offset));

	/* 0x1 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0x1;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(type) |
	    CMD_START_CE(nfc->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(4) |
	    CMD_INDEX(ONFI_FIXFLOW_GETFEATURE);

	sp_nfc_issue_cmd(chip, &cmd_f);

	sp_nfc_wait(chip);

	val =
	    readl(nfc->regs + SPARE_SRAM +
		  (nfc->cur_chan << nfc->spare_ch_offset));

	return val;
}

static int sp_nfc_onfi_sync(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	u32 val, expect_val;
	int ret = -1;

	sp_nfc_select_chip(chip, 0);
	val = sp_nfc_onfi_get_feature(chip, LEGACY_FLASH);
	pr_info("SDR feature for Ch %d, CE %d: 0x%x\n", nfc->cur_chan,
		nfc->sel_chip, val);

	//Check if the SP support DDR interface
	val = readl(nfc->regs + FEATURE_1);
	if ((val & DDR_IF_EN) == 0)
		return ret;

	expect_val = (nfc->timing_mode | 0x10);

	sp_nfc_onfi_set_feature(chip, expect_val, LEGACY_FLASH);

	val = sp_nfc_onfi_get_feature(chip, ONFI2);
	pr_info("NV-DDR feature for Ch %d, CE %d: 0x%x\n", nfc->cur_chan,
		nfc->sel_chip, val);

	expect_val |= (expect_val << 8);
	if (val != expect_val)
		return ret;

	return 0;
}

static void sp_nfc_read_raw_id(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u8 id_size = 5;

	nfc->cur_chan = 0;
	nfc->sel_chip = 0;

	// Set the flash to Legacy mode, in advance.
	if (nfc->flash_type == ONFI2 || nfc->flash_type == ONFI3)
		sp_nfc_onfi_set_feature(chip, 0x00, LEGACY_FLASH);

	// Issue the RESET cmd
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(LEGACY_FLASH) |
	    CMD_START_CE(nfc->sel_chip) | CMD_INDEX(FIXFLOW_RESET);

	sp_nfc_issue_cmd(chip, &cmd_f);

	sp_nfc_wait(chip);

	// Issue the READID cmd
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_FLASH_TYPE(LEGACY_FLASH) | CMD_COMPLETE_EN |
	    CMD_INDEX(FIXFLOW_READID) | CMD_START_CE(nfc->sel_chip) |
	    CMD_BYTE_MODE | CMD_SPARE_NUM(id_size);

	sp_nfc_issue_cmd(chip, &cmd_f);

	sp_nfc_wait(chip);

	memcpy(nfc->flash_raw_id,
	       nfc->regs + SPARE_SRAM + (nfc->cur_chan << nfc->spare_ch_offset),
	       id_size);

	DBGLEVEL2(sp_nfc_dbg("ID@(ch:%d, ce:%d):0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			     nfc->cur_chan, nfc->sel_chip, nfc->flash_raw_id[0],
			     nfc->flash_raw_id[1], nfc->flash_raw_id[2],
			     nfc->flash_raw_id[3], nfc->flash_raw_id[4]));
}

static void sp_nfc_calibrate_dqs_delay(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int i, max_dqs_delay = 0;
	int id_size = 5;
	int id_size_ddr = (id_size << 1);
	u8 *p, *golden_p;
	u8 dqs_lower_bound = 0, dqs_upper_bound = 0, state;
	u32 val;

	p = kmalloc(id_size_ddr, GFP_KERNEL);
	golden_p = kmalloc(id_size_ddr, GFP_KERNEL);

	if (nfc->flash_type == ONFI2 || nfc->flash_type == ONFI3) {
		/* Extent the nfc from SDR to DDR.
		 *  Ex. If "0xaa, 0xbb, 0xcc, 0xdd, 0xee" is in SDR,
		 *  "0xaa, 0xaa, 0xbb, 0xbb, 0xcc, 0xcc, 0xdd, 0xdd, 0xee, 0xee" is in DDR(ONFI).
		 */
		for (i = 0; i < id_size; i++) {
			*(golden_p + (i << 1) + 0) = *(nfc->flash_raw_id + i);
			*(golden_p + (i << 1) + 1) = *(nfc->flash_raw_id + i);
		}
		DBGLEVEL2(sp_nfc_dbg
			  ("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			   *golden_p, *(golden_p + 1), *(golden_p + 2),
			   *(golden_p + 3), *(golden_p + 4), *(golden_p + 5)));
		max_dqs_delay = 20;
	} else if (nfc->flash_type == TOGGLE1 || nfc->flash_type == TOGGLE2) {
		/* Extent the nfc from SDR to DDR.
		 *  Ex. If "0xaa, 0xbb, 0xcc, 0xdd, 0xee" is in SDR,
		 *  "0xaa, 0xbb, 0xbb, 0xcc, 0xcc, 0xdd, 0xdd, 0xee, 0xee" is in DDR(TOGGLE).
		 */
		for (i = 0; i < id_size; i++) {
			*(golden_p + (i << 1) + 0) = *(nfc->flash_raw_id + i);
			*(golden_p + (i << 1) + 1) = *(nfc->flash_raw_id + i);
		}
		golden_p++;

		DBGLEVEL2(sp_nfc_dbg
			  ("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			   *golden_p, *(golden_p + 1), *(golden_p + 2),
			   *(golden_p + 3), *(golden_p + 4), *(golden_p + 5)));
		max_dqs_delay = 18;
	} else {
		pr_info("%s:Type:%d isn't allowed\n", __func__, nfc->flash_type);
		goto out;
	}

	state = 0;
	for (i = 0; i <= max_dqs_delay; i++) {
		// setting the dqs delay before READID.
		writel(i, nfc->regs + DQS_DELAY);
		memset(p, 0, id_size_ddr);

		// Issuing the READID
		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 = CMD_FLASH_TYPE(nfc->flash_type) | CMD_COMPLETE_EN |
		    CMD_INDEX(FIXFLOW_READID) | CMD_BYTE_MODE |
		    CMD_START_CE(nfc->sel_chip) | CMD_SPARE_NUM(id_size_ddr);

		sp_nfc_issue_cmd(chip, &cmd_f);

		sp_nfc_wait(chip);

		if (nfc->flash_type == ONFI2 || nfc->flash_type == ONFI3) {
			memcpy(p,
			       nfc->regs + SPARE_SRAM +
			       (nfc->cur_chan << nfc->spare_ch_offset),
			       id_size_ddr);
			if (state == 0 && memcmp(golden_p, p, id_size_ddr) == 0) {
				dqs_lower_bound = i;
				state = 1;
			} else if (state == 1 && memcmp(golden_p, p, id_size_ddr) != 0) {
				dqs_upper_bound = i - 1;
				break;
			}
		} else if (nfc->flash_type == TOGGLE1 || nfc->flash_type == TOGGLE2) {
			memcpy(p, nfc->regs + SPARE_SRAM +
			       (nfc->cur_chan << nfc->spare_ch_offset),
			       id_size_ddr - 1);

			if (state == 0 && memcmp(golden_p, p, (id_size_ddr - 1)) == 0) {
				dqs_lower_bound = i;
				state = 1;
			} else if (state == 1 && memcmp(golden_p, p, (id_size_ddr - 1)) != 0) {
				dqs_upper_bound = (i - 1);
				break;
			}
		}
		DBGLEVEL2(sp_nfc_dbg
			  ("===============================================\n"));
		DBGLEVEL2(sp_nfc_dbg
			  ("ID       :0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			   *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5),
			   *(p + 6), *(p + 7), *(p + 8)));
		DBGLEVEL2(sp_nfc_dbg
			  ("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			   *golden_p, *(golden_p + 1), *(golden_p + 2),
			   *(golden_p + 3), *(golden_p + 4), *(golden_p + 5),
			   *(golden_p + 6), *(golden_p + 7), *(golden_p + 8)));
		DBGLEVEL2(sp_nfc_dbg
			  ("===============================================\n"));
	}
	// Prevent the dqs_upper_bound is zero when ID still accuracy on the max dqs delay
	if (i == max_dqs_delay + 1)
		dqs_upper_bound = max_dqs_delay;

	pr_info("Upper:%d & Lower:%d for DQS, then Middle:%d\n",
		dqs_upper_bound, dqs_lower_bound,
		((dqs_upper_bound + dqs_lower_bound) >> 1));
	// Setting the middle dqs delay
	val = readl(nfc->regs + DQS_DELAY);
	val &= ~0x1F;
	val |= (((dqs_lower_bound + dqs_upper_bound) >> 1) & 0x1F);
	writel(val, nfc->regs + DQS_DELAY);
out:
	kfree(p);
	kfree(golden_p);
}

static void sp_nfc_calibrate_rlat(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int i, max_rlat;
	int id_size = 5;
	u8 *p, *golden_p;
	u8 rlat_lower_bound = 0, rlat_upper_bound = 0, state;
	u32 ac_reg0, ac_reg1, val;

	p = kmalloc(id_size, GFP_KERNEL);
	golden_p = kmalloc(id_size, GFP_KERNEL);

	if (nfc->flash_type == LEGACY_FLASH) {
		for (i = 0; i < id_size; i++)
			*(golden_p + i) = *(nfc->flash_raw_id + i);
	} else {
		pr_info("%s:Type:%d isn't allowed\n", __func__, nfc->flash_type);
		goto out;
	}

	ac_reg0 = readl(nfc->regs + FL_AC_TIMING0(0));
	max_rlat = (ac_reg0 & 0x1F) + ((ac_reg0 >> 8) & 0xF);
	ac_reg1 = readl(nfc->regs + FL_AC_TIMING1(0));
	state = 0;
	for (i = 0; i <= max_rlat; i++) {
		// setting the trlat delay before READID.
		val = (ac_reg1 & ~(0x3F << 16)) | (i << 16);
		writel(val, nfc->regs + FL_AC_TIMING1(0));
		memset(p, 0, id_size);

		// Issuing the READID
		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 = CMD_FLASH_TYPE(nfc->flash_type) | CMD_COMPLETE_EN |
		    CMD_INDEX(FIXFLOW_READID) | CMD_BYTE_MODE |
		    CMD_START_CE(nfc->sel_chip) | CMD_SPARE_NUM(id_size);

		sp_nfc_issue_cmd(chip, &cmd_f);

		sp_nfc_wait(chip);

		memcpy(p,
		       nfc->regs + SPARE_SRAM +
		       (nfc->cur_chan << nfc->spare_ch_offset), id_size);
		if (state == 0 && memcmp(golden_p, p, id_size) == 0) {
			rlat_lower_bound = i;
			state = 1;
		} else if (state == 1 && memcmp(golden_p, p, id_size) != 0) {
			rlat_upper_bound = i - 1;
			break;
		}

		DBGLEVEL2(sp_nfc_dbg
			  ("===============================================\n"));
		DBGLEVEL2(sp_nfc_dbg
			  ("ID       :0x%x 0x%x 0x%x 0x%x 0x%x\n", *p, *(p + 1),
			   *(p + 2), *(p + 3), *(p + 4)));
		DBGLEVEL2(sp_nfc_dbg
			  ("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x\n", *golden_p,
			   *(golden_p + 1), *(golden_p + 2), *(golden_p + 3),
			   *(golden_p + 4)));
		DBGLEVEL2(sp_nfc_dbg
			  ("===============================================\n"));
	}

	// Prevent the dqs_upper_bound is zero when ID still accuracy on the max dqs delay
	if (i == max_rlat + 1)
		rlat_upper_bound = max_rlat;

	DBGLEVEL2(sp_nfc_dbg("Upper:%d & Lower:%d for tRLAT, then Middle:%d\n",
			     rlat_upper_bound, rlat_lower_bound,
			     ((rlat_upper_bound + rlat_lower_bound) >> 1)));

	// Setting the middle tRLAT
	val = ac_reg1 & ~(0x3F << 16);
	val |= ((((rlat_upper_bound + rlat_lower_bound) >> 1) & 0x3F) << 16);
	writel(val, nfc->regs + FL_AC_TIMING1(0));
out:
	kfree(p);
	kfree(golden_p);
}

static int sp_nfc_available_oob(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct sp_nfc *nfc = nand_get_controller_data(chip);

	int ret = 0;
	int consume_byte, eccbyte, eccbyte_spare;
	int available_spare;

	if (nfc->useecc < 0)
		goto out;
	if (nfc->protect_spare != 0)
		nfc->protect_spare = 1;
	else
		nfc->protect_spare = 0;

	eccbyte = (nfc->useecc * 14) / 8;
	if (((nfc->useecc * 14) % 8) != 0)
		eccbyte++;

	consume_byte = (eccbyte * nfc->sector_per_page);
	if (nfc->protect_spare == 1) {
		eccbyte_spare = (nfc->useecc_spare * 14) / 8;
		if (((nfc->useecc_spare * 14) % 8) != 0)
			eccbyte_spare++;
		consume_byte += eccbyte_spare;
	}
	consume_byte += CONFIG_BI_BYTE;
	available_spare = nfc->spare - consume_byte;

	DBGLEVEL2(sp_nfc_dbg
		  ("mtd->erasesize:%d, mtd->writesize:%d\n", mtd->erasesize,
		   mtd->writesize));
	DBGLEVEL2(sp_nfc_dbg
		  ("page num:%d, nfc->eccbasft:%d, protect_spare:%d, spare:%d Byte\n",
		   mtd->erasesize / mtd->writesize, nfc->eccbasft,
		   nfc->protect_spare, nfc->spare));
	DBGLEVEL2(sp_nfc_dbg
		  ("consume_byte:%d, eccbyte:%d, eccbytes(spare):%d, useecc:%d bit\n",
		   consume_byte, eccbyte, eccbyte_spare, nfc->useecc));

	/*----------------------------------------------------------
	 * YAFFS require 16 bytes OOB without ECC, 28 bytes with
	 * ECC enable.
	 * BBT require 5 bytes for Bad Block Table marker.
	 */
	if (available_spare >= 4) {
		if (available_spare >= nfc->max_spare) {
			ret = nfc->max_spare;
		} else {
			if (available_spare >= 64)
				ret = 64;
			else if (available_spare >= 32)
				ret = 32;
			else if (available_spare >= 16)
				ret = 16;
			else if (available_spare >= 8)
				ret = 8;
			else if (available_spare >= 4)
				ret = 4;
		}
		pr_info("Available OOB is %d byte, but we use %d bytes in page mode.\n",
			available_spare, ret);
	} else {
		pr_info("Not enough OOB, try to reduce ECC correction bits.\n");
		pr_info("(Currently ECC setting for Data:%d)\n", nfc->useecc);
		pr_info("(Currently ECC setting for Spare:%d)\n", nfc->useecc_spare);
	}
out:
	return ret;
}

static u8 sp_nfc_read_byte(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	u32 lv;
	u8 b = 0;

	switch (nfc->cur_cmd) {
	case NAND_CMD_READID:
		b = readb(nfc->regs + SPARE_SRAM +
			  (nfc->cur_chan << nfc->spare_ch_offset) +
			  nfc->byte_ofs);
		nfc->byte_ofs += 1;
		if (nfc->byte_ofs == nfc->max_spare)
			nfc->byte_ofs = 0;
		break;
	case NAND_CMD_STATUS:
		lv = readl(nfc->regs + READ_STATUS0);
		lv = lv >> (nfc->cur_chan * 8);
		b = (lv & 0xFF);
		break;
	}
	return b;
}

static void sp_nfc_cmdfunc(struct nand_chip *chip, u32 command,
			   int column, int page_addr)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int real_pg, cmd_sts;
	u8 id_size = 5;

	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(nfc->flash_type);
	nfc->cur_cmd = command;
	if (page_addr != -1)
		nfc->page_addr = page_addr;
	if (column != -1)
		nfc->column = column;

	switch (command) {
	case NAND_CMD_READID:
		DBGLEVEL2(sp_nfc_dbg
			  ("Read ID@(CH:%d, CE:%d)\n", nfc->cur_chan,
			   nfc->sel_chip));
		nfc->byte_ofs = 0;
		// ID size is doubled when the mode is DDR.
		if (nfc->flash_type == TOGGLE1 || nfc->flash_type == TOGGLE2 ||
		    nfc->flash_type == ONFI2 || nfc->flash_type == ONFI3) {
			id_size = (id_size << 1);
		}

		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(nfc->sel_chip) | CMD_BYTE_MODE |
		    CMD_SPARE_NUM(id_size) | CMD_INDEX(FIXFLOW_READID);

		cmd_sts = sp_nfc_issue_cmd(chip, &cmd_f);
		if (!cmd_sts)
			sp_nfc_wait(chip);
		else
			pr_err("Read ID err\n");
		break;
	case NAND_CMD_RESET:
		DBGLEVEL2(sp_nfc_dbg
			  ("Cmd Reset@(CH:%d, CE:%d)\n", nfc->cur_chan,
			   nfc->sel_chip));

		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = 0;
		cmd_f.cq4 |= CMD_START_CE(nfc->sel_chip);
		if (nfc->flash_type == ONFI2 || nfc->flash_type == ONFI3)
			cmd_f.cq4 |= CMD_INDEX(ONFI_FIXFLOW_SYNCRESET);
		else
			cmd_f.cq4 |= CMD_INDEX(FIXFLOW_RESET);

		cmd_sts = sp_nfc_issue_cmd(chip, &cmd_f);
		if (!cmd_sts)
			sp_nfc_wait(chip);
		else
			pr_err("Reset Flash err\n");

		break;
	case NAND_CMD_STATUS:
		DBGLEVEL2(sp_nfc_dbg("Read Status\n"));

		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |=
		    CMD_START_CE(nfc->sel_chip) | CMD_INDEX(FIXFLOW_READSTATUS);

		cmd_sts = sp_nfc_issue_cmd(chip, &cmd_f);
		if (!cmd_sts)
			sp_nfc_wait(chip);
		else
			pr_err("Read Status err\n");

		break;
	case NAND_CMD_ERASE1:
		real_pg = nfc->page_addr;
		DBGLEVEL2(sp_nfc_dbg
			  ("Erase Page: 0x%x, Real:0x%x\n", nfc->page_addr,
			   real_pg));

		cmd_f.cq1 = real_pg;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(nfc->sel_chip) | CMD_SCALE(1);

		if (nfc->large_page) {
			cmd_f.row_cycle = ROW_ADDR_3CYCLE;
			cmd_f.col_cycle = COL_ADDR_2CYCLE;
			cmd_f.cq4 |= CMD_INDEX(LARGE_FIXFLOW_ERASE);
		} else {
			cmd_f.row_cycle = ROW_ADDR_2CYCLE;
			cmd_f.col_cycle = COL_ADDR_1CYCLE;
			cmd_f.cq4 |= CMD_INDEX(SMALL_FIXFLOW_ERASE);
		}

		/* Someone may be curious the following snippet that
		 * sp_nfc_issue_cmd doesn't be followed by
		 * sp_nfc_wait.
		 * Waiting cmd complete will be call on the mtd upper layer via
		 * the registered chip->waitfunc.
		 */
		cmd_sts = sp_nfc_issue_cmd(chip, &cmd_f);
		if (cmd_sts)
			pr_err("Erase block err\n");

		break;
	case NAND_CMD_ERASE2:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_SEQIN:
	default:
		DBGLEVEL2(sp_nfc_dbg
			  ("Unimplemented command (cmd=%u)\n", command));
		break;
	}
}

void sp_nfc_select_chip(struct nand_chip *chip, int cs)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);

	nfc->cur_chan = 0;
	nfc->sel_chip = 0;
	DBGLEVEL2(sp_nfc_dbg
		  ("==>chan = %d, ce = %d\n", nfc->cur_chan, nfc->sel_chip));
}

static void sp_nand_set_ecc(struct nand_chip *chip)
{
	u32 val;
	struct sp_nfc *nfc = nand_get_controller_data(chip);

	if (nfc->useecc > 0) {
		DBGLEVEL1(sp_nfc_dbg("ECC correction bits: %d\n", nfc->useecc));
		writel(0x01010101, nfc->regs + ECC_THRES_BITREG1);
		writel(0x01010101, nfc->regs + ECC_THRES_BITREG2);
		val = (nfc->useecc - 1) | ((nfc->useecc - 1) << 8) |
		    ((nfc->useecc - 1) << 16) | ((nfc->useecc - 1) << 24);
		writel(val, nfc->regs + ECC_CORRECT_BITREG1);
		writel(val, nfc->regs + ECC_CORRECT_BITREG2);

		val = readl(nfc->regs + ECC_CONTROL);
		val &= ~ECC_BASE;
		if (nfc->eccbasft > 9)
			val |= ECC_BASE;
		val |= (ECC_EN(0xFF) | ECC_ERR_MASK(0xFF));
		writel(val, nfc->regs + ECC_CONTROL);
		writel(ECC_INTR_THRES_HIT | ECC_INTR_CORRECT_FAIL,
		       nfc->regs + ECC_INTR_EN);
	} else {
		DBGLEVEL1(sp_nfc_dbg("ECC disabled\n"));
		writel(0, nfc->regs + ECC_THRES_BITREG1);
		writel(0, nfc->regs + ECC_THRES_BITREG2);
		writel(0, nfc->regs + ECC_CORRECT_BITREG1);
		writel(0, nfc->regs + ECC_CORRECT_BITREG2);

		val = readl(nfc->regs + ECC_CONTROL);
		val &= ~ECC_BASE;
		val &= ~(ECC_EN(0xFF) | ECC_ERR_MASK(0xFF));
		val |= ECC_NO_PARITY;
		writel(val, nfc->regs + ECC_CONTROL);
	}

	// Enable the Status Check Intr
	val = readl(nfc->regs + INTR_ENABLE);
	val &= ~INTR_ENABLE_STS_CHECK_EN(0xff);
	val |= INTR_ENABLE_STS_CHECK_EN(0xff);
	writel(val, nfc->regs + INTR_ENABLE);

	// Setting the ecc capability & threshold for spare
	writel(0x01010101, nfc->regs + ECC_THRES_BIT_FOR_SPARE_REG1);
	writel(0x01010101, nfc->regs + ECC_THRES_BIT_FOR_SPARE_REG2);
	val = (nfc->useecc_spare - 1) | ((nfc->useecc_spare - 1) << 8) |
	    ((nfc->useecc_spare - 1) << 16) | ((nfc->useecc_spare - 1) << 24);
	writel(val, nfc->regs + ECC_CORRECT_BIT_FOR_SPARE_REG1);
	writel(val, nfc->regs + ECC_CORRECT_BIT_FOR_SPARE_REG2);
}

void sp_nfc_set_actiming(struct nand_chip *chip)
{
	struct sp_nfc *nfc = nand_get_controller_data(chip);

	/* TODO: calibrate the DQS delay for Sync */
	if (strcmp(nfc->name, "MT29F32G08ABXXX 4GiB 8-bit") == 0) {
		if (nfc->ddr_enable) {
			nfc->timing_mode = 3;
			nfc->flash_type = ONFI2;
		} else {
			nfc->timing_mode = 5;
			nfc->flash_type = LEGACY_FLASH;
		}
	}

	/*----------------------------------------------------------
	 * ONFI synch mode means High Speed. If fails to change to
	 * Synch mode, then use flash as Async mode(Normal speed) and
	 * use LEGACY_LARGE fix flow.
	 */
	if (nfc->flash_type == ONFI2 || nfc->flash_type == ONFI3) {
		if (sp_nfc_onfi_sync(chip) == 0) {
			sp_nfc_calc_timing(chip);
			sp_nfc_calibrate_dqs_delay(chip);
		} else {
			nfc->flash_type = LEGACY_FLASH;
		}
	}
	// Toggle & ONFI flash has set the proper timing before READ ID.
	// We don't do that twice.
	if (nfc->flash_type == LEGACY_FLASH) {
		sp_nfc_calc_timing(chip);
		sp_nfc_calibrate_rlat(chip);
	}
}

static int sp_nfc_attach_chip(struct nand_chip *chip)
{
	struct nand_memory_organization *memorg;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_nfc *nfc = nand_get_controller_data(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	struct nand_device *base = &chip->base;
	const struct nand_ecc_props *requirements =
	    nanddev_get_ecc_requirements(base);
	u32 val;
	int i;

	nfc->eccbasft = fls(requirements->step_size) - 1;
	nfc->useecc = requirements->strength;
	nfc->protect_spare = 1;
	nfc->useecc_spare = 4;
	nfc->sector_per_page = mtd->writesize >> nfc->eccbasft;
	nfc->spare = mtd->oobsize;

	memorg = nanddev_get_memorg(&chip->base);

	/* usually, spare size is 1/32 page size */
	if (nfc->spare < (mtd->writesize >> 5))
		nfc->spare = (mtd->writesize >> 5);

	val = readl(nfc->regs + MEM_ATTR_SET);
	val &= ~(0x7 << 16);

	if (mtd->writesize > 512) {
		nfc->large_page = 1;
		/* bit[18:16] 1/2/3/4 -> PageSize=2k/4k/8k/16k */
		val |= ((fls(mtd->writesize) - 11) << 16);
	} else {
		nfc->large_page = 0;
		val |= PG_SZ_512;
	}

	val &= ~(0x3FF << 2);
	val |= ((mtd->erasesize / mtd->writesize - 1) << 2);
	/*  For BI_byte test */
	val &= ~BI_BYTE_MASK;
	val |= (CONFIG_BI_BYTE << 19);

	writel(val, nfc->regs + MEM_ATTR_SET);

	val = readl(nfc->regs + MEM_ATTR_SET2);
	val &= ~(0x3FF << 16);
	val |= VALID_PAGE((mtd->erasesize / mtd->writesize - 1));
	writel(val, nfc->regs + MEM_ATTR_SET2);

	i = sp_nfc_available_oob(mtd);
	if (likely(i >= 4)) {
		if (i > nfc->max_spare)
			memorg->oobsize = nfc->max_spare;
		else
			memorg->oobsize = i;
		nfc->spare = memorg->oobsize;
	} else {
		return -ENXIO;
	}

	DBGLEVEL1(sp_nfc_dbg("total oobsize: %d\n", memorg->oobsize));

	sp_nand_set_ecc(chip);

	ecc->engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
	ecc->size = requirements->step_size;
	ecc->bytes = (nfc->useecc * 14) / 8;	//why is 0
	ecc->strength = requirements->strength;

	ecc->read_oob = sp_nfc_read_oob;
	ecc->write_oob = sp_nfc_write_oob;
	ecc->read_page_raw = sp_nfc_read_page;

	if (nfc->dmac) {
		ecc->read_page = sp_nfc_read_page_by_dma;
		ecc->write_page = sp_nfc_write_page_by_dma;
		DBGLEVEL1(sp_nfc_dbg("Transfer: DMA\n"));
	} else {
		ecc->read_page = sp_nfc_read_page;
		ecc->write_page = sp_nfc_write_page;
		DBGLEVEL1(sp_nfc_dbg("Transfer: PIO\n"));
	}

	mtd_set_ooblayout(mtd, &sp_nfc_ooblayout_ops);

	DBGLEVEL2(sp_nfc_dbg("Use nand flash %s\n", mtd->name));
	DBGLEVEL2(sp_nfc_dbg("nfc->eccbasft: %d\n", nfc->eccbasft));
	DBGLEVEL2(sp_nfc_dbg("nfc->useecc: %d\n", nfc->useecc));
	DBGLEVEL2(sp_nfc_dbg("nfc->protect_spare: %d\n", nfc->protect_spare));
	DBGLEVEL2(sp_nfc_dbg("nfc->useecc_spare: %d\n", nfc->useecc_spare));
	DBGLEVEL2(sp_nfc_dbg("nfc->spare: %d\n", nfc->spare));
	DBGLEVEL2(sp_nfc_dbg("nfc->flash_type: %d\n", nfc->flash_type));
	DBGLEVEL2(sp_nfc_dbg
		  ("nfc->sector_per_page: %d\n", nfc->sector_per_page));

	return 0;
}

static const struct nand_controller_ops sp_nfc_controller_ops = {
	.attach_chip = sp_nfc_attach_chip,
};

static void sp_clk_disable_unprepare(void *nfc)
{
	clk_disable_unprepare(nfc);
}

static void sp_reset_control_assert(void *nfc)
{
	reset_control_assert(nfc);
}

static int sp_nfc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp_nfc *nfc;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct resource *r;
	int ret = 0, chipnum = 0;
	u64 size = 0, max_sz = -1;
	int i;
	u32 val;

	/* Allocate memory for the device structure (and zero it) */

	/*
	 * Initialize the parameter
	 */
	nfc = devm_kzalloc(dev, sizeof(struct sp_nfc), GFP_KERNEL);
	if (!nfc)
		ret = -ENOMEM;

	chip = &nfc->chip;
	chip->controller = &nfc->controller;

	/* reset */
	nfc->rstc = devm_reset_control_get(dev, NULL);
	if (IS_ERR(nfc->rstc))
		return dev_err_probe(dev, PTR_ERR(nfc->rstc),
				     "Failed to get reset\n");

	reset_control_deassert(nfc->rstc);

	ret = devm_add_action_or_reset(dev, sp_reset_control_assert, nfc->rstc);
	if (ret)
		return ret;

	/* clk */
	nfc->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(nfc->clk))
		return dev_err_probe(dev, PTR_ERR(nfc->clk),
				     "Failed to get clock\n");

	ret = clk_prepare_enable(nfc->clk);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to enable clock\n");

	ret = devm_add_action_or_reset(dev, sp_clk_disable_unprepare, nfc->clk);
	if (ret)
		return ret;

	nfc->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(nfc->regs)) {
		dev_err(dev, "Failed to ioremap for register.\n");
		return PTR_ERR(nfc->regs);
	}

	DBGLEVEL2(sp_nfc_dbg("nfc->regs:0x%08lx", (unsigned long)nfc->regs));

	r = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	chip->legacy.IO_ADDR_R = devm_ioremap_resource(dev, r);
	if (IS_ERR(chip->legacy.IO_ADDR_R)) {
		dev_err(dev, "Failed to ioremap for data port.\n");
		return PTR_ERR(chip->legacy.IO_ADDR_R);
	}

	if (of_property_read_u32
	    (pdev->dev.of_node, "clock-frequency", &nfc->clkfreq))
		nfc->clkfreq = CONFIG_SP_CLK_100M;

	ret = clk_set_rate(nfc->clk, nfc->clkfreq);
	if (ret) {
		dev_err(dev, "Failed to set clk rate\n");
		return ret;
	}

	nfc->clkfreq = clk_get_rate(nfc->clk);
	DBGLEVEL1(sp_nfc_dbg("nfc->clkfreq %d\n", nfc->clkfreq));

	nfc->dmac = NULL;
#ifdef ENABLE_DMA_MODE /* turn off the DMA mode */
	/* request dma channel */
	nfc->dmac = dma_request_chan(dev, "rxtx");
	if (IS_ERR(nfc->dmac)) {
		ret = PTR_ERR(nfc->dmac);
		nfc->dmac = NULL;
		return dev_err_probe(dev, ret,
				     "Failed to request DMA channel\n");
	} else {
		struct dma_slave_config dmac_cfg = {};

		dmac_cfg.src_addr = (phys_addr_t)r->start;
		dmac_cfg.dst_addr = dmac_cfg.src_addr;
		dmac_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_16_BYTES;
		dmac_cfg.dst_addr_width = dmac_cfg.src_addr_width;
		dmac_cfg.src_maxburst = 16;
		dmac_cfg.dst_maxburst = 16;
		ret = dmaengine_slave_config(nfc->dmac, &dmac_cfg);
		if (ret < 0) {
			dev_err(dev, "Failed to configure DMA channel\n");
			dma_release_channel(nfc->dmac);
			nfc->dmac = NULL;
		}
	}
#endif
	nand_controller_init(&nfc->controller);
	nfc->controller.ops = &sp_nfc_controller_ops;

	nfc->inverse = 0;	/* disable */
	nfc->scramble = 0;	/* disable */
	nfc->seed_val = 0;
	nfc->max_spare = 128;
	nfc->spare_ch_offset = 7;	/* shift 7 means 0x80 */
	nfc->flash_type = LEGACY_FLASH;
	nfc->timing_mode = 5;
	nfc->ddr_enable = 1;
	// Reset the HW
	// Note: We can't use the function of sp_nfc_soft_reset to reset the hw
	//       because the private data field of sp_nfc_data is null.
	writel(1, nfc->regs + GLOBAL_RESET);
	do {
	} while (readl(nfc->regs + GLOBAL_RESET));

	val = BUSY_RDY_LOC(6) | CMD_STS_LOC(0) | CE_NUM(2);
	if (nfc->inverse)
		val |= DATA_INVERSE;
	if (nfc->scramble)
		val |= DATA_SCRAMBLER;

	writel(val, nfc->regs + GENERAL_SETTING);

	if (nfc->scramble) {
		/* Support FW to program scramble seed */
		val = readl(nfc->regs + NANDC_EXT_CTRL);
		for (i = 0; i < MAX_CHANNEL; i++)
			val |= SEED_SEL(i);
		writel(val, nfc->regs + NANDC_EXT_CTRL);
		/* random set, b[13:0] */
		nfc->seed_val = 0x2fa5;
	}

	val = readl(nfc->regs + AHB_SLAVEPORT_SIZE);
	val &= ~0xFFF0FF;
	val |= AHB_SLAVE_SPACE_32KB;	//64K?
	for (i = 0; i < MAX_CHANNEL; i++)
		val |= AHB_PREFETCH(i);
	val |= AHB_PRERETCH_LEN(128);
	writel(val, nfc->regs + AHB_SLAVEPORT_SIZE);

	nand_set_controller_data(&nfc->chip, nfc);
	mtd = nand_to_mtd(&nfc->chip);
	mtd->owner = THIS_MODULE;
	mtd->dev.parent = &pdev->dev;
	/* Set the name same as uboot cmdline */
	mtd->name = NAME_DEFINE_IN_UBOOT;
	nfc->dev = &pdev->dev;
	chip->legacy.IO_ADDR_W = chip->legacy.IO_ADDR_R;
	chip->legacy.select_chip = sp_nfc_select_chip;
	chip->legacy.cmdfunc = sp_nfc_cmdfunc;
	chip->legacy.read_byte = sp_nfc_read_byte;
	chip->legacy.waitfunc = sp_nfc_wait;
	chip->legacy.chip_delay = 0;
	chip->options = NAND_NO_SUBPAGE_WRITE;	// | NAND_OWN_BUFFERS;

	// FIXME: Expect bad block management properly work after removing the option.
	// chip->options |= NAND_SKIP_BBTSCAN | NAND_NO_BBM_QUIRK;

	chip->options |= NAND_USES_DMA;
	chip->bbt_options = NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB;
	platform_set_drvdata(pdev, nfc);

	// Set the default AC timing/Warmup cyc for sp_pnand.
	// The register of AC timing/Warmup  keeps the value
	// set before although the Global Reset is set.
	sp_nfc_set_default_timing(chip);
	sp_nfc_set_warmup_cycle(chip, 0, 0);

	/* Store the device id to calibrate dqs delay */
	sp_nfc_read_raw_id(chip);

	/* Scan to find existence of the device */
	for (i = startchn; i < MAX_CHANNEL; i++) {
		pr_info("Scan Channel %d...\n", i);
		nfc->cur_chan = i;
		if (!nand_scan_with_ids
		    (chip, MAX_CE, (struct nand_flash_dev *)sp_nfc_ids)) {
			if (((max_sz - size) > mtd->size) &&
			    ((chipnum + nanddev_ntargets(&chip->base)) <=
			     NAND_MAX_CHIPS)) {
				nfc->valid_chip[i] =
				    nanddev_ntargets(&chip->base);
				chipnum += nanddev_ntargets(&chip->base);
				size +=
				    (chipnum *
				     nanddev_target_size(&chip->base));
			} else {
				pr_info("Can not accept more flash chips.\n");
				break;
			}
		}
	}
	// Disable the scan-state for sp_nfc_select_chip
	//data->scan_state = 0;

	if (chipnum == 0)
		return -ENXIO;

	mtd->size = size;
	nfc->name = chip->parameters.model;

	sp_nfc_set_actiming(chip);

	ret = mtd_device_register(mtd, NULL, 0);
	if (ret) {
		dev_err(dev, "Failed to register mtd device\n");
		nand_cleanup(chip);
		return ret;
	}

	return 0;
}

/*
 * Remove a NAND device.
 */
static int sp_nfc_remove(struct platform_device *pdev)
{
	struct sp_nfc *nfc = platform_get_drvdata(pdev);
	struct nand_chip *chip = &nfc->chip;
	int ret;

	iounmap(chip->legacy.IO_ADDR_R);

	ret = mtd_device_unregister(nand_to_mtd(chip));
	WARN_ON(ret);
	nand_cleanup(chip);

	iounmap(nfc->regs);
	kfree(nfc);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static u32 regs[124];		// 7 + 55 + 60 + 1 + 1
static int sp_nfc_suspend(struct device *dev)
{
	struct sp_nfc *nfc = dev_get_drvdata(dev);
	void __iomem *base = nfc->regs;
	u32 i, index;

	//printk(">>>>>> [DEBUG] PNAND suspend <<<<<<\n");

	/* Save the reg val */
	for (i = 0x8; i < 0x24; i += 4) {
		index = (i - 0x8) / 4;
		regs[index] = readl(base + i);
	}
	for (i = 0x104; i < 0x1E4; i += 4) {
		index = (i - 0x104) / 4;
		regs[7 + index] = readl(base + i);
	}
	for (i = 0x300; i < 0x3F0; i += 4) {
		index = (i - 0x300) / 4;
		regs[62 + index] = readl(base + i);
	}
	regs[122] = readl(nfc->regs + 0x42C);
	regs[123] = readl(nfc->regs + 0x508);

	return 0;
}

static int sp_nfc_resume(struct device *dev)
{
	struct sp_nfc *nfc = dev_get_drvdata(dev);
	struct nand_chip *chip = &nfc->chip;
	void __iomem *base = nfc->regs;
	u32 i, index, val;

	//printk(">>>>>> [DEBUG] PNAND resume <<<<<<\n");

	/* Restore the reg val */
	for (i = 0x8; i < 0x24; i += 4) {
		index = (i - 0x8) / 4;
		writel(regs[index], base + i);
	}
	for (i = 0x104; i < 0x1E4; i += 4) {
		index = (i - 0x104) / 4;
		writel(regs[7 + index], base + i);
	}
	for (i = 0x300; i < 0x3F0; i += 4) {
		index = (i - 0x300) / 4;
		writel(regs[62 + index], base + i);
	}
	writel_relaxed(regs[122], base + 0x42C);
	writel_relaxed(regs[123], base + 0x508);

	sp_nfc_abort(chip);

	if (nfc->flash_type == ONFI2) {
		nfc->flash_type = LEGACY_FLASH;
		nfc->timing_mode = 0;
		sp_nfc_calc_timing(chip);
		sp_nfc_onfi_set_feature(chip, 0x13, LEGACY_FLASH);

		nfc->flash_type = ONFI2;
		nfc->timing_mode = 3;
		sp_nfc_calc_timing(chip);
		val = sp_nfc_onfi_get_feature(chip, ONFI2);
		if (val != 0x1313)
			dev_err(dev, "failed to switch to ONFI2\n");
	}

	return 0;
}
#endif

static const struct of_device_id sp_nfc_dt_ids[] = {
	{.compatible = "eys3d,ecv5546-para-nand"},
	{}
};

MODULE_DEVICE_TABLE(of, sp_nfc_dt_ids);

static const struct dev_pm_ops sp_nfc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp_nfc_suspend,
				sp_nfc_resume)
};

static struct platform_driver sp_nfc_driver __refdata = {
	.probe = sp_nfc_probe,
	.remove = sp_nfc_remove,
	.driver = {
		   .name = "ecv5546-para-nand",
		   .owner = THIS_MODULE,
		   .pm = &sp_nfc_pm_ops,
		   .of_match_table = of_match_ptr(sp_nfc_dt_ids),
		   },
};

module_platform_driver(sp_nfc_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("eys3d Technology Corporation");
MODULE_DESCRIPTION("eYs3D Parallel NAND Flash Controller Driver");

// SPDX-License-Identifier: GPL-2.0

#include "linux/seq_file.h"
#include "../core.h"
#include "../pinctrl-utils.h"
#include "../devicetree.h"
#include "pinctrl-eys3d.h"
#include "gpio-ecv5546.h"

#define P(x) PINCTRL_PIN(x, D_PIS(x))

// function: GPIO. list of groups (pins)
const unsigned int sppctlpins_group[] = {
	0,  1,	2,  3,	4,   5,	  6,   7,   8,	 9,   10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20,  21,  22,  23,  24,	 25,  26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36,  37,  38,  39,  40,	 41,  42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52,  53,  54,  55,  56,	 57,  58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68,  69,  70,  71,  72,	 73,  74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84,  85,  86,  87,  88,	 89,  90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
};

static const struct pinctrl_pin_desc sppctlpins_all[] = {
	P(0),  P(1),   P(2),   P(3),   P(4),   P(5),   P(6),   P(7),  P(8),
	P(9),  P(10),  P(11),  P(12),  P(13),  P(14),  P(15),  P(16), P(17),
	P(18), P(19),  P(20),  P(21),  P(22),  P(23),  P(24),  P(25), P(26),
	P(27), P(28),  P(29),  P(30),  P(31),  P(32),  P(33),  P(34), P(35),
	P(36), P(37),  P(38),  P(39),  P(40),  P(41),  P(42),  P(43), P(44),
	P(45), P(46),  P(47),  P(48),  P(49),  P(50),  P(51),  P(52), P(53),
	P(54), P(55),  P(56),  P(57),  P(58),  P(59),  P(60),  P(61), P(62),
	P(63), P(64),  P(65),  P(66),  P(67),  P(68),  P(69),  P(70), P(71),
	P(72), P(73),  P(74),  P(75),  P(76),  P(77),  P(78),  P(79), P(80),
	P(81), P(82),  P(83),  P(84),  P(85),  P(86),  P(87),  P(88), P(89),
	P(90), P(91),  P(92),  P(93),  P(94),  P(95),  P(96),  P(97), P(98),
	P(99), P(100), P(101), P(102), P(103), P(104), P(105),
};

static const size_t sppctlpins_all_nums = ARRAY_SIZE(sppctlpins_all);

static const unsigned int pins_spif[] = { 21, 22, 23, 24, 25, 26 };
static const struct sppctlgrp_t ecv5546grps_spif[] = {
	EGRP("SPI_FLASH", 1, pins_spif),
};

static const unsigned int pins_emmc[] = { 20, 28, 29, 30, 31, 32,
					  33, 34, 35, 36, 37 };
static const struct sppctlgrp_t ecv5546grps_emmc[] = {
	EGRP("EMMC", 1, pins_emmc),
};

static const unsigned int pins_spi_nand_x1[] = { 30, 31, 32, 33, 34, 35 };
static const unsigned int pins_spi_nand_x2[] = { 21, 22, 23, 24, 25, 26 };
static const struct sppctlgrp_t ecv5546grps_spi_nand[] = {
	EGRP("SPI_NAND_X1", 1, pins_spi_nand_x1),
	EGRP("SPI_NAND_X2", 2, pins_spi_nand_x2),
};

static const unsigned int pins_sdc30[] = { 38, 39, 40, 41, 42, 43 };
static const struct sppctlgrp_t ecv5546grps_sdc30[] = {
	EGRP("SD_CARD", 1, pins_sdc30),
};

static const unsigned int pins_sdio30[] = { 44, 45, 46, 47, 48, 49 };
static const struct sppctlgrp_t ecv5546grps_sdio30[] = {
	EGRP("SDIO", 1, pins_sdio30),
};

static const unsigned int pins_para_nand[] = { 20, 21, 22, 23, 24, 25,
					       26, 27, 28, 29, 30, 31,
					       32, 33, 34, 35, 36 };
static const struct sppctlgrp_t ecv5546grps_para_nand[] = {
	EGRP("PARA_NAND", 1, pins_para_nand),
};

static const unsigned int pins_usb_otg[] = { 18, 19 };
static const struct sppctlgrp_t ecv5546grps_usb_otg[] = {
	EGRP("USB_OTG", 1, pins_usb_otg),
};

static const unsigned int pins_gmac_rgmii[] = { 3,  4,	5,  6,	7,  8,	9,
						10, 11, 12, 13, 14, 15, 16 };
static const unsigned int pins_gmac_rmii[] = { 4, 5, 6, 7, 8, 9, 10, 11, 12 };

static const struct sppctlgrp_t ecv5546grps_gmac[] = {
	EGRP("GMAC_RGMII", 1, pins_gmac_rgmii),
	EGRP("GMAC_RMII", 2, pins_gmac_rmii),
};

static const unsigned int pins_pwm0_x1[] = { 78 };
static const unsigned int pins_pwm0_x2[] = { 58 };
static const struct sppctlgrp_t ecv5546grps_pwm0[] = {
	EGRP("PWM0_X1", 1, pins_pwm0_x1),
	EGRP("PWM0_X2", 2, pins_pwm0_x2),
};

static const unsigned int pins_pwm1_x1[] = { 79 };
static const unsigned int pins_pwm1_x2[] = { 59 };
static const struct sppctlgrp_t ecv5546grps_pwm1[] = {
	EGRP("PWM1_X1", 1, pins_pwm1_x1),
	EGRP("PWM1_X2", 2, pins_pwm1_x2),
};

static const unsigned int pins_pwm2_x1[] = { 60 };
static const unsigned int pins_pwm2_x2[] = { 92 };
static const struct sppctlgrp_t ecv5546grps_pwm2[] = {
	EGRP("PWM2_X1", 1, pins_pwm2_x1),
	EGRP("PWM2_X2", 2, pins_pwm2_x2),
};

static const unsigned int pins_pwm3_x1[] = { 61 };
static const unsigned int pins_pwm3_x2[] = { 93 };
static const struct sppctlgrp_t ecv5546grps_pwm3[] = {
	EGRP("PWM3_X1", 1, pins_pwm3_x1),
	EGRP("PWM3_X2", 2, pins_pwm3_x2),
};

static const unsigned int pins_uart0_x1[] = { 50, 51 };
static const unsigned int pins_uart0_x2[] = { 68, 69 };
static const struct sppctlgrp_t ecv5546grps_uart0[] = {
	EGRP("UART0_X1", 1, pins_uart0_x1),
	EGRP("UART0_X2", 2, pins_uart0_x2),
};

static const unsigned int pins_uart1_x1[] = { 52, 53 };
static const unsigned int pins_uart1_x2[] = { 64, 65 };
static const struct sppctlgrp_t ecv5546grps_uart1[] = {
	EGRP("UART1_X1", 1, pins_uart1_x1),
	EGRP("UART1_X2", 2, pins_uart1_x2),
};

static const unsigned int pins_uart1_fc_x1[] = { 54, 55 };
static const unsigned int pins_uart1_fc_x2[] = { 66, 67 };
static const struct sppctlgrp_t ecv5546grps_uart1_fc[] = {
	EGRP("UART1_FC_X1", 1, pins_uart1_fc_x1),
	EGRP("UART1_FC_X2", 2, pins_uart1_fc_x2),
};

static const unsigned int pins_uart2_x1[] = { 56, 57 };
static const unsigned int pins_uart2_x2[] = { 76, 77 };
static const struct sppctlgrp_t ecv5546grps_uart2[] = {
	EGRP("UART2_X1", 1, pins_uart2_x1),
	EGRP("UART2_X2", 2, pins_uart2_x2),
};

static const unsigned int pins_uart2_fc_x1[] = { 58, 59 };
static const unsigned int pins_uart2_fc_x2[] = { 78, 79 };
static const struct sppctlgrp_t ecv5546grps_uart2_fc[] = {
	EGRP("UART2_FC_X1", 1, pins_uart2_fc_x1),
	EGRP("UART2_FC_X2", 2, pins_uart2_fc_x2),
};

static const unsigned int pins_uart3_x1[] = { 62, 63 };
static const unsigned int pins_uart3_x2[] = { 7, 8 };
static const struct sppctlgrp_t ecv5546grps_uart3[] = {
	EGRP("UART3_X1", 1, pins_uart3_x1),
	EGRP("UART3_X2", 2, pins_uart3_x2),
};

static const unsigned int pins_uadbg[] = { 13, 14 };
static const struct sppctlgrp_t ecv5546grps_uadbg[] = {
	EGRP("UADBG", 1, pins_uadbg),
};

static const unsigned int pins_uart6_x1[] = { 80, 81 };
static const unsigned int pins_uart6_x2[] = { 48, 49 };
static const struct sppctlgrp_t ecv5546grps_uart6[] = {
	EGRP("UART6_X1", 1, pins_uart6_x1),
	EGRP("UART6_X2", 2, pins_uart6_x2),
};

static const unsigned int pins_uart7[] = { 82, 83 };
static const struct sppctlgrp_t ecv5546grps_uart7[] = {
	EGRP("UART7", 1, pins_uart7),
};

static const unsigned int pins_i2c_combo0_x1[] = { 68, 69 };
static const unsigned int pins_i2c_combo0_x2[] = { 54, 55 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo0[] = {
	EGRP("I2C_COMBO0_X1", 1, pins_i2c_combo0_x1),
	EGRP("I2C_COMBO0_X2", 2, pins_i2c_combo0_x2),
};

static const unsigned int pins_i2c_combo1[] = { 70, 71 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo1[] = {
	EGRP("I2C_COMBO1", 1, pins_i2c_combo1),
};

static const unsigned int pins_i2c_combo2_x1[] = { 76, 77 };
static const unsigned int pins_i2c_combo2_x2[] = { 56, 57 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo2[] = {
	EGRP("I2C_COMBO2_X1", 1, pins_i2c_combo2_x1),
	EGRP("I2C_COMBO2_X2", 2, pins_i2c_combo2_x2),
};

static const unsigned int pins_i2c_combo3[] = { 88, 89 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo3[] = {
	EGRP("I2C_COMBO3", 1, pins_i2c_combo3),
};

static const unsigned int pins_i2c_combo4[] = { 90, 91 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo4[] = {
	EGRP("I2C_COMBO4", 1, pins_i2c_combo4),
};

static const unsigned int pins_i2c_combo5[] = { 92, 93 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo5[] = {
	EGRP("I2C_COMBO5", 1, pins_i2c_combo5),
};

static const unsigned int pins_i2c_combo6_x1[] = { 84, 85 };
static const unsigned int pins_i2c_combo6_x2[] = { 1, 2 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo6[] = {
	EGRP("I2C_COMBO6_X1", 1, pins_i2c_combo6_x1),
	EGRP("I2C_COMBO6_X2", 2, pins_i2c_combo6_x2),
};

static const unsigned int pins_i2c_combo7_x1[] = { 86, 87 };
static const unsigned int pins_i2c_combo7_x2[] = { 3, 4 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo7[] = {
	EGRP("I2C_COMBO7_X1", 1, pins_i2c_combo7_x1),
	EGRP("I2C_COMBO7_X2", 2, pins_i2c_combo7_x2),
};

static const unsigned int pins_i2c_combo8_x1[] = { 95, 96 };
static const unsigned int pins_i2c_combo8_x2[] = { 9, 10 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo8[] = {
	EGRP("I2C_COMBO8_X1", 1, pins_i2c_combo8_x1),
	EGRP("I2C_COMBO8_X2", 2, pins_i2c_combo8_x2),
};

static const unsigned int pins_i2c_combo9_x1[] = { 97, 98 };
static const unsigned int pins_i2c_combo9_x2[] = { 11, 12 };
static const struct sppctlgrp_t ecv5546grps_i2c_combo9[] = {
	EGRP("I2C_COMBO9_X1", 1, pins_i2c_combo9_x1),
	EGRP("I2C_COMBO9_X2", 2, pins_i2c_combo9_x2),
};

static const unsigned int pins_spi_master0_x1[] = { 64, 65, 66, 67 };
static const unsigned int pins_spi_master0_x2[] = { 9, 10, 11, 12 };
static const struct sppctlgrp_t ecv5546grps_spi_master0[] = {
	EGRP("SPI_MASTER0_X1", 1, pins_spi_master0_x1),
	EGRP("SPI_MASTER0_X2", 2, pins_spi_master0_x2),
};

static const unsigned int pins_spi_master1_x1[] = { 80, 81, 82, 83 };
static const unsigned int pins_spi_master1_x2[] = { 14, 15, 16, 17 };
static const struct sppctlgrp_t ecv5546grps_spi_master1[] = {
	EGRP("SPI_MASTER1_X1", 1, pins_spi_master1_x1),
	EGRP("SPI_MASTER1_X2", 2, pins_spi_master1_x2),
};

static const unsigned int pins_spi_master2[] = { 88, 89, 90, 91 };
static const struct sppctlgrp_t ecv5546grps_spi_master2[] = {
	EGRP("SPI_MASTER2", 1, pins_spi_master2),
};

static const unsigned int pins_spi_master3_x1[] = { 44, 45, 46, 47 };
static const unsigned int pins_spi_master3_x2[] = { 52, 53, 54, 55 };
static const struct sppctlgrp_t ecv5546grps_spi_master3[] = {
	EGRP("SPI_MASTER3_X1", 1, pins_spi_master3_x1),
	EGRP("SPI_MASTER3_X2", 2, pins_spi_master3_x2),
};

static const unsigned int pins_spi_master4[] = { 72, 73, 74, 75 };
static const struct sppctlgrp_t ecv5546grps_spi_master4[] = {
	EGRP("SPI_MASTER4", 1, pins_spi_master4),
};

static const unsigned int pins_spi_slave0_x1[] = { 94, 95, 96, 97 };
static const unsigned int pins_spi_slave0_x2[] = { 72, 73, 74, 75 };
static const struct sppctlgrp_t ecv5546grps_spi_slave0[] = {
	EGRP("SPI_SLAVE0_X1", 1, pins_spi_slave0_x1),
	EGRP("SPI_SLAVE0_X2", 2, pins_spi_slave0_x2),
};

static const unsigned int pins_aud_tdmtx_xck[] = { 93 };
static const struct sppctlgrp_t ecv5546grps_aud_tdmtx_xck[] = {
	EGRP("AUD_TDMTX_XCK", 1, pins_aud_tdmtx_xck),
};

static const unsigned int pins_aud_dac_xck1[] = { 71 };
static const struct sppctlgrp_t ecv5546grps_aud_dac_xck1[] = {
	EGRP("AUD_DAC_XCK1", 1, pins_aud_dac_xck1),
};

static const unsigned int pins_aud_dac_xck[] = { 83 };
static const struct sppctlgrp_t ecv5546grps_aud_dac_xck[] = {
	EGRP("AUD_DAC_XCK", 1, pins_aud_dac_xck),
};

static const unsigned int pins_aud_au2_data0[] = { 82 };
static const struct sppctlgrp_t ecv5546grps_aud_au2_data0[] = {
	EGRP("AUD_AU2_DATA0", 1, pins_aud_au2_data0),
};

static const unsigned int pins_aud_au1_data0[] = { 58 };
static const struct sppctlgrp_t ecv5546grps_aud_au1_data0[] = {
	EGRP("AUD_AU1_DATA0", 1, pins_aud_au1_data0),
};

static const unsigned int pins_aud_au2_ck[] = { 80, 81 };
static const struct sppctlgrp_t ecv5546grps_aud_au2_ck[] = {
	EGRP("AUD_AU2_CK", 1, pins_aud_au2_ck),
};

static const unsigned int pins_aud_au1_ck[] = { 56, 57 };
static const struct sppctlgrp_t ecv5546grps_aud_au1_ck[] = {
	EGRP("AUD_AU1_CK", 1, pins_aud_au1_ck),
};

static const unsigned int pins_aud_au_adc_data0_x1[] = { 94, 95, 96, 97 };
static const unsigned int pins_aud_au_adc_data0_x2[] = { 72, 73, 74, 75 };
static const struct sppctlgrp_t ecv5546grps_aud_au_adc_data0[] = {
	EGRP("AUD_AU_ADC_DATA0_X1", 1, pins_aud_au_adc_data0_x1),
	EGRP("AUD_AU_ADC_DATA0_X2", 2, pins_aud_au_adc_data0_x2),
};

static const unsigned int pins_aud_adc2_data0[] = { 82 };
static const struct sppctlgrp_t ecv5546grps_aud_adc2_data0[] = {
	EGRP("AUD_ADC2_DATA0", 1, pins_aud_adc2_data0),
};

static const unsigned int pins_aud_adc1_data0[] = { 58 };
static const struct sppctlgrp_t ecv5546grps_aud_adc1_data0[] = {
	EGRP("AUD_ADC1_DATA0", 1, pins_aud_adc1_data0),
};

static const unsigned int pins_aud_tdm[] = { 94, 95, 96, 97 };
static const struct sppctlgrp_t ecv5546grps_aud_tdm[] = {
	EGRP("AUD_TDM", 1, pins_aud_tdm),
};

static const unsigned int pins_spdif_x1[] = { 91 };
static const unsigned int pins_spdif_x2[] = { 53 };
static const unsigned int pins_spdif_x3[] = { 54 };
static const unsigned int pins_spdif_x4[] = { 55 };
static const unsigned int pins_spdif_x5[] = { 62 };
static const unsigned int pins_spdif_x6[] = { 52 };
static const struct sppctlgrp_t ecv5546grps_spdif_in[] = {
	EGRP("SPDIF_IN_X1", 1, pins_spdif_x1),
	EGRP("SPDIF_IN_X2", 2, pins_spdif_x2),
	EGRP("SPDIF_IN_X3", 3, pins_spdif_x3),
	EGRP("SPDIF_IN_X4", 4, pins_spdif_x4),
	EGRP("SPDIF_IN_X5", 5, pins_spdif_x5),
	EGRP("SPDIF_IN_X6", 6, pins_spdif_x6),
};

static const struct sppctlgrp_t ecv5546grps_spdif_out[] = {
	EGRP("SPDIF_OUT_X1", 1, pins_spdif_x1),
	EGRP("SPDIF_OUT_X2", 2, pins_spdif_x2),
	EGRP("SPDIF_OUT_X3", 3, pins_spdif_x3),
	EGRP("SPDIF_OUT_X4", 4, pins_spdif_x4),
	EGRP("SPDIF_OUT_X5", 5, pins_spdif_x5),
	EGRP("SPDIF_OUT_X6", 6, pins_spdif_x6),
};

static const unsigned int pins_int0_x1[] = { 1 };
static const unsigned int pins_int0_x2[] = { 2 };
static const unsigned int pins_int0_x3[] = { 3 };
static const unsigned int pins_int0_x4[] = { 4 };
static const unsigned int pins_int0_x5[] = { 5 };
static const unsigned int pins_int0_x6[] = { 6 };
static const unsigned int pins_int0_x7[] = { 13 };
static const unsigned int pins_int0_x8[] = { 14 };
static const unsigned int pins_int0_x9[] = { 15 };
static const struct sppctlgrp_t ecv5546grps_int0[] = {
	EGRP("INT0_X1", 1, pins_int0_x1), EGRP("INT0_X2", 2, pins_int0_x2),
	EGRP("INT0_X3", 3, pins_int0_x3), EGRP("INT0_X4", 4, pins_int0_x4),
	EGRP("INT0_X5", 5, pins_int0_x5), EGRP("INT0_X6", 6, pins_int0_x6),
	EGRP("INT0_X7", 7, pins_int0_x7), EGRP("INT0_X8", 8, pins_int0_x8),
	EGRP("INT0_X9", 9, pins_int0_x9),
};

static const struct sppctlgrp_t ecv5546grps_int1[] = {
	EGRP("INT1_X1", 1, pins_int0_x1), EGRP("INT1_X2", 2, pins_int0_x2),
	EGRP("INT1_X3", 3, pins_int0_x3), EGRP("INT1_X4", 4, pins_int0_x4),
	EGRP("INT1_X5", 5, pins_int0_x5), EGRP("INT1_X6", 6, pins_int0_x6),
	EGRP("INT1_X7", 7, pins_int0_x7), EGRP("INT1_X8", 8, pins_int0_x8),
	EGRP("INT1_X9", 9, pins_int0_x9),
};

static const unsigned int pins_int2_x1[] = { 5 };
static const unsigned int pins_int2_x2[] = { 6 };
static const unsigned int pins_int2_x3[] = { 7 };
static const unsigned int pins_int2_x4[] = { 8 };
static const unsigned int pins_int2_x5[] = { 9 };
static const unsigned int pins_int2_x6[] = { 10 };
static const unsigned int pins_int2_x7[] = { 11 };
static const unsigned int pins_int2_x8[] = { 16 };
static const unsigned int pins_int2_x9[] = { 17 };
static const struct sppctlgrp_t ecv5546grps_int2[] = {
	EGRP("INT2_X1", 1, pins_int2_x1), EGRP("INT2_X2", 2, pins_int2_x2),
	EGRP("INT2_X3", 3, pins_int2_x3), EGRP("INT2_X4", 4, pins_int2_x4),
	EGRP("INT2_X5", 5, pins_int2_x5), EGRP("INT2_X6", 6, pins_int2_x6),
	EGRP("INT2_X7", 7, pins_int2_x7), EGRP("INT2_X8", 8, pins_int2_x8),
	EGRP("INT2_X9", 9, pins_int2_x9),
};

static const struct sppctlgrp_t ecv5546grps_int3[] = {
	EGRP("INT3_X1", 1, pins_int2_x1), EGRP("INT3_X2", 2, pins_int2_x2),
	EGRP("INT3_X3", 3, pins_int2_x3), EGRP("INT3_X4", 4, pins_int2_x4),
	EGRP("INT3_X5", 5, pins_int2_x5), EGRP("INT3_X6", 6, pins_int2_x6),
	EGRP("INT3_X7", 7, pins_int2_x7), EGRP("INT3_X8", 8, pins_int2_x8),
	EGRP("INT3_X9", 9, pins_int2_x9),
};

static const unsigned int pins_int4_x1[] = { 7 };
static const unsigned int pins_int4_x2[] = { 8 };
static const unsigned int pins_int4_x3[] = { 9 };
static const unsigned int pins_int4_x4[] = { 10 };
static const unsigned int pins_int4_x5[] = { 11 };
static const unsigned int pins_int4_x6[] = { 12 };
static const unsigned int pins_int4_x7[] = { 13 };
static const unsigned int pins_int4_x8[] = { 18 };
static const unsigned int pins_int4_x9[] = { 19 };
static const struct sppctlgrp_t ecv5546grps_int4[] = {
	EGRP("INT4_X1", 1, pins_int4_x1), EGRP("INT4_X2", 2, pins_int4_x2),
	EGRP("INT4_X3", 3, pins_int4_x3), EGRP("INT4_X4", 4, pins_int4_x4),
	EGRP("INT4_X5", 5, pins_int4_x5), EGRP("INT4_X6", 6, pins_int4_x6),
	EGRP("INT4_X7", 7, pins_int4_x7), EGRP("INT4_X8", 8, pins_int4_x8),
	EGRP("INT4_X9", 9, pins_int4_x9),
};

static const struct sppctlgrp_t ecv5546grps_int5[] = {
	EGRP("INT5_X1", 1, pins_int4_x1), EGRP("INT5_X2", 2, pins_int4_x2),
	EGRP("INT5_X3", 3, pins_int4_x3), EGRP("INT5_X4", 4, pins_int4_x4),
	EGRP("INT5_X5", 5, pins_int4_x5), EGRP("INT5_X6", 6, pins_int4_x6),
	EGRP("INT5_X7", 7, pins_int4_x7), EGRP("INT5_X8", 8, pins_int4_x8),
	EGRP("INT5_X9", 9, pins_int4_x9),
};

static const unsigned int pins_int6_x1[] = { 9 };
static const unsigned int pins_int6_x2[] = { 10 };
static const unsigned int pins_int6_x3[] = { 11 };
static const unsigned int pins_int6_x4[] = { 12 };
static const unsigned int pins_int6_x5[] = { 13 };
static const unsigned int pins_int6_x6[] = { 14 };
static const unsigned int pins_int6_x7[] = { 15 };
static const unsigned int pins_int6_x8[] = { 16 };
static const unsigned int pins_int6_x9[] = { 17 };
static const unsigned int pins_int6_x10[] = { 18 };
static const unsigned int pins_int6_x11[] = { 19 };
static const struct sppctlgrp_t ecv5546grps_int6[] = {
	EGRP("INT6_X1", 1, pins_int6_x1),
	EGRP("INT6_X2", 2, pins_int6_x2),
	EGRP("INT6_X3", 3, pins_int6_x3),
	EGRP("INT6_X4", 4, pins_int6_x4),
	EGRP("INT6_X5", 5, pins_int6_x5),
	EGRP("INT6_X6", 6, pins_int6_x6),
	EGRP("INT6_X7", 7, pins_int6_x7),
	EGRP("INT6_X8", 8, pins_int6_x8),
	EGRP("INT6_X9", 9, pins_int6_x9),
	EGRP("INT6_X10", 10, pins_int6_x10),
	EGRP("INT6_X11", 11, pins_int6_x11),
};

static const struct sppctlgrp_t ecv5546grps_int7[] = {
	EGRP("INT7_X1", 1, pins_int6_x1),
	EGRP("INT7_X2", 2, pins_int6_x2),
	EGRP("INT7_X3", 3, pins_int6_x3),
	EGRP("INT7_X4", 4, pins_int6_x4),
	EGRP("INT7_X5", 5, pins_int6_x5),
	EGRP("INT7_X6", 6, pins_int6_x6),
	EGRP("INT7_X7", 7, pins_int6_x7),
	EGRP("INT7_X8", 8, pins_int6_x8),
	EGRP("INT7_X9", 9, pins_int6_x9),
	EGRP("INT7_X10", 10, pins_int6_x10),
	EGRP("INT7_X11", 11, pins_int6_x11),
};

static const unsigned int pins_gpio_ao_int0_x1[] = { 52, 53, 54, 55,
						     56, 57, 58, 59 };
static const unsigned int pins_gpio_ao_int0_x2[] = { 68, 69, 70, 71,
						     72, 73, 74, 75 };
static const struct sppctlgrp_t ecv5546grps_gpio_ao_int0[] = {
	EGRP("GPIO_AO_INT0_X1", 1, pins_gpio_ao_int0_x1),
	EGRP("GPIO_AO_INT0_X2", 1, pins_gpio_ao_int0_x2),
};

static const unsigned int pins_gpio_ao_int1_x1[] = { 60, 61, 62, 63,
						     64, 65, 66, 67 };
static const unsigned int pins_gpio_ao_int1_x2[] = { 76, 77, 78, 79,
						     80, 81, 82, 83 };
static const struct sppctlgrp_t ecv5546grps_gpio_ao_int1[] = {
	EGRP("GPIO_AO_INT1_X1", 1, pins_gpio_ao_int1_x1),
	EGRP("GPIO_AO_INT1_X2", 1, pins_gpio_ao_int1_x2),
};

static const unsigned int pins_gpio_ao_int2_x1[] = { 68, 69, 70, 71,
						     72, 73, 74, 75 };
static const unsigned int pins_gpio_ao_int2_x2[] = { 84, 85, 86, 87,
						     88, 89, 90, 91 };
static const struct sppctlgrp_t ecv5546grps_gpio_ao_int2[] = {
	EGRP("GPIO_AO_INT2_X1", 1, pins_gpio_ao_int2_x1),
	EGRP("GPIO_AO_INT2_X2", 1, pins_gpio_ao_int2_x2),
};

static const unsigned int pins_gpio_ao_int3_x1[] = { 76, 77, 78, 79,
						     80, 81, 82, 83 };
static const unsigned int pins_gpio_ao_int3_x2[] = { 91, 92, 93, 94,
						     95, 96, 97, 98 };
static const struct sppctlgrp_t ecv5546grps_gpio_ao_int3[] = {
	EGRP("GPIO_AO_INT3_X1", 1, pins_gpio_ao_int3_x1),
	EGRP("GPIO_AO_INT3_X2", 1, pins_gpio_ao_int3_x2),
};

struct func_t list_funcs[] = {
	FNCN("GPIO", F_OFF_0, 0, 0, 0),
	FNCN("IOP", F_OFF_0, 0, 0, 0),

	FNCE("SPI_FLASH", F_OFF_G, 1, 0, 1, ecv5546grps_spif),
	FNCE("EMMC", F_OFF_G, 1, 1, 1, ecv5546grps_emmc),
	FNCE("SPI_NAND", F_OFF_G, 1, 2, 2, ecv5546grps_spi_nand),
	FNCE("SD_CARD", F_OFF_G, 1, 4, 1, ecv5546grps_sdc30),
	FNCE("SDIO", F_OFF_G, 1, 5, 1, ecv5546grps_sdio30),
	FNCE("PARA_NAND", F_OFF_G, 1, 6, 1, ecv5546grps_para_nand),
	FNCE("USB_OTG", F_OFF_G, 1, 7, 1, ecv5546grps_usb_otg),
	FNCE("GMAC", F_OFF_G, 1, 9, 1, ecv5546grps_gmac),
	FNCE("PWM0", F_OFF_G, 1, 10, 2, ecv5546grps_pwm0),
	FNCE("PWM1", F_OFF_G, 1, 12, 2, ecv5546grps_pwm1),
	FNCE("PWM2", F_OFF_G, 1, 14, 2, ecv5546grps_pwm2),

	FNCE("PWM3", F_OFF_G, 2, 0, 2, ecv5546grps_pwm3),
	FNCE("UART0", F_OFF_G, 2, 2, 2, ecv5546grps_uart0),
	FNCE("UART1", F_OFF_G, 2, 4, 2, ecv5546grps_uart1),
	FNCE("UART1_FC", F_OFF_G, 2, 6, 2, ecv5546grps_uart1_fc),
	FNCE("UART2", F_OFF_G, 2, 8, 2, ecv5546grps_uart2),
	FNCE("UART2_FC", F_OFF_G, 2, 10, 2, ecv5546grps_uart2_fc),
	FNCE("UART3", F_OFF_G, 2, 12, 2, ecv5546grps_uart3),
	FNCE("UADBG", F_OFF_G, 2, 14, 1, ecv5546grps_uadbg),

	FNCE("UART6", F_OFF_G, 3, 0, 2, ecv5546grps_uart6),
	FNCE("UART7", F_OFF_G, 3, 2, 1, ecv5546grps_uart7),
	FNCE("I2C_COMBO0", F_OFF_G, 3, 3, 2, ecv5546grps_i2c_combo0),
	FNCE("I2C_COMBO1", F_OFF_G, 3, 5, 1, ecv5546grps_i2c_combo1),
	FNCE("I2C_COMBO2", F_OFF_G, 3, 6, 2, ecv5546grps_i2c_combo2),
	FNCE("I2C_COMBO3", F_OFF_G, 3, 8, 1, ecv5546grps_i2c_combo3),
	FNCE("I2C_COMBO4", F_OFF_G, 3, 9, 1, ecv5546grps_i2c_combo4),
	FNCE("I2C_COMBO5", F_OFF_G, 3, 10, 1, ecv5546grps_i2c_combo5),
	FNCE("I2C_COMBO6", F_OFF_G, 3, 11, 2, ecv5546grps_i2c_combo6),
	FNCE("I2C_COMBO7", F_OFF_G, 3, 13, 2, ecv5546grps_i2c_combo7),

	FNCE("I2C_COMBO8", F_OFF_G, 4, 0, 2, ecv5546grps_i2c_combo8),
	FNCE("I2C_COMBO9", F_OFF_G, 4, 2, 2, ecv5546grps_i2c_combo9),
	FNCE("SPI_MASTER0", F_OFF_G, 4, 14, 2, ecv5546grps_spi_master0),

	FNCE("SPI_MASTER1", F_OFF_G, 5, 1, 2, ecv5546grps_spi_master1),
	FNCE("SPI_MASTER2", F_OFF_G, 5, 3, 1, ecv5546grps_spi_master2),
	FNCE("SPI_MASTER3", F_OFF_G, 5, 4, 2, ecv5546grps_spi_master3),
	FNCE("SPI_MASTER4", F_OFF_G, 5, 6, 1, ecv5546grps_spi_master4),
	FNCE("SPI_SLAVE0", F_OFF_G, 5, 7, 2, ecv5546grps_spi_slave0),
	FNCE("AUD_TDMTX_XCK", F_OFF_G, 5, 9, 1, ecv5546grps_aud_tdmtx_xck),
	FNCE("AUD_DAC_XCK1", F_OFF_G, 5, 10, 1, ecv5546grps_aud_dac_xck1),
	FNCE("AUD_DAC_XCK", F_OFF_G, 5, 11, 1, ecv5546grps_aud_dac_xck),
	FNCE("AUD_AU2_DATA0", F_OFF_G, 5, 12, 1, ecv5546grps_aud_au2_data0),
	FNCE("AUD_AU1_DATA0", F_OFF_G, 5, 13, 1, ecv5546grps_aud_au1_data0),
	FNCE("AUD_AU2_CK", F_OFF_G, 5, 14, 1, ecv5546grps_aud_au2_ck),
	FNCE("AUD_AU1_CK", F_OFF_G, 5, 15, 1, ecv5546grps_aud_au1_ck),

	FNCE("AUD_AU_ADC_DATA0", F_OFF_G, 6, 0, 2, ecv5546grps_aud_au_adc_data0),
	FNCE("AUD_ADC2_DATA0", F_OFF_G, 6, 2, 1, ecv5546grps_aud_adc2_data0),
	FNCE("AUD_ADC1_DATA0", F_OFF_G, 6, 3, 1, ecv5546grps_aud_adc1_data0),
	FNCE("AUD_TDM", F_OFF_G, 6, 4, 1, ecv5546grps_aud_tdm),
	FNCE("SPDIF_IN", F_OFF_G, 6, 5, 3, ecv5546grps_spdif_in),
	FNCE("SPDIF_OUT", F_OFF_G, 6, 8, 3, ecv5546grps_spdif_out),

	FNCE("INT0", F_OFF_G, 7, 5, 4, ecv5546grps_int0),
	FNCE("INT1", F_OFF_G, 7, 9, 4, ecv5546grps_int1),

	FNCE("INT2", F_OFF_G, 8, 0, 4, ecv5546grps_int2),
	FNCE("INT3", F_OFF_G, 8, 4, 4, ecv5546grps_int3),
	FNCE("INT4", F_OFF_G, 8, 8, 4, ecv5546grps_int4),
	FNCE("INT5", F_OFF_G, 8, 12, 4, ecv5546grps_int5),

	FNCE("INT6", F_OFF_G, 9, 0, 4, ecv5546grps_int6),
	FNCE("INT7", F_OFF_G, 9, 4, 4, ecv5546grps_int7),
	FNCE("GPIO_AO_INT0", F_OFF_G, 9, 8, 2, ecv5546grps_gpio_ao_int0),
	FNCE("GPIO_AO_INT1", F_OFF_G, 9, 10, 2, ecv5546grps_gpio_ao_int1),
	FNCE("GPIO_AO_INT2", F_OFF_G, 9, 12, 2, ecv5546grps_gpio_ao_int2),
	FNCE("GPIO_AO_INT3", F_OFF_G, 9, 14, 2, ecv5546grps_gpio_ao_int3),
};

const size_t list_func_nums = ARRAY_SIZE(list_funcs);
char const **unq_grps;
size_t unique_groups_nums;
struct grp2fp_map_t *g2fp_maps;

#define IS_DVIO(pin) ((pin) >= 20 && (pin) <= 79)

#define PIN_CONFIG_INPUT_INVERT (PIN_CONFIG_END + 1)
#define PIN_CONFIG_OUTPUT_INVERT (PIN_CONFIG_END + 2)
#define PIN_CONFIG_SLEW_RATE_CTRL (PIN_CONFIG_END + 3)
#define PIN_CONFIG_BIAS_STRONG_PULL_UP (PIN_CONFIG_END + 4)

#ifdef CONFIG_GENERIC_PINCONF
static const struct pinconf_generic_params sppctl_dt_params[] = {
	{ "eys3d,input-invert-enable", PIN_CONFIG_INPUT_INVERT, 1 },
	{ "eys3d,output-invert-enable", PIN_CONFIG_OUTPUT_INVERT, 1 },
	{ "eys3d,input-invert-disable", PIN_CONFIG_INPUT_INVERT, 0 },
	{ "eys3d,output-invert-disable", PIN_CONFIG_OUTPUT_INVERT, 0 },
	{ "eys3d,slew-rate-control-disable", PIN_CONFIG_SLEW_RATE_CTRL, 0 },
	{ "eys3d,slew-rate-control-enable", PIN_CONFIG_SLEW_RATE_CTRL, 1 },
	{ "eys3d,bias-strong-pull-up", PIN_CONFIG_BIAS_STRONG_PULL_UP, 1 },

};

#ifdef CONFIG_DEBUG_FS
static const struct pin_config_item sppctl_conf_items[] = {
	PCONFDUMP(PIN_CONFIG_INPUT_INVERT, "input invert enabled", NULL, false),
	PCONFDUMP(PIN_CONFIG_OUTPUT_INVERT, "output invert enabled", NULL,
		  false),
	PCONFDUMP(PIN_CONFIG_SLEW_RATE_CTRL, "slew rate control enabled", NULL,
		  false),
	PCONFDUMP(PIN_CONFIG_BIAS_STRONG_PULL_UP, "bias strong pull up", NULL,
		  false),
};
#endif

#endif

void sppctl_gmx_set(struct sppctl_pdata_t *pdata, u8 reg_offset, u8 bit_offset,
		    u8 bit_nums, u8 bit_value)
{
	struct sppctl_reg_t x;
	u32 *r;

	x.m = (~(~0 << bit_nums)) << bit_offset;
	x.v = ((uint16_t)bit_value) << bit_offset;

	if (pdata->debug > 1)
		KDBG(pdata->pcdp->dev,
		     "%s(reg_off[0x%X],bit_off[0x%X],bit_nums[0x%X],bit_val[0x%X]) mask_bit:0x%X control_bit:0x%X\n",
		     __func__, reg_offset, bit_offset, bit_nums, bit_value, x.m,
		     x.v);
	r = (uint32_t *)&x;
	writel(*r, pdata->moon1_regs_base + (reg_offset << 2));
}

u8 sppctl_gmx_get(struct sppctl_pdata_t *pdata, u8 reg_offset, u8 bit_offset,
		  u8 bit_nums)
{
	struct sppctl_reg_t *x;
	u8 bit_value;
	u32 r;

	r = readl(pdata->moon1_regs_base + (reg_offset << 2));

	x = (struct sppctl_reg_t *)&r;
	bit_value = (x->v >> bit_offset) & (~(~0 << bit_nums));

	if (pdata->debug > 1)
		KDBG(pdata->pcdp->dev,
		     "%s(reg_off[0x%X],bit_off[0x%X],bit_nums[0x%X]) control_bit:0x%X bit_val:0x%X\n",
		     __func__, reg_offset, bit_offset, bit_nums, x->v,
		     bit_value);

	return bit_value;
}

__attribute((unused)) static const char *
sppctl_get_function_name_by_selector(unsigned int selector)
{
	return list_funcs[selector].name;
}

static struct func_t *sppctl_get_function_by_selector(unsigned int selector)
{
	return &list_funcs[selector];
}

__attribute((unused)) static int sppctl_get_function_count(void)
{
	return list_func_nums;
}

static struct grp2fp_map_t *sppctl_get_group_by_name(const char *name)
{
	int i;

	for (i = 0; i < unique_groups_nums; i++) {
		if (!strcmp(unq_grps[i], name))
			return &g2fp_maps[i];
	}

	return NULL;
}

static struct grp2fp_map_t *
sppctl_get_group_by_selector(unsigned int group_selector,
			     unsigned int func_selector)
{
	struct func_t *func;
	int i, j;

	if (group_selector > GPIS_list_size - 1) {
		return &g2fp_maps[group_selector];
	} else if (func_selector == 0) { /* function:GPIO */
		return &g2fp_maps[group_selector];
	}

	/* group:GPIO0 ~GPIO105 */
	func = sppctl_get_function_by_selector(func_selector);

	for (i = 0; i < func->gnum; i++) {
		for (j = 0; j < func->grps[i].pnum; j++) {
			if (group_selector == func->grps[i].pins[j])
				return sppctl_get_group_by_name(
					func->grps[i].name);
		}
	}

	return NULL;
}

static int sppctl_pin_function_association_query(unsigned int pin_selector,
						 unsigned int func_selector)
{
	int i, j;
	struct func_t *func = sppctl_get_function_by_selector(func_selector);

	for (i = 0; i < func->gnum; i++) {
		for (j = 0; j < func->grps[i].pnum; j++) {
			if (pin_selector == func->grps[i].pins[j])
				return 0;
		}
	}

	return -EEXIST;
}

static int sppctl_group_function_association_query(unsigned int group_sel,
						   unsigned int func_sel)
{
	struct grp2fp_map_t *group_map =
		sppctl_get_group_by_selector(group_sel, func_sel);

	if (group_map->f_idx == func_sel)
		return 0;

	return -EEXIST;
}

static int sppctl_pinconf_get(struct pinctrl_dev *pctldev,
			      unsigned int pin_selector, unsigned long *config)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	unsigned int param;
	unsigned int arg = 0;
	int ret = 0;
	int tmp;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;
	param = pinconf_to_config_param(*config);

	//KDBG(pctldev->dev, "%s(%d)%d\n", __func__, pin_selector, param);
	switch (param) {
	case PIN_CONFIG_OUTPUT_ENABLE:
		tmp = sppctl_gpio_output_enable_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_OUTPUT:
		if (!sppctl_gpio_first_get(chip, pin_selector)) {
			ret = -EINVAL;
			break;
		}

		if (!sppctl_gpio_master_get(chip, pin_selector)) {
			ret = -EINVAL;
			break;
		}

		if (!sppctl_gpio_output_enable_query(chip, pin_selector)) {
			ret = -EINVAL;
			break;
		}

		tmp = sppctl_gpio_direction_output_query(chip, pin_selector);
		if (tmp < 0)
			ret = -EINVAL;
		else
			arg = tmp;
		break;
	case PIN_CONFIG_OUTPUT_INVERT:
		tmp = sppctl_gpio_output_invert_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
		tmp = sppctl_gpio_open_drain_mode_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH_UA:
		tmp = sppctl_gpio_drive_strength_get(chip, pin_selector);
		if (tmp < 0)
			ret = -EINVAL;
		else
			arg = tmp;
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		tmp = sppctl_gpio_input_enable_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_INPUT_INVERT:
		tmp = sppctl_gpio_input_invert_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		tmp = sppctl_gpio_schmitt_trigger_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_SLEW_RATE_CTRL:
		tmp = sppctl_gpio_slew_rate_control_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
		tmp = sppctl_gpio_high_impedance_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		tmp = sppctl_gpio_pull_up_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		else if (IS_DVIO(pin_selector))
			arg = 35 * 1000;
		else
			arg = 48 * 1000;
		break;
	case PIN_CONFIG_BIAS_STRONG_PULL_UP:
		tmp = sppctl_gpio_strong_pull_up_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		else
			arg = 2100;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		tmp = sppctl_gpio_pull_down_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		else if (IS_DVIO(pin_selector))
			arg = 28 * 1000;
		else
			arg = 44 * 1000;
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		tmp = sppctl_gpio_bias_disable_query(chip, pin_selector);
		if (tmp <= 0)
			ret = -EINVAL;
		break;
	default:
		ret = -ENOTSUPP;
		break;
	}
	if (!ret)
		*config = pinconf_to_config_packed(param, arg);

	return ret;
}

static int sppctl_pinconf_set(struct pinctrl_dev *pctldev,
			      unsigned int pin_selector, unsigned long *config,
			      unsigned int num_configs)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	u8 param;
	u32 arg;
	int i;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;

	KDBG(pctldev->dev, "%s(%d,%ld,%d)\n", __func__, pin_selector, *config,
	     num_configs);

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(config[i]);
		arg = pinconf_to_config_argument(config[i]);

		KDBG(pctldev->dev, "(%s:%d)GPIO[%d], arg:0x%x,param:0x%x)\n",
		     __func__, __LINE__, pin_selector, arg, param);
		switch (param) {
		case PIN_CONFIG_OUTPUT_ENABLE:
			KDBG(pctldev->dev, "GPIO[%d]:output %s\n", pin_selector,
			     arg == 1 ? "enable" : "disable");
			sppctl_gpio_output_enable_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_OUTPUT:
			KDBG(pctldev->dev, "GPIO[%d]:output %s\n", pin_selector,
			     arg == 1 ? "high" : "low");
			sppctl_gpio_direction_output(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_OUTPUT_INVERT:
			KDBG(pctldev->dev, "GPIO[%d]:%s output\n", pin_selector,
			     arg == 1 ? "invert" : "normalize");
			sppctl_gpio_output_invert_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_DRIVE_OPEN_DRAIN:
			KDBG(pctldev->dev, "GPIO[%d]:open drain\n",
			     pin_selector);
			sppctl_gpio_open_drain_mode_set(chip, pin_selector, 1);
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			KDBG(pctldev->dev, "GPIO[%d]:drive strength %dmA\n",
			     pin_selector, arg);
			sppctl_gpio_drive_strength_set(chip, pin_selector,
						       arg * 1000);
			break;
		case PIN_CONFIG_DRIVE_STRENGTH_UA:
			KDBG(pctldev->dev, "GPIO[%d]:drive strength %duA\n",
			     pin_selector, arg);
			sppctl_gpio_drive_strength_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_INPUT_ENABLE:
			KDBG(pctldev->dev, "GPIO[%d]:input %s\n", pin_selector,
			     arg == 0 ? "disable" : "enable");
			sppctl_gpio_input_enable_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_INPUT_INVERT:
			KDBG(pctldev->dev, "GPIO[%d]:%s input\n", pin_selector,
			     arg == 1 ? "invert" : "normalize");
			sppctl_gpio_input_invert_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			KDBG(pctldev->dev, "GPIO[%d]:%s schmitt trigger\n",
			     pin_selector, arg == 0 ? "disable" : "enable");
			sppctl_gpio_schmitt_trigger_set(chip, pin_selector,
							arg);
			break;
		case PIN_CONFIG_SLEW_RATE_CTRL:
			KDBG(pctldev->dev, "GPIO[%d]:%s slew rate control\n",
			     pin_selector, arg == 0 ? "disable" : "enable");
			sppctl_gpio_slew_rate_control_set(chip, pin_selector,
							  arg);
			break;
		case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
			KDBG(pctldev->dev, "GPIO[%d]:high-Z\n", pin_selector);
			sppctl_gpio_high_impedance(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			KDBG(pctldev->dev, "GPIO[%d]:pull up\n", pin_selector);
			sppctl_gpio_pull_up(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_STRONG_PULL_UP:
			KDBG(pctldev->dev, "GPIO[%d]:strong pull up\n",
			     pin_selector);
			sppctl_gpio_strong_pull_up(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			KDBG(pctldev->dev, "GPIO[%d]:pull down\n",
			     pin_selector);
			sppctl_gpio_pull_down(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_DISABLE:
			KDBG(pctldev->dev, "GPIO[%d]:bias disable\n",
			     pin_selector);
			sppctl_gpio_bias_disable(chip, pin_selector);
			break;
		default:
			KERR(pctldev->dev,
			     "GPIO[%d]:param:0x%x is not supported\n",
			     pin_selector, param);
			break;
		}
		// FIXME: add pullup/pulldown, irq enable/disable
	}

	return 0;
}

static int sppctl_pinconf_group_get(struct pinctrl_dev *pctldev,
				    unsigned int group_selector,
				    unsigned long *config)
{
	unsigned long old_conf = 0;
	unsigned long cur_conf = *config;
	unsigned int pin;
	int ret;
	int i;
	int j;
	int k;

	if (group_selector < GPIS_list_size) {
		pin = group_selector;
		return sppctl_pinconf_get(pctldev, pin, config);
	}

	for (i = 0; i < list_func_nums; i++) {
		struct func_t func = list_funcs[i];

		for (j = 0; j < func.gnum; j++) {
			struct sppctlgrp_t group = func.grps[j];

			if (strcmp(group.name, unq_grps[group_selector]) == 0) {
				for (k = 0; k < group.pnum; k++) {
					pin = group.pins[k];

					ret = sppctl_pinconf_get(pctldev, pin,
								 &cur_conf);
					if (ret)
						return ret;

					if (k == 0)
						old_conf = cur_conf;

					if (cur_conf != old_conf)
						return -EINVAL;
				}
			}
		}
	}

	*config = cur_conf;

	return 0;
}

static int sppctl_pinconf_group_set(struct pinctrl_dev *pctldev,
				    unsigned int group_selector,
				    unsigned long *config,
				    unsigned int num_configs)
{
	unsigned int pin;
	int i;
	int j;
	int k;

	if (group_selector < GPIS_list_size) {
		pin = group_selector;
		return sppctl_pinconf_set(pctldev, pin, config, num_configs);
	}

	for (i = 0; i < list_func_nums; i++) {
		struct func_t func = list_funcs[i];

		for (j = 0; j < func.gnum; j++) {
			struct sppctlgrp_t group = func.grps[j];

			if (strcmp(group.name, unq_grps[group_selector]) == 0) {
				for (k = 0; k < group.pnum; k++) {
					pin = group.pins[k];

					KDBG(pctldev->dev,
					     "(%s:%d)grp%d.name[%s]pin[%d]\n",
					     __func__, __LINE__, group_selector,
					     group.name, pin);
					sppctl_pinconf_set(pctldev, pin, config,
							   num_configs);
				}
			}
		}
	}

	return 0;
}

#ifdef CONFIG_DEBUG_FS
static void sppctl_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				    struct seq_file *seq, unsigned int offset)
{
	// KINF(pctldev->dev, "%s(%d)\n", __func__, offset);
	seq_printf(seq, " %s", dev_name(pctldev->dev));
}

static void sppctl_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
					  struct seq_file *seq,
					  unsigned int group_selector)
{
	// group: freescale/pinctrl-imx.c, 448
	// KINF(pctldev->dev, "%s(%d)\n", __func__, group_selector);
}

static void sppctl_pinconf_config_dbg_show(struct pinctrl_dev *pctldev,
					   struct seq_file *seq,
					   unsigned long config)
{
	// KINF(pctldev->dev, "%s(%ld)\n", __func__, config);
}
#else
#define sppctl_pinconf_dbg_show NULL
#define sppctl_pinconf_group_dbg_show NULL
#define sppctl_pinconf_config_dbg_show NULL
#endif

static int sppctl_pinmux_request(struct pinctrl_dev *pctldev,
				 unsigned int selector)
{
	//KDBG(pctldev->dev, "%s(%d)\n", __func__, selector);
	return 0;
}

static int sppctl_pinmux_free(struct pinctrl_dev *pctldev,
			      unsigned int selector)
{
	//KDBG(pctldev->dev, "%s(%d)\n", __func__, selector);
	return 0;
}

static int sppctl_pinmux_get_functions_count(struct pinctrl_dev *pctldev)
{
	return list_func_nums;
}

static const char *sppctl_pinmux_get_function_name(struct pinctrl_dev *pctldev,
						   unsigned int selector)
{
	return list_funcs[selector].name;
}

int sppctl_pinmux_get_function_groups(struct pinctrl_dev *pctldev,
				      unsigned int selector,
				      const char *const **groups,
				      unsigned int *num_groups)
{
	struct func_t *func = &list_funcs[selector];

	if (!strcmp("GPIO", func->name)) {
		*num_groups = GPIS_list_size;
		*groups = sppctlgpio_list_s;
	} else {
		*num_groups = func->gnum;
		*groups = (const char *const *)func->grps_sa;
	}

	//KDBG(pctldev->dev, "%s(fid:%d) %d\n", __func__, selector, *num_groups);
	return 0;
}

static void sppctl_gmac_gmx_set(struct sppctl_pdata_t *pdata, u8 reg_offset,
				u8 bit_offset, u8 bit_nums, u8 bit_value)
{
	sppctl_gmx_set(pdata, reg_offset, bit_offset, bit_nums, 1);
	if (bit_value == 1) {
		//RGMII
		sppctl_gmx_set(pdata, 0X57, 12, 1, 0);
	} else if (bit_value == 2) {
		//RMII
		sppctl_gmx_set(pdata, 0X57, 12, 1, 1);
	}
}

static int sppctl_pinmux_set_mux(struct pinctrl_dev *pctldev,
				 unsigned int func_selector,
				 unsigned int group_selector)
{
	struct sppctl_pdata_t *pctrl;
	struct grp2fp_map_t *g2fpm;
	struct func_t *owner_func;
	struct gpio_chip *chip;
	struct func_t *func;
	unsigned int pin;
	u8 reg_value;
	int i = -1;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	func = sppctl_get_function_by_selector(func_selector);
	g2fpm = sppctl_get_group_by_selector(group_selector, func_selector);
	chip = &pctrl->gpiod->chip;

	//KDBG(pctldev->dev, "[%s:%d]function:%d, group:%d\n",
	//	__func__, __LINE__, func_selector, group_selector);

	if (!strcmp(func->name, "GPIO")) {
		if (group_selector < GPIS_list_size) {
			pin = group_selector;

			KDBG(pctldev->dev,
			     "[%s:%d]set function[%s] on pin[%d]\n", __func__,
			     __LINE__, func->name, pin);
			sppctl_gpio_first_master_set(chip, pin, MUX_FIRST_G,
						     MUX_MASTER_G);
		} else {
			owner_func =
				sppctl_get_function_by_selector(g2fpm->f_idx);

			for (i = 0; i < owner_func->grps[g2fpm->g_idx].pnum;
			     i++) {
				pin = owner_func->grps[g2fpm->g_idx].pins[i];

				KDBG(pctldev->dev,
				     "[%s:%d]set function[%s] on pin[%d]\n",
				     __func__, __LINE__, func->name, pin);
				sppctl_gpio_first_master_set(
					chip, pin, MUX_FIRST_G, MUX_MASTER_G);
			}
		}
	} else {
		if (group_selector < GPIS_list_size) {
			if (!sppctl_pin_function_association_query(
				    group_selector, func_selector)) {
				pin = group_selector;

				KDBG(pctldev->dev,
				     "[%s:%d]set function[%s] on pin[%d]\n",
				     __func__, __LINE__, func->name, pin);
				sppctl_gpio_first_master_set(chip, pin,
							     MUX_FIRST_M,
							     MUX_MASTER_KEEP);

				reg_value = func->grps[g2fpm->g_idx].gval;
				if (!strcmp(func->name, "GMAC")) {
					sppctl_gmac_gmx_set(pctrl, func->roff,
							    func->boff,
							    func->blen,
							    reg_value);
				} else {
					sppctl_gmx_set(pctrl, func->roff,
						       func->boff, func->blen,
						       reg_value);
				}
			} else {
				KERR(pctldev->dev,
				     "invalid pin[%d] for function \"%s\"\n",
				     group_selector, func->name);
			}
		} else if (!sppctl_group_function_association_query(
				   group_selector, func_selector)) {
			for (i = 0; i < func->grps[g2fpm->g_idx].pnum; i++) {
				pin = func->grps[g2fpm->g_idx].pins[i];

				KDBG(pctldev->dev,
				     "[%s:%d]set function[%s] on pin[%d]\n",
				     __func__, __LINE__, func->name, pin);
				sppctl_gpio_first_master_set(chip, pin,
							     MUX_FIRST_M,
							     MUX_MASTER_KEEP);
			}
			reg_value = func->grps[g2fpm->g_idx].gval;
			if (!strcmp(func->name, "GMAC")) {
				sppctl_gmac_gmx_set(pctrl, func->roff,
						    func->boff, func->blen,
						    reg_value);
			} else {
				sppctl_gmx_set(pctrl, func->roff, func->boff,
					       func->blen, reg_value);
			}

		} else {
			KERR(pctldev->dev,
			     "invalid group \"%s\" for function \"%s\"\n",
			     unq_grps[group_selector], func->name);
		}
	}

	return 0;
}

static int sppctl_pinmux_gpio_request_enable(struct pinctrl_dev *pctldev,
					     struct pinctrl_gpio_range *range,
					     unsigned int pin_selector)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	struct pin_desc *pdesc;
	int g_f;
	int g_m;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;

	KDBG(pctldev->dev, "%s(%d)\n", __func__, pin_selector);
	g_f = sppctl_gpio_first_get(chip, pin_selector);
	g_m = sppctl_gpio_master_get(chip, pin_selector);
	if (g_f == MUX_FIRST_G && g_m == MUX_MASTER_G)
		return 0;

	pdesc = pin_desc_get(pctldev, pin_selector);
	// in non-gpio state: is it claimed already?
	if (pdesc->mux_owner)
		return -EACCES;

	sppctl_gpio_first_master_set(chip, pin_selector, MUX_FIRST_G,
				     MUX_MASTER_G);
	return 0;
}

static void sppctl_pinmux_gpio_disable_free(struct pinctrl_dev *pctldev,
					    struct pinctrl_gpio_range *range,
					    unsigned int pin_selector)
{
	sppctl_gpio_unmux_irq(range->gc, pin_selector);
}

int sppctl_pinmux_gpio_set_direction(struct pinctrl_dev *pctldev,
				     struct pinctrl_gpio_range *range,
				     unsigned int pin_selector,
				     bool direction_input)
{
	KDBG(pctldev->dev, "%s(%d,%d)\n", __func__, pin_selector,
	     direction_input);
	return 0;
}

// all groups
static int sppctl_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	return unique_groups_nums;
}

static const char *sppctl_pinctrl_get_group_name(struct pinctrl_dev *pctldev,
						 unsigned int selector)
{
	return unq_grps[selector];
}

static int sppctl_pinctrl_get_group_pins(struct pinctrl_dev *pctldev,
					 unsigned int selector,
					 const unsigned int **pins,
					 unsigned int *num_pins)
{
#if defined(SUPPORT_GPIO_AO_INT)
	struct sppctl_pdata_t *pctrl = pinctrl_dev_get_drvdata(pctldev);
	struct sppctlgpio_chip_t *pc = pctrl->gpiod;
	int i;
#endif
	struct grp2fp_map_t g2fpm = g2fp_maps[selector];
	struct func_t *func = &list_funcs[g2fpm.f_idx];

	//KDBG(pctldev->dev, "grp-pins g:%d f_idx:%d,g_idx:%d freg:%d...\n",
	//     selector, g2fpm.f_idx, g2fpm.g_idx, func->freg);

	*num_pins = 0;

	// MUX | GPIO | IOP: 1 pin -> 1 group
	if (func->freg != F_OFF_G) {
		*num_pins = 1;
		*pins = &sppctlpins_group[selector];
		return 0;
	}

	// IOP (several pins at once in a group)
	if (!func->grps)
		return 0;
	if (func->gnum < 1)
		return 0;
	*num_pins = func->grps[g2fpm.g_idx].pnum;
	*pins = func->grps[g2fpm.g_idx].pins;

#if defined(SUPPORT_GPIO_AO_INT)
	if (selector == 265 || selector == 266) { // GPIO_AO_INT0
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i] = (*pins)[i];
	}
	if (selector == 267 || selector == 268) { // GPIO_AO_INT1
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i + 8] = (*pins)[i];
	}
	if (selector == 269 || selector == 270) { // GPIO_AO_INT2
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i + 16] = (*pins)[i];
	}
	if (selector == 271 || selector == 272) { // GPIO_AO_INT3
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i + 24] = (*pins)[i];
	}
#endif

	return 0;
}

// /sys/kernel/debug/pinctrl/sppctl/pins add: gpio_first and ctrl_sel
#ifdef CONFIG_DEBUG_FS
static void sppctl_pinctrl_pin_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *seq,
					unsigned int selector)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	const char *tmpp;
	u8 g_f, g_m;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;

	seq_printf(seq, "%s", dev_name(pctldev->dev));
	g_f = sppctl_gpio_first_get(chip, selector);
	g_m = sppctl_gpio_master_get(chip, selector);

	tmpp = "?";
	if (g_f && g_m)
		tmpp = "GPIO";
	if (g_f && !g_m)
		tmpp = " IOP";
	if (!g_f)
		tmpp = " MUX";
	seq_printf(seq, " %s", tmpp);
}
#else
#define sppctl_ops_show NULL
#endif

static unsigned long
sppctl_pinconf_param_2_generic_pinconf_param(unsigned char param)
{
	unsigned long config = PIN_CONFIG_MAX;

	switch (param) {
	case SPPCTL_PCTL_L_OUT:
		config = PIN_CONF_PACKED(PIN_CONFIG_OUTPUT, 0);
		break;
	case SPPCTL_PCTL_L_OU1:
		config = PIN_CONF_PACKED(PIN_CONFIG_OUTPUT, 1);
		break;
	case SPPCTL_PCTL_L_INV:
		config = PIN_CONF_PACKED(PIN_CONFIG_INPUT_INVERT, 0);
		break;
	case SPPCTL_PCTL_L_ONV:
		config = PIN_CONF_PACKED(PIN_CONFIG_OUTPUT_INVERT, 0);
		break;
	case SPPCTL_PCTL_L_ODR:
		config = PIN_CONF_PACKED(PIN_CONFIG_DRIVE_OPEN_DRAIN, 0);
		break;
	default:
		break;
	}

	return config;
}

static int sppctl_pinctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
					 struct device_node *np_config,
					 struct pinctrl_map **map,
					 unsigned int *num_maps)
{
	struct pinctrl_map *generic_map = NULL;
	unsigned int num_generic_maps = 0;
#if defined(SUPPORT_GPIO_AO_INT)
	struct sppctlgpio_chip_t *pc;
	int mask, reg1, reg2;
	int ao_size = 0;
	const __be32 *ao_list;
	int ao_nm;
#endif
	struct sppctl_pdata_t *pctrl;
	struct device_node *parent;
	struct func_t *func = NULL;
	unsigned long *configs;
	const __be32 *list;
	int size = 0;
	int ret = 0;
	int nmG = 0;
	int i = 0;
	u32 dt_pin;
	u32 dt_fun;
	u8 p_p;
	u8 p_g;
	u8 p_f;
	u8 p_l;

	list = of_get_property(np_config, "eys3d,pins", &size);

	pctrl = pinctrl_dev_get_drvdata(pctldev);

#if defined(SUPPORT_GPIO_AO_INT)
	pc = pctrl->gpiod;
	ao_list = of_get_property(np_config, "eys3d,ao-pins", &ao_size);
	ao_nm = ao_size / sizeof(*ao_list);
#endif
	*num_maps = size / sizeof(*list);

	// Check if out of range or invalid?
	for (i = 0; i < (*num_maps); i++) {
		dt_pin = be32_to_cpu(list[i]);
		p_p = SPPCTL_PCTLD_P(dt_pin);
		p_g = SPPCTL_PCTLD_G(dt_pin);

		if (p_p >= sppctlpins_all_nums || p_g == SPPCTL_PCTL_G_PMUX) {
			KDBG(pctldev->dev,
			     "Invalid \'eys3d,pins\' property at index %d (0x%08x)\n",
			     i, dt_pin);
			return -EINVAL;
		}
	}

#if defined(SUPPORT_GPIO_AO_INT)
	// Check if out of range?
	for (i = 0; i < ao_nm; i++) {
		dt_pin = be32_to_cpu(ao_list[i]);
		if (SPPCTL_AOPIN_PIN(dt_pin) >= 32) {
			KDBG(pctldev->dev,
			     "Invalid \'eys3d,ao_pins\' property at index %d (0x%08x)\n",
			     i, dt_pin);
			return -EINVAL;
		}
	}
#endif

	ret = pinconf_generic_dt_node_to_map_all(
		pctldev, np_config, &generic_map, &num_generic_maps);
	if (ret < 0) {
		KERR(pctldev->dev, "L:%d;Parse generic pinconfig error on %d\n",
		     __LINE__, ret);
		return ret;
	}

	*map = kcalloc(*num_maps + nmG + num_generic_maps, sizeof(**map),
		       GFP_KERNEL);
	if (!(*map))
		return -ENOMEM;

	parent = of_get_parent(np_config);
	for (i = 0; i < (*num_maps); i++) {
		dt_pin = be32_to_cpu(list[i]);
		p_p = SPPCTL_PCTLD_P(dt_pin);
		p_g = SPPCTL_PCTLD_G(dt_pin);
		p_f = SPPCTL_PCTLD_F(dt_pin);
		p_l = SPPCTL_PCTLD_L(dt_pin);
		(*map)[i].name = parent->name;
		KDBG(pctldev->dev, "map [%d]=%08x p=%d g=%d f=%d l=%d\n", i,
		     dt_pin, p_p, p_g, p_f, p_l);

		if (p_g == SPPCTL_PCTL_G_GPIO) {
			// look into parse_dt_cfg(),
			(*map)[i].type = PIN_MAP_TYPE_CONFIGS_PIN;
			(*map)[i].data.configs.num_configs = 1;
			(*map)[i].data.configs.group_or_pin =
				pin_get_name(pctldev, p_p);
			configs = kcalloc(1, sizeof(*configs), GFP_KERNEL);
			if (!configs)
				goto sppctl_n2map_err;
			*configs = sppctl_pinconf_param_2_generic_pinconf_param(
				p_l);
			(*map)[i].data.configs.configs = configs;

			KDBG(pctldev->dev, "%s(%d) = x%X\n",
			     (*map)[i].data.configs.group_or_pin, p_p, p_l);
		} else if (p_g == SPPCTL_PCTL_G_IOPP) {
			(*map)[i].type = PIN_MAP_TYPE_CONFIGS_PIN;
			(*map)[i].data.configs.num_configs = 1;
			(*map)[i].data.configs.group_or_pin =
				pin_get_name(pctldev, p_p);
			configs = kcalloc(1, sizeof(*configs), GFP_KERNEL);
			if (!configs)
				goto sppctl_n2map_err;
			*configs = sppctl_pinconf_param_2_generic_pinconf_param(
				PIN_CONFIG_MAX);
			(*map)[i].data.configs.configs = configs;

			KDBG(pctldev->dev, "%s(%d) = x%X\n",
			     (*map)[i].data.configs.group_or_pin, p_p, p_l);
		} else {
			(*map)[i].type = PIN_MAP_TYPE_MUX_GROUP;
			(*map)[i].data.mux.function = list_funcs[p_f].name;
			(*map)[i].data.mux.group = pin_get_name(pctldev, p_p);

			KDBG(pctldev->dev, "f->p: %s(%d)->%s(%d)\n",
			     (*map)[i].data.mux.function, p_f,
			     (*map)[i].data.mux.group, p_p);
		}
	}

	for (i = 0; i < num_generic_maps; i++, (*num_maps)++) {
		struct pinctrl_map pmap = generic_map[i];

		if (pmap.type == PIN_MAP_TYPE_MUX_GROUP) {
			(*map)[*num_maps].type = pmap.type;
			(*map)[*num_maps].data.mux.group = pmap.data.mux.group;
			(*map)[*num_maps].data.mux.function =
				pmap.data.mux.function;
		} else {
			(*map)[*num_maps].type = pmap.type;
			(*map)[*num_maps].data.configs.group_or_pin =
				pmap.data.configs.group_or_pin;
			(*map)[*num_maps].data.configs.configs =
				pmap.data.configs.configs;
			(*map)[*num_maps].data.configs.num_configs =
				pmap.data.configs.num_configs;
		}
	}

#if defined(SUPPORT_GPIO_AO_INT)
	for (i = 0; i < ao_nm; i++) {
		dt_pin = be32_to_cpu(ao_list[i]);
		p_p = SPPCTL_AOPIN_PIN(dt_pin);
		p_l = SPPCTL_AOPIN_FLG(dt_pin);

		mask = 1 << p_p;
		reg1 = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		if (p_l & (SPPCTL_AOPIN_OUT0 | SPPCTL_AOPIN_OUT1)) {
			reg2 = readl(pc->gpio_ao_int_regs_base +
				     0x14); // GPIO_O
			if (p_l & SPPCTL_AOPIN_OUT1)
				reg2 |= mask;
			else
				reg2 &= ~mask;
			writel(reg2,
			       pc->gpio_ao_int_regs_base + 0x14); // GPIO_O

			reg1 |= mask;
		} else {
			reg1 &= ~mask;
		}
		writel(reg1, pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE

		reg1 = readl(pc->gpio_ao_int_regs_base + 0x08); // DEB_EN
		if (p_l & SPPCTL_AOPIN_DEB)
			reg1 |= mask;
		else
			reg1 &= ~mask;
		writel(reg1, pc->gpio_ao_int_regs_base + 0x08); // DEB_EN
	}
#endif

	// handle zero function
	list = of_get_property(np_config, "eys3d,zerofunc", &size);
	if (list) {
		for (i = 0; i < size / sizeof(*list); i++) {
			dt_fun = be32_to_cpu(list[i]);
			if (dt_fun >= list_func_nums) {
				KERR(pctldev->dev,
				     "zero func %d out of range\n", dt_fun);
				continue;
			}

			func = &list_funcs[dt_fun];
			switch (func->freg) {
			case F_OFF_M:
				KDBG(pctldev->dev, "zero func: %d (%s)\n",
				     dt_fun, func->name);
				sppctl_pin_set(pctrl, 0, dt_fun - 2);
				break;

			case F_OFF_G:
				KDBG(pctldev->dev, "zero group: %d (%s)\n",
				     dt_fun, func->name);
				sppctl_gmx_set(pctrl, func->roff, func->boff,
					       func->blen, 0);
				break;

			default:
				KERR(pctldev->dev,
				     "wrong zero group: %d (%s)\n", dt_fun,
				     func->name);
				break;
			}
		}
	}

	of_node_put(parent);
	KDBG(pctldev->dev, "%d pins or functions are mapped!\n", *num_maps);
	return 0;

sppctl_n2map_err:
	for (i = 0; i < (*num_maps); i++)
		if (((*map)[i].type == PIN_MAP_TYPE_CONFIGS_PIN) &&
		    (*map)[i].data.configs.configs)
			kfree((*map)[i].data.configs.configs);
	kfree(*map);
	of_node_put(parent);
	return -ENOMEM;
}

static void sppctl_pinctrl_dt_free_map(struct pinctrl_dev *pctldev,
				       struct pinctrl_map *map,
				       unsigned int num_maps)
{
	//KINF(pctldev->dev, "%s(%d)\n", __func__, num_maps);
	// FIXME: test
	pinctrl_utils_free_map(pctldev, map, num_maps);
}

// creates unq_grps[] uniq group names array char *
// sets unique_groups_nums
// creates XXX[group_idx]{func_idx, pins_idx}
static void group_groups(struct platform_device *pdev)
{
	int i;
	int k;
	int j;

	// fill array of all groups
	unq_grps = NULL;
	unique_groups_nums = GPIS_list_size;

	// calc unique group names array size
	for (i = 0; i < list_func_nums; i++) {
		if (list_funcs[i].freg != F_OFF_G)
			continue;
		unique_groups_nums += list_funcs[i].gnum;
	}

	// fill up unique group names array
	unq_grps = devm_kzalloc(&pdev->dev,
				(unique_groups_nums + 1) * sizeof(char *),
				GFP_KERNEL);
	g2fp_maps = devm_kzalloc(&pdev->dev,
				 (unique_groups_nums + 1) *
					 sizeof(struct grp2fp_map_t),
				 GFP_KERNEL);

	// groups == pins
	j = 0;
	for (i = 0; i < GPIS_list_size; i++) {
		unq_grps[i] = sppctlgpio_list_s[i];
		g2fp_maps[i].f_idx = 0;
		g2fp_maps[i].g_idx = i;
	}
	j = GPIS_list_size;

	// +IOP groups
	for (i = 0; i < list_func_nums; i++) {
		if (list_funcs[i].freg != F_OFF_G)
			continue;

		for (k = 0; k < list_funcs[i].gnum; k++) {
			list_funcs[i].grps_sa[k] =
				(char *)list_funcs[i].grps[k].name;
			unq_grps[j] = list_funcs[i].grps[k].name;
			g2fp_maps[j].f_idx = i;
			g2fp_maps[j].g_idx = k;
			j++;
		}
	}
	KDBG(&pdev->dev, "funcs: %zd unq_grps: %zd\n", list_func_nums,
	     unique_groups_nums);
}

static const struct pinconf_ops sppctl_pconf_ops = {
	.is_generic = true,
	.pin_config_get = sppctl_pinconf_get,
	.pin_config_set = sppctl_pinconf_set,
	.pin_config_group_get = sppctl_pinconf_group_get,
	.pin_config_group_set = sppctl_pinconf_group_set,
	.pin_config_dbg_show = sppctl_pinconf_dbg_show,
	.pin_config_group_dbg_show = sppctl_pinconf_group_dbg_show,
	.pin_config_config_dbg_show = sppctl_pinconf_config_dbg_show,
};

static const struct pinmux_ops sppctl_pinmux_ops = {
	.request = sppctl_pinmux_request,
	.free = sppctl_pinmux_free,
	.get_functions_count = sppctl_pinmux_get_functions_count,
	.get_function_name = sppctl_pinmux_get_function_name,
	.get_function_groups = sppctl_pinmux_get_function_groups,
	.set_mux = sppctl_pinmux_set_mux,
	.gpio_request_enable = sppctl_pinmux_gpio_request_enable,
	.gpio_disable_free = sppctl_pinmux_gpio_disable_free,
	.gpio_set_direction = sppctl_pinmux_gpio_set_direction,
	.strict = 1
};

static const struct pinctrl_ops sppctl_pctl_ops = {
	.get_groups_count = sppctl_pinctrl_get_groups_count,
	.get_group_name = sppctl_pinctrl_get_group_name,
	.get_group_pins = sppctl_pinctrl_get_group_pins,
#ifdef CONFIG_DEBUG_FS
	.pin_dbg_show = sppctl_pinctrl_pin_dbg_show,
#endif
	.dt_node_to_map = sppctl_pinctrl_dt_node_to_map,
	.dt_free_map = sppctl_pinctrl_dt_free_map,
};

int sppctl_pinctrl_init(struct platform_device *pdev)
{
	struct device_node *np_config;
	struct sppctl_pdata_t *pdata;
	struct device *dev;
	int err;

	dev = &pdev->dev;
	pdata = (struct sppctl_pdata_t *)pdev->dev.platform_data;
	np_config = of_node_get(dev->of_node);

	// init pdesc
	pdata->pdesc.owner = THIS_MODULE;
	pdata->pdesc.name = dev_name(&pdev->dev);
	pdata->pdesc.pins = &sppctlpins_all[0];
	pdata->pdesc.npins = sppctlpins_all_nums;
	pdata->pdesc.pctlops = &sppctl_pctl_ops;
	pdata->pdesc.confops = &sppctl_pconf_ops;
	pdata->pdesc.pmxops = &sppctl_pinmux_ops;

#ifdef CONFIG_GENERIC_PINCONF
	pdata->pdesc.custom_params = sppctl_dt_params;
	pdata->pdesc.num_custom_params = ARRAY_SIZE(sppctl_dt_params);
#endif

#ifdef CONFIG_DEBUG_FS
	pdata->pdesc.custom_conf_items = sppctl_conf_items;
#endif

	group_groups(pdev);

	err = devm_pinctrl_register_and_init(&pdev->dev, &pdata->pdesc, pdata,
					     &pdata->pcdp);
	if (err) {
		KERR(&pdev->dev, "Failed to register\n");
		of_node_put(np_config);
		return err;
	}

	pinctrl_enable(pdata->pcdp);
	return 0;
}

void sppctl_pinctrl_clean(struct platform_device *pdev)
{
	struct sppctl_pdata_t *pdata;

	pdata = (struct sppctl_pdata_t *)pdev->dev.platform_data;

	devm_pinctrl_unregister(&pdev->dev, pdata->pcdp);
}

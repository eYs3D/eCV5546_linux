/*
 * Generic DWMAC platform driver
 *
 * Copyright (C) 2007-2011  STMicroelectronics Ltd
 * Copyright (C) 2015 Joachim Eastwood <manabian@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "stmmac.h"
#include "stmmac_platform.h"

#if IS_ENABLED(CONFIG_SOC_eCV5546)
#define GMAC_TX_SOFTPAD_REG	0xF8803378
#endif

#if IS_ENABLED(CONFIG_SOC_eCV5546)
static void eys3d_fix_mac_speed(void *priv, unsigned int speed,
				unsigned int mode)
{
	struct stmmac_priv *stmmac = (struct stmmac_priv *)priv;
	unsigned long rate;
	int ret;
	void __iomem *rgmii_tx_softpad_reg  = NULL;
	unsigned int rgmii_tx_softpad;

	clk_disable(stmmac->plat->stmmac_clk);
	clk_unprepare(stmmac->plat->stmmac_clk);

	if (stmmac->plat->phy_interface == PHY_INTERFACE_MODE_RMII) {
		switch (speed) {
		case SPEED_100:
			rate = 50000000;
			break;
		case SPEED_10:
			rate = 5000000;
			break;
		default:
			dev_err(stmmac->device, "Invalid speed!\n");
			break;
		}
	} else {
		rgmii_tx_softpad_reg = ioremap(GMAC_TX_SOFTPAD_REG, 4);
		if (rgmii_tx_softpad_reg == NULL) {
			dev_err(stmmac->device, "failed to get gmac tx softpad reg\n");
		}

		switch (speed) {
		case SPEED_1000:
			rate = 125000000;
			rgmii_tx_softpad = stmmac->plat->rgmii_tx_softpad_1000m;
			break;
		case SPEED_100:
			rate = 25000000;
			rgmii_tx_softpad = stmmac->plat->rgmii_tx_softpad_100m;
			break;
		case SPEED_10:
			rate = 2500000;
			rgmii_tx_softpad = stmmac->plat->rgmii_tx_softpad_100m;
			break;
		default:
			dev_err(stmmac->device, "Invalid speed!\n");
			rgmii_tx_softpad = 0xFFFFFFFF;
			break;
		}
		if ((rgmii_tx_softpad_reg != NULL) && (rgmii_tx_softpad != 0xFFFFFFFF)) {
			writel(rgmii_tx_softpad, rgmii_tx_softpad_reg);
		}
	}

	iounmap(rgmii_tx_softpad_reg);
	ret = clk_set_rate(stmmac->plat->stmmac_clk, rate);
	if (ret)
		dev_err(stmmac->device, "Failed to configure stmmac clock rate!\n");
	clk_prepare_enable(stmmac->plat->stmmac_clk);
}
#endif

static int dwmac_generic_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	int ret;

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	if (pdev->dev.of_node) {
		plat_dat = stmmac_probe_config_dt(pdev, stmmac_res.mac);
		if (IS_ERR(plat_dat)) {
			dev_err(&pdev->dev, "dt configuration failed\n");
			return PTR_ERR(plat_dat);
		}
	} else {
		plat_dat = dev_get_platdata(&pdev->dev);
		if (!plat_dat) {
			dev_err(&pdev->dev, "no platform data provided\n");
			return  -EINVAL;
		}

		/* Set default value for multicast hash bins */
		plat_dat->multicast_filter_bins = HASH_TABLE_SIZE;

		/* Set default value for unicast filter entries */
		plat_dat->unicast_filter_entries = 1;
	}

#if IS_ENABLED(CONFIG_SOC_eCV5546)
	plat_dat->fix_mac_speed = eys3d_fix_mac_speed;
	// for RTL8211F_WOL_TEST
	//plat_dat->flags |= STMMAC_FLAG_USE_PHY_WOL;
#endif

	ret = stmmac_pltfr_probe(pdev, plat_dat, &stmmac_res);
	if (ret)
		goto err_remove_config_dt;

	return 0;

err_remove_config_dt:
	if (pdev->dev.of_node)
		stmmac_remove_config_dt(pdev, plat_dat);

	return ret;
}

static const struct of_device_id dwmac_generic_match[] = {
	{ .compatible = "st,spear600-gmac"},
	{ .compatible = "snps,dwmac-3.40a"},
	{ .compatible = "snps,dwmac-3.50a"},
	{ .compatible = "snps,dwmac-3.610"},
	{ .compatible = "snps,dwmac-3.70a"},
	{ .compatible = "snps,dwmac-3.710"},
	{ .compatible = "snps,dwmac-4.00"},
	{ .compatible = "snps,dwmac-4.10a"},
	{ .compatible = "snps,dwmac"},
	{ .compatible = "snps,dwxgmac-2.10"},
	{ .compatible = "snps,dwxgmac"},
	{ }
};
MODULE_DEVICE_TABLE(of, dwmac_generic_match);

static struct platform_driver dwmac_generic_driver = {
	.probe  = dwmac_generic_probe,
	.remove_new = stmmac_pltfr_remove,
	.driver = {
		.name           = STMMAC_RESOURCE_NAME,
		.pm		= &stmmac_pltfr_pm_ops,
		.of_match_table = dwmac_generic_match,
	},
};
module_platform_driver(dwmac_generic_driver);

MODULE_DESCRIPTION("Generic dwmac driver");
MODULE_LICENSE("GPL v2");

// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/usb/sp_usb.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "ehci-eys3d.h"

u8 sp_port0_enabled = PORT0_ENABLED;

static int ehci0_eys3d_platform_probe(struct platform_device *dev)
{
	dev->id = 1;

	return ehci_eys3d_probe(dev);
}

static const struct of_device_id ehci0_eys3d_dt_ids[] = {
	{ .compatible = "eys3d,ecv5546-usb-ehci" },
	{ }
};
MODULE_DEVICE_TABLE(of, ehci0_eys3d_dt_ids);

static struct platform_driver ehci0_hcd_eys3d_driver = {
	.probe			= ehci0_eys3d_platform_probe,
	.remove			= ehci_eys3d_remove,
	.shutdown		= usb_hcd_platform_shutdown,
	.driver = {
		.name		= "ehci0-eys3d",
		.of_match_table = ehci0_eys3d_dt_ids,
#ifdef CONFIG_PM
		.pm = &ehci_eys3d_pm_ops,
#endif
	}
};

/* ---------------------------------------------------------------------------------------------- */
static int __init ehci0_eys3d_init(void)
{
	if (sp_port0_enabled == PORT0_ENABLED) {
		pr_notice("register ehci0_hcd_eys3d_driver\n");
		return platform_driver_register(&ehci0_hcd_eys3d_driver);
	}

	pr_notice("warn,port0 not enable,not register ehci0 eys3d hcd driver\n");

	return -1;
}
module_init(ehci0_eys3d_init);

static void __exit ehci0_eys3d_cleanup(void)
{
	platform_driver_unregister(&ehci0_hcd_eys3d_driver);
}
module_exit(ehci0_eys3d_cleanup);

MODULE_AUTHOR("Vincent Shih <eys3d@eys3d.com>");
MODULE_DESCRIPTION("eYs3D USB EHCI (port 0) driver");
MODULE_LICENSE("GPL v2");


// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/usb/sp_usb.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "ohci-eys3d.h"

#ifndef CONFIG_USB_EHCI_EYS3D
u8 sp_port0_enabled = PORT0_ENABLED;
#endif

static int ohci0_eys3d_platform_probe(struct platform_device *dev)
{
	dev->id = 1;

	return ohci_eys3d_probe(dev);
}

static const struct of_device_id ohci0_eys3d_dt_ids[] = {
	{ .compatible = "eys3d,ecv5546-usb-ohci" },
	{ }
};
MODULE_DEVICE_TABLE(of, ohci0_eys3d_dt_ids);

static struct platform_driver ohci0_hcd_eys3d_driver = {
	.probe			= ohci0_eys3d_platform_probe,
	.remove			= ohci_eys3d_remove,
	.shutdown		= usb_hcd_platform_shutdown,
	.driver = {
		.name		= "ohci0-eys3d",
		.of_match_table = ohci0_eys3d_dt_ids,
#ifdef CONFIG_PM
		.pm = &ohci_eys3d_pm_ops,
#endif
	}
};

/*-------------------------------------------------------------------------*/

static int __init ohci0_eys3d_init(void)
{
	if (sp_port0_enabled == PORT0_ENABLED) {
		pr_notice("register ohci0_hcd_eys3d_driver\n");
		return platform_driver_register(&ohci0_hcd_eys3d_driver);
	}

	pr_notice("warn,port0 not enable,not register ohci0 eys3d hcd driver\n");

	return -1;
}
module_init(ohci0_eys3d_init);

static void __exit ohci0_eys3d_cleanup(void)
{
	platform_driver_unregister(&ohci0_hcd_eys3d_driver);
}
module_exit(ohci0_eys3d_cleanup);

MODULE_AUTHOR("Vincent Shih <eys3d@eys3d.com>");
MODULE_DESCRIPTION("eYs3D USB OHCI (port 0) driver");
MODULE_LICENSE("GPL v2");


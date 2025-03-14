/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __OHCI_PLATFORM_H
#define __OHCI_PLATFORM_H

#ifdef CONFIG_PM
extern struct dev_pm_ops const ohci_eys3d_pm_ops;
#endif

int ohci_eys3d_probe(struct platform_device *dev);
int ohci_eys3d_remove(struct platform_device *dev);

void usb_hcd_platform_shutdown(struct platform_device *dev);
#endif


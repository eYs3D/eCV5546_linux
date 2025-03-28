// SPDX-License-Identifier: GPL-2.0-or-later

/**************************************************************************************************/
/* Generic platform ohci driver									  */
/*												  */
/* Copyright 2007 Michael Buesch <m@bues.ch>							  */
/* Copyright 2011-2012 Hauke Mehrtens <hauke@hauke-m.de>					  */
/*												  */
/* Derived from the OCHI-SSB driver								  */
/* Derived from the OHCI-PCI driver								  */
/* Copyright 1999 Roman Weissgaerber								  */
/* Copyright 2000-2002 David Brownell								  */
/* Copyright 1999 Linus Torvalds								  */
/* Copyright 1999 Gregory P. Smith								  */
/*												  */
/* Licensed under the GNU/GPL. See COPYING for details.						  */
/**************************************************************************************************/
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/usb/ohci_pdriver.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/usb/sp_usb.h>

#ifdef CONFIG_USB_SP_UDC_HOST
#include "udc-to-host.h"
#endif

#ifdef CONFIG_USB_HOST_RESET
struct ohci_hcd_sp {
	/* must be 1st member here for hcd_to_ehci() to work */
	struct ohci_hcd ohci;

	struct task_struct *reset_thread;
	struct notifier_block ohci_notifier;

	#ifdef CONFIG_USB_SP_UDC_HOST
	struct notifier_block udc_notifier;
	#endif

	u32 flag;

#define RESET_UPHY_SIGN		BIT(0)
#define RESET_HC_SIGN		BIT(1)
#define RESET_SENDER		BIT(31)
#define FASTBOOT_FG		BIT(30)
};

#elif defined CONFIG_USB_SP_UDC_HOST

struct ohci_hcd_sp {
	/* must be 1st member here for hcd_to_ehci() to work */
	struct ohci_hcd ohci;
	struct notifier_block udc_notifier;
};
#endif

void __iomem *ohci_res_moon0;

/* fix ohci msi */
/*							*/
/* twofish suggest that latency should not		*/
/* exceed 8192 clock cycles(system clock),about 24.8us	*/
/*							*/
s32 get_td_retry_time = 24;

static struct clk *ohci_clk[USB_PORT_NUM];
static struct reset_control *ohci_rstc[USB_PORT_NUM];
static struct phy *uphy[USB_PORT_NUM];

static int ohci_platform_reset(struct usb_hcd *hcd)
{
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
	struct usb_ohci_pdata *pdata = pdev->dev.platform_data;
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int err;

	if (pdata->big_endian_desc)
		ohci->flags |= OHCI_QUIRK_BE_DESC;
	if (pdata->big_endian_mmio)
		ohci->flags |= OHCI_QUIRK_BE_MMIO;
	if (pdata->no_big_frame_no)
		ohci->flags |= OHCI_QUIRK_FRAME_NO;

	ohci->next_statechange = jiffies;
	spin_lock_init(&ohci->lock);
	INIT_LIST_HEAD(&ohci->pending);
	INIT_LIST_HEAD(&ohci->eds_in_use);

	err = ohci_init(ohci);

	return err;
}

static int ohci_platform_start(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int err;

	err = ohci_run(ohci);
	if (err < 0) {
		ohci_err(ohci, "can't start\n");
		ohci_stop(hcd);
	}

	return err;
}

u8 otg0_vbus_off;
EXPORT_SYMBOL_GPL(otg0_vbus_off);

u8 otg1_vbus_off;
EXPORT_SYMBOL_GPL(otg1_vbus_off);

static int ohci_update_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	struct platform_device *pdev = to_platform_device(hcd->self.controller);

	if ((udev->descriptor.bcdDevice & 0x1) == 0x1) {
		if (pdev->id == 1)
			otg0_vbus_off = 1;
		else if (pdev->id == 2)
			otg1_vbus_off = 1;
	}

	return 0;
}

static const struct hc_driver ohci_platform_hc_driver = {
	.description = hcd_name,
	.product_desc = "Generic Platform OHCI Controller",

#ifdef CONFIG_USB_HOST_RESET
	.hcd_priv_size = sizeof(struct ohci_hcd_sp),
#else
	.hcd_priv_size = sizeof(struct ohci_hcd),
#endif

	.irq = ohci_irq,
	.flags = HCD_MEMORY | HCD_DMA | HCD_USB11,

	.reset = ohci_platform_reset,
	.start = ohci_platform_start,
	.stop = ohci_stop,
	.shutdown = ohci_shutdown,

	.urb_enqueue = ohci_urb_enqueue,
	.urb_dequeue = ohci_urb_dequeue,
	.endpoint_disable = ohci_endpoint_disable,

	.get_frame_number = ohci_get_frame,

	.update_device = ohci_update_device,

	.hub_status_data = ohci_hub_status_data,
	.hub_control = ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend = ohci_bus_suspend,
	.bus_resume = ohci_bus_resume,
#endif

	.start_port_reset = ohci_start_port_reset,
};

#ifdef CONFIG_USB_HOST_RESET
static int ohci_reset_thread(void *arg)
{
	struct ohci_hcd_sp *sp_ohci = (struct ohci_hcd_sp *)arg;
	struct ohci_hcd *ohci = (struct ohci_hcd *)arg;
	struct usb_hcd *hcd = ohci_to_hcd(ohci);
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
	u32 val;
	int i;

	while (1) {
		if (sp_ohci->flag & (RESET_UPHY_SIGN | RESET_HC_SIGN)) {
			while (hcd->self.devmap.devicemap[0] != 2)
				fsleep(1000);

			if (hcd->self.devmap.devicemap[0] > 2)
				goto NEXT_LOOP;

			for (i = 1; i < 128 / (8 * sizeof(unsigned long)); i++) {
				if (hcd->self.devmap.devicemap[i])
					goto NEXT_LOOP;
			}

			/* hcd->irq will be set to 0 by ohci_stop() */
			if (pdev->id == 1)
				i = SP_IRQ_OHCI_USB0;
			else if (pdev->id == 2)
				i = SP_IRQ_OHCI_USB1;

			if (sp_ohci->flag & RESET_UPHY_SIGN) {
				writel(RF_MASK_V_SET(1 << (12 + (pdev->id - 1)),
						     1 << (12 + (pdev->id - 1))),
				       ohci_res_moon0 + USBC0_RESET_OFFSET);
				writel(RF_MASK_V_CLR(1 << (12 + (pdev->id - 1))),
				       ohci_res_moon0 + USBC0_RESET_OFFSET);
				writel(RF_MASK_V_SET(1 << (15 + (pdev->id - 1)),
						     1 << (15 + (pdev->id - 1))),
				       ohci_res_moon0 + USBC0_RESET_OFFSET);
				writel(RF_MASK_V_CLR(1 << (15 + (pdev->id - 1))),
				       ohci_res_moon0 + USBC0_RESET_OFFSET);

				sp_ohci->flag = RESET_SENDER;
			} else {
				sp_ohci->flag = 0;
			}

			hcd->irq = i;
			usb_remove_hcd(hcd);
			ohci_usb_reset(ohci);
			fsleep(1000);
			usb_add_hcd(hcd, i, IRQF_SHARED);
		}

NEXT_LOOP:
		sp_ohci->flag = 0;
		msleep(30);
	}

	return 0;
}
#endif

#ifdef CONFIG_USB_HOST_RESET
static int ohci_notifier_call(struct notifier_block *self, unsigned long action, void *dev)
{
	struct usb_device *udev = (struct usb_device *)dev;
	struct usb_hcd *hcd_e;
	struct platform_device *pdev_e;
	u32 *ptr;
	struct ohci_hcd_sp *sp_ohci = container_of((void *)self,
							struct ohci_hcd_sp,
							ohci_notifier);
	struct ohci_hcd *ohci = (struct ohci_hcd *)sp_ohci;
	struct usb_hcd *hcd_o = ohci_to_hcd(ohci);
	struct platform_device *pdev_o = to_platform_device(hcd_o->self.controller);

	pr_debug("notifier ohci\n");

	if (action == USB_DEVICE_ADD) {
		if (!udev->parent) {	// roothub add
			hcd_e = bus_to_hcd(udev->bus);
			pdev_e = to_platform_device(hcd_e->self.controller);
			ptr = (u32 *)((u8 *)hcd_e->hcd_priv + hcd_e->driver->hcd_priv_size -
					    sizeof(u32));
			pr_debug("notifier ohci %p 0x%x\n", ptr, *ptr);

			// EHCI&OHCI on one port
			if (pdev_e != pdev_o && pdev_e->id == pdev_o->id) {
				if (ptr && (*ptr & RESET_SENDER)) {
					sp_ohci->flag =
					    RESET_HC_SIGN & (~RESET_UPHY_SIGN);
					*ptr &= ~RESET_SENDER;
				}
			}
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_USB_SP_UDC_HOST
static int udc_notifier_call(struct notifier_block *self, unsigned long action, void *dev)
{
	struct ohci_hcd_sp *sp_ohci = container_of((void *)self, struct ohci_hcd_sp,
								udc_notifier);
	struct ohci_hcd *ohci = (struct ohci_hcd *)sp_ohci;
	struct usb_hcd *hcd = ohci_to_hcd(ohci);
	struct udc_hcd_platform_device *udc_dev = (struct udc_hcd_platform_device *)dev;

	pr_debug("udc notifier %lx %x %p\n", action, hcd->rh_registered, hcd);

	if (hcd->rh_registered == 1)

		if (action == USB_DEVICE_ADD) {
			while (HC_IS_RUNNING(hcd->state))
				fsleep(1000);

			SET_TO_UDC(udc_dev->state);
		}

	if (action == USB_DEVICE_REMOVE) {
		// TBD
		// TBD
	}

	return 0;
}
#endif

/* fix ohci msi */
static ssize_t get_td_retry_time_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	pr_debug("get get_td_retry_time\n");

	return sprintf(buf, "%d\n", get_td_retry_time);
}

static ssize_t get_td_retry_time_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	pr_debug("set get_td_retry_time\n");

	if (kstrtouint(buf, 0, &get_td_retry_time) == 0)
		pr_debug("%d\n", get_td_retry_time);
	else
		pr_debug("set get_td_retry_time fail\n");

	return count;
}
static DEVICE_ATTR_RW(get_td_retry_time);

static int sp_ohci_platform_power_on(struct platform_device *pdev)
{
	int ret;

	ret = phy_power_on(uphy[pdev->id - 1]);
	if (ret)
		return ret;

	ret = phy_init(uphy[pdev->id - 1]);
	if (ret)
		return ret;

	ret = reset_control_deassert(ohci_rstc[pdev->id - 1]);
	if (ret)
		return ret;

	ret = clk_prepare_enable(ohci_clk[pdev->id - 1]);
	if (ret)
		reset_control_assert(ohci_rstc[pdev->id - 1]);

	return ret;
}

static void sp_ohci_platform_power_off(struct platform_device *pdev)
{
	clk_disable_unprepare(ohci_clk[pdev->id - 1]);
	reset_control_assert(ohci_rstc[pdev->id - 1]);

	phy_power_off(uphy[pdev->id - 1]);
	phy_exit(uphy[pdev->id - 1]);
}

static struct usb_ohci_pdata usb_ohci_pdata = {
	.power_on = sp_ohci_platform_power_on,
	.power_suspend = sp_ohci_platform_power_off,
	.power_off = sp_ohci_platform_power_off,
};

int ohci_eys3d_probe(struct platform_device *pdev)
{
	struct usb_ohci_pdata *pdata = &usb_ohci_pdata;
	struct usb_hcd *hcd;
	struct resource *res_mem;
	int irq;
	int err = -ENOMEM;

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_SP_UDC_HOST)
	struct ohci_hcd_sp *ohci_sp;
#endif

	if (usb_disabled())
		return -ENODEV;

	pdev->dev.platform_data = &usb_ohci_pdata;

	/* phy */
	if ((pdev->id - 1) == USB_PORT0_ID) {
		uphy[pdev->id - 1] = devm_phy_get(&pdev->dev, "uphy");
		if (IS_ERR(uphy[pdev->id - 1])) {
			dev_err(&pdev->dev, "no USB phy0 configured\n");
			return PTR_ERR(uphy[pdev->id - 1]);
		}
	} else if ((pdev->id - 1) == USB_PORT1_ID) {
		uphy[pdev->id - 1] = devm_phy_get(&pdev->dev, "uphy");
		if (IS_ERR(uphy[pdev->id - 1])) {
			dev_err(&pdev->dev, "no USB phy1 configured\n");
			return PTR_ERR(uphy[pdev->id - 1]);
		}
	}

	/* reset */
	ohci_rstc[pdev->id - 1] = devm_reset_control_get_shared(&pdev->dev, NULL);
	if (IS_ERR(ohci_rstc[pdev->id - 1])) {
		err = PTR_ERR(ohci_rstc[pdev->id - 1]);
		pr_err("EHCI failed to retrieve reset controller: %d\n", err);
		return err;
	}

	/* enable usb controller clock */
	ohci_clk[pdev->id - 1] = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(ohci_clk[pdev->id - 1])) {
		pr_err("not found clk source\n");
		return PTR_ERR(ohci_clk[pdev->id - 1]);
	}

	if (pdata->power_on) {
		err = pdata->power_on(pdev);
		if (err)
			return err;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		pr_err("no irq provieded\n");
		err = irq;
		goto err_power;
	}
	pr_debug("ohci_id:%d,irq:%d\n", pdev->id, irq);

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		pr_err("no memory recourse provieded\n");
		err = -ENXIO;
		goto err_power;
	}

	hcd = usb_create_hcd(&ohci_platform_hc_driver, &pdev->dev,
			     dev_name(&pdev->dev));
	if (!hcd) {
		err = -ENOMEM;
		goto err_power;
	}

	hcd->rsrc_start = res_mem->start;
	hcd->rsrc_len = resource_size(res_mem);

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_err("controller already in use\n");
		err = -EBUSY;
		goto err_put_hcd;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs)
		goto err_release_region;

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	ohci_res_moon0 = devm_ioremap(&pdev->dev, res_mem->start, resource_size(res_mem));
	if (IS_ERR(ohci_res_moon0))
		return PTR_ERR(ohci_res_moon0);

	err = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (err)
		goto err_iounmap;

	platform_set_drvdata(pdev, hcd);

#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_SP_UDC_HOST)
	ohci_sp = (struct ohci_hcd_sp *)hcd_to_ohci(hcd);
#endif

#ifdef CONFIG_USB_SP_UDC_HOST

	/*udc/host uphy notifier */
	if (pdev->id == 3) {
		ohci_sp->udc_notifier.notifier_call = udc_notifier_call;
		udc_register_notify(&ohci_sp->udc_notifier);
	}
#endif

	/*fix ohci msi */
	device_create_file(&pdev->dev, &dev_attr_get_td_retry_time);

#ifdef CONFIG_USB_HOST_RESET
	//struct ohci_hcd_sp *ohci_sp = (struct ohci_hcd_sp *)hcd_to_ohci(hcd);
	ohci_sp->ohci_notifier.notifier_call = ohci_notifier_call;
	usb_register_notify(&ohci_sp->ohci_notifier);

	ohci_sp->flag = 0;
	pr_debug("flag *** %d %d %p\n", sizeof(struct ohci_hcd_sp),
		 hcd->driver->hcd_priv_size, &ohci_sp->flag);

	ohci_sp->reset_thread = kthread_create(ohci_reset_thread,
					       hcd_to_ohci(hcd),
					       "ohci_reset_polling");

	if (IS_ERR(ohci_sp->reset_thread)) {
		pr_err("Create OHCI(%d) reset thread fail!\n", pdev->id);
		return err;
	}

	/* Tell the thread to start working */
	wake_up_process(ohci_sp->reset_thread);

#endif
	return err;

err_iounmap:
	iounmap(hcd->regs);
err_release_region:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err_put_hcd:
	usb_put_hcd(hcd);
err_power:
	clk_disable_unprepare(ohci_clk[pdev->id - 1]);
	reset_control_assert(ohci_rstc[pdev->id - 1]);

	return err;
}
EXPORT_SYMBOL_GPL(ohci_eys3d_probe);

int ohci_eys3d_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct usb_ohci_pdata *pdata = pdev->dev.platform_data;
#if defined(CONFIG_USB_HOST_RESET) || defined(CONFIG_USB_SP_UDC_HOST)
	struct ohci_hcd_sp *ohci_sp = (struct ohci_hcd_sp *)hcd_to_ohci(hcd);
#endif

	/*fix ohci msi */
	device_remove_file(&pdev->dev, &dev_attr_get_td_retry_time);

#ifdef CONFIG_USB_SP_UDC_HOST
	if (pdev->id == 3)
		udc_unregister_notify(&ohci_sp->udc_notifier);
#endif

#ifdef CONFIG_USB_HOST_RESET
	kthread_stop(ohci_sp->reset_thread);
	usb_unregister_notify(&ohci_sp->ohci_notifier);
#endif

	usb_remove_hcd(hcd);

	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	platform_set_drvdata(pdev, NULL);

	if (pdata->power_off)
		pdata->power_off(pdev);

	return 0;
}
EXPORT_SYMBOL_GPL(ohci_eys3d_remove);

#ifdef CONFIG_PM
static int ohci_eys3d_drv_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_ohci_pdata *pdata = pdev->dev.platform_data;
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	bool do_wakeup = device_may_wakeup(dev);
	int rc;

	pr_debug("%s.%d\n", __func__, __LINE__);

	rc = ohci_suspend(hcd, do_wakeup);
	if (rc)
		return rc;

	if (pdata->power_suspend)
		pdata->power_suspend(pdev);

	return 0;
}

static int ohci_eys3d_drv_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct usb_ohci_pdata *pdata = pdev->dev.platform_data;
	int err;

	pr_debug("%s.%d\n", __func__, __LINE__);

	if (pdata->power_on) {
		err = pdata->power_on(pdev);
		if (err)
			return err;
	}

	ohci_resume(hcd, false);

	return 0;
}

struct dev_pm_ops const ohci_eys3d_pm_ops = {
	.suspend = ohci_eys3d_drv_suspend,
	.resume = ohci_eys3d_drv_resume,
};
#endif


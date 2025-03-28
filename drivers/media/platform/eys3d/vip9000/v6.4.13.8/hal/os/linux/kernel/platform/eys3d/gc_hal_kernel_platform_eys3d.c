// SPDX-License-Identifier: (GPL OR MIT)
/****************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    Copyright (c) 2014 - 2018 Vivante Corporation
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a
 *    copy of this software and associated documentation files (the "Software"),
 *    to deal in the Software without restriction, including without limitation
 *    the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *    and/or sell copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    Copyright (C) 2014 - 2018 Vivante Corporation
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************
 *
 *    Note: This software is released under dual MIT and GPL licenses. A
 *    recipient may use this file under the terms of either the MIT license or
 *    GPL License. If you wish to use only one license not the other, you can
 *    indicate your decision by deleting one of the above license notices in your
 *    version of this file.
 *
 *****************************************************************************/

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"
#include <dt-bindings/clock/sp-ecv5546.h>

#define NPU_WORKAROUND_REG	0xF880007C
#define CHIP_VERSION_REG 	0xF8800000

static void __iomem *npu_sms_reset_reg_base = NULL;
static void __iomem *chip_version_reg_base = NULL;
static void __iomem *npu_ios_reg_base = NULL;

static struct clk *clk;
static struct regulator *vcc_reg;
static struct reset_control *dec_rstc;
static bool isPowerOn;
static int 	version;

static void npu_iso_contrl(int enable)
{
	unsigned int reg_read_value;

	reg_read_value = readl(npu_ios_reg_base);

	if (enable)
		reg_read_value = reg_read_value|(0x1<<4);
	else
		reg_read_value = reg_read_value&0x0;
	writel(reg_read_value, npu_ios_reg_base);
}

static void npu_sms_reset(void)
{
	if(npu_sms_reset_reg_base == NULL)
		return;

	writel(0x5811, npu_sms_reset_reg_base);
	udelay(30);
	writel(0x5807, npu_sms_reset_reg_base);
}

static gceSTATUS npu_power_on(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu)
{
	int ret = gcvSTATUS_FALSE;
	struct device *dev = &platform->device->dev;

	if (isPowerOn) {
		dev_dbg(dev, "%s %d %d is already enabled\n", __func__, devIndex, gpu);
		return gcvSTATUS_TRUE;
	}

	if (IS_ERR(vcc_reg)) {
		dev_err(dev, "failed to get regulator\n");
		return gcvSTATUS_FALSE;
	}
	if (IS_ERR(clk)) {
		dev_err(dev, "failed to get clock source\n");
		return PTR_ERR(clk);
	}

	if (IS_ERR(dec_rstc)) {
		dev_err(dev, "failed to retrieve reset controller\n");
		return PTR_ERR(dec_rstc);
	}

	dev_dbg(dev, "%s %d %d enable\n", __func__, devIndex, gpu);

	/*NPU Power on*/
	if (!regulator_is_enabled(vcc_reg)) {
		ret = regulator_enable(vcc_reg);
		if (ret != 0) {
			dev_err(dev, "regulator get failed: %d\n", ret);
			return ret;
		}
		dev_dbg(dev, "regulator enable success\n");
	}

	/*disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
	npu_iso_contrl(0);
	dev_dbg(dev, "NPU ISO disable\n");

	/*NPU HW reset SMS*/
	if(version == 0xa30){
		npu_sms_reset();
		dev_dbg(dev, "NPU HW reset SMS\n");
	}

	/*NPU HW reset deassert*/
	ret = reset_control_deassert(dec_rstc);
	if (ret) {
		dev_err(dev, "failed to deassert reset line\n");
		return ret;
	}
	dev_dbg(dev, "NPU HW reset deassert\n");

	/*NPU HW clock enable*/
	ret = clk_enable(clk);
	if (ret) {
		dev_err(dev, "enabled clock failed\n");
		return gcvSTATUS_OUT_OF_RESOURCES;
	}
	dev_dbg(dev, "NPU HW clock enable\n");

	isPowerOn = true;

	dev_dbg(dev, "NPU HW wait for enable\n");
	udelay(300);

	dev_dbg(dev, "%s enable ret %d\n", __func__, ret);

	return ret;
}

static gceSTATUS npu_power_off(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu)
{
	int ret = gcvSTATUS_FALSE;
	struct device *dev = &platform->device->dev;

	if (!isPowerOn) {
		dev_dbg(dev, "%s %d %d is already disabled\n", __func__, devIndex, gpu);
		return gcvSTATUS_TRUE;
	}

	if (IS_ERR(vcc_reg)) {
		dev_err(dev, "failed to get regulator\n");
		return gcvSTATUS_FALSE;
	}
	if (IS_ERR(clk)) {
		dev_err(dev, "failed to get clock source\n");
		return PTR_ERR(clk);
	}

	if (IS_ERR(dec_rstc)) {
		dev_err(dev, "failed to retrieve reset controller\n");
		return PTR_ERR(dec_rstc);
	}

	dev_dbg(dev, "%s %d %d disable\n", __func__, devIndex, gpu);

	/*NPU HW clock disable*/
	clk_disable(clk);
	dev_dbg(dev, "NPU HW clock disabled\n");

	/*NPU HW Reset assert*/
	ret = reset_control_assert(dec_rstc);
	if (ret) {
		dev_err(dev, "failed to deassert reset line\n");
		return ret;
	}
	dev_dbg(dev, "NPU HW reset assert\n");

	/*enable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
	npu_iso_contrl(1);
	dev_dbg(dev, "NPU ISO enable\n");

	/*NPU Power off*/
	if (regulator_is_enabled(vcc_reg)) {
		ret = regulator_disable(vcc_reg);
		if (ret != 0)
			dev_err(dev, "regulator get failed: %d\n", ret);
		else
			dev_dbg(dev, "regulator disable success\n");
	}

	isPowerOn = false;

	dev_dbg(dev, "%s disable ret %d\n", __func__, ret);

	return ret;
}


gceSTATUS
_AdjustParam(
	IN gcsPLATFORM *platform,
	OUT gcsMODULE_PARAMETERS * args
	)
{
	int ret = gcvSTATUS_OK;
	int npu_clock;
	int irqLine;

	struct platform_device *pdev = platform->device;
	struct device *dev = &platform->device->dev;
	struct resource *mmio;

	if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
		return PTR_ERR(clk);
	}

	irqLine = platform_get_irq(pdev, 0);
	if (irqLine < 0)
		return gcvSTATUS_OUT_OF_RESOURCES;
	dev_info(dev, "galcore irq number is %d\n", irqLine);
	args->irqs[gcvCORE_MAJOR] = irqLine;

	mmio = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mmio) {
		args->registerBases[gcvCORE_MAJOR] = mmio->start;
		args->registerSizes[gcvCORE_MAJOR] = resource_size(mmio);
		dev_info(dev, "NPU mmio %llx 0x%lx\n",
			args->registerBases[gcvCORE_MAJOR], args->registerSizes[gcvCORE_MAJOR]);
	} else {
		dev_err(dev, "error: failed to get LPE base at idx\n");
		return -EINVAL;
	}

	if (of_property_read_u32(pdev->dev.of_node, "clock-frequency", &npu_clock))
		npu_clock = 500000000;

	clk_set_rate(clk, npu_clock);
	dev_info(dev, "NPU clock: %ld\n", clk_get_rate(clk));

	return ret;
}

gceSTATUS _GetPower(IN gcsPLATFORM *platform)
{
	int ret = gcvSTATUS_OK;
	struct device *dev = &platform->device->dev;

	vcc_reg = devm_regulator_get(dev, "npu_core");
	if (IS_ERR(vcc_reg)) {
		dev_err(dev, "failed to get regulator\n");
		return gcvSTATUS_FALSE;
	}

	dec_rstc = devm_reset_control_get(dev, "rstc_npu");
	if (IS_ERR(dec_rstc)) {
		dev_err(dev, "failed to retrieve reset controller\n");
		return PTR_ERR(dec_rstc);
	}

	npu_ios_reg_base = of_iomap(dev->of_node, 1);
	if (!npu_ios_reg_base) {
		dev_err(dev, "failed to get ios base\n");
		return gcvSTATUS_OUT_OF_MEMORY;
	}

	chip_version_reg_base = ioremap(CHIP_VERSION_REG, 4);
	if (chip_version_reg_base == NULL) {
		dev_err(dev, "failed to get chip version base\n");
		return gcvSTATUS_OUT_OF_MEMORY;
	}
	version = readl(chip_version_reg_base);
	iounmap(chip_version_reg_base);
	dev_dbg(dev, "version: 0x%x\n", version);

	if(version == 0xa30){
		dev_dbg(dev, "sms reset enable\n");
		npu_sms_reset_reg_base = ioremap(NPU_WORKAROUND_REG, 4);
		if (npu_sms_reset_reg_base == NULL) {
			dev_err(dev, "failed to get workaroud base\n");
			return gcvSTATUS_OUT_OF_MEMORY;
		}
	}

	clk = devm_clk_get(dev, 0);
	if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
		return PTR_ERR(clk);
	}

	ret = clk_prepare(clk);
	if (ret) {
		dev_err(dev, "prepare clock failed\n");
		return gcvSTATUS_OUT_OF_RESOURCES;
	}

	isPowerOn = false;
	dev_info(dev, "NPU get power success\n");

	return gcvSTATUS_OK;
}

gceSTATUS _PutPower(gcsPLATFORM *platform)
{
	struct device *dev = &platform->device->dev;

	if (isPowerOn)
		npu_power_off(platform, 0, 0);

	/*NPU HW clock disable*/
	if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
		return PTR_ERR(clk);
	}

	clk = NULL;
	vcc_reg = NULL;
	dec_rstc = NULL;

	iounmap(npu_ios_reg_base);

	if(version == 0xa30 && npu_sms_reset_reg_base){
		iounmap(npu_sms_reset_reg_base);
	}

	dev_info(dev, "NPU put power success\n");

	return gcvSTATUS_OK;
}

gceSTATUS _SetPower(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu, gctBOOL enable)
{
	if (enable)
		return npu_power_on(platform, devIndex, gpu);
	else
		return npu_power_off(platform, devIndex, gpu);
}

gceSTATUS _SetClock(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu, gctBOOL enable)
{
	int ret = gcvSTATUS_OK;
	struct device *dev = &platform->device->dev;

	if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
		return PTR_ERR(clk);
	}

	if (enable)
		ret = clk_enable(clk);
	else
		clk_disable(clk);

	dev_dbg(dev, "NPU clock %s %s\n", enable?"enable":"disable", (ret == gcvSTATUS_OK)?"success":"fail");

	return ret;
}

gceSTATUS _Reset(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu)
{
	int ret = gcvSTATUS_OK;
	struct device *dev = &platform->device->dev;

	dev_info(dev, "%s devIndex=%d\n", __func__, devIndex);

	/*NPU HW clock disable*/
	clk_disable(clk);
	dev_info(dev, "NPU HW clock disabled\n");

	/*NPU HW Reset assert*/
	ret = reset_control_assert(dec_rstc);
	if (ret) {
		dev_err(dev, "failed to deassert reset line\n");
		return ret;
	}
	dev_info(dev, "NPU HW reset assert\n");

	/*enable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
	npu_iso_contrl(1);
	dev_info(dev, "NPU ISO enable\n");

	/*NPU Power off*/
	if (regulator_is_enabled(vcc_reg)) {
		ret = regulator_disable(vcc_reg);
		if (ret != 0)
			dev_err(dev, "regulator get failed: %d\n", ret);
		else
			dev_info(dev, "regulator disable success\n");
	}

	if (!regulator_is_enabled(vcc_reg)) {
		ret = regulator_enable(vcc_reg);
		if (ret != 0) {
			dev_err(dev, "regulator get failed: %d\n", ret);
			return ret;
		}
		dev_dbg(dev, "regulator enable success\n");
	}

	/*disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
	npu_iso_contrl(0);
	dev_info(dev, "NPU ISO disable\n");

	if(version == 0xa30){
		/*NPU HW reset sms*/
		npu_sms_reset();
		dev_info(dev, "NPU HW reset sms\n");
	}

	/*NPU HW reset deassert*/
	ret = reset_control_deassert(dec_rstc);
	if (ret) {
		dev_err(dev, "failed to deassert reset line\n");
		return ret;
	}
	dev_info(dev, "NPU HW reset deassert\n");

	/*NPU HW clock enable*/
	ret = clk_enable(clk);
	if (ret) {
		dev_err(dev, "enabled clock failed\n");
		return gcvSTATUS_OUT_OF_RESOURCES;
	}
	dev_info(dev, "NPU HW clock enable\n");

	isPowerOn = true;

	return gcvSTATUS_OK;
}

static struct _gcsPLATFORM_OPERATIONS default_ops = {
	.adjustParam   = _AdjustParam,
	.getPower  = _GetPower,
	.putPower = _PutPower,
	.setPower = _SetPower,
	.setClock = _SetClock,
	.reset = _Reset,
};

static struct _gcsPLATFORM default_platform = {

	.name = __FILE__,
	.ops  = &default_ops,
};

static struct platform_device *default_dev;

static const struct of_device_id galcore_dev_match[] = {
	{
	.compatible = "galcore",
	},
	{ },
};

int gckPLATFORM_Init(struct platform_driver *pdrv,
		struct _gcsPLATFORM **platform)
{
	*platform = &default_platform;

	return 0;
}

int gckPLATFORM_Terminate(struct _gcsPLATFORM *platform)
{
	struct device *dev = &platform->device->dev;

	dev_info(dev, "%s\n", __func__);

	if (default_dev) {
		platform_device_unregister(default_dev);
		default_dev = NULL;
	}

	return 0;
}


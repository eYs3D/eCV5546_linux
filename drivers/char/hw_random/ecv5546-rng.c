// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/init.h>
#include <linux/random.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/hw_random.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/reset.h>

#define eCV5546_RNG_DATA		0x00	/* Data register */
#define eCV5546_RNG_CTRL		0x04	/* Control register */

#define eCV5546_RNG_DIV		0	/* 25M / (DIV + 1) */
#define eCV5546_RNG_EN		BIT(10)

#define eCV5546_RNG_HWM		0x7ff0000	/* HIWORD_MASK */

#define to_ecv5546_rng(p)	container_of(p, struct ecv5546_rng, rng)

struct ecv5546_rng {
	struct hwrng rng;
	void __iomem *base;
	struct clk *clk;
	struct reset_control *rstc;
};

static int ecv5546_rng_init(struct hwrng *rng)
{
	struct ecv5546_rng *priv = to_ecv5546_rng(rng);
	int ret;

	ret = clk_prepare_enable(priv->clk);
	if (ret)
		return ret;

	writel(eCV5546_RNG_DIV | eCV5546_RNG_EN | eCV5546_RNG_HWM,
	       priv->base + eCV5546_RNG_CTRL);

	return 0;
}

static void ecv5546_rng_cleanup(struct hwrng *rng)
{
	struct ecv5546_rng *priv = to_ecv5546_rng(rng);

	writel(eCV5546_RNG_DIV | eCV5546_RNG_HWM,
	       priv->base + eCV5546_RNG_CTRL);

	clk_disable_unprepare(priv->clk);
}

static int ecv5546_rng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	struct ecv5546_rng *priv = to_ecv5546_rng(rng);

	pm_runtime_get_sync((struct device *)priv->rng.priv);

	*(u32 *)buf = readl(priv->base + eCV5546_RNG_DATA);

	pm_runtime_mark_last_busy((struct device *)priv->rng.priv);
	pm_runtime_put_sync_autosuspend((struct device *)priv->rng.priv);

	return 4;
}

static int ecv5546_rng_probe(struct platform_device *pdev)
{
	struct ecv5546_rng *priv;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	priv->rstc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(priv->rstc))
		return PTR_ERR(priv->rstc);
	reset_control_deassert(priv->rstc);

	dev_set_drvdata(&pdev->dev, priv);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 100);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

#ifndef CONFIG_PM
	priv->rng.init = ecv5546_rng_init;
	priv->rng.cleanup = ecv5546_rng_cleanup;
#endif
	priv->rng.name = pdev->name;
	priv->rng.read = ecv5546_rng_read;
	priv->rng.priv = (unsigned long)&pdev->dev;

	ret = devm_hwrng_register(&pdev->dev, &priv->rng);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register rng device: %d\n",
			ret);
		pm_runtime_disable(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);
		return ret;
	}

	return 0;
}

static int ecv5546_rng_remove(struct platform_device *pdev)
{
	struct ecv5546_rng *priv = platform_get_drvdata(pdev);

	devm_hwrng_unregister(&pdev->dev, &priv->rng);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM
static int ecv5546_rng_runtime_suspend(struct device *dev)
{
	struct ecv5546_rng *priv = dev_get_drvdata(dev);

	ecv5546_rng_cleanup(&priv->rng);

	return 0;
}

static int ecv5546_rng_runtime_resume(struct device *dev)
{
	struct ecv5546_rng *priv = dev_get_drvdata(dev);

	return ecv5546_rng_init(&priv->rng);
}
#endif

static const struct dev_pm_ops ecv5546_rng_pm_ops = {
	SET_RUNTIME_PM_OPS(ecv5546_rng_runtime_suspend,
			   ecv5546_rng_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

static const struct of_device_id ecv5546_rng_dt_id[] __maybe_unused = {
	{ .compatible = "eys3d,ecv5546-rng",  },
	{},
};
MODULE_DEVICE_TABLE(of, ecv5546_rng_dt_id);

static struct platform_driver ecv5546_rng_driver = {
	.driver = {
		.name		= "ecv5546-rng",
		.pm		= &ecv5546_rng_pm_ops,
		.of_match_table = of_match_ptr(ecv5546_rng_dt_id),
	},
	.probe		= ecv5546_rng_probe,
	.remove		= ecv5546_rng_remove,
};

module_platform_driver(ecv5546_rng_driver);

MODULE_DESCRIPTION("eYs3D eCV5546 Random Number Generator Driver");
MODULE_AUTHOR("qinjian <eys3d@eys3d.com>");
MODULE_LICENSE("GPL v2");

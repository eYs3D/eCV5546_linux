// SPDX-License-Identifier: GPL-2.0

//========================================================================
//		H E A D E R	 F I L E S
//========================================================================
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include "spsoc_util.h"

//========================================================================
//		F U N C T I O N    D E	C L A R	A T I O	N S
//========================================================================
static int eys3d_audio_probe(struct platform_device *pdev);
static int eys3d_audio_remove(struct platform_device *pdev);

//========================================================================
//		G L O B A L	  D A T	A
//========================================================================
static const struct of_device_id eys3d_audio_dt_ids[]	= {
	{ .compatible =	"eys3d,sp7021-audio",	},
	{ .compatible =	"eys3d,audio", },
	{ },
};
MODULE_DEVICE_TABLE(of,	eys3d_audio_dt_ids);

static struct platform_driver eys3d_audio_driver = {
	.probe	= eys3d_audio_probe,
	.remove	= eys3d_audio_remove,
	.driver	= {
		.name		= "eys3d-audio",
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(eys3d_audio_dt_ids),
	},
};
module_platform_driver(eys3d_audio_driver);

static int eys3d_audio_probe(struct platform_device *pdev)
{
	struct resource	*res;
	struct device_node *np = pdev->dev.of_node;
	struct eys3d_audio_base *spauddata;
	int err	= 0;

	dev_info(&pdev->dev, "%s\n", __func__);
	spauddata = devm_kzalloc(&pdev->dev, sizeof(*spauddata), GFP_KERNEL);
	if (!spauddata)
		return -ENOMEM;

	if (!np) {
		dev_err(&pdev->dev, "invalid devicetree	node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(np)) {
		dev_err(&pdev->dev, "devicetree	status is not available\n");
		return -ENODEV;
	}

	//audio	register base
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res)) {
		dev_err(&pdev->dev, "get resource memory from devicetree node 0.\n");
		return PTR_ERR(res);
	}
	spauddata->audio_base =	devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(spauddata->audio_base)) {
		dev_err(&pdev->dev, "mapping resource memory 0.\n");
		return PTR_ERR(spauddata->audio_base);
	}
	//pr_info("start=%zx end=%zx\n", res->start, res->end);
	dev_dbg(&pdev->dev, "audio_base=%p\n", spauddata->audio_base);
	//fifo irq
#if (MMAP_IRQ)
	spauddata->irq = platform_get_irq(pdev, 0);
	if (spauddata->irq <= 0) {
		dev_dbg(spauddata->dev, "get aud fifo irq resource fail\n");
		return -EINVAL;
	}
#endif
	//clock	enable
	spauddata->aud_clocken = devm_clk_get(&pdev->dev, "aud");
	if (IS_ERR(spauddata->aud_clocken)) {
		dev_err(&pdev->dev, "get clock from devicetree node 0.\n");
		return PTR_ERR(spauddata->aud_clocken);
	}
	err = clk_prepare_enable(spauddata->aud_clocken);
	if (err) {
		dev_err(&pdev->dev, "enable clock 0 false.\n");
		return err;
	}

	//reset
	spauddata->clk_rst = devm_reset_control_get(&pdev->dev,	NULL);
	if (IS_ERR(spauddata->clk_rst))	{
		dev_err(&pdev->dev, "aud failed	to retrieve reset controlle\n");
		return PTR_ERR(spauddata->clk_rst);
	}
	err = reset_control_assert(spauddata->clk_rst);
	if (err)
		dev_err(&pdev->dev, "reset assert fail\n");

	err = reset_control_deassert(spauddata->clk_rst);
	if (err)
		dev_err(&pdev->dev, "reset deassert fail\n");

	err = clk_get_rate(spauddata->aud_clocken);
	if (err == 147456000)
		err = clk_set_rate(spauddata->aud_clocken, 135475200);
	else
		err = clk_set_rate(spauddata->aud_clocken, 147456000);
		// 135475200, 147456000, 196608000 Hz,
	if (err) {
		dev_err(&pdev->dev, "plla set rate false.\n");
		return err;
	}

	//err = clk_prepare_enable(spauddata->plla_clocken);
	//if (err) {
	//	dev_err(&pdev->dev, "enable plla false.\n");
	//	return err;
	//}
	platform_set_drvdata(pdev, spauddata);
	eys3d_i2s_register(&pdev->dev);
	eys3d_tdm_register(&pdev->dev);

	return 0;
}

static int eys3d_audio_remove(struct platform_device *pdev)
{
	struct eys3d_audio_base *spauddata = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s	IN\n", __func__);
	//audio_base = NULL;
	snd_soc_unregister_component(&pdev->dev);
	clk_disable(spauddata->aud_clocken);
	//clk_disable(spauddata->plla_clocken);
	return 0;
}

MODULE_AUTHOR("eYs3D Technology Inc.");
MODULE_DESCRIPTION("eYs3D SoC	module");
MODULE_LICENSE("GPL");

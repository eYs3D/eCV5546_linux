// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for VIN
 *
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>

#include <media/v4l2-async.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mc.h>

#include "sp-vin.h"

/*
 * The companion CSI-2 receiver driver (rcar-csi2) is known
 * and we know it has one source pad (pad 0) and four sink
 * pads (pad 1-4). So to translate a pad on the remote
 * CSI-2 receiver to/from the VIN internal channel number simply
 * subtract/add one from the pad/channel number.
 */
#define vin_group_csi_pad_to_channel(pad) ((pad) - 1)
#define vin_group_csi_channel_to_pad(channel) ((channel) + 1)

/*
 * Not all VINs are created equal, master VINs control the
 * routing for other VIN's. We can figure out which VIN is
 * master by looking at a VINs id.
 */
#define vin_group_id_to_master(vin) ((vin) < 4 ? 0 : 4)

#define v4l2_dev_to_vin(d)	container_of(d, struct vin_dev, v4l2_dev)

/* -----------------------------------------------------------------------------
 * Media Controller link notification
 */

/* group lock should be held when calling this function. */
static int vin_group_link_notify(struct media_link *link, u32 flags,
				  unsigned int notification)
{
	struct vin_group *group = container_of(link->graph_obj.mdev,
					struct vin_group, mdev);
	struct media_entity *entity;
	struct video_device *vdev;
	struct vin_dev *vin = NULL;
	int ret;

	ret = v4l2_pipeline_link_notify(link, flags, notification);
	if (ret)
		return ret;

	/* Only care about the VIN nodes. */
	if (!is_media_entity_v4l2_video_device(link->sink->entity))
		return 0;

	/*
	 * Don't allow link changes if any entity in the graph is
	 * streaming.
	 */
	media_device_for_each_entity(entity, &group->mdev)
		if (media_entity_is_streaming(entity))
			return -EBUSY;

	mutex_lock(&group->lock);

	/* Show the link information. */
	vdev = media_entity_to_video_device(link->sink->entity);
	vin = container_of(vdev, struct vin_dev, vdev);
	dev_dbg(vin->dev, "%s: VIN device is VIN%02d\n", __func__, vin->id);
	dev_dbg(vin->dev, "link %s -> %s, flags: %d\n",
		link->sink->entity->name, link->source->entity->name, flags);

	mutex_unlock(&group->lock);

	return ret;
}

static const struct media_device_ops vin_media_ops = {
	.link_notify = vin_group_link_notify,
};

/* -----------------------------------------------------------------------------
 * Gen3 CSI2 Group Allocator
 */

/* FIXME:  This should if we find a system that supports more
 * than one group for the whole system be replaced with a linked
 * list of groups. And eventually all of this should be replaced
 * with a global device allocator API.
 *
 * But for now this works as on all supported systems there will
 * be only one group for all instances.
 */

static DEFINE_MUTEX(vin_group_lock);
static struct vin_group *vin_group_data;

static void vin_group_cleanup(struct vin_group *group)
{
	media_device_cleanup(&group->mdev);
	mutex_destroy(&group->lock);
}

static int vin_group_init(struct vin_group *group, struct vin_dev *vin)
{
	struct media_device *mdev = &group->mdev;
	const struct of_device_id *match;
	struct device_node *np;

	mutex_init(&group->lock);

	/* Count number of VINs in the system */
	group->count = 0;
	for_each_matching_node(np, vin->dev->driver->of_match_table)
		if (of_device_is_available(np))
			group->count++;

	vin_dbg(vin, "found %u enabled VIN's in DT", group->count);

	mdev->dev = vin->dev;
	mdev->ops = &vin_media_ops;

	match = of_match_node(vin->dev->driver->of_match_table,
			      vin->dev->of_node);

	strscpy(mdev->driver_name, KBUILD_MODNAME, sizeof(mdev->driver_name));
	strscpy(mdev->model, match->compatible, sizeof(mdev->model));
	snprintf(mdev->bus_info, sizeof(mdev->bus_info), "platform:%s",
		 dev_name(mdev->dev));

	media_device_init(mdev);

	return 0;
}

static void vin_group_release(struct kref *kref)
{
	struct vin_group *group =
		container_of(kref, struct vin_group, refcount);

	mutex_lock(&vin_group_lock);

	vin_group_data = NULL;

	vin_group_cleanup(group);

	kfree(group);

	mutex_unlock(&vin_group_lock);
}

static int vin_group_get(struct vin_dev *vin)
{
	struct vin_group *group;
	u32 id;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	/* Make sure VIN id is present and sane */
	ret = of_property_read_u32(vin->dev->of_node, "eys3d,id", &id);
	if (ret) {
		vin_err(vin, "%pOF: No eys3d,id property found\n",
			vin->dev->of_node);
		return -EINVAL;
	}

	if (id >= VIN_MAX_NUM) {
		vin_err(vin, "%pOF: Invalid eys3d,id '%u'\n",
			vin->dev->of_node, id);
		return -EINVAL;
	}

	/* Join or create a VIN group */
	mutex_lock(&vin_group_lock);
	if (vin_group_data) {
		group = vin_group_data;
		kref_get(&group->refcount);
	} else {
		group = kzalloc(sizeof(*group), GFP_KERNEL);
		if (!group) {
			ret = -ENOMEM;
			goto err_group;
		}

		ret = vin_group_init(group, vin);
		if (ret) {
			kfree(group);
			vin_err(vin, "Failed to initialize group\n");
			goto err_group;
		}

		kref_init(&group->refcount);

		vin_group_data = group;
	}
	mutex_unlock(&vin_group_lock);

	/* Add VIN to group */
	mutex_lock(&group->lock);

	if (group->vin[id]) {
		vin_err(vin, "Duplicate eys3d,id property value %u\n", id);
		mutex_unlock(&group->lock);
		kref_put(&group->refcount, vin_group_release);
		return -EINVAL;
	}

	group->vin[id] = vin;

	vin->id = id;
	vin->group = group;
	vin->v4l2_dev.mdev = &group->mdev;

	mutex_unlock(&group->lock);

	/* Reserved memory initialization */
	ret = of_reserved_mem_device_init(vin->dev);
	if (ret) {
		dev_err(vin->dev, "Could not get reserved memory!\n");
		goto err_group;
	}

	ret = dma_set_coherent_mask(vin->dev, DMA_BIT_MASK(32));
	if (ret)
		dev_warn(vin->dev, "32-bit consistent DMA enable failed\n");

	dev_dbg(vin->dev, "Reserved memory initialization done\n");

	return 0;
err_group:
	mutex_unlock(&vin_group_lock);
	return ret;
}

static void vin_group_put(struct vin_dev *vin)
{
	struct vin_group *group = vin->group;

	mutex_lock(&group->lock);

	vin->group = NULL;
	vin->v4l2_dev.mdev = NULL;

	if (WARN_ON(group->vin[vin->id] != vin))
		goto out;

	group->vin[vin->id] = NULL;
out:
	mutex_unlock(&group->lock);

	kref_put(&group->refcount, vin_group_release);
}

/* -----------------------------------------------------------------------------
 * Controls
 */

static int vin_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vin_dev *vin =
		container_of(ctrl->handler, struct vin_dev, ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_ALPHA_COMPONENT:
		vin_set_alpha(vin, ctrl->val);
		break;
	}

	return 0;
}

static const struct v4l2_ctrl_ops vin_ctrl_ops = {
	.s_ctrl = vin_s_ctrl,
};

/* -----------------------------------------------------------------------------
 * Async notifier
 */

/* -----------------------------------------------------------------------------
 * Legacy async notifier
 */

static int vin_notify_complete(struct v4l2_async_notifier *notifier)
{
	struct vin_dev *vin = v4l2_dev_to_vin(notifier->v4l2_dev);
	const struct vin_group_route *route;
	unsigned int i;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	ret = media_device_register(&vin->group->mdev);
	if (ret)
		return ret;

	ret = v4l2_device_register_subdev_nodes(&vin->v4l2_dev);
	if (ret) {
		vin_err(vin, "Failed to register subdev nodes\n");
		return ret;
	}

	/* Register all video nodes for the group. */
	for (i = 0; i < VIN_MAX_NUM; i++) {
		if (vin->group->vin[i] &&
		    !video_is_registered(&vin->group->vin[i]->vdev)) {
			ret = vin_v4l2_register(vin->group->vin[i]);
			if (ret)
				return ret;
		}
	}

	/* Create all media device links between VINs and CSI-2's. */
	mutex_lock(&vin->group->lock);
	for (route = vin->info->routes; route->mask; route++) {
		struct media_pad *source_pad, *sink_pad;
		struct media_entity *source, *sink;
		unsigned int source_idx;

		dev_dbg(vin->dev, "csi: %d, channel: %d, vin: %d, mask: 0x%08x\n",
			route->csi, route->channel, route->vin, route->mask);
		dev_dbg(vin->dev, "vin->group->vin[route->vin]: 0x%p\n", vin->group->vin[route->vin]);

		/* Check that VIN is part of the group. */
		if (!vin->group->vin[route->vin])
			continue;

		/* Check that VIN' master is part of the group. */
		//if (!vin->group->vin[vin_group_id_to_master(route->vin)])
		//	continue;

		/* Check that CSI-2 is part of the group. */
		if (!vin->group->csi[route->csi].subdev)
			continue;

		source = &vin->group->csi[route->csi].subdev->entity;
		source_idx = vin_group_csi_channel_to_pad(route->channel);
		source_pad = &source->pads[source_idx];

		dev_dbg(vin->dev, "Bound %s pad: %d\n",
			vin->group->csi[route->csi].subdev->name, source_idx);

		sink = &vin->group->vin[route->vin]->vdev.entity;
		sink_pad = &sink->pads[0];

		/* Skip if link already exists. */
		if (media_entity_find_link(source_pad, sink_pad))
			continue;

		ret = media_create_pad_link(source, source_idx, sink, 0,
						MEDIA_LNK_FL_ENABLED);
		if (ret) {
			vin_err(vin, "Error adding link from %s to %s\n",
				source->name, sink->name);
			break;
		}

		/* Inherit the controls from sub-device */
		ret = v4l2_ctrl_add_handler(&vin->group->vin[route->vin]->ctrl_handler,
				vin->group->csi[route->csi].subdev->ctrl_handler, NULL, true);
		if (ret < 0) {
			dev_err(vin->dev, "Failed to inherit controls\n");
			v4l2_ctrl_handler_free(&vin->group->vin[route->vin]->ctrl_handler);
			return ret;
		}
	}
	mutex_unlock(&vin->group->lock);

	/* Configure all pipeline with default format. */
	for (i = 0; i < VIN_MAX_NUM; i++) {
		if (vin->group->vin[i]) {
			ret = vin_v4l2_formats_init(vin->group->vin[i]);
			if (ret) {
				dev_err(vin->dev, "No supported mediabus format found\n");
				return ret;
			}

			ret = vin_v4l2_framesizes_init(vin->group->vin[i]);
			if (ret) {
				dev_err(vin->dev, "Could not initialize framesizes\n");
				return ret;
			}

			ret = vin_v4l2_set_default_fmt(vin->group->vin[i]);
			if (ret) {
				dev_err(vin->dev, "Could not set default format\n");
				return ret;
			}
		}
	}

	return ret;
}

static void vin_notify_unbind(struct v4l2_async_notifier *notifier,
				     struct v4l2_subdev *subdev,
				     struct v4l2_async_connection *asc)
{
	struct vin_dev *vin = v4l2_dev_to_vin(notifier->v4l2_dev);
	unsigned int i;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	for (i = 0; i < VIN_MAX_NUM; i++)
		if (vin->group->vin[i])
			vin_v4l2_unregister(vin->group->vin[i]);

	mutex_lock(&vin->group->lock);

	for (i = 0; i < VIN_CSI_MAX; i++) {
		if (vin->group->csi[i].fwnode != asc->match.fwnode)
			continue;
		vin->group->csi[i].subdev = NULL;
		vin_dbg(vin, "Unbind CSI-2 %s from slot %u\n", subdev->name, i);
		break;
	}

	mutex_unlock(&vin->group->lock);

	media_device_unregister(&vin->group->mdev);
}

static int vin_notify_bound(struct v4l2_async_notifier *notifier,
				   struct v4l2_subdev *subdev,
				   struct v4l2_async_connection *asc)
{
	struct vin_dev *vin = v4l2_dev_to_vin(notifier->v4l2_dev);
	unsigned int i;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	mutex_lock(&vin->group->lock);

	for (i = 0; i < VIN_CSI_MAX; i++) {
		if (vin->group->csi[i].fwnode != asc->match.fwnode)
			continue;
		vin->group->csi[i].subdev = subdev;
		vin_dbg(vin, "Bound CSI-2 %s to slot %u\n", subdev->name, i);
		break;
	}

	mutex_unlock(&vin->group->lock);

	return 0;
}

static const struct v4l2_async_notifier_operations vin_notify_ops = {
	.bound = vin_notify_bound,
	.unbind = vin_notify_unbind,
	.complete = vin_notify_complete,
};

static int vin_parse_of_endpoint(struct device *dev,
				     struct v4l2_fwnode_endpoint *vep,
				     struct v4l2_async_connection *asc)
{
	struct vin_dev *vin = dev_get_drvdata(dev);
	int ret = 0;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	dev_dbg(vin->dev, "vep->base.port: %d, vep->base.id: %d\n", vep->base.port, vep->base.id);

	if (vep->base.port != 1 || vep->base.id >= VIN_CSI_MAX)
		return -EINVAL;

	if (!of_device_is_available(to_of_node(asc->match.fwnode))) {
		vin_dbg(vin, "OF device %pOF disabled, ignoring\n",
			to_of_node(asc->match.fwnode));
		return -ENOTCONN;
	}

	mutex_lock(&vin->group->lock);

	if (vin->group->csi[vep->base.id].fwnode) {
		vin_dbg(vin, "OF device %pOF already handled\n",
			to_of_node(asc->match.fwnode));
		ret = -ENOTCONN;
		goto out;
	}

	vin->group->csi[vep->base.id].fwnode = asc->match.fwnode;

	vin_dbg(vin, "Add group OF device %pOF to slot %u\n",
		to_of_node(asc->match.fwnode), vep->base.id);
out:
	mutex_unlock(&vin->group->lock);

	return ret;
}

static int vin_group_parse_of(struct vin_dev *vin, unsigned int port,
			       unsigned int id)
{
	struct fwnode_handle *ep, *fwnode;
	struct v4l2_fwnode_endpoint vep = {
		.bus_type = V4L2_MBUS_CSI2_DPHY,
	};
	struct v4l2_async_connection *asc;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	ep = fwnode_graph_get_endpoint_by_id(dev_fwnode(vin->dev), port, id, 0);
	if (!ep)
		return 0;

	fwnode = fwnode_graph_get_remote_endpoint(ep);
	ret = v4l2_fwnode_endpoint_parse(ep, &vep);
	fwnode_handle_put(ep);
	if (ret) {
		vin_err(vin, "Failed to parse %pOF\n", to_of_node(fwnode));
		ret = -EINVAL;
		goto out;
	}
#if defined(MIPI_CSI_DYN_REG)
	/* Check if the CSI2 device has a driver bound */
	struct fwnode_handle *camera_ep, *camera_fwnode;
	struct device *camera_dev, *csi_dev;

	csi_dev = fwnode_get_next_parent_dev(fwnode);
	if (csi_dev == NULL) {
		vin_err(vin, "get csi %p device failed\n",
				to_of_node(fwnode));
		goto out;
	}
	camera_ep = fwnode_graph_get_endpoint_by_id(dev_fwnode(csi_dev), 0, 0, 0);
	put_device(csi_dev);
	if (!camera_ep) {
		vin_err(vin, "csi camera endpoint no found\n");
		goto out;
	}
	camera_fwnode = fwnode_graph_get_remote_endpoint(camera_ep);
	fwnode_handle_put(camera_ep);
	if (!camera_fwnode) {
		vin_err(vin, "camera endpoint no found\n");
		goto out;
	}
	camera_dev = fwnode_get_next_parent_dev(camera_fwnode);
	fwnode_handle_put(camera_fwnode);
	if (camera_dev == NULL) {
		vin_err(vin, "get camera device failed\n");
		goto out;
	}
	if (camera_dev->driver == NULL) {
		vin_err(vin, "OF device %pOF probe failed. Remove VIN%d from group\n",
			to_of_node(fwnode), vin->id);
		vin->group->vin[vin->id] = NULL;
		ret = -ENOTCONN;
		put_device(camera_dev);
		goto out;
	}
	put_device(camera_dev);
#endif

	asc = v4l2_async_nf_add_fwnode(&vin->group->notifier, fwnode,
				    struct v4l2_async_connection);
	if (IS_ERR(asc)) {
		ret = PTR_ERR(asc);
		goto out;
	}
	ret = vin_parse_of_endpoint(vin->dev, &vep, asc);
	if (ret)
		goto out;
out:
	fwnode_handle_put(fwnode);

	return ret;
}

static int vin_parse_of_graph(struct vin_dev *vin)
{
	unsigned int count = 0, vin_mask = 0;
	unsigned int i, id;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	mutex_lock(&vin->group->lock);

	/* If not all VIN's are registered don't register the notifier. */
	for (i = 0; i < VIN_MAX_NUM; i++) {
		if (vin->group->vin[i]) {
			count++;
			vin_mask |= BIT(i);
		}
	}

	dev_dbg(vin->dev, "vin->group->count: %d, count: %d\n", vin->group->count, count);

	if (vin->group->count != count) {
		mutex_unlock(&vin->group->lock);
		return 0;
	}

	mutex_unlock(&vin->group->lock);

	v4l2_async_nf_init(&vin->group->notifier, &vin->v4l2_dev);

	/*
	 * Have all VIN's look for CSI-2 subdevices. Some subdevices will
	 * overlap but the parser function can handle it, so each subdevice
	 * will only be registered once with the group notifier.
	 */
	for (i = 0; i < VIN_MAX_NUM; i++) {
		if (!(vin_mask & BIT(i)))
			continue;

		for (id = 0; id < VIN_CSI_MAX; id++) {
			if (vin->group->csi[id].fwnode)
				continue;
			if (vin->group->vin[i] != NULL) {
				ret = vin_group_parse_of(vin->group->vin[i], 1, id);
				if (ret)
					vin_err(vin, "vin->group->vin[%d] ret %d\n", i, ret);
			}
		}
	}

	if (list_empty(&vin->group->notifier.waiting_list))
		return 0;

	vin->group->notifier.ops = &vin_notify_ops;
	ret = v4l2_async_nf_register(&vin->group->notifier);
	if (ret < 0) {
		vin_err(vin, "Notifier registration failed\n");
		v4l2_async_nf_cleanup(&vin->group->notifier);
		return ret;
	}

	return 0;
}

static int vin_init(struct vin_dev *vin)
{
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	vin->pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&vin->vdev.entity, 1, &vin->pad);
	if (ret)
		return ret;

	ret = vin_group_get(vin);
	if (ret)
		return ret;

	/* Initialize v4l2 control and add a control */
	ret = v4l2_ctrl_handler_init(&vin->ctrl_handler, 1);
	if (ret < 0)
		return ret;

	v4l2_ctrl_new_std(&vin->ctrl_handler, &vin_ctrl_ops,
			  V4L2_CID_ALPHA_COMPONENT, 0, 255, 1, 255);

	if (vin->ctrl_handler.error) {
		ret = vin->ctrl_handler.error;
		v4l2_ctrl_handler_free(&vin->ctrl_handler);
		return ret;
	}

	vin->vdev.ctrl_handler = &vin->ctrl_handler;

	ret = vin_parse_of_graph(vin);
	if (ret)
		vin_group_put(vin);

	return ret;
}

/* -----------------------------------------------------------------------------
 * Group async notifier
 */

static int vin_group_notify_complete(struct v4l2_async_notifier *notifier)
{
	struct vin_dev *vin = v4l2_dev_to_vin(notifier->v4l2_dev);
	const struct vin_group_route *route;
	unsigned int i;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	ret = media_device_register(&vin->group->mdev);
	if (ret)
		return ret;

	ret = v4l2_device_register_subdev_nodes(&vin->v4l2_dev);
	if (ret) {
		vin_err(vin, "Failed to register subdev nodes\n");
		return ret;
	}

	/* Register all video nodes for the group. */
	for (i = 0; i < VIN_MAX_NUM; i++) {
		if (vin->group->vin[i] &&
		    !video_is_registered(&vin->group->vin[i]->vdev)) {
			ret = vin_v4l2_register(vin->group->vin[i]);
			if (ret)
				return ret;
		}
	}

	/* Create all media device links between VINs and CSI-2's. */
	mutex_lock(&vin->group->lock);
	for (route = vin->info->routes; route->mask; route++) {
		struct media_pad *source_pad, *sink_pad;
		struct media_entity *source, *sink;
		unsigned int source_idx;

		dev_dbg(vin->dev, "csi: %d, channel: %d, vin: %d, mask: %d\n",
			route->csi, route->channel, route->vin, route->mask);
		dev_dbg(vin->dev, "vin->group->vin[route->vin]: 0x%p\n",
			vin->group->vin[route->vin]);

		/* Check that VIN is part of the group. */
		if (!vin->group->vin[route->vin])
			continue;

		/* Check that VIN' master is part of the group. */
		//if (!vin->group->vin[vin_group_id_to_master(route->vin)])
		//	continue;

		/* Check that CSI-2 is part of the group. */
		if (!vin->group->csi[route->csi].subdev)
			continue;

		source = &vin->group->csi[route->csi].subdev->entity;
		source_idx = vin_group_csi_channel_to_pad(route->channel);
		source_pad = &source->pads[source_idx];

		dev_dbg(vin->dev, "Bound %s pad: %d\n",
			vin->group->csi[route->csi].subdev->name, source_idx);

		sink = &vin->group->vin[route->vin]->vdev.entity;
		sink_pad = &sink->pads[0];

		/* Skip if link already exists. */
		if (media_entity_find_link(source_pad, sink_pad))
			continue;

#if defined(MC_MODE_DEFAULT)
		ret = media_create_pad_link(source, source_idx, sink, 0,
						MEDIA_LNK_FL_ENABLED);
#else
		ret = media_create_pad_link(source, source_idx, sink, 0, 0);
#endif
		if (ret) {
			vin_err(vin, "Error adding link from %s to %s\n",
				source->name, sink->name);
			break;
		}
	}
	mutex_unlock(&vin->group->lock);

	return ret;
}

static void vin_group_notify_unbind(struct v4l2_async_notifier *notifier,
				     struct v4l2_subdev *subdev,
				     struct v4l2_async_connection *asc)
{
	struct vin_dev *vin = v4l2_dev_to_vin(notifier->v4l2_dev);
	unsigned int i;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	for (i = 0; i < VIN_MAX_NUM; i++)
		if (vin->group->vin[i])
			vin_v4l2_unregister(vin->group->vin[i]);

	mutex_lock(&vin->group->lock);

	for (i = 0; i < VIN_CSI_MAX; i++) {
		if (vin->group->csi[i].fwnode != asc->match.fwnode)
			continue;
		vin->group->csi[i].subdev = NULL;
		vin_dbg(vin, "Unbind CSI-2 %s from slot %u\n", subdev->name, i);
		break;
	}

	mutex_unlock(&vin->group->lock);

	media_device_unregister(&vin->group->mdev);
}

static int vin_group_notify_bound(struct v4l2_async_notifier *notifier,
				   struct v4l2_subdev *subdev,
				   struct v4l2_async_connection *asc)
{
	struct vin_dev *vin = v4l2_dev_to_vin(notifier->v4l2_dev);
	unsigned int i;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	mutex_lock(&vin->group->lock);

	for (i = 0; i < VIN_CSI_MAX; i++) {
		if (vin->group->csi[i].fwnode != asc->match.fwnode)
			continue;
		vin->group->csi[i].subdev = subdev;
		vin_dbg(vin, "Bound CSI-2 %s to slot %u\n", subdev->name, i);
		break;
	}

	mutex_unlock(&vin->group->lock);

	return 0;
}

static const struct v4l2_async_notifier_operations vin_group_notify_ops = {
	.bound = vin_group_notify_bound,
	.unbind = vin_group_notify_unbind,
	.complete = vin_group_notify_complete,
};

// static int vin_mc_parse_of_endpoint(struct device *dev,
//				     struct v4l2_fwnode_endpoint *vep,
//				     struct v4l2_async_connection *asc)
// {
//	struct vin_dev *vin = dev_get_drvdata(dev);
//	int ret = 0;

//	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

//	if (vep->base.port != 1 || vep->base.id >= VIN_CSI_MAX)
//		return -EINVAL;

//	dev_dbg(vin->dev, "vep->base.port: %d, vep->base.id: %d\n", vep->base.port, vep->base.id);

//	if (!of_device_is_available(to_of_node(asc->match.fwnode))) {
//		vin_dbg(vin, "OF device %pOF disabled, ignoring\n",
//			to_of_node(asc->match.fwnode));
//		return -ENOTCONN;
//	}

//	mutex_lock(&vin->group->lock);

//	if (vin->group->csi[vep->base.id].fwnode) {
//		vin_dbg(vin, "OF device %pOF already handled\n",
//			to_of_node(asc->match.fwnode));
//		ret = -ENOTCONN;
//		goto out;
//	}

//	vin->group->csi[vep->base.id].fwnode = asc->match.fwnode;

//	vin_dbg(vin, "Add group OF device %pOF to slot %u\n",
//		to_of_node(asc->match.fwnode), vep->base.id);
// out:
//	mutex_unlock(&vin->group->lock);

//	return ret;
// }

static int vin_mc_parse_of_graph(struct vin_dev *vin)
{
	unsigned int count = 0, vin_mask = 0;
	unsigned int i;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	mutex_lock(&vin->group->lock);

	/* If not all VIN's are registered don't register the notifier. */
	for (i = 0; i < VIN_MAX_NUM; i++) {
		if (vin->group->vin[i]) {
			count++;
			vin_mask |= BIT(i);
		}
	}

	dev_dbg(vin->dev, "vin->group->count: %d, count: %d\n", vin->group->count, count);

	if (vin->group->count != count) {
		mutex_unlock(&vin->group->lock);
		return 0;
	}

	mutex_unlock(&vin->group->lock);

	v4l2_async_nf_init(&vin->group->notifier, &vin->v4l2_dev);

	/*
	 * Have all VIN's look for CSI-2 subdevices. Some subdevices will
	 * overlap but the parser function can handle it, so each subdevice
	 * will only be registered once with the group notifier.
	 */
	for (i = 0; i < VIN_MAX_NUM; i++) {
		if (!(vin_mask & BIT(i)))
			continue;

		ret = vin_group_parse_of(vin->group->vin[i], 1, i);
		if (ret)
			return ret;
	}

	if (list_empty(&vin->group->notifier.waiting_list))
		return 0;

	vin->group->notifier.ops = &vin_group_notify_ops;
	ret = v4l2_async_nf_register(&vin->group->notifier);
	if (ret < 0) {
		vin_err(vin, "Notifier registration failed\n");
		v4l2_async_nf_cleanup(&vin->group->notifier);
		return ret;
	}

	return 0;
}

static int vin_mc_init(struct vin_dev *vin)
{
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	vin->pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&vin->vdev.entity, 1, &vin->pad);
	if (ret)
		return ret;

	ret = vin_group_get(vin);
	if (ret)
		return ret;

	ret = vin_mc_parse_of_graph(vin);
	if (ret)
		vin_group_put(vin);

	ret = v4l2_ctrl_handler_init(&vin->ctrl_handler, 1);
	if (ret < 0)
		return ret;

	v4l2_ctrl_new_std(&vin->ctrl_handler, &vin_ctrl_ops,
			  V4L2_CID_ALPHA_COMPONENT, 0, 255, 1, 255);

	if (vin->ctrl_handler.error) {
		ret = vin->ctrl_handler.error;
		v4l2_ctrl_handler_free(&vin->ctrl_handler);
		return ret;
	}

	vin->vdev.ctrl_handler = &vin->ctrl_handler;

	return ret;
}

/* -----------------------------------------------------------------------------
 * Platform Device Driver
 */

/* The field description of the 'mask' flag.
 * Bit 03~00: Output channel of CSI-2
 * Bit 11~04: VIN port
 */
static const struct vin_group_route sp_info_ecv5546_routes[] = {
#if defined(MIPI_CSI_4VC)
	/* 5 pipelines, MIPI-CSI2 has 4 VCs */
	{ .csi = VIN_CSI0, .channel = 0, .vin = 0, .mask = BIT(0) | BIT(4) },
	{ .csi = VIN_CSI0, .channel = 1, .vin = 1, .mask = BIT(1) | BIT(4) },
	{ .csi = VIN_CSI1, .channel = 0, .vin = 2, .mask = BIT(0) | BIT(5) },
	{ .csi = VIN_CSI1, .channel = 1, .vin = 3, .mask = BIT(1) | BIT(5) },
	{ .csi = VIN_CSI2, .channel = 0, .vin = 4, .mask = BIT(0) | BIT(6) },
	{ .csi = VIN_CSI2, .channel = 1, .vin = 5, .mask = BIT(1) | BIT(6) },
	{ .csi = VIN_CSI2, .channel = 2, .vin = 6, .mask = BIT(0) | BIT(6) },
	{ .csi = VIN_CSI2, .channel = 3, .vin = 7, .mask = BIT(1) | BIT(6) },
	{ .csi = VIN_CSI4, .channel = 0, .vin = 8, .mask = BIT(0) | BIT(8) },
	{ .csi = VIN_CSI4, .channel = 1, .vin = 9, .mask = BIT(1) | BIT(8) },
	{ .csi = VIN_CSI5, .channel = 0, .vin = 10, .mask = BIT(0) | BIT(9) },
	{ .csi = VIN_CSI5, .channel = 1, .vin = 11, .mask = BIT(1) | BIT(9) },
	{ .csi = VIN_CSI5, .channel = 2, .vin = 12, .mask = BIT(2) | BIT(9) },
	{ .csi = VIN_CSI5, .channel = 3, .vin = 13, .mask = BIT(3) | BIT(9) },
	{ /* Sentinel */ }
#else
	/* 6 pipelines, MIPI-CSI2 has 2 VCs */
	{ .csi = VIN_CSI0, .channel = 0, .vin = 0, .mask = BIT(0) | BIT(4) },
	{ .csi = VIN_CSI0, .channel = 1, .vin = 1, .mask = BIT(1) | BIT(4) },
	{ .csi = VIN_CSI1, .channel = 0, .vin = 2, .mask = BIT(0) | BIT(5) },
	{ .csi = VIN_CSI1, .channel = 1, .vin = 3, .mask = BIT(1) | BIT(5) },
	{ .csi = VIN_CSI2, .channel = 0, .vin = 4, .mask = BIT(0) | BIT(6) },
	{ .csi = VIN_CSI2, .channel = 1, .vin = 5, .mask = BIT(1) | BIT(6) },
	{ .csi = VIN_CSI3, .channel = 0, .vin = 6, .mask = BIT(0) | BIT(7) },
	{ .csi = VIN_CSI3, .channel = 1, .vin = 7, .mask = BIT(1) | BIT(7) },
	{ .csi = VIN_CSI4, .channel = 0, .vin = 8, .mask = BIT(0) | BIT(8) },
	{ .csi = VIN_CSI4, .channel = 1, .vin = 9, .mask = BIT(1) | BIT(8) },
	{ .csi = VIN_CSI5, .channel = 0, .vin = 10, .mask = BIT(0) | BIT(9) },
	{ .csi = VIN_CSI5, .channel = 1, .vin = 11, .mask = BIT(1) | BIT(9) },
	{ .csi = VIN_CSI5, .channel = 2, .vin = 12, .mask = BIT(2) | BIT(9) },
	{ .csi = VIN_CSI5, .channel = 3, .vin = 13, .mask = BIT(3) | BIT(9) },
	{ /* Sentinel */ }
#endif
};

static const struct vin_info sp_info_ecv5546 = {
	.model = eCV5546,
	.use_mc = false,
	.nv12 = false,
	.max_width = 4608, // for imx708(12mp sensor 4608x2592)
	.max_height = 4096,
	.routes = sp_info_ecv5546_routes,
};

static const struct of_device_id sp_vin_of_id_table[] = {
	{
		.compatible = "eys3d,ecv5546-vin",
		.data = &sp_info_ecv5546,
	},
	{ /* Sentinel */ },
};
MODULE_DEVICE_TABLE(of, sp_vin_of_id_table);

static int sp_vin_probe(struct platform_device *pdev)
{
	struct vin_dev *vin;
	int fs_irq, fe_irq, ret;

	dev_dbg(&pdev->dev, "%s, %d\n", __func__, __LINE__);

	vin = devm_kzalloc(&pdev->dev, sizeof(*vin), GFP_KERNEL);
	if (!vin)
		return -ENOMEM;

	vin->dev = &pdev->dev;
	vin->info = of_device_get_match_data(&pdev->dev);
	vin->alpha = 0xff;

	dev_dbg(&pdev->dev, "vin->info->model: %d\n", vin->info->model);

	vin->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(vin->base))
		return PTR_ERR(vin->base);

	dev_dbg(&pdev->dev, "vin->base: 0x%p\n", vin->base);

	vin->clk = devm_clk_get(&pdev->dev, "clk_csiiw");
	if (IS_ERR(vin->clk))
		return PTR_ERR(vin->clk);

	vin->rstc = devm_reset_control_get(&pdev->dev, "rstc_csiiw");
	if (IS_ERR(vin->rstc))
		return PTR_ERR(vin->rstc);

	ret = clk_prepare_enable(vin->clk);
	if (ret) {
		dev_err(vin->dev, "Failed to enable clock!\n");
		return ret;
	}

	ret = reset_control_deassert(vin->rstc);
	if (ret) {
		dev_err(vin->dev, "Failed to deassert reset controller!\n");
		return ret;
	}

	fs_irq = platform_get_irq_byname(pdev, "fs_irq");
	if (fs_irq < 0)
		return fs_irq;

	fe_irq = platform_get_irq_byname(pdev, "fe_irq");
	if (fe_irq < 0)
		return fe_irq;

	ret = vin_dma_register(vin, fs_irq, fe_irq);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, vin);

	if (vin->info->use_mc)
		ret = vin_mc_init(vin);
	else
		ret = vin_init(vin);
	if (ret)
		goto error_dma_unregister;

#ifdef CONFIG_PM_RUNTIME_MIPICSI
	pm_suspend_ignore_children(&pdev->dev, true);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 5000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#endif

	dev_info(&pdev->dev, "SP VIN driver probed\n");

	return 0;

error_dma_unregister:
	vin_dma_unregister(vin);

	return ret;
}

static int sp_vin_remove(struct platform_device *pdev)
{
	struct vin_dev *vin = platform_get_drvdata(pdev);

#ifdef CONFIG_PM_RUNTIME_MIPICSI
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
#endif

	vin_v4l2_unregister(vin);

	v4l2_async_nf_unregister(&vin->group->notifier);
	v4l2_async_nf_cleanup(&vin->group->notifier);
	vin_group_put(vin);

	v4l2_ctrl_handler_free(&vin->ctrl_handler);

	vin_dma_unregister(vin);

	return 0;
}

static int sp_vin_suspend(struct device *dev)
{
	struct vin_dev *vin = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "%s, %d\n", __func__, __LINE__);

	clk_disable_unprepare(vin->clk);

	ret = reset_control_assert(vin->rstc);
	if (ret) {
		dev_err(vin->dev, "Failed to deassert reset controller!\n");
		return ret;
	}

	return 0;
}

static int sp_vin_resume(struct device *dev)
{
	struct vin_dev *vin = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "%s, %d\n", __func__, __LINE__);

	ret = reset_control_deassert(vin->rstc);
	if (ret) {
		dev_err(vin->dev, "Failed to deassert reset controller!\n");
		return ret;
	}

	ret = clk_prepare_enable(vin->clk);
	if (ret) {
		dev_err(vin->dev, "Failed to enable clock!\n");
		return ret;
	}

	vin_dma_init(vin);

	return 0;
}

#ifdef CONFIG_PM_RUNTIME_MIPICSI
static int sp_vin_runtime_suspend(struct device *dev)
{
	struct vin_dev *vin = dev_get_drvdata(dev);

	clk_disable_unprepare(vin->clk);

	return 0;
}

static int sp_vin_runtime_resume(struct device *dev)
{
	struct vin_dev *vin = dev_get_drvdata(dev);
	int ret;

	ret = clk_prepare_enable(vin->clk);
	if (ret) {
		dev_err(vin->dev, "Failed to enable clock!\n");
		return ret;
	}

	vin_dma_init(vin);

	return 0;
}
#endif

static const struct dev_pm_ops sp_vin_pm_ops = {
	.suspend = sp_vin_suspend,
	.resume = sp_vin_resume,
#ifdef CONFIG_PM_RUNTIME_MIPICSI
	.runtime_suspend = sp_vin_runtime_suspend,
	.runtime_resume = sp_vin_runtime_resume,
#endif
};

static struct platform_driver sp_vin_driver = {
	.driver = {
		.name = "sp-vin",
		.of_match_table = sp_vin_of_id_table,
		.pm = &sp_vin_pm_ops,
	},
	.probe = sp_vin_probe,
	.remove = sp_vin_remove,
};

module_platform_driver(sp_vin_driver);

MODULE_AUTHOR("Cheng Chung Ho <eys3d@eys3d.com>");
MODULE_DESCRIPTION("eYs3D VIN camera host driver");
MODULE_LICENSE("GPL");

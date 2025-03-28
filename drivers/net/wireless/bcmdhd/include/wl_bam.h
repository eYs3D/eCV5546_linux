/*
 * Bad AP Manager for ADPS
 *
 * Copyright (C) 2024 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2024, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Dual:>>
 */
#ifndef _WL_BAM_H_
#define _WL_BAM_H_
#include <typedefs.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <net/cfg80211.h>

#include <wl_cfgp2p.h>
#include <dhd.h>

#define WL_BAD_AP_MAX_ENTRY_NUM		20u

typedef struct wl_bad_ap_mngr {
	osl_t *osh;

	uint32 num;
	spinlock_t lock;
	struct list_head list;
} wl_bad_ap_mngr_t;

typedef struct wl_bad_ap_info {
	struct	ether_addr bssid;
} wl_bad_ap_info_t;

typedef struct wl_bad_ap_info_entry {
	wl_bad_ap_info_t bad_ap;
	struct list_head list;
} wl_bad_ap_info_entry_t;

void wl_bad_ap_mngr_init(struct bcm_cfg80211 *cfg);
void wl_bad_ap_mngr_deinit(struct bcm_cfg80211 *cfg);

int wl_bad_ap_mngr_add(wl_bad_ap_mngr_t *bad_ap_mngr, wl_bad_ap_info_t *bad_ap_info);
wl_bad_ap_info_entry_t* wl_bad_ap_mngr_find(wl_bad_ap_mngr_t *bad_ap_mngr,
        const struct ether_addr *bssid);

bool wl_adps_bad_ap_check(struct bcm_cfg80211 *cfg, const struct ether_addr *bssid);
int wl_adps_enabled(struct bcm_cfg80211 *cfg, struct net_device *ndev);
int wl_adps_set_suspend(struct bcm_cfg80211 *cfg, struct net_device *ndev, uint8 suspend);

s32 wl_adps_event_handler(struct bcm_cfg80211 *cfg, bcm_struct_cfgdev *cfgdev,
	const wl_event_msg_t *e, void *data);
#endif  /* _WL_BAM_H_ */

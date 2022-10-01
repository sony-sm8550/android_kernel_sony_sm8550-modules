/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <target_if_spatial_reuse.h>
#include <wlan_lmac_if_def.h>
#include <wmi_unified_api.h>
#include <wmi_unified_vdev_api.h>
#include <target_if_vdev_mgr_tx_ops.h>
#include <init_deinit_lmac.h>
#include <wlan_vdev_mlme_api.h>

static QDF_STATUS spatial_reuse_send_cfg(struct wlan_objmgr_vdev *vdev,
					 uint8_t sr_ctrl,
					 uint8_t non_srg_max_pd_offset)
{
	struct pdev_params pparam;
	wmi_unified_t wmi_handle;

	wmi_handle = target_if_vdev_mgr_wmi_handle_get(vdev);
	if (!wmi_handle) {
		mlme_err("Failed to get WMI handle!");
		return QDF_STATUS_E_INVAL;
	}

	qdf_mem_zero(&pparam, sizeof(pparam));
	pparam.param_id = WMI_PDEV_PARAM_SET_CMD_OBSS_PD_THRESHOLD;
	if (!(sr_ctrl & NON_SRG_PD_SR_DISALLOWED) &&
	    (sr_ctrl & NON_SRG_OFFSET_PRESENT)) {
		QDF_SET_BITS(pparam.param_value, NON_SRG_SPR_ENABLE_POS,
			     NON_SRG_SPR_ENABLE_SIZE, NON_SRG_SPR_ENABLE);
		QDF_SET_BITS(pparam.param_value, SR_PARAM_VAL_DBM_POS,
			     NON_SRG_PARAM_VAL_DBM_SIZE,
			     SR_PARAM_VAL_DBM_UNIT);
		QDF_SET_BITS(pparam.param_value, NON_SRG_MAX_PD_OFFSET_POS,
			     NON_SRG_MAX_PD_OFFSET_SIZE,
			     non_srg_max_pd_offset);
	}

	return wmi_unified_pdev_param_send(wmi_handle, &pparam,
					   WILDCARD_PDEV_ID);
}

static QDF_STATUS
spatial_reuse_send_sr_prohibit_cfg(struct wlan_objmgr_vdev *vdev,
				   bool he_siga_va15_allowed)
{
	struct sr_prohibit_param srp_param;
	wmi_unified_t wmi_handle;

	wmi_handle = target_if_vdev_mgr_wmi_handle_get(vdev);
	if (!wmi_handle) {
		mlme_err("Failed to get WMI handle!");
		return QDF_STATUS_E_INVAL;
	}

	srp_param.vdev_id = wlan_vdev_get_id(vdev);
	srp_param.sr_he_siga_val15_allowed = he_siga_va15_allowed;

	return wmi_unified_vdev_param_sr_prohibit_send(wmi_handle, &srp_param);
}

static QDF_STATUS
spatial_reuse_send_pd_threshold(struct wlan_objmgr_pdev *pdev,
				uint8_t vdev_id,
				uint32_t val)
{
	struct vdev_set_params vdev_param;
	struct wmi_unified *wmi_handle;
	bool sr_supported;

	wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!wmi_handle)
		return QDF_STATUS_E_INVAL;

	sr_supported =
		wmi_service_enabled(wmi_handle,
				    wmi_service_srg_srp_spatial_reuse_support);

	if (sr_supported) {
		qdf_mem_zero(&vdev_param, sizeof(vdev_param));
		vdev_param.vdev_id = vdev_id;
		vdev_param.param_id = WMI_VDEV_PARAM_SET_CMD_OBSS_PD_THRESHOLD;
		vdev_param.param_value = val;
		return wmi_unified_vdev_set_param_send(wmi_handle, &vdev_param);
	} else {
		mlme_debug("Target doesn't support SR operations");
	}
	return QDF_STATUS_SUCCESS;
}

/**
 * spatial_reuse_set_sr_enable_disable: To send wmi command to enable/disable SR
 *
 * @vdev: object manager vdev
 * @pdev: object manager pdev
 * @is_sr_enable: sr enable/disable
 * @pd_threshold: pd threshold
 *
 * Return: Success/Failure
 */
static QDF_STATUS
spatial_reuse_set_sr_enable_disable(struct wlan_objmgr_vdev *vdev,
				    struct wlan_objmgr_pdev *pdev,
				    bool is_sr_enable, int32_t pd_threshold)
{
	uint32_t val = 0;
	uint8_t sr_ctrl;
	struct wlan_objmgr_psoc *psoc;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc)
		return QDF_STATUS_E_NOENT;

	sr_ctrl = wlan_vdev_mlme_get_sr_ctrl(vdev);
	if ((!(sr_ctrl & NON_SRG_PD_SR_DISALLOWED) &&
	    (sr_ctrl & NON_SRG_OFFSET_PRESENT)) ||
	    (sr_ctrl & SRG_INFO_PRESENT)) {
		if (is_sr_enable) {
			wlan_mlme_update_sr_data(vdev, &val, pd_threshold,
						 is_sr_enable);
			wlan_vdev_obj_lock(vdev);
			wlan_vdev_mlme_set_he_spr_enabled(vdev, true);
			wlan_vdev_obj_unlock(vdev);
		} else {
			wlan_vdev_obj_lock(vdev);
			wlan_vdev_mlme_set_he_spr_enabled(vdev, false);
			wlan_vdev_obj_unlock(vdev);
		}

		mlme_debug("non-srg param val: %u, enable: %d",
			   val, is_sr_enable);
		status =
		spatial_reuse_send_pd_threshold(pdev, vdev->vdev_objmgr.vdev_id,
						val);
	} else {
		mlme_debug("Spatial reuse not enabled");
	}

	return status;
}

void target_if_spatial_reuse_register_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops)
{
	tx_ops->spatial_reuse_tx_ops.send_cfg = spatial_reuse_send_cfg;
	tx_ops->spatial_reuse_tx_ops.send_sr_prohibit_cfg =
					spatial_reuse_send_sr_prohibit_cfg;
	tx_ops->spatial_reuse_tx_ops.target_if_set_sr_enable_disable =
					spatial_reuse_set_sr_enable_disable;
}

/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
/**
 * DOC: Contains DP public data structure definations.
 *
 */

#ifndef _WLAN_DP_PUBLIC_STRUCT_H_
#define _WLAN_DP_PUBLIC_STRUCT_H_

#include "wlan_cmn.h"
#include "wlan_objmgr_cmn.h"
#include "wlan_objmgr_global_obj.h"
#include "wmi_unified.h"
#include "qdf_status.h"

/**
 * struct wlan_dp_psoc_callbacks - struct containing callback
 * to non-converged driver
 * @os_if_dp_gro_rx: OS IF Callback to handle GRO packet to n/w stack
 */
struct wlan_dp_psoc_callbacks {
	void (*os_if_dp_gro_rx)(struct sk_buff *skb, uint8_t napi_to_use,
				bool flush_gro);
};

struct wlan_dp_psoc_sb_ops {
	/*TODO to add target if TX ops*/
};

struct wlan_dp_psoc_nb_ops {
	/*TODO to add target if RX ops*/
};

/**
 * struct wlan_dp_user_config - DP component user config
 * @ipa_enable: IPA enabled/disabled config
 */
struct wlan_dp_user_config {
	bool ipa_enable;
};

#endif /* end  of _WLAN_DP_PUBLIC_STRUCT_H_ */

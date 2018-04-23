/*
 * Copyright (c) 2011-2017 The Linux Foundation. All rights reserved.
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
 * @file ol_tx.h
 * @brief Internal definitions for the high-level tx module.
 */
#ifndef _OL_TX__H_
#define _OL_TX__H_

#include <qdf_nbuf.h>           /* qdf_nbuf_t */
#include <qdf_lock.h>
#include <cdp_txrx_cmn.h>       /* ol_txrx_vdev_t, etc. */
#include <cdp_txrx_misc.h>      /* ol_tx_spec */
#include <cdp_txrx_handle.h>
#include <ol_txrx_types.h>      /* ol_tx_desc_t, ol_txrx_msdu_info_t */

#ifdef IPA_OFFLOAD
/**
 * ol_tx_send_ipa_data_frame() - send IPA data frame
 * @vdev: vdev
 * @skb: skb
 *
 * Return: skb/ NULL is for success
 */
qdf_nbuf_t ol_tx_send_ipa_data_frame(struct cdp_vdev *vdev, qdf_nbuf_t skb);
#endif

qdf_nbuf_t ol_tx_ll(ol_txrx_vdev_handle vdev, qdf_nbuf_t msdu_list);
qdf_nbuf_t ol_tx_ll_wrapper(ol_txrx_vdev_handle vdev, qdf_nbuf_t msdu_list);
#ifdef WLAN_FEATURE_FASTPATH
qdf_nbuf_t ol_tx_ll_fast(ol_txrx_vdev_handle vdev, qdf_nbuf_t msdu_list);
#endif

qdf_nbuf_t ol_tx_ll_queue(ol_txrx_vdev_handle vdev, qdf_nbuf_t msdu_list);

#ifdef CONFIG_HL_SUPPORT
#define OL_TX_SEND ol_tx_hl
#else
#define OL_TX_SEND OL_TX_LL
#endif

#ifdef QCA_LL_LEGACY_TX_FLOW_CONTROL
#define OL_TX_LL ol_tx_ll_queue
#else
#define OL_TX_LL ol_tx_ll_wrapper
#endif

#ifdef QCA_LL_LEGACY_TX_FLOW_CONTROL
void ol_tx_vdev_ll_pause_queue_send(void *context);
void ol_tx_pdev_ll_pause_queue_send_all(struct ol_txrx_pdev_t *pdev);
#else
static inline void ol_tx_vdev_ll_pause_queue_send(void *context)
{
}
static inline
void ol_tx_pdev_ll_pause_queue_send_all(struct ol_txrx_pdev_t *pdev)
{
}
#endif

/**
 * ol_tx_hl() - transmit tx frames for a HL system.
 * @vdev: the virtual device transmit the data
 * @msdu_list: the tx frames to send
 *
 * Return: NULL if all MSDUs are accepted
 */
qdf_nbuf_t
ol_tx_hl(ol_txrx_vdev_handle vdev, qdf_nbuf_t msdu_list);

/**
 * ol_tx_non_std() - Allow the control-path SW to send data frames
 * @data_vdev: which vdev should transmit the tx data frames
 * @tx_spec: what non-standard handling to apply to the tx data frames
 * @msdu_list: NULL-terminated list of tx MSDUs
 *
 * Generally, all tx data frames come from the OS shim into the txrx layer.
 * However, there are rare cases such as TDLS messaging where the UMAC
 * control-path SW creates tx data frames.
 *  This UMAC SW can call this function to provide the tx data frames to
 *  the txrx layer.
 *  The UMAC SW can request a callback for these data frames after their
 *  transmission completes, by using the ol_txrx_data_tx_cb_set function
 *  to register a tx completion callback, and by specifying
 *  ol_tx_spec_no_free as the tx_spec arg when giving the frames to
 *  ol_tx_non_std.
 *  The MSDUs need to have the appropriate L2 header type (802.3 vs. 802.11),
 *  as specified by ol_cfg_frame_type().
 *
 *  Return: null - success, skb - failure
 */
qdf_nbuf_t
ol_tx_non_std(struct cdp_vdev *pvdev,
	      enum ol_tx_spec tx_spec, qdf_nbuf_t msdu_list);

void ol_txrx_mgmt_tx_complete(void *ctxt, qdf_nbuf_t netbuf, int err);

/**
 * ol_txrx_mgmt_tx_cb_set() - Store a callback for delivery
 *	notifications for management frames.
 * @ppdev: the data physical device object
 * @type: the type of mgmt frame the callback is used for
 * @download_cb: the callback for notification of delivery to the target
 * @ota_ack_cb: the callback for notification of delivery to the peer
 * @ctxt: context to use with the callback
 *
 * When the txrx SW receives notifications from the target that a tx frame
 * has been delivered to its recipient, it will check if the tx frame
 * is a management frame.  If so, the txrx SW will check the management
 * frame type specified when the frame was submitted for transmission.
 * If there is a callback function registered for the type of managment
 * frame in question, the txrx code will invoke the callback to inform
 * the management + control SW that the mgmt frame was delivered.
 * This function is used by the control SW to store a callback pointer
 * for a given type of management frame.
 */
void
ol_txrx_mgmt_tx_cb_set(struct cdp_pdev *ppdev, uint8_t type,
		       ol_txrx_mgmt_tx_cb download_cb,
		       ol_txrx_mgmt_tx_cb ota_ack_cb, void *ctxt);

/**
 * ol_txrx_mgmt_send_ext() - Transmit a management frame
 * @pvdev: virtual device transmitting the frame
 * @tx_mgmt_frm: management frame to transmit
 * @type: the type of managment frame (determines what callback to use)
 * @use_6mbps: specify whether management frame to transmit should
 *	use 6 Mbps rather than 1 Mbps min rate(for 5GHz band or P2P)
 * @chanfreq: channel to transmit the frame on
 *
 * Send the specified management frame from the specified virtual device.
 * The type is used for determining whether to invoke a callback to inform
 * the sender that the tx mgmt frame was delivered, and if so, which
 * callback to use.
 *
 * Return: 0 - the frame is accepted for transmission
 *         1 - the frame was not accepted
 */
int
ol_txrx_mgmt_send_ext(struct cdp_vdev *pvdev,
		      qdf_nbuf_t tx_mgmt_frm,
		      uint8_t type, uint8_t use_6mbps, uint16_t chanfreq);

qdf_nbuf_t
ol_tx_reinject(struct ol_txrx_vdev_t *vdev, qdf_nbuf_t msdu, uint16_t peer_id);

#if defined(FEATURE_TSO)
void ol_tso_seg_list_init(struct ol_txrx_pdev_t *pdev, uint32_t num_seg);
void ol_tso_seg_list_deinit(struct ol_txrx_pdev_t *pdev);
void ol_tso_num_seg_list_init(struct ol_txrx_pdev_t *pdev, uint32_t num_seg);
void ol_tso_num_seg_list_deinit(struct ol_txrx_pdev_t *pdev);
#else
static inline void ol_tso_seg_list_init(struct ol_txrx_pdev_t *pdev,
	uint32_t num_seg)
{
}
static inline void ol_tso_seg_list_deinit(struct ol_txrx_pdev_t *pdev)
{
}
static inline void ol_tso_num_seg_list_init(struct ol_txrx_pdev_t *pdev,
	uint32_t num_seg)
{
}
static inline void ol_tso_num_seg_list_deinit(struct ol_txrx_pdev_t *pdev)
{
}
#endif

#if defined(HELIUMPLUS)
void ol_txrx_dump_frag_desc(char *msg, struct ol_tx_desc_t *tx_desc);
#else
static inline
void ol_txrx_dump_frag_desc(char *msg, struct ol_tx_desc_t *tx_desc)
{
}
#endif

#endif /* _OL_TX__H_ */

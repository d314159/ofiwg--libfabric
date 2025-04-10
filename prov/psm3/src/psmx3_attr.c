/*
 * Copyright (c) 2013-2019 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "psmx3.h"

/*
 * Default provider attributes are defined for:
 *
 * 	full set of capabilities
 * 	ep type = FI_EP_RDM
 * 	addr format = FI_ADDR_PSMX3
 * 	cq_data_size = 0
 *
 * This is used as a template to create actual provider info, which will
 * have some fields modified for different configurations and some fields
 * updated to environment settings.
 */

static struct fi_tx_attr psmx3_tx_attr = {
	.caps			= PSMX3_TX_CAPS, /* PSMX3_RMA_TX_CAPS */
	.mode			= FI_CONTEXT, /* 0 */
	.op_flags		= PSMX3_OP_FLAGS,
	.msg_order		= PSMX3_MSG_ORDER,
	.inject_size		= 64, /* psmx3_env.inject_size */
	.size			= UINT64_MAX,
	.iov_limit		= PSMX3_IOV_MAX_COUNT,
	.rma_iov_limit		= 1,
};

static struct fi_rx_attr psmx3_rx_attr = {
	.caps			= PSMX3_RX_CAPS, /* PSMX3_RMA_RX_CAPS */
	.mode			= FI_CONTEXT, /* 0 */
	.op_flags		= PSMX3_OP_FLAGS,
	.msg_order		= PSMX3_MSG_ORDER,
	.size			= UINT64_MAX,
	.iov_limit		= 1,
};

static struct fi_ep_attr psmx3_ep_attr = {
	.type			= FI_EP_RDM, /* FI_EP_DGRAM */
	.protocol		= FI_PROTO_PSMX3,
	.protocol_version	= PSM2_VERNO,
	.max_msg_size		= PSMX3_MAX_MSG_SIZE & ~0x0FFF,
	.msg_prefix_size	= 0,
	.max_order_raw_size	= PSMX3_RMA_ORDER_SIZE,
	.max_order_war_size	= PSMX3_RMA_ORDER_SIZE,
	.max_order_waw_size	= PSMX3_RMA_ORDER_SIZE,
	.mem_tag_format		= FI_TAG_GENERIC, /* >>= 4 */
	.tx_ctx_cnt		= 1,
	.rx_ctx_cnt		= 1,
	.auth_key_size		= sizeof(psm2_uuid_t),
	.auth_key		= NULL,
};

static struct fi_domain_attr psmx3_domain_attr = {
	.domain			= NULL,
	.name			= NULL,
	.threading		= FI_THREAD_SAFE,
	.control_progress	= FI_PROGRESS_AUTO,
	.data_progress		= FI_PROGRESS_AUTO,
	.resource_mgmt		= FI_RM_ENABLED,
	.av_type		= FI_AV_UNSPEC,
	.mr_mode		= OFI_MR_SCALABLE | OFI_MR_BASIC,
	.mr_key_size		= sizeof(uint64_t),
	.cq_data_size		= 0, /* 4, 8 */
	.cq_cnt			= 65535,
	.ep_cnt			= 65535,
	.tx_ctx_cnt		= 1, /* psmx3_domain_info.max_trx_ctxt */
	.rx_ctx_cnt		= 1, /* psmx3_domain_info.max_trx_ctxt */
	.max_ep_tx_ctx		= 1, /* psmx3_domain_info.max_trx_ctxt */
	.max_ep_rx_ctx		= 1, /* psmx3_domain_info.max_trx_ctxt */
	.max_ep_stx_ctx		= 1, /* psmx3_domain_info.max_trx_ctxt */
	.max_ep_srx_ctx		= 0,
	.cntr_cnt		= 65535,
	.mr_iov_limit		= 65535,
	.caps			= PSMX3_DOM_CAPS,
	.mode			= 0,
	.auth_key		= NULL,
	.auth_key_size		= sizeof(psm2_uuid_t),
	.max_err_data		= PSMX3_ERR_DATA_SIZE,
	.mr_cnt			= 65535,
};

static struct fi_fabric_attr psmx3_fabric_attr = {
	.name			= NULL,
};

static struct fi_info psmx3_prov_info = {
	.next			= NULL,
	.caps			= PSMX3_CAPS, /* PSMX3_RMA_CAPS */
	.mode			= FI_CONTEXT, /* 0 */
	.addr_format		= FI_ADDR_PSMX3, /* FI_ADDR_STR */
	.src_addrlen		= sizeof(struct psmx3_ep_name),
	.dest_addrlen		= sizeof(struct psmx3_ep_name),
	.src_addr		= NULL,
	.dest_addr		= NULL,
	.handle			= NULL,
	.tx_attr		= &psmx3_tx_attr,
	.rx_attr		= &psmx3_rx_attr,
	.ep_attr		= &psmx3_ep_attr,
	.domain_attr		= &psmx3_domain_attr,
	.fabric_attr		= &psmx3_fabric_attr,
};

#ifdef HAVE_PSM3_DL
static struct fi_info *psmx3_allocinfo_internal(void)
{
	struct fi_info *info;

	info = calloc(1, sizeof(*info));
	if (!info)
		return NULL;

	info->tx_attr = calloc(1, sizeof(*info->tx_attr));
	info->rx_attr = calloc(1, sizeof(*info->rx_attr));
	info->ep_attr = calloc(1, sizeof(*info->ep_attr));
	info->domain_attr = calloc(1, sizeof(*info->domain_attr));
	info->fabric_attr = calloc(1, sizeof(*info->fabric_attr));
	if (!info->tx_attr|| !info->rx_attr || !info->ep_attr ||
	    !info->domain_attr || !info->fabric_attr)
		goto err;

	return info;
err:
	fi_freeinfo(info);
	return NULL;
}
static struct fi_info *psmx3_dupinfo(const struct fi_info *info)
{
	struct fi_info *dup;
	int ret;

	if (!info)
		return psmx3_allocinfo_internal();

	dup = mem_dup(info, sizeof(*dup));
	if (dup == NULL) {
		return NULL;
	}
	dup->src_addr = NULL;
	dup->dest_addr = NULL;
	dup->tx_attr = NULL;
	dup->rx_attr = NULL;
	dup->ep_attr = NULL;
	dup->domain_attr = NULL;
	dup->fabric_attr = NULL;
	dup->next = NULL;

	if (info->src_addr != NULL) {
		dup->src_addr = mem_dup(info->src_addr, info->src_addrlen);
		if (dup->src_addr == NULL)
			goto fail;
	}
	if (info->dest_addr != NULL) {
		dup->dest_addr = mem_dup(info->dest_addr, info->dest_addrlen);
		if (dup->dest_addr == NULL)
			goto fail;
	}
	if (info->tx_attr != NULL) {
		dup->tx_attr = mem_dup(info->tx_attr, sizeof(*info->tx_attr));
		if (dup->tx_attr == NULL)
			goto fail;
	}
	if (info->rx_attr != NULL) {
		dup->rx_attr = mem_dup(info->rx_attr, sizeof(*info->rx_attr));
		if (dup->rx_attr == NULL)
			goto fail;
	}
	if (info->ep_attr != NULL) {
		dup->ep_attr = mem_dup(info->ep_attr, sizeof(*info->ep_attr));
		if (dup->ep_attr == NULL)
			goto fail;
		if (info->ep_attr->auth_key != NULL) {
			dup->ep_attr->auth_key =
				mem_dup(info->ep_attr->auth_key,
					info->ep_attr->auth_key_size);
			if (dup->ep_attr->auth_key == NULL)
				goto fail;
		}
	}
	if (info->domain_attr) {
		dup->domain_attr = mem_dup(info->domain_attr,
					   sizeof(*info->domain_attr));
		if (dup->domain_attr == NULL)
			goto fail;
		dup->domain_attr->name = NULL;
		dup->domain_attr->auth_key = NULL;
		if (info->domain_attr->name != NULL) {
			dup->domain_attr->name = strdup(info->domain_attr->name);
			if (dup->domain_attr->name == NULL)
				goto fail;
		}
		if (info->domain_attr->auth_key != NULL) {
			dup->domain_attr->auth_key =
				mem_dup(info->domain_attr->auth_key,
					info->domain_attr->auth_key_size);
			if (dup->domain_attr->auth_key == NULL)
				goto fail;
		}
	}
	if (info->fabric_attr) {
		dup->fabric_attr = mem_dup(info->fabric_attr,
					   sizeof(*info->fabric_attr));
		if (dup->fabric_attr == NULL)
			goto fail;
		dup->fabric_attr->name = NULL;
		dup->fabric_attr->prov_name = NULL;
		if (info->fabric_attr->name != NULL) {
			dup->fabric_attr->name = strdup(info->fabric_attr->name);
			if (dup->fabric_attr->name == NULL)
				goto fail;
		}
		if (info->fabric_attr->prov_name != NULL) {
			dup->fabric_attr->prov_name = strdup(info->fabric_attr->prov_name);
			if (dup->fabric_attr->prov_name == NULL)
				goto fail;
		}
	}

	if (info->nic) {
		ret = fi_control(&info->nic->fid, FI_DUP, &dup->nic);
		if (ret && ret != -FI_ENOSYS)
			goto fail;
	}

	return dup;

fail:
	fi_freeinfo(dup);
	return NULL;
}
#else
#define psmx3_dupinfo fi_dupinfo
#endif /* HAVE_PSM3_DL */

static uint64_t psmx3_check_fi_hmem_cap(void) {
#ifdef PSM_HAVE_GPU
	/* if parses as empty or invalid use default of 0 */
	/* psm3 below us will provide warning as needed when it parses it */
	int gpu = 0;
	unsigned int gpudirect = 0;
#ifdef PSM_CUDA
	(void)psm3_parse_str_int(psm3_env_get("PSM3_CUDA"), &gpu, INT_MIN, INT_MAX);
#else /* PSM_ONEAPI */
	(void)psm3_parse_str_int(psm3_env_get("PSM3_ONEAPI_ZE"), &gpu,
							INT_MIN, INT_MAX);
#endif
	(void)psm3_parse_str_uint(psm3_env_get("PSM3_GPUDIRECT"), &gpudirect,
							0, UINT_MAX);
	if ((gpu || gpudirect) && !ofi_hmem_p2p_disabled())
		return FI_HMEM;
#endif /* PSM_HAVE_GPU */
	return 0;
}

static uint64_t get_max_inject_size(void) {
	unsigned int thresh_rv;
	unsigned int temp;
	int have_shm = 1;
	int have_nic = 1;
	int devid_enabled[PTL_MAX_INIT];

	// check PSM3_DEVICES to determine if PSM3 shm enabled
	if ((PSM2_OK == psm3_parse_devices(devid_enabled))) {
		have_shm = psm3_device_is_enabled(devid_enabled, PTL_DEVID_AMSH);
		have_nic = psm3_device_is_enabled(devid_enabled, PTL_DEVID_IPS);
	}

	// figure out the smallest rendezvous threshold (GPU vs CPU ips vs shm)
	// If middleware above is not using PSM3 for shm but leaves it in
	// PSM3_DEVICES, this could be more restrictive than necessary,
	// but it's safe.  Note that PSM3_DEVICES can't be set per EP open.
	// Also not yet sure which HAL will be selected so must pick most
	// conservative ips (NIC) config
	thresh_rv = 65536;	// default in odd case of PSM3_DEVICES=self

	if (have_nic) {
		temp = PSM3_MQ_RNDV_NIC_THRESH;
		psm3_parse_str_uint(psm3_env_get("PSM3_MQ_RNDV_NIC_THRESH"), &temp,
							0, UINT_MAX);
		if (thresh_rv > temp)
			thresh_rv = temp;
	}

	if (have_shm) {
		temp = PSM3_MQ_RNDV_SHM_THRESH;
		psm3_parse_str_uint(psm3_env_get("PSM3_MQ_RNDV_SHM_THRESH"), &temp,
							0, UINT_MAX);
		if (thresh_rv > temp)
			thresh_rv = temp;
	}

#ifdef PSM_HAVE_GPU
	if (psmx3_prov_info.caps & FI_HMEM) {
		if (have_nic) {
			// GPU ips rendezvous threshold
			uint32_t out;
			if (psm3_info_query(PSM2_INFO_QUERY_GPU_THRESH_RNDV, &out, 0, NULL)) {
				PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
					"Unable to get PSM3_GPU_THRESH_RNDV.\n");
			} else if (thresh_rv > out) {
				thresh_rv = out;
			}
		}

		if (have_shm) {
			// GPU shm rendezvous threshold
			// we only have default, real value may be overriden at MQ init
			// when open PSM3 endpoint
			uint32_t out;
			if (psm3_info_query(PSM2_INFO_QUERY_MQ_RNDV_SHM_GPU_THRESH_DEFAULT, &out, 0, NULL)) {
				PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
					"Unable to get PSM3_MQ_RNDV_SHM_GPU_THRESH default.\n");
			} else if (thresh_rv > out) {
				thresh_rv = out;
			}
		}
	}
#endif

	// messages <= thresh_rv guaranteed to use eager, so thresh_rv
	// is the max allowed inject_size.
	return thresh_rv;
}

/*
 * Possible provider variations:
 *
 *  (1)  FI_ADDR_PSMX3, FI_EP_RDM,   tag64 (cq_data_size 0, FI_CONTEXT)
 *  (2)  FI_ADDR_PSMX3, FI_EP_RDM,   tag60 (cq_data_size 4, FI_CONTEXT)
 *  (3)  FI_ADDR_PSMX3, FI_EP_RDM,   rma   (cq_data_size 8)
 *  (4)  FI_ADDR_PSMX3, FI_EP_DGRAM, tag64 (cq_data_size 0, FI_CONTEXT)
 *  (5)  FI_ADDR_PSMX3, FI_EP_DGRAM, tag60 (cq_data_size 4, FI_CONTEXT)
 *  (6)  FI_ADDR_PSMX3, FI_EP_DGRAM, rma   (cq_data_size 8)
 *  (7)  FI_ADDR_STR,   FI_EP_RDM,   tag64 (cq_data_size 0, FI_CONTEXT)
 *  (8)  FI_ADDR_STR,   FI_EP_RDM,   tag60 (cq_data_size 4, FI_CONTEXT)
 *  (9)  FI_ADDR_STR,   FI_EP_RDM,   rma   (cq_data_size 8)
 *  (10) FI_ADDR_STR,   FI_EP_DGRAM, tag64 (cq_data_size 0, FI_CONTEXT)
 *  (11) FI_ADDR_STR,   FI_EP_DGRAM, tag60 (cq_data_size 4, FI_CONTEXT)
 *  (12) FI_ADDR_STR,   FI_EP_DGRAM, rma   (cq_data_size 8)
 *
 * To avoid returning all 12 provider variations for an unrestricted query,
 * "addr_format" and "ep_type" are checked first and a single value is set
 * for each of them. As the result, at most three provider instances (tag64,
 * tag60, rma) are returned.
 *
 * This also bypasses queries obviously unsuitable for this provider and
 * avoid unnecessary initialization steps.
 */

int psmx3_init_prov_info(const struct fi_info *hints, struct fi_info **info)
{
	struct fi_info *prov_info = &psmx3_prov_info;
	struct fi_info *info_out, *info_new;
	int addr_format = FI_ADDR_PSMX3;
	int addr_format2 = FI_ADDR_STR;
	int ep_type = FI_EP_RDM;
	int ep_type2 = FI_EP_DGRAM;
	uint64_t extra_caps = 0;

	/* Check if GPU is enabled */
	extra_caps |= psmx3_check_fi_hmem_cap();

	prov_info->caps |= extra_caps;
	prov_info->tx_attr->caps |= extra_caps;
	prov_info->rx_attr->caps |= extra_caps;
	prov_info->domain_attr->caps |= extra_caps;

	if (!hints)
		goto alloc_info;

	if (hints->ep_attr) {
		switch (hints->ep_attr->type) {
		case FI_EP_UNSPEC:
		case FI_EP_RDM:
			break;
		case FI_EP_DGRAM:
			ep_type = FI_EP_DGRAM;
			break;
		default:
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE,
				"Unsupported endpoint type\n");
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "Supported: %s\n",
				fi_tostr(&ep_type, FI_TYPE_EP_TYPE));
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "Supported: %s\n",
				fi_tostr(&ep_type2, FI_TYPE_EP_TYPE));
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "Requested: %s\n",
				fi_tostr(&hints->ep_attr->type, FI_TYPE_EP_TYPE));
			return -FI_ENODATA;
		}
	}

	switch (hints->addr_format) {
		case FI_FORMAT_UNSPEC:
		case FI_ADDR_PSMX3:
			break;
		case FI_ADDR_STR:
			addr_format = FI_ADDR_STR;
			break;
		default:
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE,
				"Unsupported address format\n");
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "Supported: %s\n",
				fi_tostr(&addr_format, FI_TYPE_ADDR_FORMAT));
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "Supported: %s\n",
				fi_tostr(&addr_format2, FI_TYPE_ADDR_FORMAT));
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "Requested: %s\n",
				fi_tostr(&hints->addr_format, FI_TYPE_ADDR_FORMAT));
			return -FI_ENODATA;
	}

	if ((hints->caps & prov_info->caps) != hints->caps) {
		PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "caps not supported\n");
		OFI_INFO_CHECK(&psmx3_prov, prov_info, hints, caps, FI_TYPE_CAPS);
		return -FI_ENODATA;
	}

alloc_info:
	psmx3_prov_info.fabric_attr->prov_version = get_psm3_provider_version();
	info_out = NULL;
	if (!hints || !(hints->caps & (FI_TAGGED | FI_MSG))) {
		info_new = psmx3_dupinfo(&psmx3_prov_info);
		if (info_new) {
			/* rma only, 64 bit CQ data */
			info_new->addr_format = addr_format;
			info_new->ep_attr->type = ep_type;
			info_new->caps = PSMX3_RMA_CAPS | extra_caps;
			info_new->mode = 0;
			info_new->tx_attr->caps = PSMX3_RMA_TX_CAPS | extra_caps;
			info_new->tx_attr->mode = 0;
			info_new->rx_attr->caps = PSMX3_RMA_RX_CAPS | extra_caps;
			info_new->rx_attr->mode = 0;
			info_new->domain_attr->cq_data_size = 8;
			info_out = info_new;
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE,
				"RMA only instance included\n");
		}
	}

	info_new = psmx3_dupinfo(&psmx3_prov_info);
	if (info_new) {
		/* 60 bit tag, 32 bit CQ data */
		info_new->addr_format = addr_format;
		info_new->ep_attr->type = ep_type;
		info_new->ep_attr->mem_tag_format >>= 4;
		info_new->domain_attr->cq_data_size = 4;
		info_new->next = info_out;
		info_out = info_new;
		PSMX3_INFO(&psmx3_prov, FI_LOG_CORE,
			"TAG60 instance included\n");
	}

	if (!hints || !hints->domain_attr ||
	    !hints->domain_attr->cq_data_size) {
		info_new = psmx3_dupinfo(&psmx3_prov_info);
		if (info_new) {
			/* 64 bit tag, no CQ data */
			info_new->addr_format = addr_format;
			info_new->ep_attr->type = ep_type;
			info_new->next = info_out;
			info_out = info_new;
			PSMX3_INFO(&psmx3_prov, FI_LOG_CORE,
				"TAG64 instance included\n");
		}
	}

	*info = info_out;
	return info_out ? 0 : -FI_ENODATA;
}

static void psmx3_dup_addr(int format, struct psmx3_ep_name *addr,
			   void **addr_out, size_t *len)
{
	if (!addr)
		return;

	if (format == FI_ADDR_STR) {
		*addr_out = psmx3_ep_name_to_string(addr, len);
	} else {
		*addr_out = mem_dup(addr, sizeof(*addr));
		*len = sizeof(*addr);
	}
}

static void psmx3_expand_default_unit(struct fi_info *info)
{
	struct fi_info *p, *next;
	struct psmx3_ep_name *src_addr;
	int i;

	p = info;
	while (p) {
		next = p->next;
		src_addr = p->src_addr;
		if (src_addr->unit == PSMX3_DEFAULT_UNIT) {
			/* if we only found 1 unit, report it.
			 * for MULTIRAIL we may find multiple units but
			 * only report 1, in which case we leave default as is
			 */
			if (psmx3_domain_info.num_active_units == 1) {
				src_addr->unit = psmx3_domain_info.active_units[0];
			} else if (psmx3_domain_info.num_reported_units > 1) {
				/* report all units in addition to default */
				for (i = 0; i < psmx3_domain_info.num_reported_units; i++) {
					/* for MULTIRAIL=-1 we have no default unit, so we omit
					 * the default autoselect unit
					 */
					if (i == 0 && ! psmx3_domain_info.default_domain_name[0]) {
						src_addr->unit = psmx3_domain_info.active_units[0];
						continue;
					}
					p->next = psmx3_dupinfo(p);
					if (!p->next) {
						PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
							"Failed to duplicate info for HFI unit %d\n",
							psmx3_domain_info.active_units[i]);
						break;
					}
					p = p->next;
					src_addr = p->src_addr;
					src_addr->unit = psmx3_domain_info.active_units[i];
				}
			} else {
				/* only get here when 1 reported & >1 active -> MULTIRAIL>0 */
				assert(psmx3_domain_info.default_domain_name[0]);
			}
		}
		p->next = next;
		p = next;
	}
}

/* only called if num_reported_units >= 1 which implies num_active_units >= 1 */
void psmx3_update_prov_info(struct fi_info *info,
			    struct psmx3_ep_name *src_addr,
			    struct psmx3_ep_name *dest_addr)
{
	struct fi_info *p;
	unsigned int max_inject_size;
	unsigned int inject_size;

	for (p = info; p; p = p->next) {
		psmx3_dup_addr(p->addr_format, src_addr,
			       &p->src_addr, &p->src_addrlen);
		psmx3_dup_addr(p->addr_format, dest_addr,
			       &p->dest_addr, &p->dest_addrlen);
	}

	psmx3_expand_default_unit(info);

	max_inject_size = get_max_inject_size();
	if (psmx3_env.inject_size > max_inject_size)
		inject_size = max_inject_size;
	else
		inject_size = psmx3_env.inject_size;
	PSMX3_INFO(&psmx3_prov, FI_LOG_CORE,
		"Using inject_size=%u based on FI_PSM3_INJECT_SIZE=%u with max %u\n",
		inject_size, psmx3_env.inject_size, max_inject_size);

	for (p = info; p; p = p->next) {
		int unit = ((struct psmx3_ep_name *)p->src_addr)->unit;
		int port = ((struct psmx3_ep_name *)p->src_addr)->port;

		/* when we have no default unit, default to 1st unit */
		if (unit == PSMX3_DEFAULT_UNIT &&
		    ! psmx3_domain_info.default_domain_name[0])
			unit = 0;

		p->domain_attr->tx_ctx_cnt = psmx3_domain_info.max_trx_ctxt;
		p->domain_attr->rx_ctx_cnt = psmx3_domain_info.max_trx_ctxt;
		p->domain_attr->max_ep_tx_ctx = psmx3_domain_info.max_trx_ctxt;
		p->domain_attr->max_ep_rx_ctx = psmx3_domain_info.max_trx_ctxt;
		p->domain_attr->max_ep_stx_ctx = psmx3_domain_info.max_trx_ctxt;

		free(p->domain_attr->name);
		if (unit == PSMX3_DEFAULT_UNIT)
			p->domain_attr->name = strdup(psmx3_domain_info.default_domain_name);
		else {
			char unit_name[NAME_MAX];
			psm2_info_query_arg_t args[4];
			int unit_id = psmx3_domain_info.unit_id[unit];
			int addr_index = psmx3_domain_info.addr_index[unit];

			args[0].unit = unit_id;
			args[1].port = port == PSMX3_DEFAULT_PORT ? 1 : port;
			args[2].addr_index = addr_index;
			args[3].length = sizeof(unit_name);

			if (PSM2_OK != psm3_info_query(PSM2_INFO_QUERY_UNIT_ADDR_NAME,
				unit_name, 4, args)) {
				PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
					"Failed to read domain name for NIC unit %d (id %d, port %d, index %d)\n",
					unit, unit_id, port, addr_index);
				if (asprintf(&p->domain_attr->name, "UNKNOWN") < 0) {
					PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
						"Failed to allocate memory for domain name for NIC unit %d\n", unit);
				}
			} else {
				if (asprintf(&p->domain_attr->name, "%s", unit_name) <0) {
					PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
						"Failed to allocate memory for domain name for NIC unit %d "
						"(id %d, port %d, index %d)\n",
						unit, unit_id, port, addr_index);
				}
			}
		}
		free(p->fabric_attr->name);
		if (unit == PSMX3_DEFAULT_UNIT)
			p->fabric_attr->name = strdup(psmx3_domain_info.default_fabric_name);
		else {
			char fabric_name[NAME_MAX];
			psm2_info_query_arg_t args[4];
			int unit_id = psmx3_domain_info.unit_id[unit];
			int addr_index = psmx3_domain_info.addr_index[unit];

			args[0].unit = unit_id;
			args[1].port = port == PSMX3_DEFAULT_PORT ? 1 : port;
			args[2].addr_index = addr_index;
			args[3].length = sizeof(fabric_name);

			if (PSM2_OK != psm3_info_query(PSM2_INFO_QUERY_UNIT_SUBNET_NAME,
				fabric_name, 4, args)) {
				PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
					"Failed to read unit fabric name for NIC unit_id %d port %d addr %d\n", unit_id, port, addr_index);
				if (asprintf(&p->fabric_attr->name, "UNKNOWN") < 0) {
					PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
						"Failed to allocate memory for unit fabric name for NIC unit %d\n", unit);
				}
			} else {
				if (asprintf(&p->fabric_attr->name, "%s", fabric_name) <0) {
					PSMX3_WARN(&psmx3_prov, FI_LOG_CORE,
						"Failed to allocate memory for unit fabric name for NIC unit %d port %d addr %d\n", unit, port, addr_index);
				}
			}
		}

		p->tx_attr->inject_size = inject_size;
	}
}

static int psmx3_check_info(const struct util_prov *util_prov,
                   const struct fi_info *info, uint32_t api_version,
                   const struct fi_info *hints)
{
	int ret;

	ret = ofi_check_info(util_prov, info, api_version, hints);
	if (ret)
		return ret;
	/* some revs of ofi_check_info fail to check names, so do it here */
	if (hints && hints->domain_attr && hints->domain_attr->name &&
	    strcasecmp(hints->domain_attr->name, info->domain_attr->name)) {
		PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "skipping device %s (want %s)\n",
			info->domain_attr->name, hints->domain_attr->name);
		return -FI_ENODATA;
	}
	if (hints && hints->fabric_attr && hints->fabric_attr->name &&
	    strcasecmp(hints->fabric_attr->name, info->fabric_attr->name)) {
		PSMX3_INFO(&psmx3_prov, FI_LOG_CORE, "skipping fabric %s (want %s)\n",
			info->fabric_attr->name, hints->fabric_attr->name);
		return -FI_ENODATA;
	}
	return FI_SUCCESS;
}

int psmx3_check_prov_info(uint32_t api_version,
			  const struct fi_info *hints,
			  struct fi_info **info)
{
	struct util_prov util_prov = { .prov = &psmx3_prov };
	struct fi_info *next;
	struct fi_info *prev = NULL;
	struct fi_info *curr = *info;
	struct fi_info *new_info = *info;

	while (curr) {
		next = curr->next;
		if (psmx3_check_info(&util_prov, curr, api_version, hints)) {
			if (prev)
				prev->next = next;
			else
				new_info = next;
			curr->next = NULL;
			fi_freeinfo(curr);
		} else {
			prev = curr;
		}
		curr = next;
	}

	*info = new_info;
	return new_info ? 0 : -FI_ENODATA;
}

void psmx3_alter_prov_info(uint32_t api_version,
			   const struct fi_info *hints,
			   struct fi_info *info)
{
	int cnt = 0;
	int cq_data_cnt = 0;

	ofi_alter_info(info, hints, api_version);

	/*
	 * Some of the default values are set to simplify info
	 * checking. Now change them back to the preferred values.
	 */
	for (; info; info = info->next) {
		if (!hints || !hints->domain_attr ||
		    !hints->domain_attr->control_progress)
			info->domain_attr->control_progress =
				FI_PROGRESS_MANUAL;

		if (!hints || !hints->domain_attr ||
		    !hints->domain_attr->data_progress)
			info->domain_attr->data_progress =
				FI_PROGRESS_MANUAL;

		if (info->domain_attr->mr_mode == (OFI_MR_BASIC | OFI_MR_SCALABLE))
			info->domain_attr->mr_mode = OFI_MR_SCALABLE;

		/*
		 * Avoid automatically adding secondary caps that may negatively
		 * impact performance.
		 */
		if (hints && hints->caps && !(hints->caps & FI_TRIGGER))
			info->caps &= ~FI_TRIGGER;

		if (info->domain_attr->cq_data_size)
			cq_data_cnt++;

		cnt++;
	}

	PSMX3_INFO(&psmx3_prov, FI_LOG_CORE,
		"%d instances available, %d with CQ data flag set\n",
		cnt, cq_data_cnt);
}


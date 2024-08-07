/*
 * Copyright (c) 2017-2022 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *	   Redistribution and use in source and binary forms, with or
 *	   without modification, are permitted provided that the following
 *	   conditions are met:
 *
 *		- Redistributions of source code must retain the above
 *		  copyright notice, this list of conditions and the following
 *		  disclaimer.
 *
 *		- Redistributions in binary form must reproduce the above
 *		  copyright notice, this list of conditions and the following
 *		  disclaimer in the documentation and/or other materials
 *		  provided with the distribution.
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

#include <stdlib.h>
#include <string.h>

#include "ofi_atomic.h"
#include "xnet.h"

static int xnet_mr_close(struct fid *fid)
{
	struct xnet_domain *domain;
	struct ofi_mr *mr;
	int ret;

	mr = container_of(fid, struct ofi_mr, mr_fid.fid);
	domain = container_of(&mr->domain->domain_fid, struct xnet_domain,
			      util_domain.domain_fid.fid);

	ofi_genlock_lock(domain->progress.active_lock);
	ret = ofi_mr_close(fid);
	ofi_genlock_unlock(domain->progress.active_lock);
	return ret;
}

static void xnet_subdomains_mr_close(struct xnet_domain *domain, uint64_t mr_key)
{
	int ret;
	struct fid_list_entry *item;
	struct xnet_domain *subdomain;

	assert(ofi_genlock_held(&domain->subdomain_list_lock));
	dlist_foreach_container(&domain->subdomain_list,
				struct fid_list_entry, item, entry) {
		subdomain = container_of(item->fid, struct xnet_domain,
					 util_domain.domain_fid.fid);
		ofi_genlock_lock(&subdomain->util_domain.lock);
		ret = ofi_mr_map_remove(&subdomain->util_domain.mr_map, mr_key);
		ofi_genlock_unlock(&subdomain->util_domain.lock);

		if (!ret)
			ofi_atomic_dec32(&subdomain->util_domain.ref);
	}
}

static int xnet_mplex_mr_close(struct fid *fid)
{
	struct xnet_domain *domain;
	struct ofi_mr *mr;

	mr = container_of(fid, struct ofi_mr, mr_fid.fid);
	domain = container_of(&mr->domain->domain_fid, struct xnet_domain,
			      util_domain.domain_fid.fid);

	ofi_genlock_lock(&domain->subdomain_list_lock);
	xnet_subdomains_mr_close(domain, mr->key);
	ofi_genlock_unlock(&domain->subdomain_list_lock);
	return ofi_mr_close(fid);
}

static struct fi_ops xnet_mr_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = xnet_mr_close,
	.bind = fi_no_bind,
	.control = fi_no_control,
	.ops_open = fi_no_ops_open
};

static struct fi_ops xnet_mplex_mr_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = xnet_mplex_mr_close,
	.bind = fi_no_bind,
	.control = fi_no_control,
	.ops_open = fi_no_ops_open
};

static int
xnet_mr_reg(struct fid *fid, const void *buf, size_t len,
	    uint64_t access, uint64_t offset, uint64_t requested_key,
	    uint64_t flags, struct fid_mr **mr_fid, void *context)
{
	struct xnet_domain *domain;
	struct ofi_mr *mr;
	int ret;

	domain = container_of(fid, struct xnet_domain,
			      util_domain.domain_fid.fid);
	ofi_genlock_lock(domain->progress.active_lock);
	ret = ofi_mr_reg(fid, buf, len, access, offset, requested_key, flags,
			 mr_fid, context);
	ofi_genlock_unlock(domain->progress.active_lock);

	if (!ret) {
		mr = container_of(*mr_fid, struct ofi_mr, mr_fid.fid);
		mr->mr_fid.fid.ops = &xnet_mr_fi_ops;
	}
	return ret;
}

static int
xnet_mr_regv(struct fid *fid, const struct iovec *iov,
	     size_t count, uint64_t access,
	     uint64_t offset, uint64_t requested_key,
	     uint64_t flags, struct fid_mr **mr_fid, void *context)
{
	struct xnet_domain *domain;
	struct ofi_mr *mr;
	int ret;

	domain = container_of(fid, struct xnet_domain,
			      util_domain.domain_fid.fid);
	ofi_genlock_lock(domain->progress.active_lock);
	ret = ofi_mr_regv(fid, iov, count, access, offset, requested_key, flags,
			 mr_fid, context);
	ofi_genlock_unlock(domain->progress.active_lock);

	if (!ret) {
		mr = container_of(*mr_fid, struct ofi_mr, mr_fid.fid);
		mr->mr_fid.fid.ops = &xnet_mr_fi_ops;
	}
	return ret;
}

static int
xnet_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
		uint64_t flags, struct fid_mr **mr_fid)
{
	struct xnet_domain *domain;
	struct ofi_mr *mr;
	int ret;

	domain = container_of(fid, struct xnet_domain,
			      util_domain.domain_fid.fid);
	ofi_genlock_lock(domain->progress.active_lock);
	ret = ofi_mr_regattr(fid, attr, flags, mr_fid);
	ofi_genlock_unlock(domain->progress.active_lock);

	if (!ret) {
		mr = container_of(*mr_fid, struct ofi_mr, mr_fid.fid);
		mr->mr_fid.fid.ops = &xnet_mr_fi_ops;
	}
	return ret;
}

static int
xnet_mplex_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
		uint64_t flags, struct fid_mr **mr_fid)
{
	struct xnet_domain *domain;
	struct fid_list_entry *item;
	struct fid_mr *sub_mr_fid;
	struct ofi_mr *mr;
	int ret;

	domain = container_of(fid, struct xnet_domain,
			      util_domain.domain_fid.fid);
	ret = ofi_mr_regattr(fid, attr, flags, mr_fid);
	if (ret)
		return ret;

	mr = container_of(*mr_fid, struct ofi_mr, mr_fid.fid);
	mr->mr_fid.fid.ops = &xnet_mplex_mr_fi_ops;

	ofi_genlock_lock(&domain->subdomain_list_lock);
	dlist_foreach_container(&domain->subdomain_list,
				struct fid_list_entry, item, entry) {
		ret = xnet_mr_regattr(item->fid, attr, flags, &sub_mr_fid);
		if (ret) {
			FI_WARN(&xnet_prov, FI_LOG_MR,
				"Failed to reg mr (%ld) from subdomain (%p)\n",
				mr->key, item->fid);

			xnet_subdomains_mr_close(domain, mr->key);
			(void) ofi_mr_close(&(*mr_fid)->fid);
			break;
		}
	}

	ofi_genlock_unlock(&domain->subdomain_list_lock);
	return ret;
}

static int
xnet_mplex_mr_regv(struct fid *fid, const struct iovec *iov,
		   size_t count, uint64_t access,
		   uint64_t offset, uint64_t requested_key,
		   uint64_t flags, struct fid_mr **mr_fid, void *context)
{
	struct fi_mr_attr attr;

	attr.mr_iov = iov;
	attr.iov_count = count;
	attr.access = access;
	attr.offset = offset;
	attr.requested_key = requested_key;
	attr.context = context;
	attr.iface = FI_HMEM_SYSTEM;
	attr.device.reserved = 0;
	attr.hmem_data = NULL;

	return xnet_mplex_mr_regattr(fid, &attr, flags, mr_fid);
}

static int
xnet_mplex_mr_reg(struct fid *fid, const void *buf, size_t len,
		  uint64_t access, uint64_t offset, uint64_t requested_key,
		  uint64_t flags, struct fid_mr **mr_fid, void *context)
{
	struct iovec iov;

	iov.iov_base = (void *) buf;
	iov.iov_len = len;

	return xnet_mplex_mr_regv(fid, &iov, 1, access, offset, requested_key,
				  flags, mr_fid, context);
}

static int xnet_open_ep(struct fid_domain *domain_fid, struct fi_info *info,
			struct fid_ep **ep_fid, void *context)
{
	struct xnet_domain *domain;

	domain = container_of(domain_fid, struct xnet_domain,
			      util_domain.domain_fid);
	if (domain->ep_type != info->ep_attr->type)
		return -FI_EINVAL;

	if (info->ep_attr->type == FI_EP_MSG)
		return xnet_endpoint(domain_fid, info, ep_fid, context);

	if (info->ep_attr->type == FI_EP_RDM)
		return xnet_rdm_ep(domain_fid, info, ep_fid, context);

	return -FI_EINVAL;
}

static int
xnet_query_atomic(struct fid_domain *domain, enum fi_datatype datatype,
		  enum fi_op op, struct fi_atomic_attr *attr, uint64_t flags)
{
	int ret;

	ret = ofi_atomic_valid(&xnet_prov, datatype, op, flags);
	if (ret || !attr)
		return ret;

	return -FI_EOPNOTSUPP;
}

static int xnet_domain_close(fid_t fid)
{
	struct xnet_domain *domain;
	int ret;

	domain = container_of(fid, struct xnet_domain,
			      util_domain.domain_fid.fid);

	xnet_del_domain_progress(domain);
	ret = ofi_domain_close(&domain->util_domain);
	if (ret)
		return ret;

	xnet_close_progress(&domain->progress);
	free(domain);
	return FI_SUCCESS;
}

static int xnet_mplex_domain_close(fid_t fid)
{
	struct xnet_domain *domain;
	struct fid_list_entry *item;

	domain = container_of(fid, struct xnet_domain, util_domain.domain_fid.fid);
	ofi_genlock_lock(&domain->subdomain_list_lock);
	while (!dlist_empty(&domain->subdomain_list)) {
		dlist_pop_front(&domain->subdomain_list, struct fid_list_entry,
				item, entry);
		(void)fi_close(item->fid);
		free(item);
	}
	ofi_genlock_unlock(&domain->subdomain_list_lock);

	ofi_genlock_destroy(&domain->subdomain_list_lock);
	ofi_domain_close(&domain->util_domain);
	free(domain);
	return FI_SUCCESS;
}

static struct fi_ops_domain xnet_mplex_domain_ops = {
	.size = sizeof(struct fi_ops_domain),
	.av_open = xnet_mplex_av_open,
	.cq_open = xnet_cq_open,
	.endpoint = xnet_open_ep,
	.scalable_ep = fi_no_scalable_ep,
	.cntr_open = xnet_cntr_open,
	.poll_open = fi_poll_create,
	.stx_ctx = fi_no_stx_context,
	.srx_ctx = xnet_srx_context,
	.query_atomic = xnet_query_atomic,
	.query_collective = fi_no_query_collective,
};

static struct fi_ops xnet_mplex_domain_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = xnet_mplex_domain_close,
	.bind = fi_no_bind,
	.control = fi_no_control,
	.ops_open = fi_no_ops_open,
	.tostr = fi_no_tostr,
	.ops_set = fi_no_ops_set,
};

static struct fi_ops_mr xnet_mplex_domain_fi_ops_mr = {
	.size = sizeof(struct fi_ops_mr),
	.reg = xnet_mplex_mr_reg,
	.regv = xnet_mplex_mr_regv,
	.regattr = xnet_mplex_mr_regattr,
};

int xnet_domain_multiplexed(struct fid_domain *domain_fid)
{
	return domain_fid->ops == &xnet_mplex_domain_ops;
}

static int xnet_domain_mplex_open(struct fid_fabric *fabric_fid, struct fi_info *info,
				  struct fid_domain **domain_fid, void *context)
{
	struct xnet_domain *domain;
	int ret;

	domain = calloc(1, sizeof(*domain));
	if (!domain)
		return -FI_ENOMEM;

	ret = ofi_domain_init(fabric_fid, info, &domain->util_domain, context,
			      OFI_LOCK_MUTEX);
	if (ret)
		goto free;

	ret = ofi_genlock_init(&domain->subdomain_list_lock, OFI_LOCK_MUTEX);
	if (ret)
		goto close;

	domain->subdomain_info = fi_dupinfo(info);
	if (!domain->subdomain_info) {
		ret = -FI_ENOMEM;
		goto free_lock;
	}

	domain->subdomain_info->domain_attr->threading = FI_THREAD_DOMAIN;

	dlist_init(&domain->subdomain_list);
	domain->ep_type = info->ep_attr->type;
	domain->util_domain.domain_fid.ops = &xnet_mplex_domain_ops;
	domain->util_domain.domain_fid.fid.ops = &xnet_mplex_domain_fi_ops;
	domain->util_domain.domain_fid.mr = &xnet_mplex_domain_fi_ops_mr;
	*domain_fid = &domain->util_domain.domain_fid;
	return FI_SUCCESS;

free_lock:
	ofi_genlock_destroy(&domain->subdomain_list_lock);
close:
	ofi_domain_close(&domain->util_domain);
free:
	free(domain);
	return ret;
}

static struct fi_ops_domain xnet_domain_ops = {
	.size = sizeof(struct fi_ops_domain),
	.av_open = xnet_av_open,
	.cq_open = xnet_cq_open,
	.endpoint = xnet_open_ep,
	.scalable_ep = fi_no_scalable_ep,
	.cntr_open = xnet_cntr_open,
	.poll_open = fi_poll_create,
	.stx_ctx = fi_no_stx_context,
	.srx_ctx = xnet_srx_context,
	.query_atomic = xnet_query_atomic,
	.query_collective = fi_no_query_collective,
};

static struct fi_ops xnet_domain_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = xnet_domain_close,
	.bind = ofi_domain_bind,
	.control = fi_no_control,
	.ops_open = fi_no_ops_open,
	.tostr = fi_no_tostr,
	.ops_set = fi_no_ops_set,
};

static struct fi_ops_mr xnet_domain_fi_ops_mr = {
	.size = sizeof(struct fi_ops_mr),
	.reg = xnet_mr_reg,
	.regv = xnet_mr_regv,
	.regattr = xnet_mr_regattr,
};

int xnet_domain_open(struct fid_fabric *fabric_fid, struct fi_info *info,
		     struct fid_domain **domain_fid, void *context)
{
	struct xnet_domain *domain;
	int ret;

	ret = ofi_prov_check_info(&xnet_util_prov, fabric_fid->api_version, info);
	if (ret)
		return ret;

	if (info->ep_attr->type == FI_EP_RDM &&
	    info->domain_attr->threading == FI_THREAD_COMPLETION)
		return xnet_domain_mplex_open(fabric_fid, info, domain_fid, context);

	domain = calloc(1, sizeof(*domain));
	if (!domain)
		return -FI_ENOMEM;

	ret = ofi_domain_init(fabric_fid, info, &domain->util_domain, context,
			      OFI_LOCK_NONE);
	if (ret)
		goto free;

	ret = xnet_init_progress(&domain->progress, info);
	if (ret)
		goto close;

	domain->ep_type = info->ep_attr->type;
	domain->util_domain.domain_fid.fid.ops = &xnet_domain_fi_ops;
	domain->util_domain.domain_fid.ops = &xnet_domain_ops;
	domain->util_domain.domain_fid.mr = &xnet_domain_fi_ops_mr;
	*domain_fid = &domain->util_domain.domain_fid;

	return FI_SUCCESS;

close:
	(void) ofi_domain_close(&domain->util_domain);
free:
	free(domain);
	return ret;
}

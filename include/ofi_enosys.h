/*
 * Copyright (c) 2014-2017 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL); Version 2, available from the file
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

#ifndef _OFI_ENOSYS_H_
#define _OFI_ENOSYS_H_

#include <ofi_osd.h>

#include <rdma/fabric.h>
#include <rdma/fi_atomic.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_collective.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
static struct fi_ops X = {
	.size = sizeof(struct fi_ops),
	.close = fi_no_close,
	.bind = fi_no_bind,
	.control = fi_no_control,
	.ops_open = fi_no_ops_open,
	.tostr = fi_no_ops_tostr,
	.ops_set = fi_no_ops_set,
};
 */
int fi_no_close(struct fid *fid);
int fi_no_bind(struct fid *fid, struct fid *bfid, uint64_t flags);
int fi_no_control(struct fid *fid, int command, void *arg);
int fi_no_ops_open(struct fid *fid, const char *name,
		uint64_t flags, void **ops, void *context);
int fi_no_tostr(const struct fid *fid, char *buf, size_t len);
int fi_no_ops_set(struct fid *fid, const char *name, uint64_t flags,
		  void *ops, void *context);

/*
static struct fi_ops_fabric X = {
	.size = sizeof(struct fi_ops_fabric),
	.domain = fi_no_domain,
	.passive_ep = fi_no_passive_ep,
	.eq_open = fi_no_eq_open,
	.wait_open = fi_no_wait_open,
	.trywait = fi_no_trywait,
	.domain2 = fi_no_domain2,
};
*/
int fi_no_domain(struct fid_fabric *fabric, struct fi_info *info,
		struct fid_domain **dom, void *context);
int fi_no_domain2(struct fid_fabric *fabric, struct fi_info *info,
		struct fid_domain **dom, uint64_t flags, void *context);
int fi_no_passive_ep(struct fid_fabric *fabric, struct fi_info *info,
		struct fid_pep **pep, void *context);
int fi_no_eq_open(struct fid_fabric *fabric, struct fi_eq_attr *attr,
		struct fid_eq **eq, void *context);
int fi_no_wait_open(struct fid_fabric *fabric, struct fi_wait_attr *attr,
		struct fid_wait **waitset);
int fi_no_trywait(struct fid_fabric *fabric, struct fid **fids, int count);

/*
static struct fi_ops_atomic X = {
	.size = sizeof(struct fi_ops_atomic),
	.write = fi_no_atomic_write,
	.writev = fi_no_atomic_writev,
	.writemsg = fi_no_atomic_writemsg,
	.inject = fi_no_atomic_inject,
	.readwrite = fi_no_atomic_readwrite,
	.readwritev = fi_no_atomic_readwritev,
	.readwritemsg = fi_no_atomic_readwritemsg,
	.compwrite = fi_no_atomic_compwrite,
	.compwritev = fi_no_atomic_compwritev,
	.compwritemsg = fi_no_atomic_compwritemsg,
	.writevalid = fi_no_atomic_writevalid,
	.readwritevalid = fi_no_atomic_readwritevalid,
	.compwritevalid = fi_no_atomic_compwritevalid,
};
*/
ssize_t fi_no_atomic_write(struct fid_ep *ep,
		const void *buf, size_t count, void *desc,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		enum fi_datatype datatype, enum fi_op op, void *context);
ssize_t fi_no_atomic_writev(struct fid_ep *ep,
		const struct fi_ioc *iov, void **desc, size_t count,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		enum fi_datatype datatype, enum fi_op op, void *context);
ssize_t fi_no_atomic_writemsg(struct fid_ep *ep,
		const struct fi_msg_atomic *msg, uint64_t flags);
ssize_t fi_no_atomic_inject(struct fid_ep *ep, const void *buf, size_t count,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		enum fi_datatype datatype, enum fi_op op);
ssize_t fi_no_atomic_readwrite(struct fid_ep *ep,
		const void *buf, size_t count, void *desc,
		void *result, void *result_desc,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		enum fi_datatype datatype, enum fi_op op, void *context);
ssize_t fi_no_atomic_readwritev(struct fid_ep *ep,
		const struct fi_ioc *iov, void **desc, size_t count,
		struct fi_ioc *resultv, void **result_desc, size_t result_count,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		enum fi_datatype datatype, enum fi_op op, void *context);
ssize_t fi_no_atomic_readwritemsg(struct fid_ep *ep,
		const struct fi_msg_atomic *msg,
		struct fi_ioc *resultv, void **result_desc, size_t result_count,
		uint64_t flags);
ssize_t fi_no_atomic_compwrite(struct fid_ep *ep,
		const void *buf, size_t count, void *desc,
		const void *compare, void *compare_desc,
		void *result, void *result_desc,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		enum fi_datatype datatype, enum fi_op op, void *context);
ssize_t fi_no_atomic_compwritev(struct fid_ep *ep,
		const struct fi_ioc *iov, void **desc, size_t count,
		const struct fi_ioc *comparev, void **compare_desc, size_t compare_count,
		struct fi_ioc *resultv, void **result_desc, size_t result_count,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		enum fi_datatype datatype, enum fi_op op, void *context);
ssize_t fi_no_atomic_compwritemsg(struct fid_ep *ep,
		const struct fi_msg_atomic *msg,
		const struct fi_ioc *comparev, void **compare_desc, size_t compare_count,
		struct fi_ioc *resultv, void **result_desc, size_t result_count,
		uint64_t flags);
int fi_no_atomic_writevalid(struct fid_ep *ep,
		enum fi_datatype datatype, enum fi_op op, size_t *count);
int fi_no_atomic_readwritevalid(struct fid_ep *ep,
		enum fi_datatype datatype, enum fi_op op, size_t *count);
int fi_no_atomic_compwritevalid(struct fid_ep *ep,
		enum fi_datatype datatype, enum fi_op op, size_t *count);

/*
static struct fi_ops_cm X = {
	.size = sizeof(struct fi_ops_cm),
	.setname = fi_no_setname,
	.getname = fi_no_getname,
	.getpeer = fi_no_getpeer,
	.connect = fi_no_connect,
	.listen = fi_no_listen,
	.accept = fi_no_accept,
	.reject = fi_no_reject,
	.shutdown = fi_no_shutdown,
	.join = fi_no_join,
};
*/
int fi_no_setname(fid_t fid, void *addr, size_t addrlen);
int fi_no_getname(fid_t fid, void *addr, size_t *addrlen);
int fi_no_getpeer(struct fid_ep *ep, void *addr, size_t *addrlen);
int fi_no_connect(struct fid_ep *ep, const void *addr,
		const void *param, size_t paramlen);
int fi_no_listen(struct fid_pep *pep);
int fi_no_accept(struct fid_ep *ep, const void *param, size_t paramlen);
int fi_no_reject(struct fid_pep *pep, fid_t handle,
		const void *param, size_t paramlen);
int fi_no_shutdown(struct fid_ep *ep, uint64_t flags);
int fi_no_join(struct fid_ep *ep, const void *addr, uint64_t flags,
		struct fid_mc **mc, void *context);

/*
static struct fi_ops_domain X = {
	.size = sizeof(struct fi_ops_domain),
	.av_open = fi_no_av_open,
	.cq_open = fi_no_cq_open,
	.endpoint = fi_no_endpoint,
	.scalable_ep = fi_no_scalable_ep,
	.cntr_open = fi_no_cntr_open,
	.poll_open = fi_no_poll_open,
	.stx_ctx = fi_no_stx_context,
	.srx_ctx = fi_no_srx_context,
	.query_atomic = fi_no_query_atomic,
	.query_collective = fi_no_query_collective,
	.endpoint2 = fi_no_endpoint2,
};
*/
int fi_no_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
		struct fid_av **av, void *context);
int fi_no_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
		struct fid_cq **cq, void *context);
int fi_no_endpoint(struct fid_domain *domain, struct fi_info *info,
		struct fid_ep **ep, void *context);
int fi_no_endpoint2(struct fid_domain *domain, struct fi_info *info,
		struct fid_ep **ep, uint64_t flags, void *context);
int fi_no_scalable_ep(struct fid_domain *domain, struct fi_info *info,
		struct fid_ep **sep, void *context);
int fi_no_cntr_open(struct fid_domain *domain, struct fi_cntr_attr *attr,
		struct fid_cntr **cntr, void *context);
int fi_no_poll_open(struct fid_domain *domain, struct fi_poll_attr *attr,
		struct fid_poll **pollset);
int fi_no_stx_context(struct fid_domain *domain, struct fi_tx_attr *attr,
		struct fid_stx **stx, void *context);
int fi_no_srx_context(struct fid_domain *domain, struct fi_rx_attr *attr,
		struct fid_ep **rx_ep, void *context);
int fi_no_query_atomic(struct fid_domain *domain, enum fi_datatype datatype,
		enum fi_op op, struct fi_atomic_attr *attr, uint64_t flags);
int fi_no_query_collective(struct fid_domain *domain, enum fi_collective_op coll,
			   struct fi_collective_attr *attr, uint64_t flags);

/*
static struct fi_ops_mr X = {
	.size = sizeof(struct fi_ops_mr),
	.reg = fi_no_mr_reg,
	.regv = fi_no_mr_regv,
	.regattr = fi_no_mr_regattr,
};
*/
int fi_no_mr_reg(struct fid *fid, const void *buf, size_t len,
		uint64_t access, uint64_t offset, uint64_t requested_key,
		uint64_t flags, struct fid_mr **mr, void *context);
int fi_no_mr_regv(struct fid *fid, const struct iovec *iov,
		size_t count, uint64_t access,
		uint64_t offset, uint64_t requested_key,
		uint64_t flags, struct fid_mr **mr, void *context);
int fi_no_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
		uint64_t flags, struct fid_mr **mr);

/*
static struct fi_ops_ep X = {
	.size = sizeof(struct fi_ops_ep),
	.cancel = fi_no_cancel,
	.getopt = fi_no_getopt,
	.setopt = fi_no_setopt,
	.tx_ctx = fi_no_tx_ctx,
	.rx_ctx = fi_no_rx_ctx,
	.rx_size_left = fi_no_rx_size_left,
	.tx_size_left = fi_no_tx_size_left,
};
*/
ssize_t fi_no_cancel(fid_t fid, void *context);
int fi_no_getopt(fid_t fid, int level, int optname,
		void *optval, size_t *optlen);
int fi_no_setopt(fid_t fid, int level, int optname,
		const void *optval, size_t optlen);
int fi_no_tx_ctx(struct fid_ep *sep, int index,
		struct fi_tx_attr *attr, struct fid_ep **tx_ep,
		void *context);
int fi_no_rx_ctx(struct fid_ep *sep, int index,
		struct fi_rx_attr *attr, struct fid_ep **rx_ep,
		void *context);
ssize_t fi_no_rx_size_left(struct fid_ep *ep);
ssize_t fi_no_tx_size_left(struct fid_ep *ep);

/*
static struct fi_ops_msg X = {
	.size = sizeof(struct fi_ops_msg),
	.recv = fi_no_msg_recv,
	.recvv = fi_no_msg_recvv,
	.recvmsg = fi_no_msg_recvmsg,
	.send = fi_no_msg_send,
	.sendv = fi_no_msg_sendv,
	.sendmsg = fi_no_msg_sendmsg,
	.inject = fi_no_msg_inject,
	.senddata = fi_no_msg_senddata,
	.injectdata = fi_no_msg_injectdata,
};
*/
ssize_t fi_no_msg_recv(struct fid_ep *ep, void *buf, size_t len, void *desc,
		fi_addr_t src_addr, void *context);
ssize_t fi_no_msg_recvv(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t src_addr, void *context);
ssize_t fi_no_msg_recvmsg(struct fid_ep *ep, const struct fi_msg *msg,
		uint64_t flags);
ssize_t fi_no_msg_send(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		fi_addr_t dest_addr, void *context);
ssize_t fi_no_msg_sendv(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t dest_addr, void *context);
ssize_t fi_no_msg_sendmsg(struct fid_ep *ep, const struct fi_msg *msg,
		uint64_t flags);
ssize_t fi_no_msg_inject(struct fid_ep *ep, const void *buf, size_t len,
		fi_addr_t dest_addr);
ssize_t fi_no_msg_senddata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		uint64_t data, fi_addr_t dest_addr, void *context);
ssize_t fi_no_msg_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		uint64_t data, fi_addr_t dest_addr);

/*
static struct fi_ops_wait X = {
	.size = sizeof(struct fi_ops_wait),
	.wait = X,
};
*/

/*
static struct fi_ops_poll X = {
	.size = sizeof(struct fi_ops_poll),
	.poll = X,
	.poll_add = X,
	.poll_del = X,
};
*/

/*
static struct fi_ops_eq X = {
	.size = sizeof(struct fi_ops_eq),
	.read = fi_no_eq_read,
	.readerr = fi_no_eq_readerr,
	.write = fi_no_eq_write,
	.sread = fi_no_eq_sread,
	.strerror = fi_no_eq_strerror,
};
*/
ssize_t fi_no_eq_read(struct fid_eq *eq, uint32_t *event, void *buf,
		size_t len, uint64_t flags);
ssize_t fi_no_eq_readerr(struct fid_eq *eq, struct fi_eq_err_entry *buf,
		uint64_t flags);
ssize_t fi_no_eq_write(struct fid_eq *eq, uint32_t event,
		const void *buf, size_t len, uint64_t flags);
ssize_t fi_no_eq_sread(struct fid_eq *eq, uint32_t *event,
		void *buf, size_t len, int timeout, uint64_t flags);
const char *fi_no_eq_strerror(struct fid_eq *eq, int prov_errno,
		const void *err_data, char *buf, size_t len);

/*
static struct fi_ops_cq X = {
	.size = sizeof(struct fi_ops_cq),
	.read = fi_no_cq_read,
	.readfrom = fi_no_cq_readfrom,
	.readerr = fi_no_cq_readerr,
	.sread = fi_no_cq_sread,
	.sreadfrom = fi_no_cq_sreadfrom,
	.signal = fi_no_cq_signal,
	.strerror = fi_no_cq_strerror,
};
*/
ssize_t fi_no_cq_read(struct fid_cq *cq, void *buf, size_t count);
ssize_t fi_no_cq_readfrom(struct fid_cq *cq, void *buf, size_t count,
		fi_addr_t *src_addr);
ssize_t fi_no_cq_readerr(struct fid_cq *cq, struct fi_cq_err_entry *buf,
		                uint64_t flags);
ssize_t fi_no_cq_sread(struct fid_cq *cq, void *buf, size_t count,
		const void *cond, int timeout);
ssize_t fi_no_cq_sreadfrom(struct fid_cq *cq, void *buf, size_t count,
		fi_addr_t *src_addr, const void *cond, int timeout);
int fi_no_cq_signal(struct fid_cq *cq);
const char * fi_no_cq_strerror(struct fid_cq *cq, int prov_errno,
		const void *err_data, char *buf, size_t len);

/*
static struct fi_ops_cntr X = {
	.size = sizeof(struct fi_ops_cntr),
	.read = X,
	.readerr = X,
	.add = fi_no_cntr_add,
	.set = fi_no_cntr_set,
	.wait = fi_no_cntr_wait,
};
*/
int fi_no_cntr_add(struct fid_cntr *cntr, uint64_t value);
int fi_no_cntr_set(struct fid_cntr *cntr, uint64_t value);
int fi_no_cntr_wait(struct fid_cntr *cntr, uint64_t threshold, int timeout);
uint64_t fi_no_cntr_readerr(struct fid_cntr *cntr);
int fi_no_cntr_adderr(struct fid_cntr *cntr, uint64_t value);
int fi_no_cntr_seterr(struct fid_cntr *cntr, uint64_t value);

/*
static struct fi_ops_rma X = {
	.size = sizeof(struct fi_ops_rma),
	.read = fi_no_rma_read,
	.readv = fi_no_rma_readv,
	.readmsg = fi_no_rma_readmsg,
	.write = fi_no_rma_write,
	.writev = fi_no_rma_writev,
	.writemsg = fi_no_rma_writemsg,
	.inject = fi_no_rma_inject,
	.writedata = fi_no_rma_writedata,
	.injectdata = fi_no_rma_injectdata,
};
*/
ssize_t fi_no_rma_read(struct fid_ep *ep, void *buf, size_t len, void *desc,
		fi_addr_t src_addr, uint64_t addr, uint64_t key, void *context);
ssize_t fi_no_rma_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
		void *context);
ssize_t fi_no_rma_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
		uint64_t flags);
ssize_t fi_no_rma_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context);
ssize_t fi_no_rma_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		void *context);
ssize_t fi_no_rma_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
		uint64_t flags);
ssize_t fi_no_rma_inject(struct fid_ep *ep, const void *buf, size_t len,
		fi_addr_t dest_addr, uint64_t addr, uint64_t key);
ssize_t fi_no_rma_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		uint64_t data, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		void *context);
ssize_t fi_no_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		uint64_t data, fi_addr_t dest_addr, uint64_t addr, uint64_t key);

/*
static struct fi_ops_tagged X = {
	.size = sizeof(struct fi_ops_tagged),
	.recv = fi_no_tagged_recv,
	.recvv = fi_no_tagged_recvv,
	.recvmsg = fi_no_tagged_recvmsg,
	.send = fi_no_tagged_send,
	.sendv = fi_no_tagged_sendv,
	.sendmsg = fi_no_tagged_sendmsg,
	.inject = fi_no_tagged_inject,
	.senddata = fi_no_tagged_senddata,
	.injectdata = fi_no_tagged_injectdata,
	.search = fi_no_tagged_search,
};
*/
ssize_t fi_no_tagged_recv(struct fid_ep *ep, void *buf, size_t len, void *desc,
		fi_addr_t src_addr, uint64_t tag, uint64_t ignore, void *context);
ssize_t fi_no_tagged_recvv(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t src_addr,
		uint64_t tag, uint64_t ignore, void *context);
ssize_t fi_no_tagged_recvmsg(struct fid_ep *ep, const struct fi_msg_tagged *msg,
		uint64_t flags);
ssize_t fi_no_tagged_send(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		fi_addr_t dest_addr, uint64_t tag, void *context);
ssize_t fi_no_tagged_sendv(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t dest_addr, uint64_t tag, void *context);
ssize_t fi_no_tagged_sendmsg(struct fid_ep *ep, const struct fi_msg_tagged *msg,
		uint64_t flags);
ssize_t fi_no_tagged_inject(struct fid_ep *ep, const void *buf, size_t len,
		fi_addr_t dest_addr, uint64_t tag);
ssize_t fi_no_tagged_senddata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		uint64_t data, fi_addr_t dest_addr, uint64_t tag, void *context);
ssize_t fi_no_tagged_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		uint64_t data, fi_addr_t dest_addr, uint64_t tag);
ssize_t fi_no_tagged_search(struct fid_ep *ep, uint64_t *tag, uint64_t ignore,
		uint64_t flags, fi_addr_t *src_addr, size_t *len, void *context);

/*
static struct fi_ops_av X = {
	.size = sizeof(struct fi_ops_av),
	.insert = fi_no_av_insert,
	.insertsvc = fi_no_av_insertsvc,
	.insertsym = fi_no_av_insertsym,
	.remove = fi_no_av_remove,
	.lookup = fi_no_av_lookup,
	.straddr = fi_no_av_straddr,
};
*/
int fi_no_av_insert(struct fid_av *av, const void *addr, size_t count,
			fi_addr_t *fi_addr, uint64_t flags, void *context);
int fi_no_av_insertsvc(struct fid_av *av, const char *node,
		const char *service, fi_addr_t *fi_addr, uint64_t flags,
		void *context);
int fi_no_av_insertsym(struct fid_av *av, const char *node, size_t nodecnt,
			const char *service, size_t svccnt, fi_addr_t *fi_addr,
			uint64_t flags, void *context);
int fi_no_av_remove(struct fid_av *av, fi_addr_t *fi_addr, size_t count,
			uint64_t flags);
int fi_no_av_lookup(struct fid_av *av, fi_addr_t fi_addr, void *addr,
		    size_t *addrlen);
const char *fi_no_av_straddr(struct fid_av *av, const void *addr, char *buf,
			     size_t *len);

/*
static struct fi_ops_av_set X = {
	.size = sizeof(struct fi_ops_av),
	.set_union = fi_no_av_set_union,
	.intersect = fi_no_av_set_intersect,
	.diff = fi_no_av_set_diff,
	.insert = fi_no_av_set_insert,
	.remove = fi_no_av_set_remove,
	.straddr = X,
};
*/
int fi_no_av_set(struct fid_av *av_fid, struct fi_av_set_attr *attr,
		 struct fid_av_set **av_set_fid, void *context);
int fi_no_av_set_union(struct fid_av_set *dst, const struct fid_av_set *src);
int fi_no_av_set_intersect(struct fid_av_set *dst, const struct fid_av_set *src);
int fi_no_av_set_diff(struct fid_av_set *dst, const struct fid_av_set *src);
int fi_no_av_set_insert(struct fid_av_set *set, fi_addr_t addr);
int fi_no_av_set_remove(struct fid_av_set *set, fi_addr_t addr);
int fi_no_av_set_addr(struct fid_av_set *set, fi_addr_t *coll_addr);

/*
static struct fi_ops_collective X = {
	.size = sizeof(struct fi_ops_collective),
	.barrier = fi_coll_no_barrier,
	.broadcast = fi_coll_no_broadcast,
	.alltoall = fi_coll_no_alltoall,
	.allreduce = fi_coll_no_allreduce,
	.allgather = fi_coll_no_allgather,
	.reduce_scatter = fi_coll_no_reduce_scatter,
	.reduce = fi_coll_no_reduce,
	.scatter = fi_coll_no_scatter,
	.gather = fi_coll_no_gather,
	.msg = fi_coll_no_msg,
};
*/
ssize_t fi_coll_no_barrier(struct fid_ep *ep, fi_addr_t coll_addr, void *context);
ssize_t fi_coll_no_barrier2(struct fid_ep *ep, fi_addr_t coll_addr, uint64_t flags,
			    void *context);
ssize_t fi_coll_no_broadcast(struct fid_ep *ep, void *buf, size_t count, void *desc,
			     fi_addr_t coll_addr, fi_addr_t root_addr,
			     enum fi_datatype datatype, uint64_t flags, void *context);
ssize_t fi_coll_no_alltoall(struct fid_ep *ep, const void *buf, size_t count, void *desc,
			    void *result, void *result_desc, fi_addr_t coll_addr,
			    enum fi_datatype datatype, uint64_t flags, void *context);
ssize_t fi_coll_no_allreduce(struct fid_ep *ep, const void *buf, size_t count, void *desc,
			     void *result, void *result_desc, fi_addr_t coll_addr,
			     enum fi_datatype datatype, enum fi_op op, uint64_t flags,
			     void *context);
ssize_t fi_coll_no_allgather(struct fid_ep *ep, const void *buf, size_t count, void *desc,
			     void *result, void *result_desc, fi_addr_t coll_addr,
			     enum fi_datatype datatype, uint64_t flags, void *context);
ssize_t fi_coll_no_reduce_scatter(struct fid_ep *ep, const void *buf, size_t count,
				  void *desc, void *result, void *result_desc,
				  fi_addr_t coll_addr, enum fi_datatype datatype,
				  enum fi_op op, uint64_t flags, void *context);
ssize_t fi_coll_no_reduce(struct fid_ep *ep, const void *buf, size_t count, void *desc,
			  void *result, void *result_desc, fi_addr_t coll_addr,
			  fi_addr_t root_addr, enum fi_datatype datatype, enum fi_op op,
			  uint64_t flags, void *context);
ssize_t fi_coll_no_scatter(struct fid_ep *ep, const void *buf, size_t count, void *desc,
			   void *result, void *result_desc, fi_addr_t coll_addr,
			   fi_addr_t root_addr, enum fi_datatype datatype, uint64_t flags,
			   void *context);
ssize_t fi_coll_no_gather(struct fid_ep *ep, const void *buf, size_t count, void *desc,
			  void *result, void *result_desc, fi_addr_t coll_addr,
			  fi_addr_t root_addr, enum fi_datatype datatype, uint64_t flags,
			  void *context);
ssize_t fi_coll_no_msg(struct fid_ep *ep, const struct fi_msg_collective *msg,
		       struct fi_ioc *resultv, void **result_desc, size_t result_count,
		       uint64_t flags);

#ifdef __cplusplus
}
#endif

#endif /* _OFI_ENOSYS_H_ */

/*
 * Copyright (c) 2015-2020 Intel Corporation. All rights reserved.
 * Copyright (c) 2017, Cisco Systems, Inc. All rights reserved.
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

#include "config.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <inttypes.h>

#if HAVE_GETIFADDRS
#include <net/if.h>
#include <ifaddrs.h>
#endif

#include <ofi_util.h>


enum {
	UTIL_NO_ENTRY = -1,
};

static int fi_get_src_sockaddr(const struct sockaddr *dest_addr, size_t dest_addrlen,
			       struct sockaddr **src_addr, size_t *src_addrlen)
{
	socklen_t len; /* needed for OS compatability */
	int sock, ret;

	sock = socket(dest_addr->sa_family, SOCK_DGRAM, 0);
	if (sock < 0)
		return -errno;

	ret = connect(sock, dest_addr, (socklen_t) dest_addrlen);
	if (ret)
		goto out;

	*src_addr = calloc(dest_addrlen, 1);
	if (!*src_addr) {
		ret = -FI_ENOMEM;
		goto out;
	}

	len = (socklen_t) dest_addrlen;
	ret = getsockname(sock, *src_addr, &len);
	if (ret) {
		ret = -errno;
		goto out;
	}
	*src_addrlen = len;

	switch ((*src_addr)->sa_family) {
	case AF_INET:
		((struct sockaddr_in *) (*src_addr))->sin_port = 0;
		break;
	case AF_INET6:
		((struct sockaddr_in6 *) (*src_addr))->sin6_port = 0;
		break;
	default:
		ret = -FI_ENOSYS;
		break;
	}

out:
	ofi_close_socket(sock);
	return ret;

}

void ofi_getnodename(uint16_t sa_family, char *buf, int buflen)
{
	int ret;
	struct addrinfo ai, *rai = NULL;
	struct ifaddrs *ifaddrs, *ifa;

	assert(buf && buflen > 0);
	ret = gethostname(buf, buflen);
	buf[buflen - 1] = '\0';
	if (ret == 0) {
		memset(&ai, 0, sizeof(ai));
		ai.ai_family = sa_family  ? sa_family : AF_INET;
		ret = getaddrinfo(buf, NULL, &ai, &rai);
		if (!ret) {
			freeaddrinfo(rai);
			return;
		}
	}

#if HAVE_GETIFADDRS
	ret = ofi_getifaddrs(&ifaddrs);
	if (!ret) {
		for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL || !(ifa->ifa_flags & IFF_UP))
				continue;

			if (sa_family) {
				if (ifa->ifa_addr->sa_family != sa_family)
					continue;
			} else if ((ifa->ifa_addr->sa_family != AF_INET) &&
				   (ifa->ifa_addr->sa_family != AF_INET6)) {
				continue;
			}

			ret = getnameinfo(ifa->ifa_addr,
					  (socklen_t) ofi_sizeofaddr(ifa->ifa_addr),
				  	  buf, buflen, NULL, 0, NI_NUMERICHOST);
			buf[buflen - 1] = '\0';
			if (ret == 0) {
				freeifaddrs(ifaddrs);
				return;
			}
		}
		freeifaddrs(ifaddrs);
	}
#endif
	/* no reasonable address found, use ipv4 loopback */
	strncpy(buf, "127.0.0.1", buflen);
	buf[buflen - 1] = '\0';
}

int ofi_get_src_addr(uint32_t addr_format,
		    const void *dest_addr, size_t dest_addrlen,
		    void **src_addr, size_t *src_addrlen)
{
	switch (addr_format) {
	case FI_SOCKADDR:
	case FI_SOCKADDR_IP:
	case FI_SOCKADDR_IN:
	case FI_SOCKADDR_IN6:
		return fi_get_src_sockaddr(dest_addr, dest_addrlen,
					   (struct sockaddr **) src_addr,
					   src_addrlen);
	default:
		return -FI_ENOSYS;
	}
}

static int fi_get_sockaddr(int *sa_family, uint64_t flags,
			   const char *node, const char *service,
			   struct sockaddr **addr, size_t *addrlen)
{
	struct addrinfo hints, *ai;
	int ret;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = *sa_family;
	hints.ai_socktype = SOCK_STREAM;
	if (flags & FI_SOURCE)
		hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(node, service, &hints, &ai);
	if (ret)
		return -FI_ENODATA;

	*addr = mem_dup(ai->ai_addr, ai->ai_addrlen);
	if (!*addr) {
		ret = -FI_ENOMEM;
		goto out;
	}

	*sa_family = ai->ai_family;
	*addrlen = ai->ai_addrlen;
out:
	freeaddrinfo(ai);
	return ret;
}

void ofi_get_str_addr(const char *node, const char *service,
		      char **addr, size_t *addrlen)
{
	if (!node || !strstr(node, "://"))
		return;

	*addr = strdup(node);
	*addrlen = strlen(node) + 1;
}

int ofi_get_addr(uint32_t *addr_format, uint64_t flags,
		const char *node, const char *service,
		void **addr, size_t *addrlen)
{
	int sa_family, ret;

	switch (*addr_format) {
	case FI_SOCKADDR:
	case FI_SOCKADDR_IP:
		sa_family = 0;
		ret = fi_get_sockaddr(&sa_family, flags, node, service,
				      (struct sockaddr **) addr, addrlen);
		if (ret)
			return ret;
		*addr_format = sa_family == AF_INET ?
			       FI_SOCKADDR_IN : FI_SOCKADDR_IN6;
		return 0;
	case FI_SOCKADDR_IN:
		sa_family = AF_INET;
		return fi_get_sockaddr(&sa_family, flags, node, service,
				       (struct sockaddr **) addr, addrlen);
	case FI_SOCKADDR_IN6:
		sa_family = AF_INET6;
		return fi_get_sockaddr(&sa_family, flags, node, service,
				       (struct sockaddr **) addr, addrlen);
	case FI_ADDR_STR:
		ofi_get_str_addr(node, service, (char **) addr, addrlen);
		return 0;
	default:
		return -FI_ENOSYS;
	}
}

void *ofi_av_get_addr(struct util_av *av, fi_addr_t fi_addr)
{
	struct util_av_entry *entry;

	entry = ofi_bufpool_get_ibuf(av->av_entry_pool, fi_addr);
	return entry->data;
}

void *ofi_av_addr_context(struct util_av *av, fi_addr_t fi_addr)
{
	void *addr;

	addr = ofi_av_get_addr(av, fi_addr);
	return (char *) addr + av->context_offset;
}

int ofi_verify_av_insert(struct util_av *av, uint64_t flags, void *context)
{
	if (flags & ~(FI_MORE | FI_SYNC_ERR | FI_FIREWALL_ADDR | FI_AV_USER_ID)) {
		FI_WARN(av->prov, FI_LOG_AV, "unsupported flags\n");
		return -FI_EBADFLAGS;
	}

	if ((flags & FI_SYNC_ERR) && !context) {
		FI_WARN(av->prov, FI_LOG_AV, "null context with FI_SYNC_ERR");
		return -FI_EINVAL;
	}

	return 0;
}

int ofi_av_insert_addr_at(struct util_av *av, const void *addr, fi_addr_t fi_addr)
{
	struct util_av_entry *entry = NULL;

	assert(ofi_genlock_held(&av->lock));
	ofi_av_straddr_log(av, FI_LOG_INFO, "inserting addr", addr);
	HASH_FIND(hh, av->hash, addr, av->addrlen, entry);
	if (entry) {
		if (fi_addr == ofi_buf_index(entry))
			return FI_SUCCESS;

		ofi_av_straddr_log(av, FI_LOG_WARN, "addr already in AV", addr);
		return -FI_EALREADY;
	}

	entry = ofi_ibuf_alloc_at(av->av_entry_pool, fi_addr);
	if (!entry)
		return -FI_ENOMEM;

	memcpy(entry->data, addr, av->addrlen);
	ofi_atomic_initialize32(&entry->use_cnt, 1);
	HASH_ADD(hh, av->hash, data, av->addrlen, entry);
	FI_INFO(av->prov, FI_LOG_AV, "fi_addr: %" PRIu64 "\n",
		ofi_buf_index(entry));
	return 0;
}

int ofi_av_insert_addr(struct util_av *av, const void *addr, fi_addr_t *fi_addr)
{
	struct util_av_entry *entry = NULL;

	assert(ofi_genlock_held(&av->lock));
	ofi_av_straddr_log(av, FI_LOG_INFO, "inserting addr", addr);
	HASH_FIND(hh, av->hash, addr, av->addrlen, entry);
	if (entry) {
		if (fi_addr)
			*fi_addr = ofi_buf_index(entry);
		if (ofi_atomic_inc32(&entry->use_cnt) > 1) {
			ofi_av_straddr_log(av, FI_LOG_WARN, "addr already in AV", addr);
		}
	} else {
		entry = ofi_ibuf_alloc(av->av_entry_pool);
		if (!entry) {
			if (fi_addr)
				*fi_addr = FI_ADDR_NOTAVAIL;
			return -FI_ENOMEM;
		}

		if (fi_addr)
			*fi_addr = ofi_buf_index(entry);
		memcpy(entry->data, addr, av->addrlen);
		ofi_atomic_initialize32(&entry->use_cnt, 1);
		HASH_ADD(hh, av->hash, data, av->addrlen, entry);
		FI_INFO(av->prov, FI_LOG_AV, "fi_addr: %" PRIu64 "\n",
			ofi_buf_index(entry));
	}
	return 0;
}

int ofi_av_remove_addr(struct util_av *av, fi_addr_t fi_addr)
{
	struct util_av_entry *av_entry;

	assert(ofi_genlock_held(&av->lock));
	av_entry = ofi_bufpool_get_ibuf(av->av_entry_pool, fi_addr);
	if (!av_entry)
		return -FI_ENOENT;

	if (ofi_atomic_dec32(&av_entry->use_cnt))
		return FI_SUCCESS;

	HASH_DELETE(hh, av->hash, av_entry);
	FI_DBG(av->prov, FI_LOG_AV, "av_remove fi_addr: %" PRIu64 "\n", fi_addr);
	ofi_ibuf_free(av_entry);
	return 0;
}

fi_addr_t ofi_av_lookup_fi_addr_unsafe(struct util_av *av, const void *addr)
{
	struct util_av_entry *entry = NULL;

	HASH_FIND(hh, av->hash, addr, av->addrlen, entry);
	return entry ? ofi_buf_index(entry) : FI_ADDR_NOTAVAIL;
}

fi_addr_t ofi_av_lookup_fi_addr(struct util_av *av, const void *addr)
{
	fi_addr_t fi_addr;
	ofi_genlock_lock(&av->lock);
	fi_addr = ofi_av_lookup_fi_addr_unsafe(av, addr);
	ofi_genlock_unlock(&av->lock);
	return fi_addr;
}

static void *
ofi_av_lookup_addr(struct util_av *av, fi_addr_t fi_addr, size_t *addrlen)
{
	*addrlen = av->addrlen;
	return ofi_av_get_addr(av, fi_addr);
}

static void util_av_close(struct util_av *av)
{
	HASH_CLEAR(hh, av->hash);
	ofi_bufpool_destroy(av->av_entry_pool);
}

int ofi_av_close_lightweight(struct util_av *av)
{
	if (ofi_atomic_get32(&av->ref)) {
		FI_WARN(av->prov, FI_LOG_AV, "AV is busy\n");
		return -FI_EBUSY;
	}

	ofi_genlock_destroy(&av->ep_list_lock);

	ofi_atomic_dec32(&av->domain->ref);
	ofi_genlock_destroy(&av->lock);

	return 0;
}

int ofi_av_close(struct util_av *av)
{
	int ret;

	ofi_genlock_lock(&av->lock);
	if (av->av_set) {
		ret = fi_close(&av->av_set->av_set_fid.fid);
		if (ret) {
			ofi_genlock_unlock(&av->lock);
			return ret;
		}
		av->av_set = NULL;
	}
	ofi_genlock_unlock(&av->lock);

	ret = ofi_av_close_lightweight(av);
	if (ret)
		return ret;

	util_av_close(av);
	return 0;
}

size_t ofi_av_size(struct util_av *av)
{
	return av->av_entry_pool->entry_cnt ?
	       av->av_entry_pool->entry_cnt :
	       av->av_entry_pool->attr.chunk_cnt;
}

static int util_verify_av_util_attr(struct util_domain *domain,
				    const struct util_av_attr *util_attr)
{
	if (util_attr->flags & ~(OFI_AV_DYN_ADDRLEN)) {
		FI_WARN(domain->prov, FI_LOG_AV, "invalid internal flags\n");
		return -FI_EINVAL;
	}

	return 0;
}

static int util_av_init(struct util_av *av, const struct fi_av_attr *attr,
			const struct util_av_attr *util_attr)
{
	int ret = 0;
	size_t orig_size;
	size_t offset;

	/* offset calculated on a 8-byte boundary */
	offset = util_attr->addrlen % 8;
	if (offset != 0)
		offset = 8 - offset;
	struct ofi_bufpool_attr pool_attr = {
		.size		= util_attr->addrlen + offset +
				  util_attr->context_len +
				  sizeof(struct util_av_entry),
		.alignment	= 16,
		.max_cnt	= 0,
		/* Don't use track of buffer, because user can close
		 * the AV without prior deletion of addresses */
		.flags		= OFI_BUFPOOL_NO_TRACK | OFI_BUFPOOL_INDEXED,
	};

	/* TODO: Handle FI_READ */
	/* TODO: Handle mmap - shared AV */

	ret = util_verify_av_util_attr(av->domain, util_attr);
	if (ret)
		return ret;

	orig_size = attr->count ? attr->count : ofi_universe_size;
	orig_size = roundup_power_of_two(orig_size);
	FI_INFO(av->prov, FI_LOG_AV, "AV size %zu\n", orig_size);

	av->addrlen = util_attr->addrlen;
	av->context_offset = offset + av->addrlen;
	av->flags = util_attr->flags | attr->flags;
	av->hash = NULL;

	pool_attr.chunk_cnt = orig_size;
	return ofi_bufpool_create_attr(&pool_attr, &av->av_entry_pool);
}

static int util_verify_av_attr(struct util_domain *domain,
			       const struct fi_av_attr *attr)
{
	char str1[20], str2[20];

	switch (attr->type) {
	case FI_AV_MAP:
	case FI_AV_TABLE:
		if ((domain->av_type != FI_AV_UNSPEC) &&
		    (attr->type != domain->av_type)) {
			fi_tostr_r(str1, sizeof(str1), &domain->av_type,
				   FI_TYPE_AV_TYPE),
			fi_tostr_r(str2, sizeof(str2), &attr->type,
				   FI_TYPE_AV_TYPE);
			FI_WARN(domain->prov, FI_LOG_AV,
				"Invalid AV type. domain->av_type: %s "
				"attr->type: %s\n", str1, str2);
			return -FI_EINVAL;
		}
		break;
	default:
		FI_WARN(domain->prov, FI_LOG_AV, "Invalid AV type\n");
		return -FI_EINVAL;
	}

	if (attr->name) {
		FI_WARN(domain->prov, FI_LOG_AV, "Shared AV is unsupported\n");
		return -FI_ENOSYS;
	}

	if (attr->flags & ~(FI_EVENT | FI_READ | FI_SYMMETRIC | FI_PEER)) {
		FI_WARN(domain->prov, FI_LOG_AV, "invalid flags\n");
		return -FI_EINVAL;
	}

	return 0;
}

int ofi_av_init_lightweight(struct util_domain *domain, const struct fi_av_attr *attr,
			    struct util_av *av, void *context)
{
	int ret;
	enum ofi_lock_type av_lock_type, ep_list_lock_type;

	ret = util_verify_av_attr(domain, attr);
	if (ret)
		return ret;

	ofi_atomic_initialize32(&av->ref, 0);

	av->av_fid.fid.fclass = FI_CLASS_AV;
	/*
	 * ops set by provider
	 * av->av_fid.fid.ops = &prov_av_fi_ops;
	 * av->av_fid.ops = &prov_av_ops;
	 */
	av->context = context;
	av->domain = domain;
	av->prov = domain->prov;

	av_lock_type = domain->threading == FI_THREAD_DOMAIN &&
				       domain->control_progress ==
					       FI_PROGRESS_CONTROL_UNIFIED ?
			       OFI_LOCK_NOOP :
			       OFI_LOCK_MUTEX;

	ret = ofi_genlock_init(&av->lock, av_lock_type);
	if (ret)
		return ret;

	ep_list_lock_type = ofi_progress_lock_type(domain->threading,
						   domain->control_progress);

	ret = ofi_genlock_init(&av->ep_list_lock, ep_list_lock_type);
	if (ret)
		return ret;

	dlist_init(&av->ep_list);
	ofi_atomic_inc32(&domain->ref);
	return 0;
}

int ofi_av_init(struct util_domain *domain, const struct fi_av_attr *attr,
		const struct util_av_attr *util_attr,
		struct util_av *av, void *context)
{
	int ret = ofi_av_init_lightweight(domain, attr, av, context);
	if (ret)
		return ret;

	ret = util_av_init(av, attr, util_attr);
	if (ret)
		return ret;
	return ret;
}


/*************************************************************************
 *
 * AV for IP addressing
 *
 *************************************************************************/

fi_addr_t ofi_ip_av_get_fi_addr(struct util_av *av, const void *addr)
{
	return ofi_av_lookup_fi_addr(av, addr);
}

static int ip_av_insert_addr(struct util_av *av, const void *addr,
			     fi_addr_t *fi_addr, void *context)
{
	int ret;

	if (ofi_valid_dest_ipaddr(addr)) {
		ofi_genlock_lock(&av->lock);
		ret = ofi_av_insert_addr(av, addr, fi_addr);
		ofi_genlock_unlock(&av->lock);
	} else {
		ret = -FI_EADDRNOTAVAIL;
		if (fi_addr)
			*fi_addr = FI_ADDR_NOTAVAIL;
		FI_WARN(av->prov, FI_LOG_AV, "invalid address\n");
	}

	ofi_straddr_dbg(av->prov, FI_LOG_AV, "av_insert addr", addr);
	if (fi_addr)
		FI_DBG(av->prov, FI_LOG_AV, "av_insert fi_addr: %" PRIu64 "\n",
		       *fi_addr);

	return ret;
}

int ofi_ip_av_insertv(struct util_av *av, const void *addr, size_t addrlen,
		      size_t count, fi_addr_t *fi_addr, uint64_t flags,
		      void *context)
{
	int ret, success_cnt = 0;
	int *sync_err = NULL;
	size_t i;

	if (!count)
		goto done;

	if (addrlen > av->addrlen) {
		FI_WARN(av->prov, FI_LOG_AV, "Address too large for AV\n");
		return -FI_EINVAL;
	}

	if (!(av->flags & OFI_AV_DYN_ADDRLEN)) {
		av->addrlen = addrlen;
		av->flags &= ~OFI_AV_DYN_ADDRLEN;
	}
	assert(av->addrlen == addrlen);

	FI_DBG(av->prov, FI_LOG_AV, "inserting %zu addresses\n", count);
	if (flags & FI_SYNC_ERR) {
		sync_err = context;
		memset(sync_err, 0, sizeof(*sync_err) * count);
	}

	for (i = 0; i < count; i++) {
		ret = ip_av_insert_addr(av, (const char *) addr + i * addrlen,
					fi_addr ? &fi_addr[i] : NULL, context);
		if (!ret)
			success_cnt++;
		else if (sync_err)
			sync_err[i] = -ret;
	}

done:
	FI_DBG(av->prov, FI_LOG_AV, "%d addresses successful\n", success_cnt);
	ret = success_cnt;
	return ret;
}

int ofi_ip_av_insert(struct fid_av *av_fid, const void *addr, size_t count,
		     fi_addr_t *fi_addr, uint64_t flags, void *context)
{
	struct util_av *av;
	int ret;

	av = container_of(av_fid, struct util_av, av_fid);
	ret = ofi_verify_av_insert(av, flags, context);
	if (ret)
		return ret;

	return ofi_ip_av_insertv(av, addr, count ? ofi_sizeofaddr(addr) : 0,
				 count, fi_addr, flags, context);
}

int ofi_ip_av_insertsvc(struct fid_av *av, const char *node,
			const char *service, fi_addr_t *fi_addr,
			uint64_t flags, void *context)
{
	return fi_av_insertsym(av, node, 1, service, 1, fi_addr, flags, context);
}

/* Caller should free *addr */
static int
ip_av_ip4sym_getaddr(struct util_av *av, struct in_addr ip, size_t ipcnt,
		     uint16_t port, size_t portcnt, void **addr, size_t *addrlen)
{
	struct sockaddr_in *sin;
	size_t count = ipcnt * portcnt;
	size_t i, p, k;

	*addrlen = sizeof(*sin);
	sin = calloc(count, *addrlen);
	if (!sin)
		return -FI_ENOMEM;

	for (i = 0, k = 0; i < ipcnt; i++) {
		for (p = 0; p < portcnt; p++, k++) {
			sin[k].sin_family = AF_INET;
			/* TODO: should we skip addresses x.x.x.0 and x.x.x.255? */
			sin[k].sin_addr.s_addr = htonl(ntohl(ip.s_addr) + (uint32_t) i);
			sin[k].sin_port = htons(port + (uint16_t) p);
		}
	}
	*addr = sin;
	return (int) count;
}

/* Caller should free *addr */
static int
ip_av_ip6sym_getaddr(struct util_av *av, struct in6_addr ip, size_t ipcnt,
		     uint16_t port, size_t portcnt, void **addr, size_t *addrlen)
{
	struct sockaddr_in6 *sin6, sin6_temp;
	int j, count = (int)(ipcnt * portcnt);
	size_t i, p, k;

	*addrlen = sizeof(*sin6);
	sin6 = calloc(count, *addrlen);
	if (!sin6)
		return -FI_ENOMEM;

	sin6_temp.sin6_addr = ip;

	for (i = 0, k = 0; i < ipcnt; i++) {
		for (p = 0; p < portcnt; p++, k++) {
			sin6[k].sin6_family = AF_INET6;
			sin6[k].sin6_addr = sin6_temp.sin6_addr;
			sin6[k].sin6_port = htons((uint16_t)(port + p));
		}
		/* TODO: should we skip addresses x::0 and x::255? */
		for (j = 15; j >= 0; j--) {
			if (++sin6_temp.sin6_addr.s6_addr[j] < 255)
				break;
		}
	}
	*addr = sin6;
	return count;
}

/* Caller should free *addr */
static int ip_av_nodesym_getaddr(struct util_av *av, const char *node,
				 size_t nodecnt, const char *service,
				 size_t svccnt, void **addr, size_t *addrlen)
{
	struct addrinfo hints, *ai;
	void *addr_temp;
	char name[OFI_NAME_MAX];
	char svc[OFI_NAME_MAX];
	size_t name_len, n, s;
	int ret, name_index, svc_index, count = (int)(nodecnt * svccnt);

	memset(&hints, 0, sizeof hints);

	hints.ai_socktype = SOCK_DGRAM;
	switch (av->domain->addr_format) {
	case FI_SOCKADDR_IN:
		hints.ai_family = AF_INET;
		*addrlen = sizeof(struct sockaddr_in);
		break;
	case FI_SOCKADDR_IN6:
		hints.ai_family = AF_INET6;
		*addrlen = sizeof(struct sockaddr_in6);
		break;
	default:
		FI_INFO(av->prov, FI_LOG_AV, "Unknown address format!\n");
		return -FI_EINVAL;
	}

	*addr = calloc(nodecnt * svccnt, *addrlen);
	if (!*addr)
		return -FI_ENOMEM;

	addr_temp = *addr;

	for (name_len = strlen(node); isdigit(node[name_len - 1]); )
		name_len--;

	memcpy(name, node, name_len);
	name_index = atoi(node + name_len);
	svc_index = atoi(service);

	for (n = 0; n < nodecnt; n++) {
		if (nodecnt == 1) {
			strncpy(name, node, sizeof(name) - 1);
			name[OFI_NAME_MAX - 1] = '\0';
		} else {
			snprintf(name + name_len, sizeof(name) - name_len - 1,
				 "%zu", name_index + n);
		}

		for (s = 0; s < svccnt; s++) {
			if (svccnt == 1) {
				strncpy(svc, service, sizeof(svc) - 1);
				svc[OFI_NAME_MAX - 1] = '\0';
			} else {
				snprintf(svc, sizeof(svc) - 1,
					 "%zu", svc_index + s);
			}
			FI_INFO(av->prov, FI_LOG_AV, "resolving %s:%s for AV "
				"insert\n", node, service);

			ret = getaddrinfo(node, service, &hints, &ai);
			if (ret) {
				ret = -abs(ret);
				goto err;
			}

			memcpy(addr_temp, ai->ai_addr, *addrlen);
			addr_temp = (char *)addr_temp + *addrlen;
			freeaddrinfo(ai);
		}
	}
	return count;
err:
	free(*addr);
	return ret;
}

/* Caller should free *addr */
int ofi_ip_av_sym_getaddr(struct util_av *av, const char *node,
			  size_t nodecnt, const char *service,
			  size_t svccnt, void **addr, size_t *addrlen)
{
	struct in6_addr ip6;
	struct in_addr ip4;
	int ret;

	if (strlen(node) >= OFI_NAME_MAX || strlen(service) >= OFI_NAME_MAX) {
		FI_WARN(av->prov, FI_LOG_AV,
			"node or service name is too long\n");
		return -FI_ENOSYS;
	}

	ret = inet_pton(AF_INET, node, &ip4);
	if (ret == 1) {
		FI_INFO(av->prov, FI_LOG_AV, "insert symmetric IPv4\n");
		return ip_av_ip4sym_getaddr(av, ip4, nodecnt,
					  (uint16_t) strtol(service, NULL, 0),
					  svccnt, addr, addrlen);
	}

	ret = inet_pton(AF_INET6, node, &ip6);
	if (ret == 1) {
		FI_INFO(av->prov, FI_LOG_AV, "insert symmetric IPv6\n");
		return ip_av_ip6sym_getaddr(av, ip6, nodecnt,
					  (uint16_t) strtol(service, NULL, 0),
					  svccnt, addr, addrlen);
	}

	FI_INFO(av->prov, FI_LOG_AV, "insert symmetric host names\n");
	return ip_av_nodesym_getaddr(av, node, nodecnt, service,
				     svccnt, addr, addrlen);
}

int ofi_ip_av_insertsym(struct fid_av *av_fid, const char *node,
			size_t nodecnt, const char *service, size_t svccnt,
			fi_addr_t *fi_addr, uint64_t flags, void *context)
{
	struct util_av *av;
	void *addr;
	size_t addrlen;
	int ret, count;

	av = container_of(av_fid, struct util_av, av_fid);
	ret = ofi_verify_av_insert(av, flags, context);
	if (ret)
		return ret;

	count = ofi_ip_av_sym_getaddr(av, node, nodecnt, service,
				      svccnt, &addr, &addrlen);
	if (count <= 0)
		return count;

	ret = ofi_ip_av_insertv(av, addr, addrlen, count,
				fi_addr, flags, context);
	free(addr);
	return ret;
}

int ofi_ip_av_remove(struct fid_av *av_fid, fi_addr_t *fi_addr,
		     size_t count, uint64_t flags)
{
	struct util_av *av;
	ssize_t i;
	int ret;

	av = container_of(av_fid, struct util_av, av_fid);
	if (flags) {
		FI_WARN(av->prov, FI_LOG_AV, "invalid flags\n");
		return -FI_EINVAL;
	}

	/*
	 * It's more efficient to remove addresses from high to low index.
	 * We assume that addresses are removed in the same order that they were
	 * added -- i.e. fi_addr passed in here was also passed into insert.
	 * Thus, we walk through the array backwards.
	 */
	for (i = count - 1; i >= 0; i--) {
		ofi_genlock_lock(&av->lock);
		ret = ofi_av_remove_addr(av, fi_addr[i]);
		ofi_genlock_unlock(&av->lock);
		if (ret) {
			FI_WARN(av->prov, FI_LOG_AV,
				"removal of fi_addr %"PRIu64" failed\n",
				fi_addr[i]);
		}
	}
	return 0;
}

bool ofi_ip_av_is_valid(struct fid_av *av_fid, fi_addr_t fi_addr)
{
	struct util_av *av =
		container_of(av_fid, struct util_av, av_fid);

	return ofi_bufpool_ibuf_is_valid(av->av_entry_pool, fi_addr);
}

int ofi_ip_av_lookup(struct fid_av *av_fid, fi_addr_t fi_addr,
		     void *addr, size_t *addrlen)
{
	struct util_av *av =
		container_of(av_fid, struct util_av, av_fid);
	size_t av_addrlen;
	void *av_addr = ofi_av_lookup_addr(av, fi_addr, &av_addrlen);

	memcpy(addr, av_addr, MIN(*addrlen, av_addrlen));
	*addrlen = av->addrlen;

	return 0;
}

const char *
ofi_ip_av_straddr(struct fid_av *av, const void *addr, char *buf, size_t *len)
{
	return ofi_straddr(buf, len, FI_SOCKADDR, addr);
}

static struct fi_ops_av ip_av_ops = {
	.size = sizeof(struct fi_ops_av),
	.insert = ofi_ip_av_insert,
	.insertsvc = ofi_ip_av_insertsvc,
	.insertsym = ofi_ip_av_insertsym,
	.remove = ofi_ip_av_remove,
	.lookup = ofi_ip_av_lookup,
	.straddr = ofi_ip_av_straddr,
};

static int ip_av_close(struct fid *av_fid)
{
	struct util_av *av;
	int ret;

	av = container_of(av_fid, struct util_av, av_fid.fid);
	ret = ofi_av_close(av);
	if (ret)
		return ret;
	free(av);
	return 0;
}

static struct fi_ops ip_av_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = ip_av_close,
	.bind = fi_no_bind,
	.control = fi_no_control,
	.ops_open = fi_no_ops_open,
};

int ofi_ip_av_create(struct fid_domain *domain_fid, struct fi_av_attr *attr,
		     struct fid_av **av, void *context)
{
	struct util_domain *domain;
	struct util_av_attr util_attr = { 0 };
	struct util_av *util_av;
	int ret;

	domain = container_of(domain_fid, struct util_domain, domain_fid);

	if (domain->addr_format == FI_SOCKADDR_IN) {
		util_attr.addrlen = sizeof(struct sockaddr_in);
	} else if (domain->addr_format == FI_SOCKADDR_IN6) {
		util_attr.addrlen = sizeof(struct sockaddr_in6);
	} else {
		util_attr.addrlen = sizeof(struct sockaddr_in6);
		util_attr.flags = OFI_AV_DYN_ADDRLEN;
	}

	if (attr->type == FI_AV_UNSPEC)
		attr->type = FI_AV_MAP;

	util_av = calloc(1, sizeof(*util_av));
	if (!util_av)
		return -FI_ENOMEM;

	ret = ofi_av_init(domain, attr, &util_attr, util_av, context);
	if (ret) {
		free(util_av);
		return ret;
	}

	*av = &util_av->av_fid;
	(*av)->fid.ops = &ip_av_fi_ops;
	(*av)->ops = &ip_av_ops;
	return 0;
}

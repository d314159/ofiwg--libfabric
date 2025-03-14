---
layout: page
title: fi_tagged(3)
tagline: Libfabric Programmer's Manual
---
{% include JB/setup %}

# NAME

fi_tagged \- Tagged data transfer operations

fi_trecv / fi_trecvv / fi_trecvmsg
:   Post a buffer to receive an incoming message

fi_tsend / fi_tsendv / fi_tsendmsg / fi_tinject / fi_tsenddata
:   Initiate an operation to send a message

# SYNOPSIS

```c
#include <rdma/fi_tagged.h>

ssize_t fi_trecv(struct fid_ep *ep, void *buf, size_t len, void *desc,
	fi_addr_t src_addr, uint64_t tag, uint64_t ignore, void *context);

ssize_t fi_trecvv(struct fid_ep *ep, const struct iovec *iov, void **desc,
	size_t count, fi_addr_t src_addr, uint64_t tag, uint64_t ignore,
	void *context);

ssize_t fi_trecvmsg(struct fid_ep *ep, const struct fi_msg_tagged *msg,
	uint64_t flags);

ssize_t fi_tsend(struct fid_ep *ep, const void *buf, size_t len,
	void *desc, fi_addr_t dest_addr, uint64_t tag, void *context);

ssize_t fi_tsendv(struct fid_ep *ep, const struct iovec *iov,
	void **desc, size_t count, fi_addr_t dest_addr, uint64_t tag,
	void *context);

ssize_t fi_tsendmsg(struct fid_ep *ep, const struct fi_msg_tagged *msg,
	uint64_t flags);

ssize_t fi_tinject(struct fid_ep *ep, const void *buf, size_t len,
	fi_addr_t dest_addr, uint64_t tag);

ssize_t fi_tsenddata(struct fid_ep *ep, const void *buf, size_t len,
	void *desc, uint64_t data, fi_addr_t dest_addr, uint64_t tag,
	void *context);

ssize_t fi_tinjectdata(struct fid_ep *ep, const void *buf, size_t len,
	uint64_t data, fi_addr_t dest_addr, uint64_t tag);
```

# ARGUMENTS

*fid*
: Fabric endpoint on which to initiate tagged communication operation.

*buf*
: Data buffer to send or receive.

*len*
: Length of data buffer to send or receive, specified in bytes.  Valid
  transfers are from 0 bytes up to the endpoint's max_msg_size.

*iov*
: Vectored data buffer.

*count*
: Count of vectored data entries.

*tag*
: Tag associated with the message.

*ignore*
: Mask of bits to ignore applied to the tag for receive operations.

*desc*
: Memory descriptor associated with the data buffer.
  See [`fi_mr`(3)](fi_mr.3.html).

*data*
: Remote CQ data to transfer with the sent data.

*dest_addr*
: Destination address for connectionless transfers.  Ignored for
  connected endpoints.

*src_addr*
: Applies only to connectionless endpoints configured with the FI_DIRECTED_RECV.
  For all other endpoint configurations, src_addr is ignored. src_addr defines
  the source address to receive from. By default, the src_addr is treated as a
  source endpoint address (i.e. fi_addr_t returned from fi_av_insert /
  fi_av_insertsvc / fi_av_remove). If the FI_AUTH_KEY flag is specified with
  fi_trecvmsg, src_addr is treated as a source authorization key (i.e. fi_addr_t
  returned from fi_av_insert_auth_key). If set to FI_ADDR_UNSPEC, any source
  address may match.

*msg*
: Message descriptor for send and receive operations.

*flags*
: Additional flags to apply for the send or receive operation.

*context*
: User specified pointer to associate with the operation.  This parameter is
  ignored if the operation will not generate a successful completion, unless
  an op flag specifies the context parameter be used for required input.

# DESCRIPTION

Tagged messages are data transfers which carry a key or tag with the
message buffer.  The tag is used at the receiving endpoint to match
the incoming message with a corresponding receive buffer.  Message
tags match when the receive buffer tag is the same as the send buffer
tag with the ignored bits masked out.  This can be stated as:

```c
send_tag & ~ignore == recv_tag & ~ignore

or

send_tag | ignore == recv_tag | ignore
```

In general, message tags are checked against receive buffers in the
order in which messages have been posted to the endpoint.  See the
ordering discussion below for more details.

The send functions -- fi_tsend, fi_tsendv, fi_tsendmsg,
fi_tinject, and fi_tsenddata -- are used
to transmit a tagged message from one endpoint to another endpoint.
The main difference between send functions are the number and type of
parameters that they accept as input.  Otherwise, they perform the
same general function.

The receive functions -- fi_trecv, fi_trecvv, fi_recvmsg
-- post a data buffer to an endpoint to receive inbound tagged
messages.  Similar to the send operations, receive operations operate
asynchronously.  Users should not touch the posted data buffer(s)
until the receive operation has completed.  Posted receive buffers are
matched with inbound send messages based on the tags associated with
the send and receive buffers.

An endpoint must be enabled before an application can post send
or receive operations to it.  For connected endpoints, receive
buffers may be posted prior to connect or accept being called on
the endpoint.  This ensures that buffers are available to receive
incoming data immediately after the connection has been established.

Completed message operations are reported to the user through one or
more event collectors associated with the endpoint.  Users provide
context which are associated with each operation, and is returned to
the user as part of the event completion.  See fi_cq for completion
event details.

## fi_tsend

The call fi_tsend transfers the data contained in the user-specified
data buffer to a remote endpoint, with message boundaries being
maintained.  The local endpoint must be connected to a remote endpoint
or destination before fi_tsend is called.  Unless the endpoint has
been configured differently, the data buffer passed into fi_tsend must
not be touched by the application until the fi_tsend call completes
asynchronously.

## fi_tsendv

The fi_tsendv call adds support for a scatter-gather list to fi_tsend.
The fi_sendv transfers the set of data buffers
referenced by the iov parameter to a remote endpoint as a single
message.

## fi_tsendmsg

The fi_tsendmsg call supports data transfers over both connected and
connectionless endpoints, with the ability to control the send operation
per call through the use of flags.  The fi_tsendmsg function takes a
struct fi_msg_tagged as input.

```c
struct fi_msg_tagged {
	const struct iovec *msg_iov; /* scatter-gather array */
	void               *desc;    /* data descriptor */
	size_t             iov_count;/* # elements in msg_iov */
	fi_addr_t          addr;    /* optional endpoint address */
	uint64_t           tag;      /* tag associated with message */
	uint64_t           ignore;   /* mask applied to tag for receives */
	void               *context; /* user-defined context */
	uint64_t           data;     /* optional immediate data */
};
```

## fi_tinject

The tagged inject call is an optimized version of fi_tsend.  It provides
similar completion semantics as fi_inject [`fi_msg`(3)](fi_msg.3.html).

## fi_tsenddata

The tagged send data call is similar to fi_tsend, but allows for the
sending of remote CQ data (see FI_REMOTE_CQ_DATA flag) as part of the
transfer.

## fi_tinjectdata

The tagged inject data call is similar to fi_tinject, but allows for the
sending of remote CQ data (see FI_REMOTE_CQ_DATA flag) as part of the
transfer.

## fi_trecv

The fi_trecv call posts a data buffer to the receive queue of the
corresponding endpoint.  Posted receives are searched in the order in
which they were posted in order to match sends.  Message boundaries are
maintained.  The order in which the receives complete is dependent on
the endpoint type and protocol.

## fi_trecvv

The fi_trecvv call adds support for a scatter-gather list to fi_trecv.
The fi_trecvv posts the set of data buffers referenced by the iov
parameter to a receive incoming data.

## fi_trecvmsg

The fi_trecvmsg call supports posting buffers over both connected and
connectionless endpoints, with the ability to control the receive
operation per call through the use of flags.  The fi_trecvmsg function
takes a struct fi_msg_tagged as input.

# FLAGS

The fi_trecvmsg and fi_tsendmsg calls allow the user to specify flags
which can change the default message handling of the endpoint.  Flags
specified with fi_trecvmsg / fi_tsendmsg override most flags
previously configured with the endpoint, except where noted (see
fi_endpoint).  The following list of flags are usable with fi_trecvmsg
and/or fi_tsendmsg.

*FI_REMOTE_CQ_DATA*
: Applies to fi_tsendmsg.  Indicates that remote CQ data is available
  and should be sent as part of the request.  See fi_getinfo for
  additional details on FI_REMOTE_CQ_DATA.  This flag is implicitly
  set for fi_tsenddata and fi_tinjectdata.

*FI_COMPLETION*
: Indicates that a completion entry should be generated for the
  specified operation.  The endpoint must be bound to a completion queue
  with FI_SELECTIVE_COMPLETION that corresponds to the specified operation,
  or this flag is ignored.

*FI_MORE*
: Indicates that the user has additional requests that will
  immediately be posted after the current call returns.  Use of this
  flag may improve performance by enabling the provider to optimize
  its access to the fabric hardware.

*FI_INJECT*
: Applies to fi_tsendmsg.  Indicates that the outbound data buffer
  should be returned to user immediately after the send call returns,
  even if the operation is handled asynchronously.  This may require
  that the underlying provider implementation copy the data into a
  local buffer and transfer out of that buffer. This flag can only
  be used with messages smaller than inject_size.

*FI_MULTI_RECV*
: Applies to posted tagged receive operations when the FI_TAGGED_MULTI_RECV
  capability is enabled.  This flag allows the user to post a single
  tagged receive buffer that will receive multiple incoming messages.
  Received messages will be packed into the receive buffer until the
  buffer has been consumed.  Use of this flag may cause a single
  posted receive operation to generate multiple events as messages are
  placed into the buffer.  The placement of received data into the
  buffer may be subjected to provider specific alignment restrictions.

  The buffer will be released by the provider when the available buffer
  space falls below the specified minimum (see FI_OPT_MIN_MULTI_RECV).
  Note that an entry to the associated receive completion queue will
  always be generated when the buffer has been consumed, even if other
  receive completions have been suppressed (i.e. the Rx context has been
  configured for FI_SELECTIVE_COMPLETION).  See the FI_MULTI_RECV
  completion flag [`fi_cq`(3)](fi_cq.3.html).

*FI_INJECT_COMPLETE*
: Applies to fi_tsendmsg.  Indicates that a completion should be
  generated when the source buffer(s) may be reused.

*FI_TRANSMIT_COMPLETE*
: Applies to fi_tsendmsg.  Indicates that a completion should not be
  generated until the operation has been successfully transmitted and
  is no longer being tracked by the provider.

*FI_MATCH_COMPLETE*
: Applies to fi_tsendmsg.  Indicates that a completion should be generated
  only after the message has either been matched with a tagged
  buffer or was discarded by the target application.

*FI_FENCE*
: Applies to transmits.  Indicates that the requested operation, also
  known as the fenced operation, and any operation posted after the
  fenced operation will be deferred until all previous operations
  targeting the same peer endpoint have completed.  Operations posted
  after the fencing will see and/or replace the results of any
  operations initiated prior to the fenced operation.

  The ordering of operations starting at the posting of the fenced
  operation (inclusive) to the posting of a subsequent fenced operation
  (exclusive) is controlled by the endpoint's ordering semantics.

*FI_AUTH_KEY*
: Only valid with domains configured with FI_AV_AUTH_KEY and connectionless
  endpoints configured with FI_DIRECTED_RECV or FI_TAGGED_DIRECTED_RECV. When
  used with fi_trecvmsg, this flag denotes that the src_addr is an authorization
  key fi_addr_t instead of an endpoint fi_addr_t.

The following flags may be used with fi_trecvmsg.

*FI_PEEK*
: The peek flag may be used to see if a specified message has arrived.
  A peek request is often useful on endpoints that have provider
  allocated buffering enabled.
  Unlike standard receive operations, a receive operation with the FI_PEEK
  flag set does not remain queued with the provider after the peek completes
  successfully. The peek operation operates asynchronously, and the results
  of the peek operation are available in the completion queue associated with
  the endpoint. If no message is found matching the tags specified in the peek
  request, then a completion queue error entry with err field set to FI_ENOMSG
  will be available.

  If a peek request locates a matching message, the operation will complete
  successfully.  The returned completion data will indicate the meta-data
  associated with the message, such as the message length, completion flags,
  available CQ data, tag, and source address.  The data available is subject to
  the completion entry format (e.g. struct fi_cq_tagged_entry).

*FI_CLAIM*
: If this flag is used in conjunction with FI_PEEK, it indicates if the
  peek request completes successfully -- indicating that a matching message
  was located -- the message is claimed by caller.  Claimed messages can only
  be retrieved using a subsequent, paired receive operation with the FI_CLAIM
  flag set.  A receive operation with the FI_CLAIM flag set, but FI_PEEK not
  set is used to retrieve a previously claimed message.

  In order to use the FI_CLAIM flag, an application must supply a struct
  fi_context structure as the context for the receive operation.  The same
  fi_context structure used for an FI_PEEK + FI_CLAIM operation must be used
  by the paired FI_CLAIM request.

*FI_DISCARD*
: This flag may be used in conjunction with either FI_PEEK or FI_CLAIM.
  If this flag is used in conjunction with FI_PEEK, it indicates if the
  peek request completes successfully -- indicating that a matching message
  was located -- the message is discarded by the provider, as the data is not
  needed by the application.  This flag may also be used in conjunction with
  FI_CLAIM in order to discard a message previously claimed
  using an FI_PEEK + FI_CLAIM request.

  If this flag is set, the input buffer(s) and length parameters are ignored.

# RETURN VALUE

The tagged send and receive calls return 0 on success.  On error, a
negative value corresponding to fabric _errno _ is returned. Fabric
errno values are defined in `fi_errno.h`.

# ERRORS

*-FI_EAGAIN*
: See [`fi_msg`(3)](fi_msg.3.html) for a detailed description of handling
  FI_EAGAIN.

*-FI_EINVAL*
: Indicates that an invalid argument was supplied by the user.

*-FI_EOTHER*
: Indicates that an unspecified error occurred.

# SEE ALSO

[`fi_getinfo`(3)](fi_getinfo.3.html),
[`fi_endpoint`(3)](fi_endpoint.3.html),
[`fi_domain`(3)](fi_domain.3.html),
[`fi_cq`(3)](fi_cq.3.html)

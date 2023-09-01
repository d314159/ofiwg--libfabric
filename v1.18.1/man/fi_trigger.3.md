---
layout: page
title: fi_trigger(3)
tagline: Libfabric Programmer's Manual
---
{% include JB/setup %}

# NAME

fi_trigger - Triggered operations

# SYNOPSIS

```c
#include <rdma/fi_trigger.h>
```

# DESCRIPTION

Triggered operations allow an application to queue a data transfer
request that is deferred until a specified condition is met.  A typical
use is to send a message only after receiving all input data.  Triggered
operations can help reduce the latency needed to initiate a transfer by
removing the need to return control back to an application prior to
the data transfer starting.

An endpoint must be created with the FI_TRIGGER capability in order
for triggered operations to be specified.  A triggered operation is
requested by specifying the FI_TRIGGER flag as part of the operation.
Such an endpoint is referred to as a trigger-able endpoint.

Any data transfer operation is potentially trigger-able, subject to
provider constraints.  Trigger-able endpoints are initialized such that
only those interfaces supported by the provider which are trigger-able
are available.

Triggered operations require that applications use struct
fi_triggered_context as their per operation context parameter, or if
the provider requires the FI_CONTEXT2 mode, struct fi_trigger_context2.  The
use of struct fi_triggered_context[2] replaces struct fi_context[2], if
required by the provider.  Although struct fi_triggered_context[2] is not
opaque to the application, the contents of the structure may be
modified by the provider once it has been submitted as an operation.
This structure has similar requirements as struct fi_context[2].  It
must be allocated by the application and remain valid until the
corresponding operation completes or is successfully canceled.

Struct fi_triggered_context[2] is used to specify the condition that must
be met before the triggered data transfer is initiated.  If the
condition is met when the request is made, then the data transfer may
be initiated immediately.  The format of struct fi_triggered_context[2]
is described below.

```c
struct fi_triggered_context {
	enum fi_trigger_event event_type;   /* trigger type */
	union {
		struct fi_trigger_threshold threshold;
		struct fi_trigger_xpu xpu;
		void *internal[3]; /* reserved */
	} trigger;
};

struct fi_triggered_context2 {
	enum fi_trigger_event event_type;   /* trigger type */
	union {
		struct fi_trigger_threshold threshold;
		struct fi_trigger_xpu xpu;
		void *internal[7]; /* reserved */
	} trigger;
};
```

The triggered context indicates the type of event assigned to the
trigger, along with a union of trigger details that is based on the
event type.

# COMPLETION BASED TRIGGERS

Completion based triggers defer a data transfer until one or more
related data transfers complete.  For example, a send operation may
be deferred until a receive operation completes, indicating that the
data to be transferred is now available.

The following trigger event related to completion based transfers
is defined.

*FI_TRIGGER_THRESHOLD*
: This indicates that the data transfer operation will be deferred
  until an event counter crosses an application specified threshold
  value.  The threshold is specified using struct fi_trigger_threshold:

```c
struct fi_trigger_threshold {
	struct fid_cntr *cntr; /* event counter to check */
	size_t threshold;      /* threshold value */
};
```

  Threshold operations are triggered in the order of the threshold
  values.  This is true even if the counter increments by a value
  greater than 1.  If two triggered operations have the same threshold,
  they will be triggered in the order in which they were submitted to
  the endpoint.

# XPU TRIGGERS

XPU based triggers work in conjunction with heterogenous memory (FI_HMEM
capability).  XPU triggers define a split execution model for specifying
a data transfer separately from initiating the transfer.  Unlike completion
triggers, the user controls the timing of when the transfer starts by
writing data into a trigger variable location.

XPU transfers allow the requesting and triggering to occur on separate
computational domains.  For example, a process running on the host CPU can
setup a data transfer, with a compute kernel running on a GPU signaling
the start of the transfer.  XPU refers to a CPU, GPU, FPGA, or other
acceleration device with some level of computational ability.

Endpoints must be created with both the FI_TRIGGER and FI_XPU capabilities
to use XPU triggers.  XPU triggered enabled endpoints only support XPU
triggered operations.  The behavior of mixing XPU triggered operations with
normal data transfers or non-XPU triggered operations is not defined by
the API and subject to provider support and implementation.

The use of XPU triggers requires coordination between the fabric provider,
application, and submitting XPU.  The result is that hardware
implementation details need to be conveyed across the computational domains.
The XPU trigger API abstracts those details.  When submitting a XPU trigger
operation, the user identifies the XPU where the triggering will
occur.  The triggering XPU must match with the location of the local memory
regions.  For example, if triggering will be done by a GPU kernel, the
type of GPU and its local identifier are given.  As output, the fabric
provider will return a list of variables and corresponding values.
The XPU signals that the data transfer is safe to initiate by writing
the given values to the specified variable locations.  The number of
variables and their sizes are provider specific.

XPU trigger operations are submitted using the FI_TRIGGER flag with
struct fi_triggered_context or struct fi_triggered_context2, as
required by the provider.  The trigger event_type is:

*FI_TRIGGER_XPU*
: Indicates that the data transfer operation will be deferred until
  the user writes provider specified data to provider indicated
  memory locations.  The user indicates which device will initiate
  the write.  The struct fi_trigger_xpu is used to convey both
  input and output data regarding the signaling of the trigger.

```c
struct fi_trigger_var {
	enum fi_datatype datatype;
	int count;
	void *addr;
	union {
		uint8_t val8;
		uint16_t val16;
		uint32_t val32;
		uint64_t val64;
		uint8_t *data;
	} value;
};

struct fi_trigger_xpu {
	int count;
	enum fi_hmem_iface iface;
	union {
		uint64_t reserved;
		int cuda;
		int ze;
	} device;
	struct fi_trigger_var *var;
};
```

On input to a triggered operation, the iface field indicates the software
interface that will be used to write the variables.  The device union
specifies the device identifier.  For valid iface and device values, see
[`fi_mr`(3)](fi_mr.3.html).  The iface and device must match with the
iface and device of any local HMEM memory regions.  Count should be set
to the number of fi_trigger_var structures available, with the var field
pointing to an array of struct fi_trigger_var.  The user is responsible for
ensuring that there are sufficient fi_trigger_var structures available and of
an appropriate size.  The count and size of fi_trigger_var structures
can be obtained by calling fi_getopt() on the endpoint with the
FI_OPT_XPU_TRIGGER option.  See [`fi_endpoint`(3)](fi_endpoint.3.html)
for details.

Each fi_trigger_var structure referenced should have the datatype
and count fields initialized to the number of values referenced by the
struct fi_trigger_val.  If the count is 1, one of the val fields will be used
to return the necessary data (val8, val16, etc.).  If count > 1, the data
field will return all necessary data used to signal the trigger.  The data
field must reference a buffer large enough to hold the returned bytes.

On output, the provider will set the fi_trigger_xpu count to the number of
fi_trigger_var variables that must be signaled.  Count will be less than or
equal to the input value.  The provider will initialize each valid
fi_trigger_var entry with information needed to signal the trigger.  The
datatype indicates the size of the data that must be written.  Valid datatype
values are FI_UINT8, FI_UINT16, FI_UINT32, and FI_UINT64.  For signal
variables <= 64 bits, the count field will be 1.  If a trigger requires writing
more than 64-bits, the datatype field will be set to FI_UINT8, with count set
to the number of bytes that must be written.  The data that must be written
to signal the start of an operation is returned through either the value
union val fields or data array.

Users signal the start of a transfer by writing the returned data to the
given memory address.  The write must occur from the specified input XPU
location (based on the iface and device fields).  If a transfer cannot
be initiated for some reason, such as an error occurring before the
transfer can start, the triggered operation should
be canceled to release any allocated resources.  If multiple variables are
specified, they must be updated in order.

Note that the provider will not modify the fi_trigger_xpu or fi_trigger_var
structures after returning from the data transfer call.

In order to support multiple provider implementations, users should trigger
data transfer operations in the same order that they are queued and should
serialize the writing of triggers that reference the same endpoint.  Providers
may return the same trigger variable for multiple data transfer requests.

# DEFERRED WORK QUEUES

The following feature and description are enhancements to triggered
operation support.

The deferred work queue interface is designed as primitive constructs
that can be used to implement application-level collective operations.
They are a more advanced form of triggered operation.  They
allow an application to queue operations to a deferred work queue
that is associated with the domain.  Note that the deferred work queue
is a conceptual construct, rather than an implementation requirement.
Deferred work requests consist of three main components: an event or
condition that must first be met, an operation to perform, and a
completion notification.

Because deferred work requests are posted directly to the domain, they
can support a broader set of conditions and operations.  Deferred
work requests are submitted using struct fi_deferred_work.  That structure,
along with the corresponding operation structures (referenced through
the op union) used to describe the work must remain valid until the
operation completes or is canceled.  The format of the deferred work
request is as follows:

```c
struct fi_deferred_work {
	struct fi_context2    context;

	uint64_t              threshold;
	struct fid_cntr       *triggering_cntr;
	struct fid_cntr       *completion_cntr;

	enum fi_trigger_op    op_type;

	union {
		struct fi_op_msg            *msg;
		struct fi_op_tagged         *tagged;
		struct fi_op_rma            *rma;
		struct fi_op_atomic         *atomic;
		struct fi_op_fetch_atomic   *fetch_atomic;
		struct fi_op_compare_atomic *compare_atomic;
		struct fi_op_cntr           *cntr;
	} op;
};

```

Once a work request has been posted to the deferred work queue, it will
remain on the queue until the triggering counter (success plus error
counter values) has reached the indicated threshold.  If the triggering
condition has already been met at the time the work request is queued,
the operation will be initiated immediately.

On the completion of a deferred data transfer, the specified completion
counter will be incremented by one.  Note that deferred counter operations do
not update the completion counter; only the counter specified through the
fi_op_cntr is modified.  The completion_cntr field must be NULL for counter
operations.

Because deferred work targets support of collective communication operations,
posted work requests do not generate any completions at the endpoint by
default.  For example, completed operations are not written to the EP's
completion queue or update the EP counter (unless the EP counter is
explicitly referenced as the completion_cntr).  An application may request
EP completions by specifying the FI_COMPLETION flag as part of the
operation.

It is the responsibility of the application to detect and handle situations
that occur which could result in a deferred work request's condition not
being met.  For example, if a work request is dependent upon the successful
completion of a data transfer operation, which fails, then the application
must cancel the work request.

To submit a deferred work request, applications should use the domain's
fi_control function with command FI_QUEUE_WORK and struct fi_deferred_work
as the fi_control arg parameter.  To cancel a deferred work request, use
fi_control with command FI_CANCEL_WORK and the corresponding struct
fi_deferred_work to cancel.  The fi_control command FI_FLUSH_WORK will
cancel all queued work requests.  FI_FLUSH_WORK may be used to flush all
work queued to the domain, or may be used to cancel all requests waiting
on a specific triggering_cntr.

Deferred work requests are not acted upon by the provider until the
associated event has occurred; although, certain validation checks
may still occur when a request is submitted.  Referenced data buffers are
not read or otherwise accessed.  But the provider may validate fabric
objects, such as endpoints and counters, and that input parameters fall
within supported ranges.  If a specific request is not supported by the
provider, it will fail the operation with -FI_ENOSYS.

# SEE ALSO

[`fi_getinfo`(3)](fi_getinfo.3.html),
[`fi_endpoint`(3)](fi_endpoint.3.html),
[`fi_mr`(3)](fi_mr.3.html),
[`fi_alias`(3)](fi_alias.3.html),
[`fi_cntr`(3)](fi_cntr.3.html)
/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2019 Philippe Proulx <pproulx@efficios.com>
 */

#define BT_LOG_TAG "LIB/MSG-DISCARDED-ITEMS"
#include "lib/logging.h"

#include <stdbool.h>

#include "lib/assert-cond.h"
#include "lib/object.h"
#include "compat/compiler.h"
#include <babeltrace2/trace-ir/clock-class.h>
#include "lib/trace-ir/clock-snapshot.h"
#include "lib/trace-ir/stream-class.h"
#include "lib/trace-ir/stream.h"
#include "lib/property.h"
#include "lib/graph/message/message.h"

#include "discarded-items.h"

#define BT_ASSERT_PRE_MSG_IS_DISC_EVENTS(_msg)				\
	BT_ASSERT_PRE_MSG_HAS_TYPE("message", (_msg),			\
		"discarded-events", BT_MESSAGE_TYPE_DISCARDED_EVENTS);

#define BT_ASSERT_PRE_DEV_MSG_IS_DISC_EVENTS(_msg)			\
	BT_ASSERT_PRE_DEV_MSG_HAS_TYPE("message", (_msg),		\
		"discarded-events", BT_MESSAGE_TYPE_DISCARDED_EVENTS);

#define BT_ASSERT_PRE_MSG_IS_DISC_PACKETS(_msg)				\
	BT_ASSERT_PRE_MSG_HAS_TYPE("message", (_msg),			\
		"discarded-packets", BT_MESSAGE_TYPE_DISCARDED_PACKETS);

#define BT_ASSERT_PRE_DEV_MSG_IS_DISC_PACKETS(_msg)			\
	BT_ASSERT_PRE_DEV_MSG_HAS_TYPE("message", (_msg),		\
		"discarded-packets", BT_MESSAGE_TYPE_DISCARDED_PACKETS);

#define BT_ASSERT_PRE_DEV_COUNT_OUTPUT_NON_NULL(_count)			\
	BT_ASSERT_PRE_DEV_NON_NULL("count-output", count,		\
		"Count (output)");

static
void destroy_discarded_items_message(struct bt_object *obj)
{
	struct bt_message_discarded_items *message = (void *) obj;

	BT_LIB_LOGD("Destroying discarded items message: %!+n", message);
	BT_LIB_LOGD("Putting stream: %!+s", message->stream);
	BT_OBJECT_PUT_REF_AND_RESET(message->stream);

	if (message->default_begin_cs) {
		bt_clock_snapshot_recycle(message->default_begin_cs);
		message->default_begin_cs = NULL;
	}

	if (message->default_end_cs) {
		bt_clock_snapshot_recycle(message->default_end_cs);
		message->default_end_cs = NULL;
	}

	g_free(message);
}

static inline
struct bt_message *create_discarded_items_message(
		struct bt_self_message_iterator *self_msg_iter,
		enum bt_message_type type, struct bt_stream *stream,
		bool with_cs,
		uint64_t beginning_raw_value, uint64_t end_raw_value,
		const char *api_func,
		const char *sc_supports_disc_precond_id)
{
	struct bt_message_discarded_items *message;
	struct bt_stream_class *stream_class;
	bool has_support;
	bool need_cs;

	BT_ASSERT_PRE_MSG_ITER_NON_NULL_FROM_FUNC(api_func, self_msg_iter);
	BT_ASSERT_PRE_STREAM_NON_NULL_FROM_FUNC(api_func, stream);
	stream_class = bt_stream_borrow_class(stream);
	BT_ASSERT(stream_class);

	if (type == BT_MESSAGE_TYPE_DISCARDED_EVENTS) {
		has_support = stream_class->supports_discarded_events;
		need_cs = stream_class->discarded_events_have_default_clock_snapshots;
	} else {
		has_support = stream_class->supports_discarded_packets;
		need_cs = stream_class->discarded_packets_have_default_clock_snapshots;
	}

	BT_ASSERT_PRE_FROM_FUNC(api_func, sc_supports_disc_precond_id,
		has_support,
		"Stream class does not support discarded events or packets: "
		"type=%s, %![stream-]+s, %![sc-]+S",
		bt_common_message_type_string(type), stream, stream_class);
	BT_ASSERT_PRE_FROM_FUNC(api_func, "with-default-clock-snapshots",
		need_cs ? with_cs : true,
		"Unexpected stream class configuration when creating "
		"a discarded events or discarded packets message: "
		"default clock snapshots are needed, but none was provided: "
		"type=%s, %![stream-]+s, %![sc-]+S, with-cs=%d, "
		"cs-begin-val=%" PRIu64 ", cs-end-val=%" PRIu64,
		bt_common_message_type_string(type), stream, stream_class,
		with_cs, beginning_raw_value, end_raw_value);
	BT_ASSERT_PRE_FROM_FUNC(api_func, "without-default-clock-snapshots",
		!need_cs ? !with_cs : true,
		"Unexpected stream class configuration when creating "
		"a discarded events or discarded packets message: "
		"no default clock snapshots are needed, but two were provided: "
		"type=%s, %![stream-]+s, %![sc-]+S, with-cs=%d, "
		"cs-begin-val=%" PRIu64 ", cs-end-val=%" PRIu64,
		bt_common_message_type_string(type), stream, stream_class,
		with_cs, beginning_raw_value, end_raw_value);
	BT_LIB_LOGD("Creating discarded items message object: "
		"type=%s, %![stream-]+s, %![sc-]+S, with-cs=%d, "
		"cs-begin-val=%" PRIu64 ", cs-end-val=%" PRIu64,
		bt_common_message_type_string(type), stream, stream_class,
		with_cs, beginning_raw_value, end_raw_value);
	message = g_new0(struct bt_message_discarded_items, 1);
	if (!message) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one discarded items message.");
		goto error;
	}

	bt_message_init(&message->parent, type,
		destroy_discarded_items_message, NULL);
	message->stream = stream;
	bt_object_get_ref_no_null_check(message->stream);

	if (with_cs) {
		BT_ASSERT(stream_class->default_clock_class);
		message->default_begin_cs = bt_clock_snapshot_create(
			stream_class->default_clock_class);
		if (!message->default_begin_cs) {
			goto error;
		}

		bt_clock_snapshot_set_raw_value(message->default_begin_cs,
			beginning_raw_value);

		message->default_end_cs = bt_clock_snapshot_create(
			stream_class->default_clock_class);
		if (!message->default_end_cs) {
			goto error;
		}

		bt_clock_snapshot_set_raw_value(message->default_end_cs,
			end_raw_value);
	}

	bt_property_uint_init(&message->count,
		BT_PROPERTY_AVAILABILITY_NOT_AVAILABLE, 0);
	BT_LIB_LOGD("Created discarded items message object: "
		"%![msg-]+n, %![stream-]+s, %![sc-]+S", message,
		stream, stream_class);

	return (void *) &message->parent;

error:
	return NULL;
}

static inline
struct bt_stream *borrow_discarded_items_message_stream(
		struct bt_message *message)
{
	struct bt_message_discarded_items *disc_items_msg = (void *) message;

	BT_ASSERT_DBG(message);
	return disc_items_msg->stream;
}

static inline
void set_discarded_items_message_count(struct bt_message *message,
		uint64_t count)
{
	struct bt_message_discarded_items *disc_items_msg = (void *) message;

	BT_ASSERT(message);
	bt_property_uint_set(&disc_items_msg->count, count);
}

static inline
enum bt_property_availability get_discarded_items_message_count(
		const struct bt_message *message, uint64_t *count)
{
	struct bt_message_discarded_items *disc_items_msg = (void *) message;

	BT_ASSERT_DBG(message);
	BT_ASSERT_DBG(count);
	*count = disc_items_msg->count.value;
	return disc_items_msg->count.base.avail;
}

static inline
const struct bt_clock_snapshot *
borrow_discarded_items_message_beginning_default_clock_snapshot_const(
		const struct bt_message *message, const char *api_func)
{
	struct bt_message_discarded_items *disc_items_msg = (void *) message;

	BT_ASSERT_DBG(message);
	BT_ASSERT_PRE_DEV_MSG_SC_DEF_CLK_CLS_FROM_FUNC(api_func, message,
		disc_items_msg->stream->class);
	return disc_items_msg->default_begin_cs;
}

static inline
const struct bt_clock_snapshot *
borrow_discarded_items_message_end_default_clock_snapshot_const(
		const struct bt_message *message, const char *api_func)
{
	struct bt_message_discarded_items *disc_items_msg = (void *) message;

	BT_ASSERT_DBG(message);
	BT_ASSERT_PRE_DEV_MSG_SC_DEF_CLK_CLS_FROM_FUNC(api_func, message,
		disc_items_msg->stream->class);
	return disc_items_msg->default_end_cs;
}

#define SC_SUPPORTS_DISC_PRECOND_ID(_item_type)				\
	"stream-class-supports-discarded-" _item_type

struct bt_message *bt_message_discarded_events_create(
		struct bt_self_message_iterator *message_iterator,
		const struct bt_stream *stream)
{
	BT_ASSERT_PRE_DEV_NO_ERROR();

	return create_discarded_items_message(message_iterator,
		BT_MESSAGE_TYPE_DISCARDED_EVENTS, (void *) stream,
		false, 0, 0, __func__,
		SC_SUPPORTS_DISC_PRECOND_ID("events"));
}

struct bt_message *bt_message_discarded_events_create_with_default_clock_snapshots(
		struct bt_self_message_iterator *message_iterator,
		const struct bt_stream *stream, uint64_t beginning_raw_value,
		uint64_t end_raw_value)
{
	BT_ASSERT_PRE_DEV_NO_ERROR();
	BT_ASSERT_PRE_MSG_CS_BEGIN_LE_END(message_iterator,
		beginning_raw_value, end_raw_value);

	return create_discarded_items_message(message_iterator,
		BT_MESSAGE_TYPE_DISCARDED_EVENTS, (void *) stream,
		true, beginning_raw_value, end_raw_value, __func__,
		SC_SUPPORTS_DISC_PRECOND_ID("events"));
}

struct bt_stream *bt_message_discarded_events_borrow_stream(
		struct bt_message *message)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(message);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_EVENTS(message);
	return borrow_discarded_items_message_stream(message);
}

void bt_message_discarded_events_set_count(struct bt_message *message,
		uint64_t count)
{
	BT_ASSERT_PRE_MSG_NON_NULL(message);
	BT_ASSERT_PRE_MSG_IS_DISC_EVENTS(message);
	BT_ASSERT_PRE_DEV_MSG_HOT(message);
	BT_ASSERT_PRE("count-gt-0", count > 0, "Discarded event count is 0.");
	set_discarded_items_message_count(message, count);
}

const struct bt_clock_snapshot *
bt_message_discarded_events_borrow_beginning_default_clock_snapshot_const(
		const struct bt_message *msg)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(msg);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_EVENTS(msg);
	return borrow_discarded_items_message_beginning_default_clock_snapshot_const(
		msg, __func__);
}

const struct bt_clock_snapshot *
bt_message_discarded_events_borrow_end_default_clock_snapshot_const(
		const struct bt_message *msg)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(msg);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_EVENTS(msg);
	return borrow_discarded_items_message_end_default_clock_snapshot_const(
		msg, __func__);
}

const struct bt_stream *
bt_message_discarded_events_borrow_stream_const(const struct bt_message *message)
{
	return (void *) bt_message_discarded_events_borrow_stream(
		(void *) message);
}

enum bt_property_availability bt_message_discarded_events_get_count(
		const struct bt_message *message, uint64_t *count)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(message);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_EVENTS(message);
	BT_ASSERT_PRE_DEV_COUNT_OUTPUT_NON_NULL(count);
	return get_discarded_items_message_count(message, count);
}

struct bt_message *bt_message_discarded_packets_create(
		struct bt_self_message_iterator *message_iterator,
		const struct bt_stream *stream)
{
	BT_ASSERT_PRE_DEV_NO_ERROR();

	return create_discarded_items_message(message_iterator,
		BT_MESSAGE_TYPE_DISCARDED_PACKETS, (void *) stream,
		false, 0, 0, __func__, SC_SUPPORTS_DISC_PRECOND_ID("packets"));
}

struct bt_message *bt_message_discarded_packets_create_with_default_clock_snapshots(
		struct bt_self_message_iterator *message_iterator,
		const struct bt_stream *stream, uint64_t beginning_raw_value,
		uint64_t end_raw_value)
{
	BT_ASSERT_PRE_DEV_NO_ERROR();
	BT_ASSERT_PRE_MSG_CS_BEGIN_LE_END(message_iterator,
		beginning_raw_value, end_raw_value);

	return create_discarded_items_message(message_iterator,
		BT_MESSAGE_TYPE_DISCARDED_PACKETS, (void *) stream,
		true, beginning_raw_value, end_raw_value, __func__,
		SC_SUPPORTS_DISC_PRECOND_ID("packets"));
}

struct bt_stream *bt_message_discarded_packets_borrow_stream(
		struct bt_message *message)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(message);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_PACKETS(message);
	return borrow_discarded_items_message_stream(message);
}

void bt_message_discarded_packets_set_count(struct bt_message *message,
		uint64_t count)
{
	BT_ASSERT_PRE_MSG_NON_NULL(message);
	BT_ASSERT_PRE_MSG_IS_DISC_PACKETS(message);
	BT_ASSERT_PRE_DEV_MSG_HOT(message);
	BT_ASSERT_PRE("count-gt-0", count > 0, "Discarded packet count is 0.");
	set_discarded_items_message_count(message, count);
}

const struct bt_clock_snapshot *
bt_message_discarded_packets_borrow_beginning_default_clock_snapshot_const(
		const struct bt_message *msg)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(msg);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_PACKETS(msg);
	return borrow_discarded_items_message_beginning_default_clock_snapshot_const(
		msg, __func__);
}

const struct bt_clock_snapshot *
bt_message_discarded_packets_borrow_end_default_clock_snapshot_const(
		const struct bt_message *msg)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(msg);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_PACKETS(msg);
	return borrow_discarded_items_message_end_default_clock_snapshot_const(
		msg, __func__);
}

const struct bt_stream *
bt_message_discarded_packets_borrow_stream_const(const struct bt_message *message)
{
	return (void *) bt_message_discarded_packets_borrow_stream(
		(void *) message);
}

enum bt_property_availability bt_message_discarded_packets_get_count(
		const struct bt_message *message, uint64_t *count)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(message);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_PACKETS(message);
	BT_ASSERT_PRE_DEV_COUNT_OUTPUT_NON_NULL(count);
	return get_discarded_items_message_count(message, count);
}

static inline
const struct bt_clock_class *
borrow_discarded_items_message_stream_class_default_clock_class(
		const struct bt_message *msg)
{
	struct bt_message_discarded_items *disc_items_msg = (void *) msg;

	BT_ASSERT_DBG(msg);
	return disc_items_msg->stream->class->default_clock_class;
}

const struct bt_clock_class *
bt_message_discarded_events_borrow_stream_class_default_clock_class_const(
		const struct bt_message *msg)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(msg);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_EVENTS(msg);
	return borrow_discarded_items_message_stream_class_default_clock_class(
		msg);
}

const struct bt_clock_class *
bt_message_discarded_packets_borrow_stream_class_default_clock_class_const(
		const struct bt_message *msg)
{
	BT_ASSERT_PRE_DEV_MSG_NON_NULL(msg);
	BT_ASSERT_PRE_DEV_MSG_IS_DISC_PACKETS(msg);
	return borrow_discarded_items_message_stream_class_default_clock_class(
		msg);
}

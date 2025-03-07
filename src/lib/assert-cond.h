/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2018 EfficiOS Inc. and Linux Foundation
 * Copyright (c) 2018-2020 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_ASSERT_COND_INTERNAL_H
#define BABELTRACE_ASSERT_COND_INTERNAL_H

#include "assert-cond-base.h"

/*
 * Asserts that a given variable `_obj` named `_obj_name` (capitalized)
 * and having the ID `_obj_id` (within the function's context) is not
 * `NULL`.
 */
#define BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func, _obj_id, _obj, _obj_name) \
	BT_ASSERT_PRE_FROM_FUNC(_func, "not-null:" _obj_id, (_obj), 	\
		"%s is NULL.", _obj_name)

#define BT_ASSERT_PRE_NON_NULL(_obj_id, _obj, _obj_name)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(__func__, _obj_id, (_obj), _obj_name)

/*
 * Asserts that a given index `_index` is less than a given length
 * `_length`.
 */
#define BT_ASSERT_PRE_VALID_INDEX_FROM_FUNC(_func, _index, _length)	\
	BT_ASSERT_PRE_FROM_FUNC(_func, "valid-index", (_index) < (_length), \
		"Index is out of bounds: index=%" PRIu64 ", "		\
		"count=%" PRIu64, (uint64_t) (_index), (uint64_t) (_length))

#define BT_ASSERT_PRE_VALID_INDEX(_index, _length)			\
	BT_ASSERT_PRE_VALID_INDEX_FROM_FUNC(__func__, (_index), (_length))

/*
 * Asserts that the current thread has no error set.
 */
#define BT_ASSERT_PRE_NO_ERROR_FROM_FUNC(_func)				\
	do {								\
		const struct bt_error *err = bt_current_thread_take_error(); \
		if (err) {						\
			bt_current_thread_move_error(err);		\
		}							\
		BT_ASSERT_PRE_FROM_FUNC(_func, "no-error", !err,	\
			"API function called while current thread has an " \
			"error: function=%s", _func);			\
	} while (0)

#define BT_ASSERT_PRE_NO_ERROR()					\
	BT_ASSERT_PRE_NO_ERROR_FROM_FUNC(__func__)

/*
 * Asserts that, if the current thread has an error, `_status` is an
 * error status code.
 *
 * See _BT_ASSERT_COND() for details about the `_func` parameter.
 *
 * Puts back the error in place (if there is one) such that if this
 * macro aborts, it will be possible to inspect it with a debugger.
 */
#define BT_ASSERT_POST_NO_ERROR_IF_NO_ERROR_STATUS(_func, _status)		\
	do {									\
		const struct bt_error *_err = bt_current_thread_take_error();	\
		if (_err) {							\
			bt_current_thread_move_error(_err);			\
		}								\
		BT_ASSERT_POST(_func, "no-error-if-no-error-status",		\
			(_status) < 0 || !_err,					\
			"Current thread has an error, but user function "	\
			"returned a non-error status: status=%s",		\
			bt_common_func_status_string(_status));			\
	} while (0)

/*
 * Asserts that the current thread has no error.
 *
 * See _BT_ASSERT_COND() for details about the `_func` parameter.
 */
#define BT_ASSERT_POST_NO_ERROR(_func)					\
	do {								\
		const struct bt_error *_err = bt_current_thread_take_error(); \
		if (_err) {						\
			bt_current_thread_move_error(_err);		\
		}							\
		BT_ASSERT_POST(_func, "no-error", !_err,		\
			"Current thread has an error");			\
	} while (0)

#ifdef BT_DEV_MODE
/* Developer mode version of BT_ASSERT_PRE_NON_NULL_FROM_FUNC() */
# define BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func, _obj_id, _obj, _obj_name) \
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func, _obj_id, (_obj), (_obj_name))

/* Developer mode version of BT_ASSERT_PRE_NON_NULL() */
# define BT_ASSERT_PRE_DEV_NON_NULL(_obj_id, _obj, _obj_name)		\
	BT_ASSERT_PRE_NON_NULL(_obj_id, (_obj), (_obj_name))

/*
 * Developer mode: asserts that a given object `_obj` named `_obj_name`
 * (capitalized) and having the ID `_obj_id` (within the function's
 * context) is NOT frozen.
 *
 * This macro checks the `frozen` field of `_obj`.
 *
 * This currently only exists in developer mode because some freezing
 * functions can be called on the fast path, so they too are only
 * enabled in developer mode.
 */
# define BT_ASSERT_PRE_DEV_HOT_FROM_FUNC(_func, _obj_id, _obj, _obj_name, _fmt, ...) \
	BT_ASSERT_PRE_FROM_FUNC(_func, "not-frozen:" _obj_id,		\
		!(_obj)->frozen, "%s is frozen" _fmt, _obj_name, ##__VA_ARGS__)

# define BT_ASSERT_PRE_DEV_HOT(_obj_id, _obj, _obj_name, _fmt, ...)	\
	BT_ASSERT_PRE_DEV_HOT_FROM_FUNC(__func__, _obj_id, (_obj),	\
		_obj_name, _fmt, ##__VA_ARGS__)

/* Developer mode version of BT_ASSERT_PRE_VALID_INDEX_FROM_FUNC() */
# define BT_ASSERT_PRE_DEV_VALID_INDEX_FROM_FUNC(_func, _index, _length) \
	BT_ASSERT_PRE_VALID_INDEX_FROM_FUNC(_func, (_index), (_length))

/* Developer mode version of BT_ASSERT_PRE_VALID_INDEX() */
# define BT_ASSERT_PRE_DEV_VALID_INDEX(_index, _length)	\
	BT_ASSERT_PRE_VALID_INDEX((_index), (_length))

/* Developer mode version of BT_ASSERT_PRE_NO_ERROR_FROM_FUNC(). */
# define BT_ASSERT_PRE_DEV_NO_ERROR_FROM_FUNC(_func)			\
	BT_ASSERT_PRE_NO_ERROR_FROM_FUNC(_func)

/* Developer mode version of BT_ASSERT_PRE_NO_ERROR(). */
# define BT_ASSERT_PRE_DEV_NO_ERROR()	BT_ASSERT_PRE_NO_ERROR()

/*
 * Developer mode version of
 * BT_ASSERT_POST_NO_ERROR_IF_NO_ERROR_STATUS().
 */
# define BT_ASSERT_POST_DEV_NO_ERROR_IF_NO_ERROR_STATUS(_func, _status)	\
	BT_ASSERT_POST_NO_ERROR_IF_NO_ERROR_STATUS(_func, (_status))

/* Developer mode version of BT_ASSERT_POST_NO_ERROR(). */
# define BT_ASSERT_POST_DEV_NO_ERROR(_func)				\
	BT_ASSERT_POST_NO_ERROR(_func)

/*
 * Marks a function as being only used within a BT_ASSERT_PRE_DEV() or
 * BT_ASSERT_POST_DEV() context.
 */
# define BT_ASSERT_COND_DEV_FUNC
#else
# define BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func, _obj_id, _obj, _obj_name) \
	BT_USE_EXPR4(_func, _obj_id, (_obj), _obj_name)

# define BT_ASSERT_PRE_DEV_NON_NULL(_obj_id, _obj, _obj_name)		\
	BT_USE_EXPR3(_obj_id, (_obj), _obj_name)

# define BT_ASSERT_PRE_DEV_HOT_FROM_FUNC(_func, _obj_id, _obj, _obj_name, _fmt, ...) \
	BT_USE_EXPR5(_func, _obj_id, (_obj), _obj_name, _fmt)

# define BT_ASSERT_PRE_DEV_HOT(_obj_id, _obj, _obj_name, _fmt, ...) \
	BT_USE_EXPR4(_obj_id, (_obj), _obj_name, _fmt)

# define BT_ASSERT_PRE_DEV_VALID_INDEX_FROM_FUNC(_func, _index, _length) \
	BT_USE_EXPR3(_func, (_index), (_length))

# define BT_ASSERT_PRE_DEV_VALID_INDEX(_index, _length)			\
	BT_USE_EXPR2((_index), (_length))

# define BT_ASSERT_PRE_DEV_NO_ERROR_FROM_FUNC(_func)			\
	BT_USE_EXPR(_func)

# define BT_ASSERT_PRE_DEV_NO_ERROR()

# define BT_ASSERT_POST_DEV_NO_ERROR_IF_NO_ERROR_STATUS(_func, _status)	\
	BT_USE_EXPR2(_func, _status)

# define BT_ASSERT_POST_DEV_NO_ERROR(_func)				\
	BT_USE_EXPR(_func)
#endif /* BT_DEV_MODE */

#define _BT_ASSERT_PRE_CLK_CLS_NAME	"Clock class"
#define _BT_ASSERT_PRE_CLK_CLS_ID	"clock-class"

#define BT_ASSERT_PRE_CLK_CLS_NON_NULL(_cc)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_CLK_CLS_ID, (_cc),	\
		_BT_ASSERT_PRE_CLK_CLS_NAME)

#define BT_ASSERT_PRE_DEV_CLK_CLS_NON_NULL(_cc)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_CLK_CLS_ID, (_cc),	\
		_BT_ASSERT_PRE_CLK_CLS_NAME)

#define _BT_ASSERT_PRE_DEF_CLK_CLS_NAME	"Default clock class"
#define _BT_ASSERT_PRE_DEF_CLK_CLS_ID	"default-clock-class"

#define BT_ASSERT_PRE_DEF_CLK_CLS_NON_NULL(_cc)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_DEF_CLK_CLS_ID, (_cc),	\
		_BT_ASSERT_PRE_DEF_CLK_CLS_NAME)

#define BT_ASSERT_PRE_DEV_DEF_CLK_CLS_NON_NULL(_cc)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_DEF_CLK_CLS_ID,	\
		(_cc), _BT_ASSERT_PRE_DEF_CLK_CLS_NAME)

#define _BT_ASSERT_PRE_CS_NAME	"Clock snapshot"
#define _BT_ASSERT_PRE_CS_ID	"clock-snapshot"

#define BT_ASSERT_PRE_CS_NON_NULL(_cs)					\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_CS_ID, (_cs),		\
		_BT_ASSERT_PRE_CS_NAME)

#define BT_ASSERT_PRE_DEV_CS_NON_NULL(_cs)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_CS_ID, (_cs),		\
		_BT_ASSERT_PRE_CS_NAME)

#define _BT_ASSERT_PRE_EVENT_NAME	"Event"
#define _BT_ASSERT_PRE_EVENT_ID		"event"

#define BT_ASSERT_PRE_EVENT_NON_NULL(_event)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_EVENT_ID, (_event),	\
		_BT_ASSERT_PRE_EVENT_NAME)

#define BT_ASSERT_PRE_DEV_EVENT_NON_NULL(_event)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_EVENT_ID, (_event),	\
		_BT_ASSERT_PRE_EVENT_NAME)

#define _BT_ASSERT_PRE_EC_NAME	"Event class"
#define _BT_ASSERT_PRE_EC_ID	"event-class"

#define BT_ASSERT_PRE_EC_NON_NULL_FROM_FUNC(_func, _ec)			\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func, _BT_ASSERT_PRE_EC_ID,	\
		(_ec), _BT_ASSERT_PRE_EC_NAME)

#define BT_ASSERT_PRE_EC_NON_NULL(_ec)					\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_EC_ID, (_ec),		\
		_BT_ASSERT_PRE_EC_NAME)

#define BT_ASSERT_PRE_DEV_EC_NON_NULL_FROM_FUNC(_func, _ec)		\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_EC_ID, (_ec), _BT_ASSERT_PRE_EC_NAME)

#define BT_ASSERT_PRE_DEV_EC_NON_NULL(_ec)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_EC_ID, (_ec),		\
		_BT_ASSERT_PRE_EC_NAME)

#define _BT_ASSERT_PRE_FC_IS_INT_COND(_fc)				\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_SIGNED_INTEGER || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION)

#define _BT_ASSERT_PRE_FC_IS_INT_FMT(_name)				\
	_name " is not an integer field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_INT_ID(_fc_id)	"is-int-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_COND(_fc)			\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION)

#define _BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_FMT(_name)			\
	_name " is not an unsigned integer field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_ID(_fc_id)			\
	"is-unsigned-integer-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_SIGNED_INT_COND(_fc)			\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_SIGNED_INTEGER || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION)

#define _BT_ASSERT_PRE_FC_IS_SIGNED_INT_FMT(_name)			\
	_name " is not a signed integer field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_SIGNED_INT_ID(_fc_id)			\
	"is-signed-integer-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_ENUM_COND(_fc)				\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION)

#define _BT_ASSERT_PRE_FC_IS_ENUM_FMT(_name)				\
	_name " is not an enumeration field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_ENUM_ID(_fc_id)				\
	"is-enumeration-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_ARRAY_COND(_fc)				\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_STATIC_ARRAY || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITHOUT_LENGTH_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITH_LENGTH_FIELD)

#define _BT_ASSERT_PRE_FC_IS_ARRAY_FMT(_name)				\
	_name " is not an array field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_ARRAY_ID(_fc_id)				\
	"is-array-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_OPTION_COND(_fc)				\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITHOUT_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_BOOL_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_SIGNED_INTEGER_SELECTOR_FIELD)

#define _BT_ASSERT_PRE_FC_IS_OPTION_FMT(_name)				\
	_name " is not an option field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_OPTION_ID(_fc_id)				\
	"is-option-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_COND(_fc)			\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_BOOL_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_SIGNED_INTEGER_SELECTOR_FIELD)

#define _BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_FMT(_name)		\
	_name " is not an option field class with a selector: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_ID(_fc_id)			\
	"is-option-field-class-with-selector:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_COND(_fc)		\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_SIGNED_INTEGER_SELECTOR_FIELD)

#define _BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_FMT(_name)		\
	_name " is not an option field class with an integer selector: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_ID(_fc_id)		\
	"is-option-field-class-with-integer-selector:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_STRUCT_FMT(_name)				\
	_name " is not a structure field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_STRUCT_COND(_fc)				\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_STRUCTURE)

#define _BT_ASSERT_PRE_FC_IS_STRUCT_ID(_fc_id)				\
	"is-structure-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_VARIANT_COND(_fc)				\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_VARIANT_WITHOUT_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_INTEGER_SELECTOR_FIELD)

#define _BT_ASSERT_PRE_FC_IS_VARIANT_FMT(_name)				\
	_name " is not a variant field class: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_VARIANT_ID(_fc_id)				\
	"is-variant-field-class:" _fc_id

#define _BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_COND(_fc)			\
	(((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD || \
	((const struct bt_field_class *) (_fc))->type == BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_INTEGER_SELECTOR_FIELD)

#define _BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_FMT(_name)		\
	_name " is not a variant field class with a selector: %![fc-]+F"

#define _BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_ID(_fc_id)		\
	"is-variant-field-class-with-selector:" _fc_id

#define _BT_ASSERT_PRE_FC_HAS_TYPE_COND(_fc, _type)			\
	(((const struct bt_field_class *) (_fc))->type == (_type))

#define _BT_ASSERT_PRE_FC_HAS_TYPE_FMT(_name)				\
	_name " has the wrong type: expected-type=%s, %![fc-]+F"

#define _BT_ASSERT_PRE_FC_HAS_TYPE_ID(_fc_id, _type_id)			\
	"is-" _type_id ":" _fc_id

#define BT_ASSERT_PRE_FC_IS_INT_FROM_FUNC(_func, _fc_id, _fc, _name)	\
	BT_ASSERT_PRE_FROM_FUNC(_func,					\
		_BT_ASSERT_PRE_FC_IS_INT_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_INT_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_INT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_INT(_fc_id, _fc, _name)			\
	BT_ASSERT_PRE_FC_IS_INT_FROM_FUNC(__func__, _fc_id, (_fc), _name)

#define BT_ASSERT_PRE_FC_IS_UNSIGNED_INT(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_ID(_fc_id),		\
		_BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_COND(_fc),		\
		_BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_SIGNED_INT(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_SIGNED_INT_ID(_fc_id),		\
		_BT_ASSERT_PRE_FC_IS_SIGNED_INT_COND(_fc),		\
		_BT_ASSERT_PRE_FC_IS_SIGNED_INT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_ENUM(_fc_id, _fc, _name)			\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_ENUM_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_ENUM_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_ENUM_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_ARRAY(_fc_id, _fc, _name)			\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_ARRAY_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_ARRAY_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_ARRAY_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_STRUCT(_fc_id, _fc, _name)			\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_STRUCT_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_STRUCT_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_STRUCT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_OPTION(_fc_id, _fc, _name)			\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_OPTION_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_OPTION_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_OPTION_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_ID(_fc_id),	\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_COND(_fc),		\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL(_fc_id, _fc, _name)	\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_ID(_fc_id),	\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_COND(_fc),	\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_VARIANT(_fc_id, _fc, _name)			\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_VARIANT_ID(_fc_id),		\
		_BT_ASSERT_PRE_FC_IS_VARIANT_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_VARIANT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL(_fc_id, _fc, _name)	\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_ID(_fc_id),	\
		_BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_COND(_fc),	\
		_BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_FMT(_name), (_fc))

#define BT_ASSERT_PRE_FC_HAS_TYPE_FROM_FUNC(_func, _fc_id, _fc, _type_id, _type, _name) \
	BT_ASSERT_PRE_FROM_FUNC(_func,					\
		_BT_ASSERT_PRE_FC_HAS_TYPE_ID(_fc_id, _type_id),		\
		_BT_ASSERT_PRE_FC_HAS_TYPE_COND((_fc), (_type)),		\
		_BT_ASSERT_PRE_FC_HAS_TYPE_FMT(_name),			\
		bt_common_field_class_type_string(_type), (_fc))

#define BT_ASSERT_PRE_FC_HAS_TYPE(_fc_id, _fc, _type_id, _type, _name)	\
	BT_ASSERT_PRE_FC_HAS_TYPE_FROM_FUNC(__func__, _fc_id, (_fc),	\
		_type_id, (_type), _name)

#define BT_ASSERT_PRE_DEV_FC_IS_INT(_fc_id, _fc, _name)			\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_INT_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_INT_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_INT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_UNSIGNED_INT(_fc_id, _fc, _name)	\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_ID(_fc_id),		\
		_BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_COND(_fc),		\
		_BT_ASSERT_PRE_FC_IS_UNSIGNED_INT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_SIGNED_INT(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_SIGNED_INT_ID(_fc_id),		\
		_BT_ASSERT_PRE_FC_IS_SIGNED_INT_COND(_fc),		\
		_BT_ASSERT_PRE_FC_IS_SIGNED_INT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_ENUM(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_ENUM_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_ENUM_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_ENUM_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_ARRAY(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_ARRAY_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_ARRAY_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_ARRAY_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_STRUCT(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_STRUCT_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_STRUCT_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_STRUCT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_OPTION(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_OPTION_ID(_fc_id),			\
		_BT_ASSERT_PRE_FC_IS_OPTION_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_OPTION_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_OPTION_WITH_SEL(_fc_id, _fc, _name)	\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_COND(_fc),		\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_SEL_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_OPTION_WITH_INT_SEL(_fc_id, _fc, _name)	\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_ID(_fc_id),	\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_COND(_fc),	\
		_BT_ASSERT_PRE_FC_IS_OPTION_WITH_INT_SEL_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_VARIANT(_fc_id, _fc, _name)		\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_VARIANT_ID(_fc_id),		\
		_BT_ASSERT_PRE_FC_IS_VARIANT_COND(_fc),			\
		_BT_ASSERT_PRE_FC_IS_VARIANT_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_IS_VARIANT_WITH_SEL(_fc_id, _fc, _name)	\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_ID(_fc_id),	\
		_BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_COND(_fc),	\
		_BT_ASSERT_PRE_FC_IS_VARIANT_WITH_SEL_FMT(_name), (_fc))

#define BT_ASSERT_PRE_DEV_FC_HAS_TYPE(_fc_id, _fc, _type_id, _type, _name) \
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_FC_HAS_TYPE_ID(_fc_id, _type_id),		\
		_BT_ASSERT_PRE_FC_HAS_TYPE_COND((_fc), (_type)),		\
		_BT_ASSERT_PRE_FC_HAS_TYPE_FMT(_name),			\
		bt_common_field_class_type_string(_type), (_fc))

#define BT_ASSERT_PRE_DEV_FC_HOT_FROM_FUNC(_func, _fc)			\
	BT_ASSERT_PRE_DEV_HOT_FROM_FUNC(_func, "field-class",		\
		(const struct bt_field_class *) (_fc),			\
		"Field class", ": %!+F", (_fc))

#define BT_ASSERT_PRE_DEV_FC_HOT(_fc)					\
	BT_ASSERT_PRE_DEV_FC_HOT_FROM_FUNC(__func__, (_fc))

#define _BT_ASSERT_PRE_FC_NAME	"Field class"
#define _BT_ASSERT_PRE_FC_ID	"field-class"

#define BT_ASSERT_PRE_FC_NON_NULL(_fc)					\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_FC_ID, (_fc),		\
		_BT_ASSERT_PRE_FC_NAME)

#define BT_ASSERT_PRE_DEV_FC_NON_NULL(_fc)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_FC_ID, (_fc),		\
		_BT_ASSERT_PRE_FC_NAME)

#define _BT_ASSERT_PRE_STRUCT_FC_MEMBER_NAME	"Structure field class member"
#define _BT_ASSERT_PRE_STRUCT_FC_MEMBER_ID	"structure-field-class-member"

#define BT_ASSERT_PRE_STRUCT_FC_MEMBER_NON_NULL(_fc)			\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_STRUCT_FC_MEMBER_ID,	\
		(_fc), _BT_ASSERT_PRE_STRUCT_FC_MEMBER_NAME)

#define BT_ASSERT_PRE_DEV_STRUCT_FC_MEMBER_NON_NULL(_fc)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_STRUCT_FC_MEMBER_ID,	\
		(_fc), _BT_ASSERT_PRE_STRUCT_FC_MEMBER_NAME)

#define _BT_ASSERT_PRE_VAR_FC_OPT_NAME	"Variant field class option"
#define _BT_ASSERT_PRE_VAR_FC_OPT_ID	"variant-field-class-option-id"

#define BT_ASSERT_PRE_VAR_FC_OPT_NON_NULL(_fc)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_VAR_FC_OPT_ID, (_fc),	\
		_BT_ASSERT_PRE_VAR_FC_OPT_NAME)

#define BT_ASSERT_PRE_DEV_VAR_FC_OPT_NON_NULL(_fc)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_VAR_FC_OPT_ID, (_fc),	\
		_BT_ASSERT_PRE_VAR_FC_OPT_NAME)

#define _BT_ASSERT_PRE_FP_NAME	"Field path"
#define _BT_ASSERT_PRE_FP_ID	"field-path"

#define BT_ASSERT_PRE_FP_NON_NULL(_fp)					\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_FP_ID, (_fp),		\
		_BT_ASSERT_PRE_FP_NAME)

#define BT_ASSERT_PRE_DEV_FP_NON_NULL(_fp)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_FP_ID, (_fp),		\
		_BT_ASSERT_PRE_FP_NAME)

#define BT_ASSERT_PRE_DEV_FIELD_HAS_CLASS_TYPE_FROM_FUNC(_func, _field_id, _field, _cls_type_id, _cls_type, _name) \
	BT_ASSERT_PRE_DEV_FROM_FUNC(_func, "is-" _cls_type_id ":" _field_id, \
		((const struct bt_field *) (_field))->class->type == (_cls_type), \
		_name " has the wrong class type: expected-class-type=%s, " \
		"%![field-]+f",						\
		bt_common_field_class_type_string(_cls_type), (_field))

#define BT_ASSERT_PRE_DEV_FIELD_HAS_CLASS_TYPE(_field_id, _field, _cls_type_id, _cls_type, _name) \
	BT_ASSERT_PRE_DEV_FIELD_HAS_CLASS_TYPE_FROM_FUNC(__func__, 	\
		_field_id, (_field), _cls_type_id, _cls_type, _name)

#define BT_ASSERT_PRE_DEV_FIELD_IS_UNSIGNED_INT(_field_id, _field, _name) \
	BT_ASSERT_PRE_DEV(						\
		"is-unsigned-integer-field:" _field_id,			\
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION, \
		_name " is not an unsigned integer field: %![field-]+f", \
		(_field))

#define BT_ASSERT_PRE_DEV_FIELD_IS_SIGNED_INT(_field_id, _field, _name)	\
	BT_ASSERT_PRE_DEV(						\
		"is-signed-integer-field:" _field_id,			\
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_SIGNED_INTEGER || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION, \
		_name " is not a signed integer field: %![field-]+f",	\
		(_field))

#define BT_ASSERT_PRE_DEV_FIELD_IS_ARRAY_FROM_FUNC(_func, _field_id, _field, _name) \
	BT_ASSERT_PRE_DEV_FROM_FUNC(_func,				\
		"is-array-field:" _field_id,				\
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_STATIC_ARRAY || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITHOUT_LENGTH_FIELD || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITH_LENGTH_FIELD, \
		_name " is not an array field: %![field-]+f", (_field))

#define BT_ASSERT_PRE_DEV_FIELD_IS_ARRAY(_field_id, _field, _name)	\
	BT_ASSERT_PRE_DEV_FIELD_IS_ARRAY_FROM_FUNC(__func__, _field_id,	\
		(_field), _name)

#define BT_ASSERT_PRE_DEV_FIELD_IS_DYNAMIC_ARRAY(_field_id, _field, _name) \
	BT_ASSERT_PRE_DEV(						\
		"is-dynamic-array-field:" _field_id,			\
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITHOUT_LENGTH_FIELD || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY_WITH_LENGTH_FIELD, \
		_name " is not a dynamic array field: %![field-]+f", (_field))

#define BT_ASSERT_PRE_DEV_FIELD_IS_OPTION(_field_id, _field, _name)	\
	BT_ASSERT_PRE_DEV(						\
		"is-option-field:" _field_id,				\
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_OPTION_WITHOUT_SELECTOR_FIELD || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_BOOL_SELECTOR_FIELD || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_OPTION_WITH_SIGNED_INTEGER_SELECTOR_FIELD, \
		_name " is not an option field: %![field-]+f", (_field))

#define BT_ASSERT_PRE_DEV_FIELD_IS_VARIANT(_field_id, _field, _name)	\
	BT_ASSERT_PRE_DEV(						\
		"is-variant-field:" _field_id,				\
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_VARIANT_WITHOUT_SELECTOR_FIELD || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_INTEGER_SELECTOR_FIELD || \
		((const struct bt_field *) (_field))->class->type == BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_INTEGER_SELECTOR_FIELD, \
		_name " is not a variant field: %![field-]+f", (_field))

#define BT_ASSERT_PRE_DEV_FIELD_IS_SET(_field_id, _field)		\
	BT_ASSERT_PRE_DEV("is-field-set:" _field_id,			\
		bt_field_is_set(_field),				\
		"Field is not set: %!+f", (_field))

#define _BT_ASSERT_PRE_FIELD_NAME	"Field"
#define _BT_ASSERT_PRE_FIELD_ID		"field"

#define BT_ASSERT_PRE_FIELD_NON_NULL(_field)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_FIELD_ID, (_field),	\
		_BT_ASSERT_PRE_FIELD_NAME)

#define BT_ASSERT_PRE_DEV_FIELD_NON_NULL_FROM_FUNC(_func, _field)	\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_FIELD_ID, (_field),			\
		_BT_ASSERT_PRE_FIELD_NAME)

#define BT_ASSERT_PRE_DEV_FIELD_NON_NULL(_field)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_FIELD_ID, (_field),	\
		_BT_ASSERT_PRE_FIELD_NAME)

#define _BT_ASSERT_PRE_PACKET_NAME	"Packet"
#define _BT_ASSERT_PRE_PACKET_ID	"packet"

#define BT_ASSERT_PRE_PACKET_NON_NULL_FROM_FUNC(_func, _packet)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_PACKET_ID, (_packet), _BT_ASSERT_PRE_PACKET_NAME)

#define BT_ASSERT_PRE_PACKET_NON_NULL(_packet)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_PACKET_ID, (_packet),	\
		_BT_ASSERT_PRE_PACKET_NAME)

#define BT_ASSERT_PRE_DEV_PACKET_NON_NULL_FROM_FUNC(_func, _packet)	\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_PACKET_ID, (_packet), _BT_ASSERT_PRE_PACKET_NAME)

#define BT_ASSERT_PRE_DEV_PACKET_NON_NULL(_packet)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_PACKET_ID, (_packet),	\
		_BT_ASSERT_PRE_PACKET_NAME)

#define _BT_ASSERT_PRE_SC_NAME	"Stream class"
#define _BT_ASSERT_PRE_SC_ID	"stream-class"

#define BT_ASSERT_PRE_SC_NON_NULL(_sc)					\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_SC_ID, (_sc),		\
		_BT_ASSERT_PRE_SC_NAME)

#define BT_ASSERT_PRE_DEV_SC_NON_NULL(_sc)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_SC_ID, (_sc),		\
		_BT_ASSERT_PRE_SC_NAME)

#define _BT_ASSERT_PRE_STREAM_NAME	"Stream"
#define _BT_ASSERT_PRE_STREAM_ID	"stream"

#define BT_ASSERT_PRE_STREAM_NON_NULL_FROM_FUNC(_func, _stream)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_STREAM_ID, (_stream),			\
		_BT_ASSERT_PRE_STREAM_NAME)

#define BT_ASSERT_PRE_STREAM_NON_NULL(_stream)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_STREAM_ID, (_stream),	\
		_BT_ASSERT_PRE_STREAM_NAME)

#define BT_ASSERT_PRE_DEV_STREAM_NON_NULL_FROM_FUNC(_func, _stream)	\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_STREAM_ID, (_stream),			\
		_BT_ASSERT_PRE_STREAM_NAME)

#define BT_ASSERT_PRE_DEV_STREAM_NON_NULL(_stream)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_STREAM_ID, (_stream),	\
		_BT_ASSERT_PRE_STREAM_NAME)

#define _BT_ASSERT_PRE_TC_NAME	"Trace class"
#define _BT_ASSERT_PRE_TC_ID	"trace-class"

#define BT_ASSERT_PRE_TC_NON_NULL_FROM_FUNC(_func, _tc)			\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func, _BT_ASSERT_PRE_TC_ID,	\
		(_tc), _BT_ASSERT_PRE_TC_NAME)

#define BT_ASSERT_PRE_TC_NON_NULL(_tc)					\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_TC_ID, (_tc),		\
		_BT_ASSERT_PRE_TC_NAME)

#define BT_ASSERT_PRE_DEV_TC_NON_NULL(_tc)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_TC_ID, (_tc),		\
		_BT_ASSERT_PRE_TC_NAME)

#define _BT_ASSERT_PRE_TRACE_NAME	"Trace"
#define _BT_ASSERT_PRE_TRACE_ID		"trace"

#define BT_ASSERT_PRE_TRACE_NON_NULL(_trace)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_TRACE_ID, (_trace),	\
		_BT_ASSERT_PRE_TRACE_NAME)

#define BT_ASSERT_PRE_DEV_TRACE_NON_NULL(_trace)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_TRACE_ID, (_trace),	\
		_BT_ASSERT_PRE_TRACE_NAME)

#define _BT_ASSERT_PRE_USER_ATTRS_NAME	"User attributes value object"
#define _BT_ASSERT_PRE_USER_ATTRS_ID	"user-attributes-value-object"

#define BT_ASSERT_PRE_USER_ATTRS_NON_NULL_FROM_FUNC(_func, _ua)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_USER_ATTRS_ID, (_ua),			\
		_BT_ASSERT_PRE_USER_ATTRS_NAME)

#define BT_ASSERT_PRE_USER_ATTRS_NON_NULL(_ua)				\
	BT_ASSERT_PRE_USER_ATTRS_NON_NULL_FROM_FUNC(__func__, (_ua))

#define BT_ASSERT_PRE_DEV_USER_ATTRS_NON_NULL(_ua)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_USER_ATTRS_ID, (_ua),	\
		_BT_ASSERT_PRE_USER_ATTRS_NAME)

#define BT_ASSERT_PRE_USER_ATTRS_IS_MAP_FROM_FUNC(_func, _ua)		\
	BT_ASSERT_PRE_FROM_FUNC(_func, "is-map-value:user-attributes",	\
		(_ua)->type == BT_VALUE_TYPE_MAP,			\
		_BT_ASSERT_PRE_USER_ATTRS_NAME				\
		" object is not a map value object.")

#define BT_ASSERT_PRE_USER_ATTRS_IS_MAP(_ua)				\
	BT_ASSERT_PRE_USER_ATTRS_IS_MAP_FROM_FUNC(__func__, (_ua))

#define BT_ASSERT_COND_LISTENER_FUNC_NAME	"Listener function"
#define BT_ASSERT_COND_LISTENER_FUNC_ID		"listener-function"

#define BT_ASSERT_PRE_LISTENER_FUNC_NON_NULL(_listener_func)		\
	BT_ASSERT_PRE_NON_NULL(BT_ASSERT_COND_LISTENER_FUNC_ID,		\
		(_listener_func), BT_ASSERT_COND_LISTENER_FUNC_NAME)

#define BT_ASSERT_PRE_DEV_LISTENER_FUNC_NON_NULL(_listener_func)			\
	BT_ASSERT_PRE_DEV_NON_NULL(BT_ASSERT_COND_LISTENER_FUNC_ID,	\
		(_listener_func), BT_ASSERT_COND_LISTENER_FUNC_NAME)

#define _BT_ASSERT_PRE_MSG_ITER_NAME	"Message iterator"
#define _BT_ASSERT_PRE_MSG_ITER_ID	"message-iterator"

#define BT_ASSERT_PRE_MSG_ITER_NON_NULL_FROM_FUNC(_func, _msg_iter)	\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_MSG_ITER_ID, (_msg_iter),		\
		_BT_ASSERT_PRE_MSG_ITER_NAME)

#define BT_ASSERT_PRE_MSG_ITER_NON_NULL(_msg_iter)			\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_MSG_ITER_ID, (_msg_iter),	\
		_BT_ASSERT_PRE_MSG_ITER_NAME)

#define BT_ASSERT_PRE_DEV_MSG_ITER_NON_NULL_FROM_FUNC(_func, _msg_iter)	\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_MSG_ITER_ID,				\
		(_msg_iter), _BT_ASSERT_PRE_MSG_ITER_NAME)

#define BT_ASSERT_PRE_DEV_MSG_ITER_NON_NULL(_msg_iter)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_MSG_ITER_ID,		\
		(_msg_iter), _BT_ASSERT_PRE_MSG_ITER_NAME)

#define _BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_COND(_sc) 			\
	((_sc)->default_clock_class)

#define _BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_FMT 				\
	"Message's stream's class has no default clock class: "		\
	"%![msg-]+n, %![sc-]+S"

#define _BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_ID				\
	"message-stream-class-has-default-clock-class"

#define BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_FROM_FUNC(_func, _msg, _sc)	\
	BT_ASSERT_PRE_FROM_FUNC(_func,					\
		_BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_ID,			\
		_BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_COND(_sc),		\
		_BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_FMT,			\
		(_msg), (_sc));

#define BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS(_msg, _sc)			\
	BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_FROM_FUNC(__func__, (_msg), (_sc))

#define BT_ASSERT_PRE_DEV_MSG_SC_DEF_CLK_CLS_FROM_FUNC(_func, _msg, _sc) \
	BT_ASSERT_PRE_DEV_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_ID,			\
		_BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_COND(_sc),		\
		_BT_ASSERT_PRE_MSG_SC_DEF_CLK_CLS_FMT,			\
		(_msg), (_sc));

#define BT_ASSERT_PRE_DEV_MSG_SC_DEF_CLK_CLS(_msg, _sc)			\
	BT_ASSERT_PRE_DEV_MSG_SC_DEF_CLK_CLS_FROM_FUNC(__func__, (_msg), (_sc))

#define _BT_ASSERT_PRE_MSG_HAS_TYPE_COND(_msg, _type)			\
	(((struct bt_message *) (_msg))->type == (_type))

#define _BT_ASSERT_PRE_MSG_HAS_TYPE_FMT					\
	"Message has the wrong type: expected-type=%s, %![msg-]+n"

#define _BT_ASSERT_PRE_MSG_HAS_TYPE_ID(_msg_id, _type_id)		\
	"is-" _type_id "-message:" _msg_id

#define BT_ASSERT_PRE_MSG_HAS_TYPE(_msg_id, _msg, _type_id, _type)	\
	BT_ASSERT_PRE(							\
		_BT_ASSERT_PRE_MSG_HAS_TYPE_ID(_msg_id, _type_id),	\
		_BT_ASSERT_PRE_MSG_HAS_TYPE_COND((_msg), (_type)),	\
		_BT_ASSERT_PRE_MSG_HAS_TYPE_FMT,			\
		bt_common_message_type_string(_type), (_msg))

#define BT_ASSERT_PRE_DEV_MSG_HAS_TYPE(_msg_id, _msg, _type_id, _type)	\
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_MSG_HAS_TYPE_ID(_msg_id, _type_id),	\
		_BT_ASSERT_PRE_MSG_HAS_TYPE_COND((_msg), (_type)),	\
		_BT_ASSERT_PRE_MSG_HAS_TYPE_FMT,			\
		bt_common_message_type_string(_type), (_msg))

#define _BT_ASSERT_PRE_MSG_NAME	"Message"
#define _BT_ASSERT_PRE_MSG_ID	"message"

#define BT_ASSERT_PRE_MSG_NON_NULL(_msg_iter)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_MSG_ID, (_msg_iter),	\
		_BT_ASSERT_PRE_MSG_NAME)

#define BT_ASSERT_PRE_DEV_MSG_NON_NULL(_msg_iter)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_MSG_ID, (_msg_iter),	\
		_BT_ASSERT_PRE_MSG_NAME)

#define BT_ASSERT_PRE_MSG_CS_BEGIN_LE_END(_msg_iter, _begin, _end) 	\
	BT_ASSERT_PRE("beginning-default-clock-snapshot-lteq-end",	\
		(_begin) <= (_end),					\
		"Beginning default clock snapshot value is greater "	\
		"than end default clock snapshot value: "		\
		"cs-begin-val=%" PRIu64 ", cs-end-val=%" PRIu64 ", " 	\
		"%![msg-iter-]+i", (_begin), (_end), _msg_iter);

#define BT_ASSERT_PRE_DEV_MSG_HOT(_msg)					\
	BT_ASSERT_PRE_DEV_HOT("message", (_msg), "Message", ": %!+n", (_msg));

#define _BT_ASSERT_PRE_MSG_ITER_CLS_NAME	"Message iterator class"
#define _BT_ASSERT_PRE_MSG_ITER_CLS_ID		"message-iterator-class"

#define BT_ASSERT_PRE_MSG_ITER_CLS_NON_NULL(_msg_iter_cls)		\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_MSG_ITER_CLS_ID,		\
		(_msg_iter_cls), _BT_ASSERT_PRE_MSG_ITER_CLS_NAME)

#define BT_ASSERT_PRE_DEV_MSG_ITER_CLS_NON_NULL(_msg_iter_cls)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_MSG_ITER_CLS_ID,	\
		(_msg_iter_cls), _BT_ASSERT_PRE_MSG_ITER_CLS_NAME)

#define _BT_ASSERT_PRE_COMP_CLS_NAME	"Component class"
#define _BT_ASSERT_PRE_COMP_CLS_ID	"component-class"

#define BT_ASSERT_PRE_COMP_CLS_NON_NULL_FROM_FUNC(_func, _comp_cls)	\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_COMP_CLS_ID, (_comp_cls),		\
		_BT_ASSERT_PRE_COMP_CLS_NAME)

#define BT_ASSERT_PRE_COMP_CLS_NON_NULL(_comp_cls)			\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_COMP_CLS_ID, (_comp_cls),	\
		_BT_ASSERT_PRE_COMP_CLS_NAME)

#define BT_ASSERT_PRE_DEV_COMP_CLS_NON_NULL(_comp_cls)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_COMP_CLS_ID,		\
		(_comp_cls), _BT_ASSERT_PRE_COMP_CLS_NAME)

#define _BT_ASSERT_PRE_COMP_DESCR_SET_NAME	"Component descriptor set"
#define _BT_ASSERT_PRE_COMP_DESCR_SET_ID	"component-descriptor-set"

#define BT_ASSERT_PRE_COMP_DESCR_SET_NON_NULL(_comp_descr_set)		\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_COMP_DESCR_SET_ID,	\
		(_comp_descr_set), _BT_ASSERT_PRE_COMP_DESCR_SET_NAME)

#define BT_ASSERT_PRE_DEV_COMP_DESCR_SET_NON_NULL(_comp_descr_set)	\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_COMP_DESCR_SET_ID,	\
		(_comp_descr_set), _BT_ASSERT_PRE_COMP_DESCR_SET_NAME)

#define _BT_ASSERT_PRE_COMP_NAME	"Component"
#define _BT_ASSERT_PRE_COMP_ID		"component"

#define BT_ASSERT_PRE_COMP_NON_NULL_FROM_FUNC(_func, _comp)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func, _BT_ASSERT_PRE_COMP_ID,	\
		(_comp), _BT_ASSERT_PRE_COMP_NAME)

#define BT_ASSERT_PRE_COMP_NON_NULL(_comp)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_COMP_ID, (_comp),		\
		_BT_ASSERT_PRE_COMP_NAME)

#define BT_ASSERT_PRE_DEV_COMP_NON_NULL_FROM_FUNC(_func, _comp)		\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_COMP_ID, (_comp), _BT_ASSERT_PRE_COMP_NAME)

#define BT_ASSERT_PRE_DEV_COMP_NON_NULL(_comp)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_COMP_ID, (_comp),	\
		_BT_ASSERT_PRE_COMP_NAME)

#define _BT_ASSERT_PRE_CONN_NAME	"Connection"
#define _BT_ASSERT_PRE_CONN_ID		"connection"

#define BT_ASSERT_PRE_CONN_NON_NULL(_conn)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_CONN_ID, (_conn),		\
		_BT_ASSERT_PRE_CONN_NAME)

#define BT_ASSERT_PRE_DEV_CONN_NON_NULL(_conn)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_CONN_ID, (_conn),	\
		_BT_ASSERT_PRE_CONN_NAME)

#define _BT_ASSERT_PRE_GRAPH_NAME	"Graph"
#define _BT_ASSERT_PRE_GRAPH_ID		"graph"

#define BT_ASSERT_PRE_GRAPH_NON_NULL_FROM_FUNC(_func, _graph)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_GRAPH_ID, (_graph), _BT_ASSERT_PRE_GRAPH_NAME)

#define BT_ASSERT_PRE_GRAPH_NON_NULL(_graph)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_GRAPH_ID, (_graph),	\
		_BT_ASSERT_PRE_GRAPH_NAME)

#define BT_ASSERT_PRE_DEV_GRAPH_NON_NULL(_graph)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_GRAPH_ID, (_graph),	\
		_BT_ASSERT_PRE_GRAPH_NAME)

#define _BT_ASSERT_PRE_INTR_NAME	"Interrupter"
#define _BT_ASSERT_PRE_INTR_ID		"interrupter"

#define BT_ASSERT_PRE_INTR_NON_NULL(_intr)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_INTR_ID, (_intr),		\
		_BT_ASSERT_PRE_INTR_NAME)

#define BT_ASSERT_PRE_DEV_INTR_NON_NULL(_intr)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_INTR_ID, (_intr),	\
		_BT_ASSERT_PRE_INTR_NAME)

#define _BT_ASSERT_PRE_PORT_NAME	"Port"
#define _BT_ASSERT_PRE_PORT_ID		"port"

#define BT_ASSERT_PRE_PORT_NON_NULL(_port)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_PORT_ID, (_port),		\
		_BT_ASSERT_PRE_PORT_NAME)

#define BT_ASSERT_PRE_DEV_PORT_NON_NULL(_port)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_PORT_ID, (_port),	\
		_BT_ASSERT_PRE_PORT_NAME)

#define _BT_ASSERT_PRE_QUERY_EXEC_NAME	"Query executor"
#define _BT_ASSERT_PRE_QUERY_EXEC_ID	"query-executor"

#define BT_ASSERT_PRE_QUERY_EXEC_NON_NULL(_query_exec)			\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_QUERY_EXEC_ID,		\
		(_query_exec), _BT_ASSERT_PRE_QUERY_EXEC_NAME)

#define BT_ASSERT_PRE_DEV_QUERY_EXEC_NON_NULL(_query_exec)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_QUERY_EXEC_ID,	\
		(_query_exec), _BT_ASSERT_PRE_QUERY_EXEC_NAME)

#define _BT_ASSERT_PRE_PLUGIN_SET_NAME	"Plugin set"
#define _BT_ASSERT_PRE_PLUGIN_SET_ID	"plugin-set"

#define BT_ASSERT_PRE_PLUGIN_SET_NON_NULL(_plugin_set)			\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_PLUGIN_SET_ID,		\
		(_plugin_set), _BT_ASSERT_PRE_PLUGIN_SET_NAME)

#define BT_ASSERT_PRE_DEV_PLUGIN_SET_NON_NULL(_plugin_set)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_PLUGIN_SET_ID,	\
		(_plugin_set), _BT_ASSERT_PRE_PLUGIN_SET_NAME)

#define _BT_ASSERT_PRE_PLUGIN_SET_OUT_NAME				\
	_BT_ASSERT_PRE_PLUGIN_SET_NAME " (output)"
#define _BT_ASSERT_PRE_PLUGIN_SET_OUT_ID				\
	_BT_ASSERT_PRE_PLUGIN_SET_ID "-output"

#define BT_ASSERT_PRE_PLUGIN_SET_OUT_NON_NULL(_plugin_set)		\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_PLUGIN_SET_OUT_ID,	\
		(_plugin_set), _BT_ASSERT_PRE_PLUGIN_SET_OUT_NAME)

#define BT_ASSERT_PRE_DEV_PLUGIN_SET_OUT_NON_NULL(_plugin_set)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_PLUGIN_SET_OUT_ID,	\
		(_plugin_set), _BT_ASSERT_PRE_PLUGIN_SET_OUT_NAME)

#define _BT_ASSERT_PRE_PLUGIN_NAME	"Plugin"
#define _BT_ASSERT_PRE_PLUGIN_ID	"plugin"

#define BT_ASSERT_PRE_PLUGIN_NON_NULL(_plugin)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_PLUGIN_ID, (_plugin),	\
		_BT_ASSERT_PRE_PLUGIN_NAME)

#define BT_ASSERT_PRE_DEV_PLUGIN_NON_NULL(_plugin)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_PLUGIN_ID, (_plugin),	\
		_BT_ASSERT_PRE_PLUGIN_NAME)

#define _BT_ASSERT_PRE_PLUGIN_OUT_NAME					\
	_BT_ASSERT_PRE_PLUGIN_NAME " (output)"
#define _BT_ASSERT_PRE_PLUGIN_OUT_ID					\
	_BT_ASSERT_PRE_PLUGIN_ID "-output"

#define BT_ASSERT_PRE_PLUGIN_OUT_NON_NULL(_plugin)			\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_PLUGIN_OUT_ID, (_plugin),	\
		_BT_ASSERT_PRE_PLUGIN_OUT_NAME)

#define BT_ASSERT_PRE_DEV_PLUGIN_OUT_NON_NULL(_plugin)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_PLUGIN_OUT_ID,	\
		(_plugin), _BT_ASSERT_PRE_PLUGIN_OUT_NAME)

#define _BT_ASSERT_PRE_ERROR_NAME	"Error"
#define _BT_ASSERT_PRE_ERROR_ID		"error"

#define BT_ASSERT_PRE_ERROR_NON_NULL(_error)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_ERROR_ID, (_error),	\
		_BT_ASSERT_PRE_ERROR_NAME)

#define BT_ASSERT_PRE_DEV_ERROR_NON_NULL(_error)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_ERROR_ID, (_error),	\
		_BT_ASSERT_PRE_ERROR_NAME)

#define _BT_ASSERT_PRE_ERROR_CAUSE_NAME	"Error cause"
#define _BT_ASSERT_PRE_ERROR_CAUSE_ID	"error-cause"

#define BT_ASSERT_PRE_ERROR_CAUSE_NON_NULL(_error_cause)		\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_ERROR_CAUSE_ID,		\
		(_error_cause), _BT_ASSERT_PRE_ERROR_CAUSE_NAME)

#define BT_ASSERT_PRE_DEV_ERROR_CAUSE_NON_NULL(_error_cause)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_ERROR_CAUSE_ID,	\
		(_error_cause), _BT_ASSERT_PRE_ERROR_CAUSE_NAME)

#define _BT_ASSERT_PRE_INT_RANGE_NAME	"Integer range"
#define _BT_ASSERT_PRE_INT_RANGE_ID	"integer-range"

#define BT_ASSERT_PRE_INT_RANGE_NON_NULL(_int_range)			\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_INT_RANGE_ID,		\
		(_int_range), _BT_ASSERT_PRE_INT_RANGE_NAME)

#define BT_ASSERT_PRE_DEV_INT_RANGE_NON_NULL(_int_range)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_INT_RANGE_ID,		\
		(_int_range), _BT_ASSERT_PRE_INT_RANGE_NAME)

#define BT_ASSERT_PRE_INT_RANGE_SET_NOT_EMPTY_FROM_FUNC(_func, _range_set) \
	BT_ASSERT_PRE_FROM_FUNC(_func, "integer-range-set-is-not-empty", \
		(_range_set)->ranges->len > 0,				\
		"Integer range set is empty: %!+R", (_range_set))

#define BT_ASSERT_PRE_INT_RANGE_SET_NOT_EMPTY(_range_set)		\
	BT_ASSERT_PRE_INT_RANGE_SET_NOT_EMPTY_FROM_FUNC(__func__,	\
		(_range_set))

#define _BT_ASSERT_PRE_INT_RANGE_SET_NAME	"Integer range set"
#define _BT_ASSERT_PRE_INT_RANGE_SET_ID		"integer-range-set"

#define BT_ASSERT_PRE_INT_RANGE_SET_NON_NULL_FROM_FUNC(_func, _range_set) \
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_INT_RANGE_SET_ID, (_range_set),		\
		_BT_ASSERT_PRE_INT_RANGE_SET_NAME)

#define BT_ASSERT_PRE_INT_RANGE_SET_NON_NULL(_range_set)		\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_INT_RANGE_SET_ID,		\
		(_range_set), _BT_ASSERT_PRE_INT_RANGE_SET_NAME)

#define BT_ASSERT_PRE_DEV_INT_RANGE_SET_NON_NULL(_range_set)		\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_INT_RANGE_SET_ID,	\
		(_range_set), _BT_ASSERT_PRE_INT_RANGE_SET_NAME)

#define _BT_ASSERT_PRE_VALUE_HAS_TYPE_COND(_value, _type)		\
	(((struct bt_value *) (_value))->type == (_type))

#define _BT_ASSERT_PRE_VALUE_HAS_TYPE_FMT				\
	"Value has the wrong type: expected-type=%s, %![value-]+v"

#define _BT_ASSERT_PRE_VALUE_HAS_TYPE_ID(_value_id, _type_id)		\
	"is-" _type_id "-value:" _value_id

#define BT_ASSERT_PRE_VALUE_HAS_TYPE_FROM_FUNC(_func, _value_id, _value, _type_id, _type) \
	BT_ASSERT_PRE_FROM_FUNC(_func,					\
		_BT_ASSERT_PRE_VALUE_HAS_TYPE_ID(_value_id, _type_id),	\
		_BT_ASSERT_PRE_VALUE_HAS_TYPE_COND((_value), (_type)),	\
		_BT_ASSERT_PRE_VALUE_HAS_TYPE_FMT,			\
		bt_common_value_type_string(_type), (_value))

#define BT_ASSERT_PRE_VALUE_HAS_TYPE(_value_id, _value, _type_id, _type) \
	BT_ASSERT_PRE_VALUE_HAS_TYPE_FROM_FUNC(__func__, _value_id,	\
		(_value), _type_id, (_type));

#define BT_ASSERT_PRE_VALUE_IS_BOOL(_value) 				\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"boolean", BT_VALUE_TYPE_BOOL);

#define BT_ASSERT_PRE_VALUE_IS_UNSIGNED_INT(_value) 			\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"unsigned-int", BT_VALUE_TYPE_UNSIGNED_INTEGER);

#define BT_ASSERT_PRE_VALUE_IS_SIGNED_INT(_value) 			\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"signed-int", BT_VALUE_TYPE_SIGNED_INTEGER);

#define BT_ASSERT_PRE_VALUE_IS_REAL(_value)	 			\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"real", BT_VALUE_TYPE_REAL);

#define BT_ASSERT_PRE_VALUE_IS_STRING(_value)	 			\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"string", BT_VALUE_TYPE_STRING);

#define BT_ASSERT_PRE_VALUE_IS_ARRAY(_value) 				\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"array", BT_VALUE_TYPE_ARRAY);

#define BT_ASSERT_PRE_VALUE_IS_MAP_FROM_FUNC(_func, _value)		\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"map", BT_VALUE_TYPE_MAP);

#define BT_ASSERT_PRE_VALUE_IS_MAP(_value) 				\
	BT_ASSERT_PRE_VALUE_HAS_TYPE("value-object", (_value),		\
		"map", BT_VALUE_TYPE_MAP);

#define BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE(_value_id, _value, _type_id, _type) \
	BT_ASSERT_PRE_DEV(						\
		_BT_ASSERT_PRE_VALUE_HAS_TYPE_ID(_value_id, _type_id),	\
		_BT_ASSERT_PRE_VALUE_HAS_TYPE_COND((_value), (_type)),	\
		_BT_ASSERT_PRE_VALUE_HAS_TYPE_FMT,			\
		bt_common_value_type_string(_type), (_value))

#define BT_ASSERT_PRE_DEV_VALUE_IS_BOOL(_value) 			\
	BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE("value-object", (_value),	\
		"boolean", BT_VALUE_TYPE_BOOL);

#define BT_ASSERT_PRE_DEV_VALUE_IS_UNSIGNED_INT(_value) 		\
	BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE("value-object", (_value),	\
		"unsigned-int", BT_VALUE_TYPE_UNSIGNED_INTEGER);

#define BT_ASSERT_PRE_DEV_VALUE_IS_SIGNED_INT(_value) 			\
	BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE("value-object", (_value),	\
		"signed-int", BT_VALUE_TYPE_SIGNED_INTEGER);

#define BT_ASSERT_PRE_DEV_VALUE_IS_REAL(_value)	 			\
	BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE("value-object", (_value),	\
		"real", BT_VALUE_TYPE_REAL);

#define BT_ASSERT_PRE_DEV_VALUE_IS_STRING(_value)	 		\
	BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE("value-object", (_value),	\
		"string", BT_VALUE_TYPE_STRING);

#define BT_ASSERT_PRE_DEV_VALUE_IS_ARRAY(_value) 			\
	BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE("value-object", (_value),	\
		"array", BT_VALUE_TYPE_ARRAY);

#define BT_ASSERT_PRE_DEV_VALUE_IS_MAP(_value) 				\
	BT_ASSERT_PRE_DEV_VALUE_HAS_TYPE("value-object", (_value),	\
		"map", BT_VALUE_TYPE_MAP);

#define _BT_ASSERT_PRE_VALUE_NAME	"Value object"
#define _BT_ASSERT_PRE_VALUE_ID		"value-object"

#define BT_ASSERT_PRE_VALUE_NON_NULL_FROM_FUNC(_func, _value)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func,				\
		_BT_ASSERT_PRE_VALUE_ID, (_value), _BT_ASSERT_PRE_VALUE_NAME)

#define BT_ASSERT_PRE_VALUE_NON_NULL(_value)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_VALUE_ID, (_value),	\
		_BT_ASSERT_PRE_VALUE_NAME)

#define BT_ASSERT_PRE_DEV_VALUE_NON_NULL_FROM_FUNC(_func, _value)	\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_VALUE_ID, (_value), _BT_ASSERT_PRE_VALUE_NAME)

#define BT_ASSERT_PRE_DEV_VALUE_NON_NULL(_value)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_VALUE_ID, (_value),	\
		_BT_ASSERT_PRE_VALUE_NAME)

#define BT_ASSERT_PRE_PARAM_VALUE_IS_MAP_FROM_FUNC(_func, _value)	\
	BT_ASSERT_PRE_FROM_FUNC(_func,					\
		"is-map-value:parameters-value-object",			\
		!(_value) || bt_value_is_map(_value),			\
		"Parameters value object is not a map value: %!+v", (_value));

#define BT_ASSERT_PRE_PARAM_VALUE_IS_MAP(_value)			\
	BT_ASSERT_PRE_PARAM_VALUE_IS_MAP_FROM_FUNC(__func__, (_value))

#define _BT_ASSERT_PRE_RES_OUT_NAME	"Result (output)"
#define _BT_ASSERT_PRE_RES_OUT_ID	"result-output"

#define BT_ASSERT_PRE_RES_OUT_NON_NULL(_res)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_RES_OUT_ID, (_res),	\
		_BT_ASSERT_PRE_RES_OUT_NAME)

#define BT_ASSERT_PRE_DEV_RES_OUT_NON_NULL(_res)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_RES_OUT_ID, (_res),	\
		_BT_ASSERT_PRE_RES_OUT_NAME)

#define BT_ASSERT_PRE_METHOD_NON_NULL(_method)				\
	BT_ASSERT_PRE_NON_NULL("method", (_method), "Method");

#define _BT_ASSERT_PRE_NAME_NAME	"Name"
#define _BT_ASSERT_PRE_NAME_ID		"name"

#define BT_ASSERT_PRE_NAME_NON_NULL_FROM_FUNC(_func, _name)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func, _BT_ASSERT_PRE_NAME_ID,	\
		(_name), _BT_ASSERT_PRE_NAME_NAME)

#define BT_ASSERT_PRE_NAME_NON_NULL(_name)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_NAME_ID, (_name),		\
		_BT_ASSERT_PRE_NAME_NAME)

#define BT_ASSERT_PRE_DEV_NAME_NON_NULL_FROM_FUNC(_func, _name)		\
	BT_ASSERT_PRE_DEV_NON_NULL_FROM_FUNC(_func,			\
		_BT_ASSERT_PRE_NAME_ID, (_name), _BT_ASSERT_PRE_NAME_NAME)

#define BT_ASSERT_PRE_DEV_NAME_NON_NULL(_name)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_NAME_ID, (_name),	\
		_BT_ASSERT_PRE_NAME_NAME)

#define _BT_ASSERT_PRE_DESCR_NAME	"Description"
#define _BT_ASSERT_PRE_DESCR_ID		"description"

#define BT_ASSERT_PRE_DESCR_NON_NULL(_descr)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_DESCR_ID, (_descr),	\
		_BT_ASSERT_PRE_DESCR_NAME)

#define BT_ASSERT_PRE_DEV_DESCR_NON_NULL(_descr)			\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_DESCR_ID, (_descr),	\
		_BT_ASSERT_PRE_DESCR_NAME)

#define _BT_ASSERT_PRE_UUID_NAME	"UUID"
#define _BT_ASSERT_PRE_UUID_ID		"uuid"

#define BT_ASSERT_PRE_UUID_NON_NULL(_uuid)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_UUID_ID, (_uuid),		\
		_BT_ASSERT_PRE_UUID_NAME)

#define BT_ASSERT_PRE_DEV_UUID_NON_NULL(_uuid)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_UUID_ID, (_uuid),	\
		_BT_ASSERT_PRE_UUID_NAME)

#define _BT_ASSERT_PRE_KEY_NAME	"Key"
#define _BT_ASSERT_PRE_KEY_ID	"key"

#define BT_ASSERT_PRE_KEY_NON_NULL_FROM_FUNC(_func, _key)		\
	BT_ASSERT_PRE_NON_NULL_FROM_FUNC(_func, _BT_ASSERT_PRE_KEY_ID,	\
		(_key), _BT_ASSERT_PRE_KEY_NAME)

#define BT_ASSERT_PRE_KEY_NON_NULL(_key)				\
	BT_ASSERT_PRE_NON_NULL(_BT_ASSERT_PRE_KEY_ID, (_key),		\
		_BT_ASSERT_PRE_KEY_NAME)

#define BT_ASSERT_PRE_DEV_KEY_NON_NULL(_key)				\
	BT_ASSERT_PRE_DEV_NON_NULL(_BT_ASSERT_PRE_KEY_ID, (_key),	\
		_BT_ASSERT_PRE_KEY_NAME)

#endif /* BABELTRACE_ASSERT_COND_INTERNAL_H */

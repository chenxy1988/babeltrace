/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 */

#define BT_LOG_TAG "LIB/PLUGIN-SO"
#include "lib/logging.h"

#include "common/assert.h"
#include "lib/assert-cond.h"
#include "compat/compiler.h"
#include <babeltrace2/plugin/plugin-dev.h>
#include "lib/graph/component-class.h"
#include <babeltrace2/graph/component-class.h>
#include <babeltrace2/types.h>
#include "common/list.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>

#include "plugin.h"
#include "plugin-so.h"
#include "lib/func-status.h"
#include "common/common.h"

#define NATIVE_PLUGIN_SUFFIX		"." G_MODULE_SUFFIX
#define NATIVE_PLUGIN_SUFFIX_LEN	sizeof(NATIVE_PLUGIN_SUFFIX)
#define LIBTOOL_PLUGIN_SUFFIX		".la"
#define LIBTOOL_PLUGIN_SUFFIX_LEN	sizeof(LIBTOOL_PLUGIN_SUFFIX)

#define PLUGIN_SUFFIX_LEN	bt_max_t(size_t, sizeof(NATIVE_PLUGIN_SUFFIX), \
					sizeof(LIBTOOL_PLUGIN_SUFFIX))

BT_PLUGIN_MODULE();

/*
 * This list, global to the library, keeps all component classes that
 * have a reference to their shared library handles. It allows iteration
 * on all component classes still present when the destructor executes
 * to release the shared library handle references they might still have.
 *
 * The list items are the component classes created with
 * bt_plugin_add_component_class(). They keep the shared library handle
 * object created by their plugin alive so that the plugin's code is
 * not discarded when it could still be in use by living components
 * created from those component classes:
 *
 *     [component] --ref-> [component class]-> [shlib handle]
 *
 * It allows this use-case:
 *
 *        my_plugins = bt_plugin_find_all_from_file("/path/to/my-plugin.so");
 *        // instantiate components from a plugin's component classes
 *        // put plugins and free my_plugins here
 *        // user code of instantiated components still exists
 *
 * An entry is removed from this list when a component class is
 * destroyed thanks to a custom destroy listener. When the entry is
 * removed, the entry is removed from the list, and we release the
 * reference on the shlib handle. Assuming the original plugin object
 * which contained some component classes is put first, when the last
 * component class is removed from this list, the shared library handle
 * object's reference count falls to zero and the shared library is
 * finally closed.
 *
 * We're not using a GLib linked list here because this destructor is
 * called after GLib's thread-specific data is destroyed, which contains
 * the allocated memory for GLib data structures (what's used by
 * g_slice_alloc()).
 */

static
BT_LIST_HEAD(component_class_list);

__attribute__((destructor)) static
void fini_comp_class_list(void)
{
	struct bt_component_class *comp_class, *tmp;

	bt_list_for_each_entry_safe(comp_class, tmp, &component_class_list, node) {
		bt_list_del(&comp_class->node);
		BT_OBJECT_PUT_REF_AND_RESET(comp_class->so_handle);
	}

	BT_LOGD_STR("Released references from all component classes to shared library handles.");
}

static
void bt_plugin_so_shared_lib_handle_destroy(struct bt_object *obj)
{
	struct bt_plugin_so_shared_lib_handle *shared_lib_handle;

	BT_ASSERT(obj);
	shared_lib_handle = container_of(obj,
		struct bt_plugin_so_shared_lib_handle, base);
	const char *path = shared_lib_handle->path ?
		shared_lib_handle->path->str : NULL;

	BT_LOGI("Destroying shared library handle: addr=%p, path=\"%s\"",
		shared_lib_handle, path);

	if (shared_lib_handle->init_called && shared_lib_handle->exit) {
		BT_LOGD_STR("Calling user's plugin exit function.");
		shared_lib_handle->exit();
		BT_LOGD_STR("User function returned.");
	}

	if (shared_lib_handle->module) {
#ifdef BT_DEBUG_MODE
		/*
		 * Valgrind shows incomplete stack traces when
		 * dynamically loaded libraries are closed before it
		 * finishes. Use the LIBBABELTRACE2_NO_DLCLOSE in a debug
		 * build to avoid this.
		 */
		const char *var = getenv("LIBBABELTRACE2_NO_DLCLOSE");

		if (!var || strcmp(var, "1") != 0) {
#endif
			BT_LOGI("Closing GModule: path=\"%s\"", path);

			if (!g_module_close(shared_lib_handle->module)) {
				/*
				 * Just log here: we're in a destructor,
				 * so we cannot append an error cause
				 * (there's no returned status).
				 */
				BT_LOGE("Cannot close GModule: %s: path=\"%s\"",
					g_module_error(), path);
			}

			shared_lib_handle->module = NULL;
#ifdef BT_DEBUG_MODE
		} else {
			BT_LOGI("Not closing GModule because `LIBBABELTRACE2_NO_DLCLOSE=1`: "
				"path=\"%s\"", path);
		}
#endif
	}

	if (shared_lib_handle->path) {
		g_string_free(shared_lib_handle->path, TRUE);
		shared_lib_handle->path = NULL;
	}

	g_free(shared_lib_handle);
}

static
int bt_plugin_so_shared_lib_handle_create(
		const char *path,
		struct bt_plugin_so_shared_lib_handle **shared_lib_handle)
{
	int status = BT_FUNC_STATUS_OK;

	BT_ASSERT(shared_lib_handle);
	BT_LOGI("Creating shared library handle: path=\"%s\"", path ? path : "(null)");
	*shared_lib_handle = g_new0(struct bt_plugin_so_shared_lib_handle, 1);
	if (!*shared_lib_handle) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate one shared library handle.");
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	bt_object_init_shared(&(*shared_lib_handle)->base,
		bt_plugin_so_shared_lib_handle_destroy);

	if (!path) {
		goto end;
	}

	(*shared_lib_handle)->path = g_string_new(path);
	if (!(*shared_lib_handle)->path) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GString.");
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	(*shared_lib_handle)->module = g_module_open(path, G_MODULE_BIND_LOCAL);
	if (!(*shared_lib_handle)->module) {
		/*
		 * INFO-level logging because we're only _trying_ to
		 * open this file as a Babeltrace plugin: if it's not,
		 * it's not an error. And because this can be tried
		 * during bt_plugin_find_all_from_dir(), it's not even a
		 * warning.
		 */
		BT_LOGI("Cannot open GModule: %s: path=\"%s\"",
			g_module_error(), path);
		BT_OBJECT_PUT_REF_AND_RESET(*shared_lib_handle);
		status = BT_FUNC_STATUS_NOT_FOUND;
		goto end;
	}

	goto end;

end:
	BT_ASSERT(*shared_lib_handle || status != BT_FUNC_STATUS_OK);
	if (*shared_lib_handle) {
		BT_LOGI("Created shared library handle: path=\"%s\", addr=%p",
			path ? path : "(null)", *shared_lib_handle);
	}

	return status;
}

static
void bt_plugin_so_destroy_spec_data(struct bt_plugin *plugin)
{
	struct bt_plugin_so_spec_data *spec = plugin->spec_data;

	if (!plugin->spec_data) {
		return;
	}

	BT_ASSERT(plugin->type == BT_PLUGIN_TYPE_SO);
	BT_ASSERT(spec);
	BT_OBJECT_PUT_REF_AND_RESET(spec->shared_lib_handle);
	g_free(plugin->spec_data);
	plugin->spec_data = NULL;
}

/*
 * This function does the following:
 *
 * 1. Iterate on the plugin descriptor attributes section and set the
 *    plugin's attributes depending on the attribute types. This
 *    includes the name of the plugin, its description, and its
 *    initialization function, for example.
 *
 * 2. Iterate on the component class descriptors section and create one
 *    "full descriptor" (temporary structure) for each one that is found
 *    and attached to our plugin descriptor.
 *
 * 3. Iterate on the component class descriptor attributes section and
 *    set the corresponding full descriptor's attributes depending on
 *    the attribute types. This includes the description of the
 *    component class, as well as its initialization and destroy
 *    methods.
 *
 * 4. Call the user's plugin initialization function, if any is
 *    defined.
 *
 * 5. For each full component class descriptor, create a component class
 *    object, set its optional attributes, and add it to the plugin
 *    object.
 *
 * 6. Freeze the plugin object.
 */
static
int bt_plugin_so_init(struct bt_plugin *plugin,
		bool fail_on_load_error,
		const struct __bt_plugin_descriptor *descriptor,
		struct __bt_plugin_descriptor_attribute const * const *attrs_begin,
		struct __bt_plugin_descriptor_attribute const * const *attrs_end,
		struct __bt_plugin_component_class_descriptor const * const *cc_descriptors_begin,
		struct __bt_plugin_component_class_descriptor const * const *cc_descriptors_end,
		struct __bt_plugin_component_class_descriptor_attribute const * const *cc_descr_attrs_begin,
		struct __bt_plugin_component_class_descriptor_attribute const * const *cc_descr_attrs_end)
{
	/*
	 * This structure's members point to the plugin's memory
	 * (do NOT free).
	 */
	struct comp_class_full_descriptor {
		const struct __bt_plugin_component_class_descriptor *descriptor;
		const char *description;
		const char *help;

		union {
			struct {
				bt_component_class_source_get_supported_mip_versions_method get_supported_mip_versions;
				bt_component_class_source_initialize_method init;
				bt_component_class_source_finalize_method finalize;
				bt_component_class_source_query_method query;
				bt_component_class_source_output_port_connected_method output_port_connected;
				bt_message_iterator_class_initialize_method msg_iter_initialize;
				bt_message_iterator_class_finalize_method msg_iter_finalize;
				bt_message_iterator_class_seek_ns_from_origin_method msg_iter_seek_ns_from_origin;
				bt_message_iterator_class_seek_beginning_method msg_iter_seek_beginning;
				bt_message_iterator_class_can_seek_ns_from_origin_method msg_iter_can_seek_ns_from_origin;
				bt_message_iterator_class_can_seek_beginning_method msg_iter_can_seek_beginning;
			} source;

			struct {
				bt_component_class_filter_get_supported_mip_versions_method get_supported_mip_versions;
				bt_component_class_filter_initialize_method init;
				bt_component_class_filter_finalize_method finalize;
				bt_component_class_filter_query_method query;
				bt_component_class_filter_input_port_connected_method input_port_connected;
				bt_component_class_filter_output_port_connected_method output_port_connected;
				bt_message_iterator_class_initialize_method msg_iter_initialize;
				bt_message_iterator_class_finalize_method msg_iter_finalize;
				bt_message_iterator_class_seek_ns_from_origin_method msg_iter_seek_ns_from_origin;
				bt_message_iterator_class_seek_beginning_method msg_iter_seek_beginning;
				bt_message_iterator_class_can_seek_ns_from_origin_method msg_iter_can_seek_ns_from_origin;
				bt_message_iterator_class_can_seek_beginning_method msg_iter_can_seek_beginning;
			} filter;

			struct {
				bt_component_class_sink_get_supported_mip_versions_method get_supported_mip_versions;
				bt_component_class_sink_initialize_method init;
				bt_component_class_sink_finalize_method finalize;
				bt_component_class_sink_query_method query;
				bt_component_class_sink_input_port_connected_method input_port_connected;
				bt_component_class_sink_graph_is_configured_method graph_is_configured;
			} sink;
		} methods;
	};

	int status = BT_FUNC_STATUS_OK;
	struct __bt_plugin_descriptor_attribute const * const *cur_attr_ptr;
	struct __bt_plugin_component_class_descriptor const * const *cur_cc_descr_ptr;
	struct __bt_plugin_component_class_descriptor_attribute const * const *cur_cc_descr_attr_ptr;
	struct bt_plugin_so_spec_data *spec = plugin->spec_data;
	GArray *comp_class_full_descriptors;
	size_t i;
	int ret;
	struct bt_message_iterator_class *msg_iter_class = NULL;

	BT_LOGI("Initializing plugin object from descriptors found in sections: "
		"plugin-addr=%p, plugin-path=\"%s\", "
		"attrs-begin-addr=%p, attrs-end-addr=%p, "
		"cc-descr-begin-addr=%p, cc-descr-end-addr=%p, "
		"cc-descr-attrs-begin-addr=%p, cc-descr-attrs-end-addr=%p",
		plugin,
		spec->shared_lib_handle->path ?
			spec->shared_lib_handle->path->str : NULL,
		attrs_begin, attrs_end,
		cc_descriptors_begin, cc_descriptors_end,
		cc_descr_attrs_begin, cc_descr_attrs_end);
	comp_class_full_descriptors = g_array_new(FALSE, TRUE,
		sizeof(struct comp_class_full_descriptor));
	if (!comp_class_full_descriptors) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GArray.");
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	/* Set mandatory attributes */
	spec->descriptor = descriptor;
	bt_plugin_set_name(plugin, descriptor->name);

	/*
	 * Find and set optional attributes attached to this plugin
	 * descriptor.
	 */
	for (cur_attr_ptr = attrs_begin; cur_attr_ptr != attrs_end; cur_attr_ptr++) {
		const struct __bt_plugin_descriptor_attribute *cur_attr =
			*cur_attr_ptr;

		if (!cur_attr) {
			continue;
		}

		if (cur_attr->plugin_descriptor != descriptor) {
			continue;
		}

		switch (cur_attr->type) {
		case BT_PLUGIN_DESCRIPTOR_ATTRIBUTE_TYPE_INIT:
			spec->init = cur_attr->value.init;
			break;
		case BT_PLUGIN_DESCRIPTOR_ATTRIBUTE_TYPE_EXIT:
			spec->shared_lib_handle->exit = cur_attr->value.exit;
			break;
		case BT_PLUGIN_DESCRIPTOR_ATTRIBUTE_TYPE_AUTHOR:
			bt_plugin_set_author(plugin, cur_attr->value.author);
			break;
		case BT_PLUGIN_DESCRIPTOR_ATTRIBUTE_TYPE_LICENSE:
			bt_plugin_set_license(plugin, cur_attr->value.license);
			break;
		case BT_PLUGIN_DESCRIPTOR_ATTRIBUTE_TYPE_DESCRIPTION:
			bt_plugin_set_description(plugin, cur_attr->value.description);
			break;
		case BT_PLUGIN_DESCRIPTOR_ATTRIBUTE_TYPE_VERSION:
			bt_plugin_set_version(plugin,
				(unsigned int) cur_attr->value.version.major,
				(unsigned int) cur_attr->value.version.minor,
				(unsigned int) cur_attr->value.version.patch,
				cur_attr->value.version.extra);
			break;
		default:
			if (fail_on_load_error) {
				BT_LIB_LOGW_APPEND_CAUSE(
					"Unknown plugin descriptor attribute: "
					"plugin-path=\"%s\", plugin-name=\"%s\", "
					"attr-type-name=\"%s\", attr-type-id=%d",
					spec->shared_lib_handle->path ?
						spec->shared_lib_handle->path->str :
						NULL,
					descriptor->name, cur_attr->type_name,
					cur_attr->type);
				status = BT_FUNC_STATUS_ERROR;
				goto end;
			} else {
				BT_LIB_LOGW(
					"Ignoring unknown plugin descriptor attribute: "
					"plugin-path=\"%s\", plugin-name=\"%s\", "
					"attr-type-name=\"%s\", attr-type-id=%d",
					spec->shared_lib_handle->path ?
						spec->shared_lib_handle->path->str :
						NULL,
					descriptor->name, cur_attr->type_name,
					cur_attr->type);
			}

			break;
		}
	}

	/*
	 * Find component class descriptors attached to this plugin
	 * descriptor and initialize corresponding full component class
	 * descriptors in the array.
	 */
	for (cur_cc_descr_ptr = cc_descriptors_begin; cur_cc_descr_ptr != cc_descriptors_end; cur_cc_descr_ptr++) {
		const struct __bt_plugin_component_class_descriptor *cur_cc_descr =
			*cur_cc_descr_ptr;
		struct comp_class_full_descriptor full_descriptor = {0};

		if (!cur_cc_descr) {
			continue;
		}

		if (cur_cc_descr->plugin_descriptor != descriptor) {
			continue;
		}

		full_descriptor.descriptor = cur_cc_descr;
		g_array_append_val(comp_class_full_descriptors,
			full_descriptor);
	}

	/*
	 * Find component class descriptor attributes attached to this
	 * plugin descriptor and update corresponding full component
	 * class descriptors in the array.
	 */
	for (cur_cc_descr_attr_ptr = cc_descr_attrs_begin; cur_cc_descr_attr_ptr != cc_descr_attrs_end; cur_cc_descr_attr_ptr++) {
		const struct __bt_plugin_component_class_descriptor_attribute *cur_cc_descr_attr =
			*cur_cc_descr_attr_ptr;
		enum bt_component_class_type cc_type;

		if (!cur_cc_descr_attr) {
			continue;
		}

		if (cur_cc_descr_attr->comp_class_descriptor->plugin_descriptor !=
				descriptor) {
			continue;
		}

		cc_type = cur_cc_descr_attr->comp_class_descriptor->type;

		/* Find the corresponding component class descriptor entry */
		for (i = 0; i < comp_class_full_descriptors->len; i++) {
			struct comp_class_full_descriptor *cc_full_descr =
				&g_array_index(comp_class_full_descriptors,
					struct comp_class_full_descriptor, i);

			if (cur_cc_descr_attr->comp_class_descriptor !=
					cc_full_descr->descriptor) {
				continue;
			}

			switch (cur_cc_descr_attr->type) {
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_DESCRIPTION:
				cc_full_descr->description =
					cur_cc_descr_attr->value.description;
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_HELP:
				cc_full_descr->help =
					cur_cc_descr_attr->value.help;
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_GET_SUPPORTED_MIP_VERSIONS_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.get_supported_mip_versions =
						cur_cc_descr_attr->value.source_get_supported_mip_versions_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.get_supported_mip_versions =
						cur_cc_descr_attr->value.filter_get_supported_mip_versions_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_SINK:
					cc_full_descr->methods.sink.get_supported_mip_versions =
						cur_cc_descr_attr->value.sink_get_supported_mip_versions_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_INITIALIZE_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.init =
						cur_cc_descr_attr->value.source_initialize_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.init =
						cur_cc_descr_attr->value.filter_initialize_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_SINK:
					cc_full_descr->methods.sink.init =
						cur_cc_descr_attr->value.sink_initialize_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_FINALIZE_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.finalize =
						cur_cc_descr_attr->value.source_finalize_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.finalize =
						cur_cc_descr_attr->value.filter_finalize_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_SINK:
					cc_full_descr->methods.sink.finalize =
						cur_cc_descr_attr->value.sink_finalize_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_QUERY_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.query =
						cur_cc_descr_attr->value.source_query_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.query =
						cur_cc_descr_attr->value.filter_query_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_SINK:
					cc_full_descr->methods.sink.query =
						cur_cc_descr_attr->value.sink_query_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_INPUT_PORT_CONNECTED_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.input_port_connected =
						cur_cc_descr_attr->value.filter_input_port_connected_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_SINK:
					cc_full_descr->methods.sink.input_port_connected =
						cur_cc_descr_attr->value.sink_input_port_connected_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_OUTPUT_PORT_CONNECTED_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.output_port_connected =
						cur_cc_descr_attr->value.source_output_port_connected_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.output_port_connected =
						cur_cc_descr_attr->value.filter_output_port_connected_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_GRAPH_IS_CONFIGURED_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SINK:
					cc_full_descr->methods.sink.graph_is_configured =
						cur_cc_descr_attr->value.sink_graph_is_configured_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_MSG_ITER_INITIALIZE_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.msg_iter_initialize =
						cur_cc_descr_attr->value.msg_iter_initialize_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.msg_iter_initialize =
						cur_cc_descr_attr->value.msg_iter_initialize_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_MSG_ITER_FINALIZE_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.msg_iter_finalize =
						cur_cc_descr_attr->value.msg_iter_finalize_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.msg_iter_finalize =
						cur_cc_descr_attr->value.msg_iter_finalize_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_MSG_ITER_SEEK_NS_FROM_ORIGIN_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.msg_iter_seek_ns_from_origin =
						cur_cc_descr_attr->value.msg_iter_seek_ns_from_origin_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.msg_iter_seek_ns_from_origin =
						cur_cc_descr_attr->value.msg_iter_seek_ns_from_origin_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_MSG_ITER_SEEK_BEGINNING_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.msg_iter_seek_beginning =
						cur_cc_descr_attr->value.msg_iter_seek_beginning_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.msg_iter_seek_beginning =
						cur_cc_descr_attr->value.msg_iter_seek_beginning_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_MSG_ITER_CAN_SEEK_NS_FROM_ORIGIN_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.msg_iter_can_seek_ns_from_origin =
						cur_cc_descr_attr->value.msg_iter_can_seek_ns_from_origin_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.msg_iter_can_seek_ns_from_origin =
						cur_cc_descr_attr->value.msg_iter_can_seek_ns_from_origin_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			case BT_PLUGIN_COMPONENT_CLASS_DESCRIPTOR_ATTRIBUTE_TYPE_MSG_ITER_CAN_SEEK_BEGINNING_METHOD:
				switch (cc_type) {
				case BT_COMPONENT_CLASS_TYPE_SOURCE:
					cc_full_descr->methods.source.msg_iter_can_seek_beginning =
						cur_cc_descr_attr->value.msg_iter_can_seek_beginning_method;
					break;
				case BT_COMPONENT_CLASS_TYPE_FILTER:
					cc_full_descr->methods.filter.msg_iter_can_seek_beginning =
						cur_cc_descr_attr->value.msg_iter_can_seek_beginning_method;
					break;
				default:
					bt_common_abort();
				}
				break;
			default:
				if (fail_on_load_error) {
					BT_LIB_LOGW_APPEND_CAUSE(
						"Unknown component class descriptor attribute: "
						"plugin-path=\"%s\", "
						"plugin-name=\"%s\", "
						"comp-class-name=\"%s\", "
						"comp-class-type=%s, "
						"attr-type-name=\"%s\", "
						"attr-type-id=%d",
						spec->shared_lib_handle->path ?
							spec->shared_lib_handle->path->str :
							NULL,
						descriptor->name,
						cur_cc_descr_attr->comp_class_descriptor->name,
						bt_common_component_class_type_string(
							cur_cc_descr_attr->comp_class_descriptor->type),
						cur_cc_descr_attr->type_name,
						cur_cc_descr_attr->type);
					status = BT_FUNC_STATUS_ERROR;
					goto end;
				} else {
					BT_LIB_LOGW(
						"Ignoring unknown component class descriptor attribute: "
						"plugin-path=\"%s\", "
						"plugin-name=\"%s\", "
						"comp-class-name=\"%s\", "
						"comp-class-type=%s, "
						"attr-type-name=\"%s\", "
						"attr-type-id=%d",
						spec->shared_lib_handle->path ?
							spec->shared_lib_handle->path->str :
							NULL,
						descriptor->name,
						cur_cc_descr_attr->comp_class_descriptor->name,
						bt_common_component_class_type_string(
							cur_cc_descr_attr->comp_class_descriptor->type),
						cur_cc_descr_attr->type_name,
						cur_cc_descr_attr->type);
				}

				break;
			}
		}
	}

	/* Initialize plugin */
	if (spec->init) {
		enum bt_plugin_initialize_func_status init_status;

		BT_LOGD_STR("Calling user's plugin initialization function.");
		init_status = spec->init((void *) plugin);
		BT_LOGD("User function returned: status=%s",
			bt_common_func_status_string(init_status));

		if (init_status < 0) {
			if (fail_on_load_error) {
				BT_LIB_LOGW_APPEND_CAUSE(
					"User's plugin initialization function failed: "
					"status=%s",
					bt_common_func_status_string(init_status));
				status = init_status;
				goto end;
			} else {
				BT_LIB_LOGW(
					"User's plugin initialization function failed: "
					"status=%s",
					bt_common_func_status_string(init_status));
				status = BT_FUNC_STATUS_NOT_FOUND;
			}

			goto end;
		}
	}

	spec->shared_lib_handle->init_called = BT_TRUE;

	/* Add described component classes to plugin */
	for (i = 0; i < comp_class_full_descriptors->len; i++) {
		struct comp_class_full_descriptor *cc_full_descr =
			&g_array_index(comp_class_full_descriptors,
				struct comp_class_full_descriptor, i);
		struct bt_component_class *comp_class = NULL;
		struct bt_component_class_source *src_comp_class = NULL;
		struct bt_component_class_filter *flt_comp_class = NULL;
		struct bt_component_class_sink *sink_comp_class = NULL;

		BT_LOGI("Creating and setting properties of plugin's component class: "
			"plugin-path=\"%s\", plugin-name=\"%s\", "
			"comp-class-name=\"%s\", comp-class-type=%s",
			spec->shared_lib_handle->path ?
				spec->shared_lib_handle->path->str :
				NULL,
			descriptor->name,
			cc_full_descr->descriptor->name,
			bt_common_component_class_type_string(
				cc_full_descr->descriptor->type));

		if (cc_full_descr->descriptor->type == BT_COMPONENT_CLASS_TYPE_SOURCE ||
				cc_full_descr->descriptor->type == BT_COMPONENT_CLASS_TYPE_FILTER) {
			bt_message_iterator_class_next_method next_method;
			bt_message_iterator_class_initialize_method init_method;
			bt_message_iterator_class_finalize_method fini_method;
			bt_message_iterator_class_seek_ns_from_origin_method seek_ns_from_origin_method;
			bt_message_iterator_class_seek_beginning_method seek_beginning_method;
			bt_message_iterator_class_can_seek_ns_from_origin_method can_seek_ns_from_origin_method;
			bt_message_iterator_class_can_seek_beginning_method can_seek_beginning_method;

			if (cc_full_descr->descriptor->type == BT_COMPONENT_CLASS_TYPE_SOURCE) {
				next_method = cc_full_descr->descriptor->methods.source.msg_iter_next;
				init_method = cc_full_descr->methods.source.msg_iter_initialize;
				fini_method = cc_full_descr->methods.source.msg_iter_finalize;
				seek_ns_from_origin_method = cc_full_descr->methods.source.msg_iter_seek_ns_from_origin;
				can_seek_ns_from_origin_method = cc_full_descr->methods.source.msg_iter_can_seek_ns_from_origin;
				seek_beginning_method = cc_full_descr->methods.source.msg_iter_seek_beginning;
				can_seek_beginning_method = cc_full_descr->methods.source.msg_iter_can_seek_beginning;
			} else {
				next_method = cc_full_descr->descriptor->methods.filter.msg_iter_next;
				init_method = cc_full_descr->methods.filter.msg_iter_initialize;
				fini_method = cc_full_descr->methods.filter.msg_iter_finalize;
				seek_ns_from_origin_method = cc_full_descr->methods.filter.msg_iter_seek_ns_from_origin;
				can_seek_ns_from_origin_method = cc_full_descr->methods.filter.msg_iter_can_seek_ns_from_origin;
				seek_beginning_method = cc_full_descr->methods.filter.msg_iter_seek_beginning;
				can_seek_beginning_method = cc_full_descr->methods.filter.msg_iter_can_seek_beginning;
			}

			msg_iter_class = bt_message_iterator_class_create(next_method);
			if (!msg_iter_class) {
				BT_LIB_LOGE_APPEND_CAUSE(
					"Cannot create message iterator class.");
				status = BT_FUNC_STATUS_MEMORY_ERROR;
				goto end;
			}

			if (init_method) {
				ret = bt_message_iterator_class_set_initialize_method(
					msg_iter_class, init_method);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set message iterator initialization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					goto end;
				}
			}

			if (fini_method) {
				ret = bt_message_iterator_class_set_finalize_method(
					msg_iter_class, fini_method);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set message iterator finalization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					goto end;
				}
			}

			if (seek_ns_from_origin_method) {
				ret = bt_message_iterator_class_set_seek_ns_from_origin_methods(
					msg_iter_class,
					seek_ns_from_origin_method,
					can_seek_ns_from_origin_method);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set message iterator \"seek nanoseconds from origin\" methods.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					goto end;
				}
			}

			if (seek_beginning_method) {
				ret = bt_message_iterator_class_set_seek_beginning_methods(
					msg_iter_class,
					seek_beginning_method,
					can_seek_beginning_method);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set message iterator \"seek beginning\" methods.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					goto end;
				}
			}
		}

		switch (cc_full_descr->descriptor->type) {
		case BT_COMPONENT_CLASS_TYPE_SOURCE:
			BT_ASSERT(msg_iter_class);

			src_comp_class = bt_component_class_source_create(
				cc_full_descr->descriptor->name, msg_iter_class);
			comp_class = bt_component_class_source_as_component_class(
				src_comp_class);
			break;
		case BT_COMPONENT_CLASS_TYPE_FILTER:
			BT_ASSERT(msg_iter_class);

			flt_comp_class = bt_component_class_filter_create(
				cc_full_descr->descriptor->name, msg_iter_class);
			comp_class = bt_component_class_filter_as_component_class(
				flt_comp_class);
			break;
		case BT_COMPONENT_CLASS_TYPE_SINK:
			BT_ASSERT(!msg_iter_class);

			sink_comp_class = bt_component_class_sink_create(
				cc_full_descr->descriptor->name,
				cc_full_descr->descriptor->methods.sink.consume);
			comp_class = bt_component_class_sink_as_component_class(
				sink_comp_class);
			break;
		default:
			if (fail_on_load_error) {
				BT_LIB_LOGW_APPEND_CAUSE(
					"Unknown component class type: "
					"plugin-path=\"%s\", plugin-name=\"%s\", "
					"comp-class-name=\"%s\", comp-class-type=%d",
					spec->shared_lib_handle->path->str ?
						spec->shared_lib_handle->path->str :
						NULL,
					descriptor->name,
					cc_full_descr->descriptor->name,
					cc_full_descr->descriptor->type);
				status = BT_FUNC_STATUS_ERROR;
				goto end;
			} else {
				BT_LIB_LOGW(
					"Ignoring unknown component class type: "
					"plugin-path=\"%s\", plugin-name=\"%s\", "
					"comp-class-name=\"%s\", comp-class-type=%d",
					spec->shared_lib_handle->path->str ?
						spec->shared_lib_handle->path->str :
						NULL,
					descriptor->name,
					cc_full_descr->descriptor->name,
					cc_full_descr->descriptor->type);
				continue;
			}
		}

		if (!comp_class) {
			BT_LIB_LOGE_APPEND_CAUSE(
				"Cannot create component class.");
			status = BT_FUNC_STATUS_MEMORY_ERROR;
			goto end;
		}

		/*
		 * The component class has taken a reference on the message
		 * iterator class, so we can drop ours.  The message iterator
		 * class will get destroyed at the same time as the component
		 * class.
		 */
		bt_message_iterator_class_put_ref(msg_iter_class);
		msg_iter_class = NULL;

		if (cc_full_descr->description) {
			ret = bt_component_class_set_description(
				comp_class, cc_full_descr->description);
			if (ret) {
				BT_LIB_LOGE_APPEND_CAUSE(
					"Cannot set component class's description.");
				status = BT_FUNC_STATUS_MEMORY_ERROR;
				BT_OBJECT_PUT_REF_AND_RESET(comp_class);
				goto end;
			}
		}

		if (cc_full_descr->help) {
			ret = bt_component_class_set_help(comp_class,
				cc_full_descr->help);
			if (ret) {
				BT_LIB_LOGE_APPEND_CAUSE(
					"Cannot set component class's help string.");
				status = BT_FUNC_STATUS_MEMORY_ERROR;
				BT_OBJECT_PUT_REF_AND_RESET(comp_class);
				goto end;
			}
		}

		switch (cc_full_descr->descriptor->type) {
		case BT_COMPONENT_CLASS_TYPE_SOURCE:
			if (cc_full_descr->methods.source.get_supported_mip_versions) {
				ret = bt_component_class_source_set_get_supported_mip_versions_method(
					src_comp_class,
					cc_full_descr->methods.source.get_supported_mip_versions);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set source component class's \"get supported MIP versions\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(src_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.source.init) {
				ret = bt_component_class_source_set_initialize_method(
					src_comp_class,
					cc_full_descr->methods.source.init);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set source component class's initialization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(src_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.source.finalize) {
				ret = bt_component_class_source_set_finalize_method(
					src_comp_class,
					cc_full_descr->methods.source.finalize);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set source component class's finalization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(src_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.source.query) {
				ret = bt_component_class_source_set_query_method(
					src_comp_class,
					cc_full_descr->methods.source.query);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set source component class's query method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(src_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.source.output_port_connected) {
				ret = bt_component_class_source_set_output_port_connected_method(
					src_comp_class,
					cc_full_descr->methods.source.output_port_connected);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set source component class's \"output port connected\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(src_comp_class);
					goto end;
				}
			}

			break;
		case BT_COMPONENT_CLASS_TYPE_FILTER:
			if (cc_full_descr->methods.filter.get_supported_mip_versions) {
				ret = bt_component_class_filter_set_get_supported_mip_versions_method(
					flt_comp_class,
					cc_full_descr->methods.filter.get_supported_mip_versions);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set filter component class's \"get supported MIP versions\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(flt_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.filter.init) {
				ret = bt_component_class_filter_set_initialize_method(
					flt_comp_class,
					cc_full_descr->methods.filter.init);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set filter component class's initialization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(flt_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.filter.finalize) {
				ret = bt_component_class_filter_set_finalize_method(
					flt_comp_class,
					cc_full_descr->methods.filter.finalize);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set filter component class's finalization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(flt_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.filter.query) {
				ret = bt_component_class_filter_set_query_method(
					flt_comp_class,
					cc_full_descr->methods.filter.query);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set filter component class's query method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(flt_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.filter.input_port_connected) {
				ret = bt_component_class_filter_set_input_port_connected_method(
					flt_comp_class,
					cc_full_descr->methods.filter.input_port_connected);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set filter component class's \"input port connected\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(flt_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.filter.output_port_connected) {
				ret = bt_component_class_filter_set_output_port_connected_method(
					flt_comp_class,
					cc_full_descr->methods.filter.output_port_connected);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set filter component class's \"output port connected\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(flt_comp_class);
					goto end;
				}
			}

			break;
		case BT_COMPONENT_CLASS_TYPE_SINK:
			if (cc_full_descr->methods.sink.get_supported_mip_versions) {
				ret = bt_component_class_sink_set_get_supported_mip_versions_method(
					sink_comp_class,
					cc_full_descr->methods.sink.get_supported_mip_versions);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set sink component class's \"get supported MIP versions\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(sink_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.sink.init) {
				ret = bt_component_class_sink_set_initialize_method(
					sink_comp_class,
					cc_full_descr->methods.sink.init);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set sink component class's initialization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(sink_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.sink.finalize) {
				ret = bt_component_class_sink_set_finalize_method(
					sink_comp_class,
					cc_full_descr->methods.sink.finalize);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set sink component class's finalization method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(sink_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.sink.query) {
				ret = bt_component_class_sink_set_query_method(
					sink_comp_class,
					cc_full_descr->methods.sink.query);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set sink component class's query method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(sink_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.sink.input_port_connected) {
				ret = bt_component_class_sink_set_input_port_connected_method(
					sink_comp_class,
					cc_full_descr->methods.sink.input_port_connected);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set sink component class's \"input port connected\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(sink_comp_class);
					goto end;
				}
			}

			if (cc_full_descr->methods.sink.graph_is_configured) {
				ret = bt_component_class_sink_set_graph_is_configured_method(
					sink_comp_class,
					cc_full_descr->methods.sink.graph_is_configured);
				if (ret) {
					BT_LIB_LOGE_APPEND_CAUSE(
						"Cannot set sink component class's \"graph is configured\" method.");
					status = BT_FUNC_STATUS_MEMORY_ERROR;
					BT_OBJECT_PUT_REF_AND_RESET(sink_comp_class);
					goto end;
				}
			}

			break;
		default:
			bt_common_abort();
		}

		/*
		 * Add component class to the plugin object.
		 *
		 * This will call back
		 * bt_plugin_so_on_add_component_class() so that we can
		 * add a mapping in the component class list when we
		 * know the component class is successfully added.
		 */
		status = bt_plugin_add_component_class(plugin,
			(void *) comp_class);
		BT_OBJECT_PUT_REF_AND_RESET(comp_class);
		if (status < 0) {
			BT_LIB_LOGE_APPEND_CAUSE(
				"Cannot add component class to plugin.");
			goto end;
		}
	}

end:
	bt_message_iterator_class_put_ref(msg_iter_class);
	g_array_free(comp_class_full_descriptors, TRUE);
	return status;
}

static
struct bt_plugin *bt_plugin_so_create_empty(
		struct bt_plugin_so_shared_lib_handle *shared_lib_handle)
{
	struct bt_plugin *plugin;
	struct bt_plugin_so_spec_data *spec;

	plugin = bt_plugin_create_empty(BT_PLUGIN_TYPE_SO);
	if (!plugin) {
		goto error;
	}

	plugin->destroy_spec_data = bt_plugin_so_destroy_spec_data;
	plugin->spec_data = g_new0(struct bt_plugin_so_spec_data, 1);
	if (!plugin->spec_data) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one SO plugin specific data structure.");
		goto error;
	}

	spec = plugin->spec_data;
	spec->shared_lib_handle = shared_lib_handle;
	bt_object_get_ref_no_null_check(spec->shared_lib_handle);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(plugin);

end:
	return plugin;
}

static
size_t count_non_null_items_in_section(const void *begin, const void *end)
{
	size_t count = 0;
	const int * const *begin_int = (const int * const *) begin;
	const int * const *end_int = (const int * const *) end;
	const int * const *iter;

	for (iter = begin_int; iter != end_int; iter++) {
		if (*iter) {
			count++;
		}
	}

	return count;
}

static
int bt_plugin_so_create_all_from_sections(
		struct bt_plugin_so_shared_lib_handle *shared_lib_handle,
		bool fail_on_load_error,
		struct __bt_plugin_descriptor const * const *descriptors_begin,
		struct __bt_plugin_descriptor const * const *descriptors_end,
		struct __bt_plugin_descriptor_attribute const * const *attrs_begin,
		struct __bt_plugin_descriptor_attribute const * const *attrs_end,
		struct __bt_plugin_component_class_descriptor const * const *cc_descriptors_begin,
		struct __bt_plugin_component_class_descriptor const * const *cc_descriptors_end,
		struct __bt_plugin_component_class_descriptor_attribute const * const *cc_descr_attrs_begin,
		struct __bt_plugin_component_class_descriptor_attribute const * const *cc_descr_attrs_end,
		struct bt_plugin_set **plugin_set_out)
{
	int status = BT_FUNC_STATUS_OK;
	size_t descriptor_count;
	size_t attrs_count;
	size_t cc_descriptors_count;
	size_t cc_descr_attrs_count;
	size_t i;

	BT_ASSERT(shared_lib_handle);
	BT_ASSERT(plugin_set_out);
	*plugin_set_out = NULL;
	descriptor_count = count_non_null_items_in_section(descriptors_begin, descriptors_end);
	attrs_count = count_non_null_items_in_section(attrs_begin, attrs_end);
	cc_descriptors_count = count_non_null_items_in_section(cc_descriptors_begin, cc_descriptors_end);
	cc_descr_attrs_count =  count_non_null_items_in_section(cc_descr_attrs_begin, cc_descr_attrs_end);
	BT_LOGI("Creating all SO plugins from sections: "
		"plugin-path=\"%s\", "
		"descr-begin-addr=%p, descr-end-addr=%p, "
		"attrs-begin-addr=%p, attrs-end-addr=%p, "
		"cc-descr-begin-addr=%p, cc-descr-end-addr=%p, "
		"cc-descr-attrs-begin-addr=%p, cc-descr-attrs-end-addr=%p, "
		"descr-count=%zu, attrs-count=%zu, "
		"cc-descr-count=%zu, cc-descr-attrs-count=%zu",
		shared_lib_handle->path ? shared_lib_handle->path->str : NULL,
		descriptors_begin, descriptors_end,
		attrs_begin, attrs_end,
		cc_descriptors_begin, cc_descriptors_end,
		cc_descr_attrs_begin, cc_descr_attrs_end,
		descriptor_count, attrs_count,
		cc_descriptors_count, cc_descr_attrs_count);
	*plugin_set_out = bt_plugin_set_create();
	if (!*plugin_set_out) {
		BT_LIB_LOGE_APPEND_CAUSE("Cannot create empty plugin set.");
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto error;
	}

	for (i = 0; i < descriptors_end - descriptors_begin; i++) {
		const struct __bt_plugin_descriptor *descriptor =
			descriptors_begin[i];
		struct bt_plugin *plugin;

		if (!descriptor) {
			continue;
		}

		BT_LOGI("Creating plugin object for plugin: name=\"%s\"",
			descriptor->name);
		plugin = bt_plugin_so_create_empty(shared_lib_handle);
		if (!plugin) {
			BT_LIB_LOGE_APPEND_CAUSE(
				"Cannot create empty shared library handle.");
			status = BT_FUNC_STATUS_MEMORY_ERROR;
			goto error;
		}

		if (shared_lib_handle->path) {
			bt_plugin_set_path(plugin,
				shared_lib_handle->path->str);
		}

		status = bt_plugin_so_init(plugin, fail_on_load_error,
			descriptor, attrs_begin, attrs_end,
			cc_descriptors_begin, cc_descriptors_end,
			cc_descr_attrs_begin, cc_descr_attrs_end);
		if (status == BT_FUNC_STATUS_OK) {
			/* Add to plugin set */
			bt_plugin_set_add_plugin(*plugin_set_out, plugin);
			BT_OBJECT_PUT_REF_AND_RESET(plugin);
		} else if (status < 0) {
			/*
			 * bt_plugin_so_init() handles
			 * `fail_on_load_error`, so this is a "real"
			 * error.
			 */
			BT_LIB_LOGW_APPEND_CAUSE(
				"Cannot initialize SO plugin object from sections.");
			BT_OBJECT_PUT_REF_AND_RESET(plugin);
			goto error;
		}

		BT_ASSERT(!plugin);
	}

	BT_ASSERT(*plugin_set_out);

	if ((*plugin_set_out)->plugins->len == 0) {
		BT_OBJECT_PUT_REF_AND_RESET(*plugin_set_out);
		status = BT_FUNC_STATUS_NOT_FOUND;
	}

	goto end;

error:
	BT_ASSERT(status < 0);
	BT_OBJECT_PUT_REF_AND_RESET(*plugin_set_out);

end:
	return status;
}

BT_HIDDEN
int bt_plugin_so_create_all_from_static(bool fail_on_load_error,
		struct bt_plugin_set **plugin_set_out)
{
	int status;
	struct bt_plugin_so_shared_lib_handle *shared_lib_handle = NULL;

	BT_ASSERT(plugin_set_out);
	*plugin_set_out = NULL;
	status = bt_plugin_so_shared_lib_handle_create(NULL,
		&shared_lib_handle);
	if (status != BT_FUNC_STATUS_OK) {
		BT_ASSERT(!shared_lib_handle);
		goto end;
	}

	BT_ASSERT(shared_lib_handle);
	BT_LOGD_STR("Creating all SO plugins from built-in plugins.");
	status = bt_plugin_so_create_all_from_sections(shared_lib_handle,
		fail_on_load_error,
		__bt_get_begin_section_plugin_descriptors(),
		__bt_get_end_section_plugin_descriptors(),
		__bt_get_begin_section_plugin_descriptor_attributes(),
		__bt_get_end_section_plugin_descriptor_attributes(),
		__bt_get_begin_section_component_class_descriptors(),
		__bt_get_end_section_component_class_descriptors(),
		__bt_get_begin_section_component_class_descriptor_attributes(),
		__bt_get_end_section_component_class_descriptor_attributes(),
		plugin_set_out);
	BT_ASSERT((status == BT_FUNC_STATUS_OK && *plugin_set_out &&
		(*plugin_set_out)->plugins->len > 0) || !*plugin_set_out);

end:
	BT_OBJECT_PUT_REF_AND_RESET(shared_lib_handle);
	return status;
}

BT_HIDDEN
int bt_plugin_so_create_all_from_file(const char *path,
		bool fail_on_load_error, struct bt_plugin_set **plugin_set_out)
{
	size_t path_len;
	int status;
	struct __bt_plugin_descriptor const * const *descriptors_begin = NULL;
	struct __bt_plugin_descriptor const * const *descriptors_end = NULL;
	struct __bt_plugin_descriptor_attribute const * const *attrs_begin = NULL;
	struct __bt_plugin_descriptor_attribute const * const *attrs_end = NULL;
	struct __bt_plugin_component_class_descriptor const * const *cc_descriptors_begin = NULL;
	struct __bt_plugin_component_class_descriptor const * const *cc_descriptors_end = NULL;
	struct __bt_plugin_component_class_descriptor_attribute const * const *cc_descr_attrs_begin = NULL;
	struct __bt_plugin_component_class_descriptor_attribute const * const *cc_descr_attrs_end = NULL;
	struct __bt_plugin_descriptor const * const *(*get_begin_section_plugin_descriptors)(void);
	struct __bt_plugin_descriptor const * const *(*get_end_section_plugin_descriptors)(void);
	struct __bt_plugin_descriptor_attribute const * const *(*get_begin_section_plugin_descriptor_attributes)(void);
	struct __bt_plugin_descriptor_attribute const * const *(*get_end_section_plugin_descriptor_attributes)(void);
	struct __bt_plugin_component_class_descriptor const * const *(*get_begin_section_component_class_descriptors)(void);
	struct __bt_plugin_component_class_descriptor const * const *(*get_end_section_component_class_descriptors)(void);
	struct __bt_plugin_component_class_descriptor_attribute const * const *(*get_begin_section_component_class_descriptor_attributes)(void);
	struct __bt_plugin_component_class_descriptor_attribute const * const *(*get_end_section_component_class_descriptor_attributes)(void);
	bt_bool is_libtool_wrapper = BT_FALSE, is_shared_object = BT_FALSE;
	struct bt_plugin_so_shared_lib_handle *shared_lib_handle = NULL;

	BT_ASSERT(path);
	BT_ASSERT(plugin_set_out);
	*plugin_set_out = NULL;
	path_len = strlen(path);

	/*
	 * An SO plugin file must have a known plugin file suffix. So the file
	 * path must be longer than the suffix length.
	 */
	if (path_len <= PLUGIN_SUFFIX_LEN) {
		BT_LOGI("Path is too short to be an `.so` or `.la` plugin file:"
			"path=%s, path-length=%zu, min-length=%zu",
			path, path_len, PLUGIN_SUFFIX_LEN);
		status = BT_FUNC_STATUS_NOT_FOUND;
		goto end;
	}

	BT_LOGI("Trying to create all SO plugins from file: path=\"%s\"", path);
	path_len++;

	/*
	 * Check if the file ends with a known plugin file type suffix
	 * (i.e. .so or .la on Linux).
	 */
	is_libtool_wrapper = !strncmp(LIBTOOL_PLUGIN_SUFFIX,
		path + path_len - LIBTOOL_PLUGIN_SUFFIX_LEN,
		LIBTOOL_PLUGIN_SUFFIX_LEN);
	is_shared_object = !strncmp(NATIVE_PLUGIN_SUFFIX,
		path + path_len - NATIVE_PLUGIN_SUFFIX_LEN,
		NATIVE_PLUGIN_SUFFIX_LEN);
	if (!is_shared_object && !is_libtool_wrapper) {
		/* Name indicates this is not a plugin file; not an error */
		BT_LOGI("File is not an SO plugin file: path=\"%s\"", path);
		status = BT_FUNC_STATUS_NOT_FOUND;
		goto end;
	}

	status = bt_plugin_so_shared_lib_handle_create(path,
		&shared_lib_handle);
	if (status != BT_FUNC_STATUS_OK) {
		/* bt_plugin_so_shared_lib_handle_create() logs more details */
		BT_ASSERT(!shared_lib_handle);
		goto end;
	}

	if (g_module_symbol(shared_lib_handle->module, "__bt_get_begin_section_plugin_descriptors",
			(gpointer *) &get_begin_section_plugin_descriptors)) {
		descriptors_begin = get_begin_section_plugin_descriptors();
	} else {
		/*
		 * Use this first symbol to know whether or not this
		 * shared object _looks like_ a Babeltrace plugin. Since
		 * g_module_symbol() failed, assume that this is not a
		 * Babeltrace plugin, so it's not an error.
		 */
		BT_LOGI("Cannot resolve plugin symbol: path=\"%s\", "
			"symbol=\"%s\"", path,
			"__bt_get_begin_section_plugin_descriptors");
		status = BT_FUNC_STATUS_NOT_FOUND;
		goto end;
	}

	/*
	 * If g_module_symbol() fails for any of the other symbols, fail
	 * if `fail_on_load_error` is true.
	 */
	if (g_module_symbol(shared_lib_handle->module, "__bt_get_end_section_plugin_descriptors",
			(gpointer *) &get_end_section_plugin_descriptors)) {
		descriptors_end = get_end_section_plugin_descriptors();
	} else {
		if (fail_on_load_error) {
			BT_LIB_LOGW_APPEND_CAUSE(
				"Cannot resolve plugin symbol: path=\"%s\", "
				"symbol=\"%s\"", path,
				"__bt_get_end_section_plugin_descriptors");
			status = BT_FUNC_STATUS_ERROR;
		} else {
			BT_LIB_LOGW(
				"Cannot resolve plugin symbol: path=\"%s\", "
				"symbol=\"%s\"", path,
				"__bt_get_end_section_plugin_descriptors");
			status = BT_FUNC_STATUS_NOT_FOUND;
		}

		goto end;
	}

	if (g_module_symbol(shared_lib_handle->module, "__bt_get_begin_section_plugin_descriptor_attributes",
			(gpointer *) &get_begin_section_plugin_descriptor_attributes)) {
		 attrs_begin = get_begin_section_plugin_descriptor_attributes();
	} else {
		BT_LOGI("Cannot resolve plugin symbol: path=\"%s\", "
			"symbol=\"%s\"", path,
			"__bt_get_begin_section_plugin_descriptor_attributes");
	}

	if (g_module_symbol(shared_lib_handle->module, "__bt_get_end_section_plugin_descriptor_attributes",
			(gpointer *) &get_end_section_plugin_descriptor_attributes)) {
		attrs_end = get_end_section_plugin_descriptor_attributes();
	} else {
		BT_LOGI("Cannot resolve plugin symbol: path=\"%s\", "
			"symbol=\"%s\"", path,
			"__bt_get_end_section_plugin_descriptor_attributes");
	}

	if ((!!attrs_begin - !!attrs_end) != 0) {
		if (fail_on_load_error) {
			BT_LIB_LOGW_APPEND_CAUSE(
				"Found section start or end symbol, but not both: "
				"path=\"%s\", symbol-start=\"%s\", "
				"symbol-end=\"%s\", symbol-start-addr=%p, "
				"symbol-end-addr=%p",
				path, "__bt_get_begin_section_plugin_descriptor_attributes",
				"__bt_get_end_section_plugin_descriptor_attributes",
				attrs_begin, attrs_end);
			status = BT_FUNC_STATUS_ERROR;
		} else {
			BT_LIB_LOGW(
				"Found section start or end symbol, but not both: "
				"path=\"%s\", symbol-start=\"%s\", "
				"symbol-end=\"%s\", symbol-start-addr=%p, "
				"symbol-end-addr=%p",
				path, "__bt_get_begin_section_plugin_descriptor_attributes",
				"__bt_get_end_section_plugin_descriptor_attributes",
				attrs_begin, attrs_end);
			status = BT_FUNC_STATUS_NOT_FOUND;
		}

		goto end;
	}

	if (g_module_symbol(shared_lib_handle->module, "__bt_get_begin_section_component_class_descriptors",
			(gpointer *) &get_begin_section_component_class_descriptors)) {
		cc_descriptors_begin = get_begin_section_component_class_descriptors();
	} else {
		BT_LOGI("Cannot resolve plugin symbol: path=\"%s\", "
			"symbol=\"%s\"", path,
			"__bt_get_begin_section_component_class_descriptors");
	}

	if (g_module_symbol(shared_lib_handle->module, "__bt_get_end_section_component_class_descriptors",
			(gpointer *) &get_end_section_component_class_descriptors)) {
		cc_descriptors_end = get_end_section_component_class_descriptors();
	} else {
		BT_LOGI("Cannot resolve plugin symbol: path=\"%s\", "
			"symbol=\"%s\"", path,
			"__bt_get_end_section_component_class_descriptors");
	}

	if ((!!cc_descriptors_begin - !!cc_descriptors_end) != 0) {
		if (fail_on_load_error) {
			BT_LIB_LOGW_APPEND_CAUSE(
				"Found section start or end symbol, but not both: "
				"path=\"%s\", symbol-start=\"%s\", "
				"symbol-end=\"%s\", symbol-start-addr=%p, "
				"symbol-end-addr=%p",
				path, "__bt_get_begin_section_component_class_descriptors",
				"__bt_get_end_section_component_class_descriptors",
				cc_descriptors_begin, cc_descriptors_end);
			status = BT_FUNC_STATUS_ERROR;
		} else {
			BT_LIB_LOGW(
				"Found section start or end symbol, but not both: "
				"path=\"%s\", symbol-start=\"%s\", "
				"symbol-end=\"%s\", symbol-start-addr=%p, "
				"symbol-end-addr=%p",
				path, "__bt_get_begin_section_component_class_descriptors",
				"__bt_get_end_section_component_class_descriptors",
				cc_descriptors_begin, cc_descriptors_end);
			status = BT_FUNC_STATUS_NOT_FOUND;
		}

		goto end;
	}

	if (g_module_symbol(shared_lib_handle->module, "__bt_get_begin_section_component_class_descriptor_attributes",
			(gpointer *) &get_begin_section_component_class_descriptor_attributes)) {
		cc_descr_attrs_begin = get_begin_section_component_class_descriptor_attributes();
	} else {
		BT_LOGI("Cannot resolve plugin symbol: path=\"%s\", "
			"symbol=\"%s\"", path,
			"__bt_get_begin_section_component_class_descriptor_attributes");
	}

	if (g_module_symbol(shared_lib_handle->module, "__bt_get_end_section_component_class_descriptor_attributes",
			(gpointer *) &get_end_section_component_class_descriptor_attributes)) {
		cc_descr_attrs_end = get_end_section_component_class_descriptor_attributes();
	} else {
		BT_LOGI("Cannot resolve plugin symbol: path=\"%s\", "
			"symbol=\"%s\"", path,
			"__bt_get_end_section_component_class_descriptor_attributes");
	}

	if ((!!cc_descr_attrs_begin - !!cc_descr_attrs_end) != 0) {
		if (fail_on_load_error) {
			BT_LIB_LOGW_APPEND_CAUSE(
				"Found section start or end symbol, but not both: "
				"path=\"%s\", symbol-start=\"%s\", "
				"symbol-end=\"%s\", symbol-start-addr=%p, "
				"symbol-end-addr=%p",
				path, "__bt_get_begin_section_component_class_descriptor_attributes",
				"__bt_get_end_section_component_class_descriptor_attributes",
				cc_descr_attrs_begin, cc_descr_attrs_end);
			status = BT_FUNC_STATUS_ERROR;
		} else {
			BT_LIB_LOGW(
				"Found section start or end symbol, but not both: "
				"path=\"%s\", symbol-start=\"%s\", "
				"symbol-end=\"%s\", symbol-start-addr=%p, "
				"symbol-end-addr=%p",
				path, "__bt_get_begin_section_component_class_descriptor_attributes",
				"__bt_get_end_section_component_class_descriptor_attributes",
				cc_descr_attrs_begin, cc_descr_attrs_end);
			status = BT_FUNC_STATUS_NOT_FOUND;
		}

		goto end;
	}

	/* Initialize plugin */
	BT_LOGD_STR("Initializing plugin object.");
	status = bt_plugin_so_create_all_from_sections(shared_lib_handle,
		fail_on_load_error,
		descriptors_begin, descriptors_end, attrs_begin, attrs_end,
		cc_descriptors_begin, cc_descriptors_end,
		cc_descr_attrs_begin, cc_descr_attrs_end, plugin_set_out);

end:
	BT_OBJECT_PUT_REF_AND_RESET(shared_lib_handle);
	return status;
}

static
void plugin_comp_class_destroy_listener(struct bt_component_class *comp_class,
		void *data)
{
	bt_list_del(&comp_class->node);
	BT_OBJECT_PUT_REF_AND_RESET(comp_class->so_handle);
	BT_LOGD("Component class destroyed: removed entry from list: "
		"comp-cls-addr=%p", comp_class);
}

void bt_plugin_so_on_add_component_class(struct bt_plugin *plugin,
		struct bt_component_class *comp_class)
{
	struct bt_plugin_so_spec_data *spec = plugin->spec_data;

	BT_ASSERT(plugin->spec_data);
	BT_ASSERT(plugin->type == BT_PLUGIN_TYPE_SO);

	bt_list_add(&comp_class->node, &component_class_list);
	comp_class->so_handle = spec->shared_lib_handle;
	bt_object_get_ref_no_null_check(comp_class->so_handle);

	/* Add our custom destroy listener */
	bt_component_class_add_destroy_listener(comp_class,
		plugin_comp_class_destroy_listener, NULL);
}

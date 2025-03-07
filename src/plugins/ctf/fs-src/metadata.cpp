/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2010-2011 EfficiOS Inc. and Linux Foundation
 */

#define BT_COMP_LOG_SELF_COMP self_comp
#define BT_LOG_OUTPUT_LEVEL   log_level
#define BT_LOG_TAG            "PLUGIN/SRC.CTF.FS/META"
#include "logging/comp-logging.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "common/assert.h"
#include <glib.h>
#include "common/uuid.h"
#include "compat/memstream.h"
#include <babeltrace2/babeltrace.h>

#include "fs.hpp"
#include "file.hpp"
#include "metadata.hpp"
#include "../common/metadata/decoder.hpp"

BT_HIDDEN
FILE *ctf_fs_metadata_open_file(const char *trace_path)
{
    GString *metadata_path;
    FILE *fp = NULL;

    metadata_path = g_string_new(trace_path);
    if (!metadata_path) {
        goto end;
    }

    g_string_append(metadata_path, G_DIR_SEPARATOR_S CTF_FS_METADATA_FILENAME);
    fp = fopen(metadata_path->str, "rb");
    g_string_free(metadata_path, TRUE);
end:
    return fp;
}

static struct ctf_fs_file *get_file(const char *trace_path, bt_logging_level log_level,
                                    bt_self_component *self_comp)
{
    struct ctf_fs_file *file = ctf_fs_file_create(log_level, self_comp);

    if (!file) {
        goto error;
    }

    g_string_append(file->path, trace_path);
    g_string_append(file->path, G_DIR_SEPARATOR_S CTF_FS_METADATA_FILENAME);

    if (ctf_fs_file_open(file, "rb")) {
        goto error;
    }

    goto end;

error:
    if (file) {
        ctf_fs_file_destroy(file);
        file = NULL;
    }

end:
    return file;
}

BT_HIDDEN
int ctf_fs_metadata_set_trace_class(bt_self_component *self_comp, struct ctf_fs_trace *ctf_fs_trace,
                                    struct ctf_fs_metadata_config *config)
{
    int ret = 0;
    struct ctf_fs_file *file = NULL;
    bt_logging_level log_level = ctf_fs_trace->log_level;

    ctf_metadata_decoder_config decoder_config {};
    decoder_config.log_level = ctf_fs_trace->log_level, decoder_config.self_comp = self_comp,
    decoder_config.clock_class_offset_s = config ? config->clock_class_offset_s : 0,
    decoder_config.clock_class_offset_ns = config ? config->clock_class_offset_ns : 0,
    decoder_config.force_clock_class_origin_unix_epoch =
        config ? config->force_clock_class_origin_unix_epoch : false,
    decoder_config.create_trace_class = true,

    file = get_file(ctf_fs_trace->path->str, log_level, self_comp);
    if (!file) {
        BT_COMP_LOGE("Cannot create metadata file object.");
        ret = -1;
        goto end;
    }

    ctf_fs_trace->metadata->decoder = ctf_metadata_decoder_create(&decoder_config);
    if (!ctf_fs_trace->metadata->decoder) {
        BT_COMP_LOGE("Cannot create metadata decoder object.");
        ret = -1;
        goto end;
    }

    ret = ctf_metadata_decoder_append_content(ctf_fs_trace->metadata->decoder, file->fp);
    if (ret) {
        BT_COMP_LOGE("Cannot update metadata decoder's content.");
        goto end;
    }

    ctf_fs_trace->metadata->trace_class =
        ctf_metadata_decoder_get_ir_trace_class(ctf_fs_trace->metadata->decoder);
    BT_ASSERT(!self_comp || ctf_fs_trace->metadata->trace_class);
    ctf_fs_trace->metadata->tc =
        ctf_metadata_decoder_borrow_ctf_trace_class(ctf_fs_trace->metadata->decoder);
    BT_ASSERT(ctf_fs_trace->metadata->tc);

end:
    ctf_fs_file_destroy(file);
    return ret;
}

BT_HIDDEN
int ctf_fs_metadata_init(struct ctf_fs_metadata *metadata)
{
    /* Nothing to initialize for the moment. */
    return 0;
}

BT_HIDDEN
void ctf_fs_metadata_fini(struct ctf_fs_metadata *metadata)
{
    free(metadata->text);

    if (metadata->trace_class) {
        BT_TRACE_CLASS_PUT_REF_AND_RESET(metadata->trace_class);
    }

    if (metadata->decoder) {
        ctf_metadata_decoder_destroy(metadata->decoder);
    }
}

/*
 * text-to-ctf.c
 *
 * BabelTrace - Convert Text to CTF
 *
 * Copyright 2010, 2011 - Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Depends on glibc 2.10 for getline().
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <uuid/uuid.h>
#include <string.h>

#include <babeltrace/babeltrace.h>
#include <babeltrace/ctf/types.h>

int babeltrace_debug = 0;

static uuid_t s_uuid;

static
void write_packet_header(struct stream_pos *pos, uuid_t uuid)
{
	struct stream_pos dummy;

	/* magic */
	dummy_pos(pos, &dummy);
	align_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	move_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	assert(!pos_packet(&dummy));
	
	align_pos(pos, sizeof(uint32_t) * CHAR_BIT);
	*(uint32_t *) get_pos_addr(pos) = 0xC1FC1FC1;
	move_pos(pos, sizeof(uint32_t) * CHAR_BIT);

	/* trace_uuid */
	dummy_pos(pos, &dummy);
	align_pos(&dummy, sizeof(uint8_t) * CHAR_BIT);
	move_pos(&dummy, 16 * CHAR_BIT);
	assert(!pos_packet(&dummy));

	align_pos(pos, sizeof(uint8_t) * CHAR_BIT);
	memcpy(get_pos_addr(pos), uuid, 16);
	move_pos(pos, 16 * CHAR_BIT);
}

static
void write_packet_context(struct stream_pos *pos)
{
	struct stream_pos dummy;

	/* content_size */
	dummy_pos(pos, &dummy);
	align_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	move_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	assert(!pos_packet(&dummy));
	
	align_pos(pos, sizeof(uint32_t) * CHAR_BIT);
	*(uint32_t *) get_pos_addr(pos) = -1U;	/* Not known yet */
	pos->content_size_loc = (uint32_t *) get_pos_addr(pos);
	move_pos(pos, sizeof(uint32_t) * CHAR_BIT);

	/* packet_size */
	dummy_pos(pos, &dummy);
	align_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	move_pos(&dummy, sizeof(uint32_t) * CHAR_BIT);
	assert(!pos_packet(&dummy));
	
	align_pos(pos, sizeof(uint32_t) * CHAR_BIT);
	*(uint32_t *) get_pos_addr(pos) = pos->packet_size;
	move_pos(pos, sizeof(uint32_t) * CHAR_BIT);
}

static
void trace_string(char *line, struct stream_pos *pos, size_t len)
{
	struct stream_pos dummy;
	int attempt = 0;

	printf_debug("read: %s\n", line);
retry:
	dummy_pos(pos, &dummy);
	align_pos(&dummy, sizeof(uint8_t) * CHAR_BIT);
	move_pos(&dummy, len * CHAR_BIT);
	if (pos_packet(&dummy)) {
		/* TODO write content size */
		pos_pad_packet(pos);
		write_packet_header(pos, s_uuid);
		write_packet_context(pos);
		if (attempt++ == 1) {
			fprintf(stdout, "[Error] Line too large for packet size (%zukB) (discarded)\n",
				pos->packet_size / CHAR_BIT / 1024);
			return;
		}
		goto retry;
	}

	align_pos(pos, sizeof(uint8_t) * CHAR_BIT);
	memcpy(get_pos_addr(pos), line, len);
	move_pos(pos, len * CHAR_BIT);
}

static
void trace_text(FILE *input, int output)
{
	struct stream_pos pos;
	ssize_t len;
	char *line = NULL, *nl;
	size_t linesize;

	init_pos(&pos, output);

	write_packet_header(&pos, s_uuid);
	write_packet_context(&pos);
	for (;;) {
		len = getline(&line, &linesize, input);
		if (len < 0)
			break;
		nl = strrchr(line, '\n');
		if (nl)
			*nl = '\0';
		trace_string(line, &pos, nl - line + 1);
	}
	fini_pos(&pos);
}

int main(int argc, char **argv)
{
	int fd, ret;

	ret = unlink("dummystream");
	if (ret < 0) {
		perror("unlink");
		return -1;
	}
	fd = open("dummystream", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	ret = uuid_parse("2a6422d0-6cee-11e0-8c08-cb07d7b3a564", s_uuid);
	if (ret) {
		printf("uuid parse error\n");
		close(fd);
		return -1;
	}

	trace_text(stdin, fd);
	close(fd);
	return 0;
}

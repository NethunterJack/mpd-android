/*
 * Copyright (C) 2003-2012 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_OGG_UTIL_HXX
#define MPD_OGG_UTIL_HXX

#include "check.h"

#include <ogg/ogg.h>

#include <stddef.h>

struct input_stream;
struct decoder;

/**
 * Feed data from the #input_stream into the #ogg_sync_state.
 *
 * @return false on error or end-of-file
 */
bool
OggFeed(ogg_sync_state &oy, struct decoder *decoder, input_stream *is,
	size_t size);

/**
 * Feed into the #ogg_sync_state until a page gets available.  Garbage
 * data at the beginning is considered a fatal error.
 *
 * @return true if a page is available
 */
bool
OggExpectPage(ogg_sync_state &oy, ogg_page &page,
	      decoder *decoder, input_stream *input_stream);

/**
 * Combines OggExpectPage(), ogg_stream_init() and
 * ogg_stream_pagein().
 *
 * @return true if the stream was initialized and the first page was
 * delivered to it
 */
bool
OggExpectFirstPage(ogg_sync_state &oy, ogg_stream_state &os,
		   decoder *decoder, input_stream *is);

/**
 * Combines OggExpectPage() and ogg_stream_pagein().
 *
 * @return true if a page was delivered to the stream
 */
bool
OggExpectPageIn(ogg_sync_state &oy, ogg_stream_state &os,
		decoder *decoder, input_stream *is);

/**
 * Like OggExpectPage(), but allow skipping garbage (after seeking).
 */
bool
OggExpectPageSeek(ogg_sync_state &oy, ogg_page &page,
		  decoder *decoder, input_stream *input_stream);

/**
 * Combines OggExpectPageSeek() and ogg_stream_pagein().
 *
 * @return true if a page was delivered to the stream
 */
bool
OggExpectPageSeekIn(ogg_sync_state &oy, ogg_stream_state &os,
		    decoder *decoder, input_stream *is);

#endif

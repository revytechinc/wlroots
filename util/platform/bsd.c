#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wlr/util/log.h>
#include "bsd.h"

/*
 * BSD: real filesystem sockets.
 * BSD lacks abstract sockets, so the socket is created as a regular file in
 * /tmp/.X11-unix/.  X11 tradition requires a trailing underscore on the
 * socket name to avoid colliding with the Xorg lock file.
 */

static const char socket_dir[]  = "/tmp/.X11-unix";
static const char socket_fmt[]  = "/tmp/.X11-unix/X%d";
static const char socket_fmt_[] = "/tmp/.X11-unix/X%d_";
static const char lock_fmt[]    = "/tmp/.X%d-lock";

static int bsd_format_socket_path(char *dst, size_t dst_size, int display) {
	return snprintf(dst, dst_size, socket_fmt_, display);
}

static const char *bsd_socket_dir(void) {
	return socket_dir;
}

static const char *bsd_lock_fmt(void) {
	return lock_fmt;
}

static void bsd_unlink_display_socket(int display) {
	char path[64];
	snprintf(path, sizeof(path), socket_fmt_, display);
	if (unlink(path) != 0 && errno != ENOENT) {
		wlr_log_errno(WLR_DEBUG, "failed to unlink BSD display socket %s", path);
	}
	snprintf(path, sizeof(path), socket_fmt, display);
	if (unlink(path) != 0 && errno != ENOENT) {
		wlr_log_errno(WLR_DEBUG, "failed to unlink BSD display socket %s", path);
	}
}

const struct wlr_unix_socket_vtable wlr_bsd_unix_socket = {
	.format_socket_path      = bsd_format_socket_path,
	.socket_dir              = bsd_socket_dir,
	.lock_fmt                = bsd_lock_fmt,
	.unlink_display_socket   = bsd_unlink_display_socket,
};

static void bsd_get_time(struct timespec *ts) {
	clock_gettime(CLOCK_MONOTONIC, ts);
}

const struct wlr_clock_vtable wlr_bsd_clock = {
	.get_time = bsd_get_time,
};

const struct wlr_platform_t wlr_platform_bsd = {
	.unix_socket = &wlr_bsd_unix_socket,
	.clock       = &wlr_bsd_clock,
};

#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include "linux.h"

/*
 * Linux: abstract Unix-domain sockets.
 * The socket path is stored after sun_path[0] = 0, so the display number
 * is formatted into sun_path+1.  The socket file does not appear in the
 * filesystem.
 */

/* Format: "/tmp/.X11-unix/X%display%" — stored after the NUL byte. */
static const char socket_fmt[] = ".X11-unix/X%d";

static int linux_format_socket_path(char *dst, size_t dst_size, int display) {
	return snprintf(dst, dst_size, socket_fmt, display);
}

static const char *linux_socket_dir(void) {
	return "/tmp/.X11-unix";
}

static const char *linux_lock_fmt(void) {
	return "/tmp/.X%d-lock";
}

static void linux_unlink_display_socket(int display) {
	char path[64];
	snprintf(path, sizeof(path), ".X11-unix/X%d", display);
	/* The abstract socket has no filesystem path; nothing to unlink. */
	(void)display;
	(void)path;
}

const struct wlr_unix_socket_vtable wlr_linux_unix_socket = {
	.format_socket_path = linux_format_socket_path,
	.socket_dir        = linux_socket_dir,
	.lock_fmt          = linux_lock_fmt,
	.unlink_display_socket = linux_unlink_display_socket,
};

static void linux_get_time(struct timespec *ts) {
	clock_gettime(CLOCK_MONOTONIC, ts);
}

const struct wlr_clock_vtable wlr_linux_clock = {
	.get_time = linux_get_time,
};

const struct wlr_platform_t wlr_platform_linux = {
	.unix_socket = &wlr_linux_unix_socket,
	.clock       = &wlr_linux_clock,
};

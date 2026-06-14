#ifndef WLR_UTIL_PLATFORM_H
#define WLR_UTIL_PLATFORM_H

#include <stddef.h>
#include <time.h>

/**
 * wlr_unix_socket_vtable — OS-specific Unix-domain socket naming.
 *
 * On Linux, abstract sockets use sun_path[0] = 0 with the name stored
 * after the NUL byte (e.g. "\0/tmp/.X11-unix/X0").
 * On BSD, IllumOS, and most other Unix systems, the socket is a real file
 * in /tmp, and a trailing underscore is appended to avoid collisions with
 * Xorg's lock-file convention.
 *
 * Implementations also supply platform-specific socket directory and lock
 * file format strings.
 */
struct wlr_unix_socket_vtable {
	/**
	 * Write the socket path into @p dst, using @p display as the display
	 * number. Returns the number of characters written (excluding the
	 * terminating NUL), or -1 on failure.
	 *
	 * On Linux, the first byte of sun_path is set to 0 by the caller;
	 * this function writes only the abstract-socket name after it.
	 */
	int (*format_socket_path)(char *dst, size_t dst_size, int display);

	/** Return the socket directory path (e.g. "/tmp/.X11-unix"). */
	const char *(*socket_dir)(void);

	/** Return the lock-file format string (e.g. "/tmp/.X%d-lock"). */
	const char *(*lock_fmt)(void);

	/**
	 * Unlink any socket files this platform created for the given display.
	 * Called by open_display_sockets() cleanup on failure, and by
	 * unlink_display_sockets().
	 */
	void (*unlink_display_socket)(int display);
};

/**
 * wlr_clock_vtable — OS-specific monotonic clock source.
 *
 * All supported platforms have clock_gettime(2) with CLOCK_MONOTONIC,
 * so the implementation is the same call everywhere. The vtable exists
 * so that platforms that need a different clock source (e.g. virtualized
 * environments with clock drift) can override it.
 */
struct wlr_clock_vtable {
	/** Fill @p ts with the current monotonic time. */
	void (*get_time)(struct timespec *ts);
};

/**
 * wlr_platform_t — the top-level platform descriptor.
 *
 * wlr_platform() returns a pointer to the active platform's descriptor.
 * All other code in wlroots queries the platform through this singleton;
 * there are no #ifdef checks outside util/platform.c.
 */
struct wlr_platform_t {
	const struct wlr_unix_socket_vtable *unix_socket;
	const struct wlr_clock_vtable *clock;
};

/**
 * Return a pointer to the active platform descriptor.
 * The result is a compile-time constant; no runtime allocation occurs.
 */
const struct wlr_platform_t *wlr_platform(void);

/**
 * A human-readable identifier for the active platform, e.g. "linux",
 * "freebsd", "openbsd", "illumos".  Useful for log messages.
 */
const char *wlr_platform_name(void);

#endif /* WLR_UTIL_PLATFORM_H */

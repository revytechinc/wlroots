#include <wlr/util/platform.h>
#include "linux.h"
#include "bsd.h"
#include "platform-shim.h"

/*
 * Platform descriptor table.
 *
 * Each entry maps a compile-time OS macro to the corresponding platform
 * descriptor.  The shim layer (platform-shim.h) defines WLR_PLATFORM_*
 * macros so that the dispatch logic here stays clean: no #ifdefs at the
 * call site, only in this file and the private shim header.
 *
 * Adding a new OS:
 *   1. Create util/platform/<name>.c and util/platform/<name>.h defining
 *      wlr_platform_<name> and wlr_<name>_unix_socket / wlr_<name>_clock.
 *   2. Add a new #elif arm below pointing to your platform descriptor.
 *
 * When a BSD variant (FreeBSD, OpenBSD, etc.) needs its own behavior,
 * create a new platform descriptor and add a dedicated arm for it here.
 */

/*
 * Module-scope singleton — initialized on first call, never deallocated.
 * C guarantees thread-safe initialization of function-scope statics.
 */
const struct wlr_platform_t *wlr_platform(void) {
	static const struct wlr_platform_t *instance = NULL;
	if (instance == NULL) {
#if defined(WLR_PLATFORM_LINUX)
		instance = &wlr_platform_linux;
#elif defined(__FreeBSD__)
		instance = &wlr_platform_bsd;
#elif defined(__OpenBSD__)
		instance = &wlr_platform_bsd;
#elif defined(__NetBSD__)
		instance = &wlr_platform_bsd;
#elif defined(__DragonFly__)
		instance = &wlr_platform_bsd;
#else
		/* Illumos / legacy Solaris land here — same BSD socket behavior. */
		instance = &wlr_platform_bsd;
#endif
	}
	return instance;
}

const char *wlr_platform_name(void) {
#if defined(WLR_PLATFORM_LINUX)
	return "linux";
#elif defined(__FreeBSD__)
	return "freebsd";
#elif defined(__OpenBSD__)
	return "openbsd";
#elif defined(__NetBSD__)
	return "netbsd";
#elif defined(__DragonFly__)
	return "dragonfly";
#else
	/* Illumos / legacy Solaris */
	return "unknown";
#endif
}

/*
 * platform-shim.h — private compile-time platform selector.
 *
 * This is the ONLY file in wlroots that contains #ifdef __linux__,
 * #ifdef __FreeBSD__, etc.  All dispatch logic lives in platform.c,
 * which includes this header to obtain the WLR_PLATFORM_* macro for the
 * current build target.
 *
 * Adding a new OS:
 *   1. Pick a WLR_PLATFORM_<NAME> symbol below.
 *   2. Add #elif defined(__<os>__) || defined(__<oslike>__)
 *      pointing to your platform descriptor.
 *   3. Create util/platform/<name>.c / .h (see bsd.c / linux.c as examples).
 */

#ifndef WLR_UTIL_PLATFORM_SHIM_H
#define WLR_UTIL_PLATFORM_SHIM_H

#if defined(__linux__)
	#define WLR_PLATFORM_LINUX

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || \
      defined(__DragonFly__)
	/*
	 * BSDs: FreeBSD, OpenBSD, NetBSD, DragonFly.
	 * Split into separate WLR_PLATFORM_FREEBSD / WLR_PLATFORM_OPENBSD /
	 * WLR_PLATFORM_NETBSD / WLR_PLATFORM_DRAGONFLY below when they need
	 * different behavior.
	 */
	#define WLR_PLATFORM_BSD

#elif defined(__illumos__) || (defined(__sun) && defined(__SVR4))
	/* Non-Linux, non-BSD: falls back to the BSD socket behavior (no change). */
	#define WLR_PLATFORM_BSD

#else
	/* Catch-all for unknown platforms. platform.c will emit a compile error. */
	#define WLR_PLATFORM_UNKNOWN
#endif

#endif /* WLR_UTIL_PLATFORM_SHIM_H */

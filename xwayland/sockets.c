#undef _POSIX_C_SOURCE
#define _XOPEN_SOURCE 700 // for S_ISVTX
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <wlr/util/log.h>
#include <wlr/util/platform.h>
#include "sockets.h"

/*
 * X11 display sockets.
 *
 * Platform differences:
 *  - Linux: abstract Unix-domain sockets. sun_path[0] is set to '\0';
 *           the socket name follows that byte. No filesystem file exists.
 *  - BSD, IllumOS: real filesystem sockets in /tmp/.X11-unix/.  A trailing
 *           underscore is appended to the first socket to avoid collisions
 *           with the Xorg lock-file convention.
 *
 * The wlr_unix_socket_vtable hides these differences.  The only OS-specific
 * conditional in this file is the inclusion of platform.h (which is itself
 * a clean dispatch table with no #ifdefs).
 */

bool set_cloexec(int fd, bool cloexec) {
	int flags = fcntl(fd, F_GETFD);
	if (flags == -1) {
		wlr_log_errno(WLR_ERROR, "fcntl failed");
		return false;
	}
	if (cloexec) {
		flags = flags | FD_CLOEXEC;
	} else {
		flags = flags & ~FD_CLOEXEC;
	}
	if (fcntl(fd, F_SETFD, flags) == -1) {
		wlr_log_errno(WLR_ERROR, "fcntl failed");
		return false;
	}
	return true;
}

static int open_socket(struct sockaddr_un *addr, size_t path_size) {
	int fd, rc;
	socklen_t size = offsetof(struct sockaddr_un, sun_path) + path_size + 1;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		wlr_log_errno(WLR_ERROR, "Failed to create socket %c%s",
			addr->sun_path[0] ? addr->sun_path[0] : '@',
			addr->sun_path + 1);
		return -1;
	}
	if (!set_cloexec(fd, true)) {
		close(fd);
		return -1;
	}

	if (addr->sun_path[0]) {
		unlink(addr->sun_path);
	}
	if (bind(fd, (struct sockaddr*)addr, size) < 0) {
		rc = errno;
		wlr_log_errno(WLR_ERROR, "Failed to bind socket %c%s",
			addr->sun_path[0] ? addr->sun_path[0] : '@',
			addr->sun_path + 1);
		goto cleanup;
	}
	if (listen(fd, 1) < 0) {
		rc = errno;
		wlr_log_errno(WLR_ERROR, "Failed to listen to socket %c%s",
			addr->sun_path[0] ? addr->sun_path[0] : '@',
			addr->sun_path + 1);
		goto cleanup;
	}

	return fd;

cleanup:
	close(fd);
	if (addr->sun_path[0]) {
		unlink(addr->sun_path);
	}
	errno = rc;
	return -1;
}

static bool check_socket_dir(void) {
	const char *dir = wlr_platform()->unix_socket->socket_dir();
	struct stat buf;

	if (lstat(dir, &buf)) {
		wlr_log_errno(WLR_ERROR, "Failed to stat %s", dir);
		return false;
	}
	if (!(buf.st_mode & S_IFDIR)) {
		wlr_log(WLR_ERROR, "%s is not a directory", dir);
		return false;
	}
	if (!((buf.st_uid == 0) || (buf.st_uid == getuid()))) {
		wlr_log(WLR_ERROR, "%s not owned by root or us", dir);
		return false;
	}
	if (!(buf.st_mode & S_ISVTX)) {
		/* we can deal with no sticky bit... */
		if ((buf.st_mode & (S_IWGRP | S_IWOTH))) {
			/* but not if other users can mess with our sockets */
			wlr_log(WLR_ERROR, "sticky bit not set on %s", dir);
			return false;
		}
	}
	return true;
}

/**
 * Fill @p addr with the socket address for display @p display.
 *
 * @param addr        Pre-cleared sockaddr_un to populate.
 * @param display     X11 display number.
 * @param is_abstract Pass true for the first socket (socks[0]);
 *                    false for the second (socks[1]).  The vtable's
 *                    format_socket_path() decides what this means
 *                    for the current platform.
 */
static void make_sockaddr(struct sockaddr_un *addr, int display, bool is_abstract) {
	const struct wlr_unix_socket_vtable *vt = wlr_platform()->unix_socket;

	addr->sun_family = AF_UNIX;
	if (is_abstract) {
		/* Abstract socket: sun_path[0] is the NUL separator. */
		addr->sun_path[0] = 0;
		vt->format_socket_path(addr->sun_path + 1,
			sizeof(addr->sun_path) - 1, display);
	} else {
		/* Regular filesystem socket: format the full path. */
		char fmt[64];
		snprintf(fmt, sizeof(fmt), "%s/X%%d", vt->socket_dir());
		snprintf(addr->sun_path, sizeof(addr->sun_path), fmt, display);
	}
}

static bool open_sockets(int socks[2], int display) {
	struct sockaddr_un addr = { .sun_family = AF_UNIX };

	if (mkdir(wlr_platform()->unix_socket->socket_dir(), 0755) == 0) {
		wlr_log(WLR_INFO, "Created %s ourselves -- other users will "
			"be unable to create X11 UNIX sockets of their own",
			wlr_platform()->unix_socket->socket_dir());
	} else if (errno != EEXIST) {
		wlr_log_errno(WLR_ERROR, "Unable to mkdir %s",
			wlr_platform()->unix_socket->socket_dir());
		return false;
	} else if (!check_socket_dir()) {
		return false;
	}

	/* socks[0]: abstract / underscore socket (platform-specific) */
	make_sockaddr(&addr, display, true);
	socks[0] = open_socket(&addr, strlen(addr.sun_path) + 1);
	if (socks[0] < 0) {
		return false;
	}

	/* socks[1]: regular filesystem socket */
	make_sockaddr(&addr, display, false);
	socks[1] = open_socket(&addr, strlen(addr.sun_path));
	if (socks[1] < 0) {
		close(socks[0]);
		socks[0] = -1;
		return false;
	}

	return true;
}

void unlink_display_sockets(int display) {
	wlr_platform()->unix_socket->unlink_display_socket(display);

	char path[64];
	snprintf(path, sizeof(path),
		wlr_platform()->unix_socket->lock_fmt(), display);
	unlink(path);
}

int open_display_sockets(int socks[2]) {
	int lock_fd, display;
	char lock_name[64];
	const char *lock_fmt = wlr_platform()->unix_socket->lock_fmt();

	for (display = 0; display <= 32; display++) {
		snprintf(lock_name, sizeof(lock_name), lock_fmt, display);
		if ((lock_fd = open(lock_name, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0444)) >= 0) {
			if (!open_sockets(socks, display)) {
				unlink(lock_name);
				close(lock_fd);
				continue;
			}
			char pid[12];
			snprintf(pid, sizeof(pid), "%10d", getpid());
			if (write(lock_fd, pid, sizeof(pid) - 1) != sizeof(pid) - 1) {
				unlink(lock_name);
				close(lock_fd);
				continue;
			}
			close(lock_fd);
			break;
		}

		if ((lock_fd = open(lock_name, O_RDONLY | O_CLOEXEC)) < 0) {
			continue;
		}

		char pid[12] = { 0 }, *end_pid;
		ssize_t bytes = read(lock_fd, pid, sizeof(pid) - 1);
		close(lock_fd);

		if (bytes != sizeof(pid) - 1) {
			continue;
		}
		long int read_pid;
		read_pid = strtol(pid, &end_pid, 10);
		if (read_pid < 0 || read_pid > INT32_MAX || end_pid != pid + sizeof(pid) - 2) {
			continue;
		}
		errno = 0;
		if (kill((pid_t)read_pid, 0) != 0 && errno == ESRCH) {
			if (unlink(lock_name) != 0) {
				continue;
			}
			// retry
			display--;
			continue;
		}
	}

	if (display > 32) {
		wlr_log(WLR_ERROR, "No display available in the first 33");
		return -1;
	}

	return display;
}

/*
 * apfsxattr.c - Userspace library for APFS extended attributes
 *
 * Implements a set of functions that wrap APFS IOCTL operations to provide
 * access to extended attributes without VFS 64 KiB limitations.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "apfsxattr.h"
#include "../apfs_xattr_ioctl.h"

/* Helper: open file for reading, with optional symlink follow control */
static int open_target(const char *path, int follow_symlinks)
{
	int flags = O_RDONLY;
	if (!follow_symlinks)
		flags |= O_NOFOLLOW;
	return open(path, flags);
}

/* Helper: pass attribute names through as-is (kernel handles VFS prefixes) */
static const char *normalize_name(const char *name)
{
	return name;
}

/* Helper to perform an IOCTL operation and handle errors */
static int do_ioctl(int fd, unsigned long cmd, void *arg)
{
	int ret = ioctl(fd, cmd, arg);
	if (ret < 0)
		return -1;
	return ret;
}

/* ============================================================================
 * LIST OPERATIONS
 * ============================================================================ */

static ssize_t list_xattr_impl(int fd, char *list, size_t size)
{
	struct apfs_ioctl_xattr_list uarg;
	memset(&uarg, 0, sizeof(uarg));

	/* First call: probe for required size */
	uarg.buf_len = 0;
	uarg.buf = NULL;
	ssize_t probe_result = do_ioctl(fd, APFS_IOC_XATTR_LIST, &uarg);
	if (probe_result < 0) {
		if (errno == EOVERFLOW) {
			errno = ERANGE;
		}
		return -1;
	}

	ssize_t required = probe_result;

	/* If caller doesn't want data, just return the size */
	if (!list || size == 0)
		return required;

	/* If buffer is too small, return required size (caller decides) */
	if (required > (ssize_t)size) {
		errno = ERANGE;
		return -1;
	}

	/* Second call: get the actual list */
	void *kbuf = malloc(required);
	if (!kbuf)
		return -1;

	uarg.buf_len = required;
	uarg.buf = kbuf;
	ssize_t result = do_ioctl(fd, APFS_IOC_XATTR_LIST, &uarg);
	if (result < 0) {
		free(kbuf);
		if (errno == EOVERFLOW)
			errno = ERANGE;
		return -1;
	}

	memcpy(list, kbuf, result);
	free(kbuf);
	return result;
}

ssize_t apfs_listxattr(const char *path, char *list, size_t size)
{
	int fd = open_target(path, 1);
	if (fd < 0)
		return -1;
	ssize_t ret = list_xattr_impl(fd, list, size);
	close(fd);
	return ret;
}

ssize_t apfs_llistxattr(const char *path, char *list, size_t size)
{
	int fd = open_target(path, 0);
	if (fd < 0)
		return -1;
	ssize_t ret = list_xattr_impl(fd, list, size);
	close(fd);
	return ret;
}

ssize_t apfs_flistxattr(int fd, char *list, size_t size)
{
	return list_xattr_impl(fd, list, size);
}

/* ============================================================================
 * GET OPERATIONS
 * ============================================================================ */

static ssize_t get_xattr_impl(int fd, const char *name, void *value, size_t size)
{
	struct apfs_ioctl_xattr_rw uarg;
	const char *norm_name = normalize_name(name);

	memset(&uarg, 0, sizeof(uarg));
	uarg.name_len = (uint32_t)strlen(norm_name);
	uarg.name = (char *)norm_name;
	uarg.value_len = 0;
	uarg.value = NULL;

	/* First call: probe for the attribute size */
	ssize_t probe_result = do_ioctl(fd, APFS_IOC_XATTR_GET, &uarg);
	if (probe_result < 0) {
		if (errno == EOVERFLOW)
			errno = ERANGE;
		return -1;
	}

	ssize_t full_size = probe_result;

	/* If caller doesn't want data, just return the size */
	if (!value || size == 0)
		return full_size;

	/* If buffer is too small, fail with ERANGE */
	if (full_size > (ssize_t)size) {
		errno = ERANGE;
		return -1;
	}

	/* Second call: get the actual data */
	void *kbuf = malloc(full_size);
	if (!kbuf)
		return -1;

	uarg.value_len = full_size;
	uarg.value = kbuf;
	ssize_t result = do_ioctl(fd, APFS_IOC_XATTR_GET, &uarg);
	if (result < 0) {
		free(kbuf);
		if (errno == EOVERFLOW)
			errno = ERANGE;
		return -1;
	}

	memcpy(value, kbuf, full_size);
	free(kbuf);
	return full_size;
}

ssize_t apfs_getxattr(const char *path, const char *name, void *value, size_t size)
{
	int fd = open_target(path, 1);
	if (fd < 0)
		return -1;
	ssize_t ret = get_xattr_impl(fd, name, value, size);
	close(fd);
	return ret;
}

ssize_t apfs_lgetxattr(const char *path, const char *name, void *value, size_t size)
{
	int fd = open_target(path, 0);
	if (fd < 0)
		return -1;
	ssize_t ret = get_xattr_impl(fd, name, value, size);
	close(fd);
	return ret;
}

ssize_t apfs_fgetxattr(int fd, const char *name, void *value, size_t size)
{
	return get_xattr_impl(fd, name, value, size);
}

/* ============================================================================
 * SET OPERATIONS
 * ============================================================================ */

static int set_xattr_impl(int fd, const char *name, const void *value,
                          size_t size, int flags)
{
	struct apfs_ioctl_xattr_rw uarg;
	const char *norm_name = normalize_name(name);

	memset(&uarg, 0, sizeof(uarg));
	uarg.name_len = (uint32_t)strlen(norm_name);
	uarg.name = (char *)norm_name;
	uarg.value_len = (uint32_t)size;
	uarg.value = (void *)value;
	uarg.flags = (uint32_t)flags;

	int ret = do_ioctl(fd, APFS_IOC_XATTR_SET, &uarg);
	if (ret < 0) {
		if (errno == EOVERFLOW)
			errno = ERANGE;
		return -1;
	}
	return 0;
}

int apfs_setxattr(const char *path, const char *name, const void *value,
                   size_t size, int flags)
{
	int fd = open_target(path, 1);
	if (fd < 0)
		return -1;
	int ret = set_xattr_impl(fd, name, value, size, flags);
	close(fd);
	return ret;
}

int apfs_lsetxattr(const char *path, const char *name, const void *value,
                    size_t size, int flags)
{
	int fd = open_target(path, 0);
	if (fd < 0)
		return -1;
	int ret = set_xattr_impl(fd, name, value, size, flags);
	close(fd);
	return ret;
}

int apfs_fsetxattr(int fd, const char *name, const void *value,
                    size_t size, int flags)
{
	return set_xattr_impl(fd, name, value, size, flags);
}

/* ============================================================================
 * REMOVE OPERATIONS
 * ============================================================================ */

static int remove_xattr_impl(int fd, const char *name)
{
	struct apfs_ioctl_xattr_rw uarg;
	const char *norm_name = normalize_name(name);

	memset(&uarg, 0, sizeof(uarg));
	uarg.name_len = (uint32_t)strlen(norm_name);
	uarg.name = (char *)norm_name;
	uarg.value_len = 0;
	uarg.value = NULL;

	int ret = do_ioctl(fd, APFS_IOC_XATTR_REMOVE, &uarg);
	if (ret < 0)
		return -1;
	return 0;
}

int apfs_removexattr(const char *path, const char *name)
{
	int fd = open_target(path, 1);
	if (fd < 0)
		return -1;
	int ret = remove_xattr_impl(fd, name);
	close(fd);
	return ret;
}

int apfs_lremovexattr(const char *path, const char *name)
{
	int fd = open_target(path, 0);
	if (fd < 0)
		return -1;
	int ret = remove_xattr_impl(fd, name);
	close(fd);
	return ret;
}

int apfs_fremovexattr(int fd, const char *name)
{
	return remove_xattr_impl(fd, name);
}

/* ============================================================================
 * INFO OPERATIONS
 * ============================================================================ */

static int info_xattr_impl(int fd, uint64_t *value_size, uint64_t *list_size,
                           uint32_t *count)
{
	struct apfs_ioctl_xattr_info uarg;
	memset(&uarg, 0, sizeof(uarg));

	int ret = do_ioctl(fd, APFS_IOC_XATTR_INFO, &uarg);
	if (ret < 0)
		return -1;

	/* Success: copy results to caller */
	if (value_size)
		*value_size = uarg.total_value_size;
	if (list_size)
		*list_size = uarg.total_list_size;
	if (count)
		*count = uarg.count;

	return 0;
}

int apfs_xattr_info(const char *path, uint64_t *value_size,
                    uint64_t *list_size, uint32_t *count)
{
	int fd = open_target(path, 1);
	if (fd < 0)
		return -1;
	int ret = info_xattr_impl(fd, value_size, list_size, count);
	close(fd);
	return ret;
}

int apfs_lxattr_info(const char *path, uint64_t *value_size,
                     uint64_t *list_size, uint32_t *count)
{
	int fd = open_target(path, 0);
	if (fd < 0)
		return -1;
	int ret = info_xattr_impl(fd, value_size, list_size, count);
	close(fd);
	return ret;
}

int apfs_fxattr_info(int fd, uint64_t *value_size,
                     uint64_t *list_size, uint32_t *count)
{
	return info_xattr_impl(fd, value_size, list_size, count);
}

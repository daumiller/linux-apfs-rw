/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * APFS extended attribute IOCTL interface - shared between kernel and userspace
 * Both the kernel driver and userspace tools include this header.
 */

#ifndef _APFS_XATTR_IOCTL_H
#define _APFS_XATTR_IOCTL_H

/* For kernel code, __user marks userspace pointers. For userspace, it's undefined. */
#ifndef __user
#define __user
#include <sys/ioctl.h>
#include <stdint.h>
#endif

/* Maximum size for a single extended attribute value (64 MiB) */
#define APFS_XATTR_MAX_SIZE		(64ULL * 1024 * 1024)

/* Maximum size for the combined list of all attribute names (64 MiB) */
#define APFS_XATTR_MAX_LIST_SIZE	(64ULL * 1024 * 1024)

/*
 * APFS_IOC_XATTR_LIST: List all extended attributes on a file
 *
 * Input: apfs_ioctl_xattr_list with buf_len = 0 to query required size,
 *        or buf_len > 0 to copy list into buffer.
 * Return: Total size needed for all attribute names (sum of name + NUL),
 *         or negative error code.
 *
 * The returned list is a sequence of NUL-terminated names.
 * Format: [name\0] [name\0] ... repeated for each attribute.
 */
struct apfs_ioctl_xattr_list {
	uint32_t buf_len;        /* size of user buffer (0 to query required size) */
	void __user *buf;        /* user pointer to receive list (NUL-terminated concatenated names) */
	uint32_t reserved;       /* reserved for future use */
};

#define APFS_IOC_XATTR_LIST   _IOWR('@', 0x90, struct apfs_ioctl_xattr_list)

/*
 * APFS_IOC_XATTR_GET: Read an extended attribute value
 *
 * Input: apfs_ioctl_xattr_rw with:
 *   - name_len: length of name (excluding NUL)
 *   - name: user pointer to name buffer
 *   - value_len: size of user buffer to receive data (0 to query full size)
 *   - value: user pointer to receive value data
 * Return: Full attribute size (or negative error code).
 *
 * If value_len is 0, returns the full attribute size without copying.
 * If value_len > 0, copies up to value_len bytes to the value buffer and
 * returns the full attribute size (allowing caller to detect truncation).
 */
struct apfs_ioctl_xattr_rw {
	uint32_t name_len;       /* length of the name (excluding terminating NUL) */
	void __user *name;       /* user pointer to name buffer */
	uint64_t value_len;      /* length of value buffer (for get: provided buf size; for set: size of data) */
	void __user *value;      /* user pointer to value buffer */
	uint32_t flags;          /* for set: XATTR_CREATE/XATTR_REPLACE; reserved otherwise */
	uint32_t reserved2;      /* reserved for future use */
};

#define APFS_IOC_XATTR_GET    _IOWR('@', 0x91, struct apfs_ioctl_xattr_rw)

/*
 * APFS_IOC_XATTR_SET: Write or create an extended attribute
 *
 * Input: apfs_ioctl_xattr_rw with:
 *   - name_len: length of name (excluding NUL)
 *   - name: user pointer to name buffer
 *   - value_len: size of data to write
 *   - value: user pointer to value data
 *   - flags: XATTR_CREATE (fail if exists), XATTR_REPLACE (fail if missing), or 0 (always succeed)
 * Return: 0 on success, negative error code on failure.
 *
 * Creates or replaces an extended attribute with the given value.
 * Attribute size is limited by APFS_XATTR_MAX_SIZE.
 */
#define APFS_IOC_XATTR_SET    _IOW('@', 0x92, struct apfs_ioctl_xattr_rw)

/*
 * APFS_IOC_XATTR_REMOVE: Delete an extended attribute
 *
 * Input: apfs_ioctl_xattr_rw with:
 *   - name_len: length of name (excluding NUL)
 *   - name: user pointer to name buffer
 *   - other fields: ignored
 * Return: 0 on success, negative error code on failure.
 *
 * Deletes the named extended attribute if it exists.
 */
#define APFS_IOC_XATTR_REMOVE _IOW('@', 0x93, struct apfs_ioctl_xattr_rw)

/*
 * APFS_IOC_XATTR_INFO: Query metadata about extended attributes
 *
 * Input: apfs_ioctl_xattr_info (all fields pre-initialized, typically to zero)
 * Return: 0 on success, negative error code on failure.
 *         On success, the struct is filled with total sizes and count.
 *
 * Returns aggregate information about all extended attributes on the target:
 * - total_value_size: sum of all attribute values (bytes)
 * - total_list_size: sum of (name + NUL) for each attribute
 * - count: number of attributes
 *
 * Useful for pre-allocating buffers or checking if any attributes exist.
 */
struct apfs_ioctl_xattr_info {
	uint64_t total_value_size;   /* sum of all attribute value sizes */
	uint64_t total_list_size;    /* sum of (name + NUL) for each attribute */
	uint32_t count;              /* number of attributes */
	uint32_t reserved;           /* reserved for future use */
};

#define APFS_IOC_XATTR_INFO   _IOR('@', 0x94, struct apfs_ioctl_xattr_info)

#endif /* _APFS_XATTR_IOCTL_H */

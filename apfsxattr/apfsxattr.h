/*
 * apfsxattr.h - Userspace library for APFS extended attributes via IOCTLs
 *
 * This library provides a set of functions that mirror the sys/xattr.h API
 * but use the APFS-specific IOCTLs to work around VFS 64 KiB limitations.
 *
 * All functions are named with an "apfs" prefix (e.g., apfs_setxattr instead
 * of setxattr) and work only on files/directories on APFS filesystems.
 */

#ifndef _APFSXATTR_H
#define _APFSXATTR_H 1

#include <sys/types.h>
#include <stdint.h>

__BEGIN_DECLS

/* Constants matching sys/xattr.h */
#define XATTR_CREATE  1	/* set value, fail if attr already exists */
#define XATTR_REPLACE 2	/* set value, fail if attr does not exist */

/*
 * apfs_setxattr - Set an extended attribute on a file by path
 * @path:  Path to the file (or directory)
 * @name:  Attribute name (prefix like "user.foo" is handled transparently)
 * @value: Pointer to attribute data
 * @size:  Size of @value in bytes
 * @flags: XATTR_CREATE or XATTR_REPLACE
 *
 * Returns 0 on success, or -1 on error (errno is set).
 *
 * Note: The VFS prefix "user." is handled by the kernel; pass just the name.
 */
extern int apfs_setxattr(const char *path, const char *name,
                          const void *value, size_t size, int flags)
	__THROW;

/*
 * apfs_lsetxattr - Like apfs_setxattr but doesn't follow symlinks
 */
extern int apfs_lsetxattr(const char *path, const char *name,
                           const void *value, size_t size, int flags)
	__THROW;

/*
 * apfs_fsetxattr - Set an extended attribute using an open file descriptor
 */
extern int apfs_fsetxattr(int fd, const char *name,
                           const void *value, size_t size, int flags)
	__THROW;

/*
 * apfs_getxattr - Get an extended attribute from a file by path
 * @path:  Path to the file (or directory)
 * @name:  Attribute name
 * @value: Buffer to receive attribute data (can be NULL to get size)
 * @size:  Size of @value buffer
 *
 * Returns the number of bytes in the attribute value on success,
 * or -1 on error (errno is set).
 * If @value is NULL or @size is 0, returns the full size of the attribute.
 */
extern ssize_t apfs_getxattr(const char *path, const char *name,
                              void *value, size_t size)
	__THROW;

/*
 * apfs_lgetxattr - Like apfs_getxattr but doesn't follow symlinks
 */
extern ssize_t apfs_lgetxattr(const char *path, const char *name,
                               void *value, size_t size)
	__THROW;

/*
 * apfs_fgetxattr - Get an extended attribute using an open file descriptor
 */
extern ssize_t apfs_fgetxattr(int fd, const char *name,
                               void *value, size_t size)
	__THROW;

/*
 * apfs_listxattr - List extended attributes on a file by path
 * @path: Path to the file (or directory)
 * @list: Buffer to receive null-separated attribute names (can be NULL)
 * @size: Size of @list buffer
 *
 * Each attribute name in the list is stored as a null-terminated string.
 * The total size is the sum of (strlen(name) + 1) for each attribute.
 *
 * Returns the total size of the attribute list on success,
 * or -1 on error (errno is set).
 * If @list is NULL or @size is 0, returns the size needed.
 */
extern ssize_t apfs_listxattr(const char *path, char *list, size_t size)
	__THROW;

/*
 * apfs_llistxattr - Like apfs_listxattr but doesn't follow symlinks
 */
extern ssize_t apfs_llistxattr(const char *path, char *list, size_t size)
	__THROW;

/*
 * apfs_flistxattr - List extended attributes using an open file descriptor
 */
extern ssize_t apfs_flistxattr(int fd, char *list, size_t size)
	__THROW;

/*
 * apfs_removexattr - Remove an extended attribute from a file by path
 * @path: Path to the file (or directory)
 * @name: Attribute name to remove
 *
 * Returns 0 on success, or -1 on error (errno is set).
 */
extern int apfs_removexattr(const char *path, const char *name)
	__THROW;

/*
 * apfs_lremovexattr - Like apfs_removexattr but doesn't follow symlinks
 */
extern int apfs_lremovexattr(const char *path, const char *name)
	__THROW;

/*
 * apfs_fremovexattr - Remove an extended attribute using an open file descriptor
 */
extern int apfs_fremovexattr(int fd, const char *name)
	__THROW;

/*
 * apfs_xattr_info - Get aggregate xattr metadata for a file
 * @path: Path to the file (or directory)
 * @value_size: Output parameter for total size of all attribute values
 * @list_size: Output parameter for total size of attribute list (prefix + names + NULs)
 * @count: Output parameter for number of attributes
 *
 * Returns 0 on success, or -1 on error (errno is set).
 * On success, outputs are filled with aggregate metadata about all xattrs.
 *
 * Useful for:
 * - Checking if any attributes exist (count > 0)
 * - Pre-allocating buffers before fetching attributes
 * - Getting total attribute data size for quota management
 */
extern int apfs_xattr_info(const char *path, uint64_t *value_size,
                           uint64_t *list_size, uint32_t *count)
	__THROW;

/*
 * apfs_lxattr_info - Like apfs_xattr_info but doesn't follow symlinks
 */
extern int apfs_lxattr_info(const char *path, uint64_t *value_size,
                            uint64_t *list_size, uint32_t *count)
	__THROW;

/*
 * apfs_fxattr_info - Get xattr metadata using an open file descriptor
 */
extern int apfs_fxattr_info(int fd, uint64_t *value_size,
                            uint64_t *list_size, uint32_t *count)
	__THROW;

__END_DECLS

#endif /* _APFSXATTR_H */

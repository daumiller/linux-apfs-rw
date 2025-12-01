# apfsxattr - Userspace Library for APFS Extended Attributes

A userspace library that provides extended attribute (xattr) access on APFS filesystems without the Linux VFS 64 KiB size limitations. It wraps the new APFS kernel IOCTLs to enable seamless large-attribute support.

## Overview

The standard Linux `sys/xattr.h` API has built-in limitations:
- Maximum 64 KiB per attribute value
- Maximum 64 KiB for the total list of attribute names

This library bypasses those limitations by using APFS-specific IOCTLs that operate directly on APFS data structures, allowing attributes up to 64 MiB in size.

## Features

- **Drop-in replacement API**: Matches `sys/xattr.h` interface but with `apfs_` prefix
- **Large attribute support**: Set/get xattrs up to 64 MiB (configurable)
- **Directory support**: Works on both files and directories
- **Permission enforcement**: Uses VFS-style write permission checks
- **Shared and static libraries**: Both `libapfsxattr.so` and `libapfsxattr.a` provided

## Building

```bash
cd apfsxattr
make
```

To install system-wide:
```bash
sudo make install
```

## API Reference

All functions mirror the standard `sys/xattr.h` interface but with an `apfs_` prefix:

### Setting Attributes

```c
int apfs_setxattr(const char *path, const char *name,
                  const void *value, size_t size, int flags);
int apfs_lsetxattr(const char *path, const char *name,
                   const void *value, size_t size, int flags);
int apfs_fsetxattr(int fd, const char *name,
                   const void *value, size_t size, int flags);
```

Set an extended attribute on a file/directory. Flags can be:
- `0`: Set attribute (create if missing, replace if exists)
- `XATTR_CREATE`: Fail if attribute already exists
- `XATTR_REPLACE`: Fail if attribute does not exist

### Getting Attributes

```c
ssize_t apfs_getxattr(const char *path, const char *name,
                      void *value, size_t size);
ssize_t apfs_lgetxattr(const char *path, const char *name,
                       void *value, size_t size);
ssize_t apfs_fgetxattr(int fd, const char *name,
                       void *value, size_t size);
```

Get an extended attribute value. Pass `NULL` for value or `0` for size to get the full size.

### Listing Attributes

```c
ssize_t apfs_listxattr(const char *path, char *list, size_t size);
ssize_t apfs_llistxattr(const char *path, char *list, size_t size);
ssize_t apfs_flistxattr(int fd, char *list, size_t size);
```

List all attributes on a file/directory. Returns the size of the list (null-separated attribute names).

### Removing Attributes

```c
int apfs_removexattr(const char *path, const char *name);
int apfs_lremovexattr(const char *path, const char *name);
int apfs_fremovexattr(int fd, const char *name);
```

Remove an extended attribute.

## Function Variants

- **`*setxattr`**: Follow symlinks (operate on target)
- **`l*setxattr`**: Do not follow symlinks (operate on link itself)
- **`f*setxattr`**: Use open file descriptor instead of path

The same pattern applies to `get`, `list`, and `remove` operations.

## Example Usage

See `example.c` for a complete demonstration:

```bash
# List attributes
./example list /mnt/apfs/myfile

# Get an attribute into a file
./example get /mnt/apfs/myfile myattr /tmp/out.bin

# Set an attribute from a file
./example set /mnt/apfs/myfile myattr /tmp/data.bin

# Remove an attribute
./example remove /mnt/apfs/myfile myattr
```

## Compilation

To use in your own code:

```c
#include <apfsxattr.h>

int main() {
    ssize_t size = apfs_getxattr("/path/to/file", "myattr", NULL, 0);
    if (size > 0) {
        char *buf = malloc(size);
        apfs_getxattr("/path/to/file", "myattr", buf, size);
        // Use buf...
        free(buf);
    }
    return 0;
}
```

Compile with:
```bash
gcc -o myapp myapp.c -lapfsxattr
# Or with a custom library path:
gcc -o myapp myapp.c -I/path/to/apfsxattr -L/path/to/apfsxattr -lapfsxattr
```

## Kernel Requirements

- APFS kernel module with IOCTL support for xattr operations
- IOCTLs: `APFS_IOC_XATTR_LIST`, `APFS_IOC_XATTR_GET`, `APFS_IOC_XATTR_SET`, `APFS_IOC_XATTR_REMOVE`, `APFS_IOC_XATTR_INFO`

## Query Metadata

The library provides functions to query aggregate xattr metadata without fetching all data:

```c
uint64_t value_size, list_size;
uint32_t count;
apfs_xattr_info("/path/to/file", &value_size, &list_size, &count);
// Returns: total size of all attribute values, total list size, and count
```

Useful for:
- Checking if attributes exist (`count > 0`)
- Pre-allocating buffers before fetching
- Quota management
- Metadata-only operations

Functions:
- `apfs_xattr_info()` — by path, following symlinks
- `apfs_lxattr_info()` — by path, not following symlinks
- `apfs_fxattr_info()` — by file descriptor

## Limits

Default limits (can be modified in kernel):
- Maximum attribute size: 64 MiB
- Maximum list size: 64 MiB

## Error Handling

Functions follow standard POSIX conventions:
- Return `-1` on error with `errno` set appropriately
- Return `0` or positive values on success

Common error codes:
- `ENODATA`: Attribute not found
- `ERANGE`: Buffer too small / attribute too large
- `EACCES`: Permission denied
- `EROFS`: Read-only filesystem
- `ENOENT`: File not found
- `ENOTSUP`: Operation not supported on this filesystem

## Thread Safety

The library is thread-safe. Each function opens/closes file descriptors independently.

## License

Compatible with the APFS kernel module license.

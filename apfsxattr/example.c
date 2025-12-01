/*
 * example.c - Simple example demonstrating the apfsxattr library
 *
 * Usage: ./example <operation> <file> [arguments]
 *   Operations:
 *     list <file>                     - List all xattrs on the file
 *     get <file> <name> <outfile>     - Get xattr into a file
 *     set <file> <name> <infile>      - Set xattr from a file
 *     remove <file> <name>            - Remove an xattr
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "apfsxattr.h"
#include <stdint.h>

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage:\n"
		"  %s list <file>\n"
		"  %s get <file> <name> <outfile>\n"
		"  %s set <file> <name> <infile>\n"
		"  %s remove <file> <name>\n"
		"  %s info <file>\n",
		prog, prog, prog, prog, prog);
	exit(2);
}

static int do_list(const char *file)
{
	printf("Extended attributes on %s:\n", file);

	/* First call to get the size */
	ssize_t size = apfs_listxattr(file, NULL, 0);
	if (size < 0) {
		perror("apfs_listxattr (probe)");
		return -1;
	}

	if (size == 0) {
		printf("  (none)\n");
		return 0;
	}

	/* Allocate buffer and fetch */
	char *buf = malloc(size);
	if (!buf) {
		perror("malloc");
		return -1;
	}

	ssize_t got = apfs_listxattr(file, buf, size);
	if (got < 0) {
		perror("apfs_listxattr");
		free(buf);
		return -1;
	}

	/* Parse and display */
	size_t off = 0;
	while (off < (size_t)got) {
		const char *name = buf + off;
		size_t len = strlen(name);
		printf("  %s\n", name);
		off += len + 1;
	}

	free(buf);
	return 0;
}

static int do_get(const char *file, const char *name, const char *outfile)
{
	printf("Getting xattr '%s' from %s into %s\n", name, file, outfile);

	/* Probe for size */
	ssize_t size = apfs_getxattr(file, name, NULL, 0);
	if (size < 0) {
		perror("apfs_getxattr (probe)");
		return -1;
	}

	printf("  Attribute size: %zd bytes\n", size);

	if (size == 0) {
		/* Create empty file */
		int fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (fd < 0) {
			perror("open outfile");
			return -1;
		}
		close(fd);
		return 0;
	}

	/* Allocate buffer */
	void *buf = malloc(size);
	if (!buf) {
		perror("malloc");
		return -1;
	}

	/* Get attribute */
	ssize_t got = apfs_getxattr(file, name, buf, size);
	if (got < 0) {
		perror("apfs_getxattr");
		free(buf);
		return -1;
	}

	/* Write to file */
	int fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open outfile");
		free(buf);
		return -1;
	}

	ssize_t written = write(fd, buf, got);
	if (written < 0 || written != got) {
		perror("write");
		close(fd);
		free(buf);
		return -1;
	}

	close(fd);
	free(buf);
	printf("  Wrote %zd bytes to %s\n", got, outfile);
	return 0;
}

static int do_set(const char *file, const char *name, const char *infile)
{
	printf("Setting xattr '%s' on %s from %s\n", name, file, infile);

	/* Read input file */
	int fd = open(infile, O_RDONLY);
	if (fd < 0) {
		perror("open infile");
		return -1;
	}

	off_t end = lseek(fd, 0, SEEK_END);
	if (end < 0) {
		perror("lseek");
		close(fd);
		return -1;
	}
	lseek(fd, 0, SEEK_SET);

	size_t size = (size_t)end;
	void *buf = malloc(size);
	if (!buf) {
		perror("malloc");
		close(fd);
		return -1;
	}

	ssize_t r = read(fd, buf, size);
	if (r < 0 || (size_t)r != size) {
		perror("read");
		close(fd);
		free(buf);
		return -1;
	}
	close(fd);

	/* Set attribute */
	int ret = apfs_setxattr(file, name, buf, size, 0);
	if (ret < 0) {
		perror("apfs_setxattr");
		free(buf);
		return -1;
	}

	printf("  Set %zu bytes for attribute '%s'\n", size, name);
	free(buf);
	return 0;
}

static int do_remove(const char *file, const char *name)
{
	printf("Removing xattr '%s' from %s\n", name, file);

	int ret = apfs_removexattr(file, name);
	if (ret < 0) {
		perror("apfs_removexattr");
		return -1;
	}

	printf("  Removed successfully\n");
	return 0;
}

static int do_info(const char *file)
{
	uint64_t value_size = 0, list_size = 0;
	uint32_t count = 0;

	printf("Querying xattr metadata for %s\n", file);
	int ret = apfs_xattr_info(file, &value_size, &list_size, &count);
	if (ret < 0) {
		perror("apfs_xattr_info");
		return -1;
	}

	printf("  attribute count: %u\n", count);
	printf("  total value size: %llu bytes\n", (unsigned long long)value_size);
	printf("  total list size: %llu bytes\n", (unsigned long long)list_size);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 3)
		usage(argv[0]);

	const char *op = argv[1];
	const char *file = argv[2];

	int ret = 0;

	if (strcmp(op, "list") == 0) {
		ret = do_list(file);
	} else if (strcmp(op, "get") == 0) {
		if (argc < 5) usage(argv[0]);
		ret = do_get(file, argv[3], argv[4]);
	} else if (strcmp(op, "set") == 0) {
		if (argc < 5) usage(argv[0]);
		ret = do_set(file, argv[3], argv[4]);
	} else if (strcmp(op, "remove") == 0) {
		if (argc < 4) usage(argv[0]);
		ret = do_remove(file, argv[3]);
	} else if (strcmp(op, "info") == 0) {
		ret = do_info(file);
	} else {
		usage(argv[0]);
	}

	return (ret < 0) ? 1 : 0;
}

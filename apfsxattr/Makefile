# Makefile for apfsxattr userspace library
# Builds shared and static libraries for APFS extended attribute access via IOCTLs

CC ?= gcc
CFLAGS ?= -O2 -fPIC -Wall -Wextra
LDFLAGS ?=

# Directories
LIBDIR ?= /usr/local/lib
INCLUDEDIR ?= /usr/local/include

# Library targets
SHARED_LIB = libapfsxattr.so
STATIC_LIB = libapfsxattr.a
HEADERS = apfsxattr.h

# Source files
SOURCES = apfsxattr.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(SHARED_LIB) $(STATIC_LIB)

# Shared library
$(SHARED_LIB): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) -o $@ $^

# Static library
$(STATIC_LIB): $(OBJECTS)
	ar rcs $@ $^

# Object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Installation
install: $(SHARED_LIB) $(STATIC_LIB)
	install -d $(LIBDIR)
	install -d $(INCLUDEDIR)
	install -m 755 $(SHARED_LIB) $(LIBDIR)/
	install -m 644 $(STATIC_LIB) $(LIBDIR)/
	install -m 644 $(HEADERS) $(INCLUDEDIR)/

# Cleanup
clean:
	rm -f $(OBJECTS) $(SHARED_LIB) $(STATIC_LIB)

.PHONY: all install clean

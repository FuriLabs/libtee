CC = gcc

CFLAGS = $(shell pkg-config --cflags glib-2.0 gio-2.0) -Iinclude
LDFLAGS = $(shell pkg-config --libs glib-2.0 gio-2.0)

SOURCES_LIBTEE = src/teec.c
HEADERS = include/teec.h include/tee_client_api.h

TARGET_LIBTEE = libtee.so

PREFIX ?= /usr
TRIPLET ?= $(shell $(CC) -dumpmachine)

.PHONY: all clean install

all: $(TARGET_LIBTEE)

$(TARGET_LIBTEE): $(SOURCES_LIBTEE)
	$(CC) $(CFLAGS) -fPIC -shared $^ $(LDFLAGS) -o $@

install: all
	install -d $(DESTDIR)$(PREFIX)/lib/$(TRIPLET)/
	install -m 0644 $(TARGET_LIBTEE) $(DESTDIR)$(PREFIX)/lib/$(TRIPLET)/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 0644 $(HEADERS) $(DESTDIR)$(PREFIX)/include/

clean:
	rm -f $(TARGET_LIBTEE)

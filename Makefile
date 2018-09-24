VERSION := 0.2.0

PREFIX ?= /usr/local
MANPREFIX := $(PREFIX)/share/man

CFLAGS := -Werror -Wall -Wextra -Wpedantic -std=c99 -O2 `pkg-config --cflags libbsd sqlite3`
LDFLAGS := `pkg-config --libs libbsd sqlite3`

SRC := $(shell find src -name *.c)
HEADERS := $(shell find src -name *.h)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

DISTFILES := src tad tagmage.1 tad.1 Makefile LICENSE README.md


all: options tagmage

options:
	@echo tagmage build options:
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC = $(CC)"
	@echo "PREFIX = $(PREFIX)"
	@echo "MANPREFIX = $(MANPREFIX)"
	@echo

build/%.o: src/%.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ -c $<

tagmage: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

install: tagmage
	install -m 755 -d $(MANPREFIX)/man1 $(PREFIX)/bin
	install -m 755 tagmage tad $(PREFIX)/bin/

	sed "s/@@VERSION@@/$(VERSION)/g" < tagmage.1 > $(MANPREFIX)/man1/tagmage.1
	sed "s/@@VERSION@@/$(VERSION)/g" < tad.1 > $(MANPREFIX)/man1/tad.1
	chmod 644 $(MANPREFIX)/man1/tagmage.1 $(MANPREFIX)/man1/tad.1

uninstall:
	$(RM) $(MANPREFIX)/man1/tagmage.1 $(MANPREFIX)/man1/tad.1
	$(RM) $(PREFIX)/bin/tagmage $(PREFIX)/bin/tad

dist:
	mkdir -p tagmage-$(VERSION)
	cp -r $(DISTFILES) tagmage-$(VERSION)
	tar -cf tagmage-$(VERSION).tar tagmage-$(VERSION)
	gzip tagmage-$(VERSION).tar
	$(RM) -r tagmage-$(VERSION)

clean:
	$(RM) -r build tagmage tagmage-*.tar.gz

.PHONY: all options install uninstall dist clean
